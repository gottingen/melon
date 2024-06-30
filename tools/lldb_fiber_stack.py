#!/usr/bin/env python
# coding=utf-8

#
# Copyright 2023 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Fiber Stack Print Tool

this only for running process, core dump is not supported.

Get Started:
    1. lldb attach -p <pid>
    2. command script import lldb_fiber_stack.py
    3. fiber_begin
    4. fiber_list
    5. fiber_frame 0
    6. bt / up / down
    7. fiber_end

Commands:
    1. fiber_num: print all fiber nums
    2. fiber_begin <num>: enter fiber debug mode, `num` is max scanned fibers, default will scan all
    3. fiber_list: list all fibers
    4. fiber_frame <id>: switch stack to fiber, id will displayed in fiber_list
    5. fiber_meta <id>: print fiber meta
    6. fiber_reg_restore: fiber_frame will modify registers, reg_restore will restore them
    7. fiber_end: exit fiber debug mode
    8. fiber_regs <id>: print fiber registers
    9. fiber_all: print all fiber frames

when call fiber_frame, registers will be modified,
remember to call fiber_end after debug, or process will be corrupted

after call fiber_frame, you can call `bt`/`up`/`down`, or other gdb command
"""

import lldb


class GlobalState():
    def __init__(self):
        self.started: bool = False
        self.fibers: list = []
        self.saved_regs: dict = {}

    def reset(self) -> None:
        self.started = False
        self.fibers.clear()

    def get_fiber(self, idx_str: str) -> lldb.SBValue:
        if not self.started:
            print("Not in fiber debug mode")
            return None
        if len(idx_str) == 0:
            print("fiber_frame <id>, see 'fiber_list'")
        try:
            fiber_sessionx = int(idx_str)
        except ValueError:
            print("please input a valid interger.")
            return None

        if fiber_sessionx >= len(self.fibers):
            print("id {} exceeds max fiber nums {}".format(
                fiber_sessionx, len(self.fibers)))
            return None
        return self.fibers[fiber_sessionx]


global_state = GlobalState()


def get_child(value: lldb.SBValue, childs_value_name: str) -> lldb.SBValue:
    r"""get child value by value name str split by '.'"""
    result = value
    childs_value_list = childs_value_name.split('.')
    for child_value_name in childs_value_list:
        result = result.GetChildMemberWithName(child_value_name)
    return result


def find_global_value(target: lldb.SBTarget, value_name: str) -> lldb.SBValue:
    r""" find global value by value name"""
    name_list = value_name.split('.')
    root_name = name_list[0]
    root_value = target.FindGlobalVariables(
        root_name, 1, lldb.eMatchTypeNormal)[0]
    if (len(name_list) == 1):
        return root_value
    return get_child(root_value, '.'.join(name_list[1:]))


def get_fibers_num(target: lldb.SBTarget):
    root_agent = find_global_value(
        target, "fiber::g_task_control._nfibers._combiner._agents.root_")
    global_res = find_global_value(
        target, "fiber::g_task_control._nfibers._combiner._global_result").GetValueAsSigned()
    long_type = target.GetBasicType(lldb.eBasicTypeLong)

    last_node = root_agent
    # agent_type: melon::var::detail::AgentCombiner<long, long, melon::var::detail::AddTo<long> >::Agent>
    agent_type: lldb.SBType = last_node.GetType().GetTemplateArgumentType(0)
    while True:
        agent = last_node.Cast(agent_type)
        if (last_node.GetLocation() != root_agent.GetLocation()):
            val = get_child(agent, "element._value").Cast(
                long_type).GetValueAsSigned()
            global_res += val
        if (get_child(agent, "next_").Dereference().GetLocation() == root_agent.GetLocation()):
            return global_res
        last_node = get_child(agent, "next_").Dereference()


def get_all_fibers(target: lldb.SBTarget, total: int):
    fibers = []
    groups = find_global_value(
        target, "mutil::ResourcePool<fiber::TaskMeta>::_ngroup.val").GetValueAsUnsigned()
    long_type = target.GetBasicType(lldb.eBasicTypeLong)
    uint32_t_type = target.FindFirstType("uint32_t")
    block_groups = find_global_value(
        target, "mutil::ResourcePool<fiber::TaskMeta>::_block_groups")
    for group in range(groups):
        block_group = get_child(
            block_groups.GetChildAtIndex(group), "val").Dereference()
        nblock = get_child(block_group, "nblock").Cast(
            long_type).GetValueAsUnsigned()
        blocks = get_child(block_group, "blocks")
        for block in range(nblock):
            # block_type: mutil::ResourcePool<fiber::TaskMeta>::Block
            block_type = blocks.GetChildAtIndex(
                block).GetType().GetTemplateArgumentType(0)
            block = blocks.GetChildAtIndex(
                block).Cast(block_type).Dereference()
            nitem = get_child(block, "nitem").GetValueAsUnsigned()
            task_meta_array_type = target.FindFirstType(
                "fiber::TaskMeta").GetArrayType(nitem)
            tasks = get_child(block, "items").Cast(task_meta_array_type)
            for i in range(nitem):
                task_meta = tasks.GetChildAtIndex(i)
                version_tid = get_child(
                    task_meta, "tid").GetValueAsUnsigned() >> 32
                version_butex = get_child(task_meta, "version_butex").Cast(
                    uint32_t_type.GetPointerType()).Dereference().GetValueAsUnsigned()
                # stack_type: fiber::ContextualStack
                stack_type = get_child(
                    task_meta, "attr.stack_type").GetValueAsUnsigned()
                if version_tid == version_butex and stack_type != 0:
                    if len(fibers) >= total:
                        return fibers
                    fibers.append(task_meta)
    return fibers

# lldb fiber commands
def fiber_begin(debugger, command, result, internal_dict):
    if global_state.started:
        print("Already in fiber debug mode, do not switch thread before exec 'fiber_end' !!!")
        return
    target = debugger.GetSelectedTarget()
    active_fibers = get_fibers_num(target)

    if len(command) == 0:
        request_bthreds = active_fibers
    else:
        try:
            request_bthreds = int(command)
        except ValueError:
            print("please input a valid interger.")
            return

    scanned_bthreds = active_fibers
    if request_bthreds > active_fibers:
        print("requested fibers {} more than actived, will display {} fibers".format(
            request_bthreds, active_fibers))
    else:
        scanned_bthreds = request_bthreds
    print("Active fibers: {}, will display {} fibers".format(
        active_fibers, scanned_bthreds))
    global_state.fibers = get_all_fibers(target, scanned_bthreds)

    # backup registers
    current_frame = target.GetProcess().GetSelectedThread().GetSelectedFrame()
    saved_regs = dict()
    saved_regs["rip"] = current_frame.FindRegister("rip").GetValueAsUnsigned()
    saved_regs["rsp"] = current_frame.FindRegister("rsp").GetValueAsUnsigned()
    saved_regs["rbp"] = current_frame.FindRegister("rbp").GetValueAsUnsigned()
    global_state.saved_regs = saved_regs

    global_state.started = True
    print("Enter fiber debug mode, do not switch thread before exec 'fiber_end' !!!")


def fiber_list(debugger, command, result, internal_dict):
    r"""list all fibers, print format is 'id\ttid\tfunction\thas stack'"""
    if not global_state.started:
        print("Not in fiber debug mode")
        return

    print("id\t\ttid\t\tfunction\t\t\t\thas stack\t\t\ttotal:{}".format(
        len(global_state.fibers)))
    for i, t in enumerate(global_state.fibers):
        tid = get_child(t, "tid").GetValueAsUnsigned()
        fn = get_child(t, "fn")
        has_stack = get_child(t, "stack").GetLocation() == "0x0"
        print("#{}\t\t{}\t\t{}\t\t{}".format(
            i, tid, fn, "no" if has_stack else "yes"))


def fiber_num(debugger, command, result, internal_dict):
    r"""list active fibers num"""
    if not global_state.started:
        print("Not in fiber debug mode")
        return

    target = debugger.GetSelectedTarget()
    active_fibers = get_fibers_num(target)
    print(active_fibers)


def fiber_frame(debugger, command, result, internal_dict):
    r"""fiber_frame <id>, select fiber frame by id"""
    fiber = global_state.get_fiber(command)
    if fiber is None:
        return

    stack = fiber.GetChildMemberWithName("stack")
    context = stack.Dereference().GetChildMemberWithName("context")

    target = debugger.GetSelectedTarget()
    uint64_t_type = target.FindFirstType("uint64_t")
    target = debugger.GetSelectedTarget()

    rip = target.CreateValueFromAddress("rip",  lldb.SBAddress(
        context.GetValueAsUnsigned() + 7*8, target), uint64_t_type).GetValueAsUnsigned()
    rbp = target.CreateValueFromAddress("rbp",  lldb.SBAddress(
        context.GetValueAsUnsigned() + 6*8, target), uint64_t_type).GetValueAsUnsigned()
    rsp = context.GetValueAsUnsigned() + 8*8

    debugger.HandleCommand(f"register write rip {rip}")
    debugger.HandleCommand(f"register write rbp {rbp}")
    debugger.HandleCommand(f"register write rsp {rsp}")


def fiber_all(debugger, command, result, internal_dict):
    r"""print all fiber frames"""
    if not global_state.started:
        print("Not in fiber debug mode")
        return

    fibers = global_state.fibers
    fiber_num = len(fibers)
    for i in range(fiber_num):
        fiber_frame(debugger, str(i), result, internal_dict)
        debugger.HandleCommand("bt")


def fiber_meta(debugger, command, result, internal_dict):
    r"""fiber_meta <id>, print task meta by id"""
    fiber = global_state.get_fiber(command)
    if fiber is None:
        return
    print(fiber)


def fiber_regs(debugger, command, result, internal_dict):
    r"""fiber_regs <id>, print fiber registers"""
    fiber = global_state.get_fiber(command)
    if fiber is None:
        return
    target = debugger.GetSelectedTarget()
    stack = get_child(fiber, "stack").Dereference()
    context = get_child(stack, "context")
    ctx_addr = context.GetValueAsUnsigned()
    uint64_t_type = target.FindFirstType("uint64_t")

    rip = target.CreateValueFromAddress("rip",  lldb.SBAddress(
        ctx_addr + 7*8, target), uint64_t_type).GetValueAsUnsigned()
    rbp = target.CreateValueFromAddress("rbp",  lldb.SBAddress(
        ctx_addr + 6*8, target), uint64_t_type).GetValueAsUnsigned()
    rbx = target.CreateValueFromAddress("rbx",  lldb.SBAddress(
        ctx_addr + 5*8, target), uint64_t_type).GetValueAsUnsigned()
    r15 = target.CreateValueFromAddress("r15",  lldb.SBAddress(
        ctx_addr + 4*8, target), uint64_t_type).GetValueAsUnsigned()
    r14 = target.CreateValueFromAddress("r14",  lldb.SBAddress(
        ctx_addr + 3*8, target), uint64_t_type).GetValueAsUnsigned()
    r13 = target.CreateValueFromAddress("r13",  lldb.SBAddress(
        ctx_addr + 2*8, target), uint64_t_type).GetValueAsUnsigned()
    r12 = target.CreateValueFromAddress("r12",  lldb.SBAddress(
        ctx_addr + 1*8, target), uint64_t_type).GetValueAsUnsigned()
    rsp = ctx_addr + 8*8

    print("rip: 0x{:x}\nrsp: 0x{:x}\nrbp: 0x{:x}\nrbx: 0x{:x}\nr15: 0x{:x}\nr14: 0x{:x}\nr13: 0x{:x}\nr12: 0x{:x}".format(
        rip, rsp, rbp, rbx, r15, r14, r13, r12))


def fiber_reg_restore(debugger, command, result, internal_dict):
    r"""restore registers"""
    if not global_state.started:
        print("Not in fiber debug mode")
        return
    for reg_name, reg_value in global_state.saved_regs.items():
        debugger.HandleCommand(f"register write {reg_name} {reg_value}")


def fiber_end(debugger, command, result, internal_dict):
    r"""exit fiber debug mode"""
    if not global_state.started:
        print("Not in fiber debug mode")
        return
    fiber_reg_restore(debugger, command, result, internal_dict)
    global_state.reset()
    print("Exit fiber debug mode")


# And the initialization code to add commands.
def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand(
        'command script add -f lldb_fiber_stack.fiber_begin fiber_begin')
    debugger.HandleCommand(
        'command script add -f lldb_fiber_stack.fiber_list fiber_list')
    debugger.HandleCommand(
        'command script add -f lldb_fiber_stack.fiber_frame fiber_frame')
    debugger.HandleCommand(
        'command script add -f lldb_fiber_stack.fiber_num fiber_num')
    debugger.HandleCommand(
        'command script add -f lldb_fiber_stack.fiber_all fiber_all')
    debugger.HandleCommand(
        'command script add -f lldb_fiber_stack.fiber_meta fiber_meta')
    debugger.HandleCommand(
        'command script add -f lldb_fiber_stack.fiber_regs fiber_regs')
    debugger.HandleCommand(
        'command script add -f lldb_fiber_stack.fiber_reg_restore fiber_reg_restore')
    debugger.HandleCommand(
        'command script add -f lldb_fiber_stack.fiber_end fiber_end')
