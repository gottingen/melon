#!/usr/bin/env python
# coding=utf-8

# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

"""
Fiber Stack Print Tool

this only for running process, core dump is not supported.

Get Started:
    1. gdb attach <pid>
    2. source gdb_fiber_stack.py
    3. fibers_begin
    4. fiber_list
    5. fiber_frame 0
    6. bt / up / down
    7. fibers_end

Commands:
    1. fibers_num: print all fiber nums
    2. fibers_begin <num>: enter fiber debug mode, `num` is max scanned fibers, default will scan all
    3. fiber_list: list all fibers
    4. fiber_frame <id>: switch stack to fiber, id will displayed in fiber_list
    5. fibers_meta <id>: print fiber meta
    6. fibers_reg_restore: fiber_frame will modify registers, reg_restore will restore them
    7. fibers_end: exit fiber debug mode
    8. fibers_regs <id>: print fiber registers

when call fiber_frame, registers will be modified,
remember to call fibers_end after debug, or process will be corrupted

after call fiber_frame, you can call `bt`/`up`/`down`, or other gdb command
"""

import gdb

fibers = []
status = False

def get_fiber_num():
    root_agent = gdb.parse_and_eval("&(((((*melon::fiber_internal::g_task_control)._nfibers)._combiner)._agents).root_)")
    global_res = int(gdb.parse_and_eval("((*melon::fiber_internal::g_task_control)._nfibers)._combiner._global_result"))
    get_agent = "(*(('melon::detail::agent_combiner<long, long, melon::detail::AddTo<long> >::Agent' *){}))"
    last_node = root_agent
    while True:
        agent = gdb.parse_and_eval(get_agent.format(last_node))
        if last_node != root_agent:
            val = int(agent["element"]["_value"]["_M_i"])
            global_res += val
        if agent["next_"] == root_agent:
            return global_res
        last_node = agent["next_"]

def get_all_fibers(total):
    global fibers
    fibers = []
    count = 0
    groups = int(gdb.parse_and_eval("'melon::ResourcePool<melon::fiber_internal::fiber_entity>::_ngroup'")["val"])
    for group in range(groups):
        blocks = int(gdb.parse_and_eval("(*((*((('melon::static_atomic<melon::ResourcePool<melon::fiber_internal::fiber_entity>::BlockGroup*>' *)('melon::ResourcePool<melon::fiber_internal::fiber_entity>::_block_groups')) + {})).val)).nblock._M_i".format(group)))
        for block in range(blocks):
            items = int(gdb.parse_and_eval("(*(*(('std::atomic<melon::ResourcePool<melon::fiber_internal::fiber_entity>::Block*>' *)((*((*((('melon::static_atomic<melon::ResourcePool<melon::fiber_internal::fiber_entity>::BlockGroup*>' *)('melon::ResourcePool<melon::fiber_internal::fiber_entity>::_block_groups')) + {})).val)).blocks) + {}))._M_b._M_p).nitem".format(group, block)))
            for item in range(items):
                task_meta = gdb.parse_and_eval("*(('melon::fiber_internal::fiber_entity' *)((*(*(('std::atomic<melon::ResourcePool<melon::fiber_internal::fiber_entity>::Block*>' *)((*((*((('melon::static_atomic<melon::ResourcePool<melon::fiber_internal::fiber_entity>::BlockGroup*>' *)('melon::ResourcePool<melon::fiber_internal::fiber_entity>::_block_groups')) + {})).val)).blocks) + {}))._M_b._M_p).items) + {})".format(group, block, item))
                version_tid = (int(task_meta["tid"]) >> 32)
                version_butex = gdb.parse_and_eval("*(uint32_t *){}".format(task_meta["version_butex"]))
                if version_tid == int(version_butex) and int(task_meta["attr"]["stack_type"]) != 0:
                    fibers.append(task_meta)
                    count += 1
                    if count >= total:
                        return

class FiberListCmd(gdb.Command):
    """list all fibers, print format is 'id\ttid\tfunction\thas stack'"""
    def __init__(self):
        gdb.Command.__init__(self, "fiber_list", gdb.COMMAND_STACK, gdb.COMPLETE_NONE)

    def invoke(self, arg, tty):
        global status
        global fibers
        if not status:
            print("Not in fiber debug mode")
            return
        print("id\t\ttid\t\tfunction\t\thas stack\t\t\ttotal:{}".format(len(fibers)))
        for i, t in enumerate(fibers):
            print("#{}\t\t{}\t\t{}\t\t{}".format(i, t["tid"], t["fn"], "no" if str(t["stack"]) == "0x0" else "yes"))

class FiberNumCmd(gdb.Command):
    """list active fibers num"""
    def __init__(self):
        gdb.Command.__init__(self, "fibers_num", gdb.COMMAND_STACK, gdb.COMPLETE_NONE)

    def invoke(self, arg, tty):
        res = get_fiber_num()
        print(res)

class FiberFrameCmd(gdb.Command):
    """fiber_frame <id>, select fiber frame by id"""
    def __init__(self):
        gdb.Command.__init__(self, "fiber_frame", gdb.COMMAND_STACK, gdb.COMPLETE_NONE)

    def invoke(self, arg, tty):
        global status
        global fibers
        if not status:
            print("Not in fiber debug mode")
            return
        if not arg:
            print("fiber_frame <id>, see 'fiber_list'")
            return
        fibers_id = int(arg)
        if fibers_id >= len(fibers):
            print("id {} exceeds max fiber nums {}".format(fibers_id, len(fibers)))
            return
        stack = fibers[fibers_id]["stack"]
        if str(stack) == "0x0":
            print("this fiber has no stack")
            return
        context = gdb.parse_and_eval("(*(('melon::fiber_internal::fiber_contextual_stack' *){})).context".format(stack))
        rip = gdb.parse_and_eval("*(uint64_t*)({}+7*8)".format(context))
        rbp = gdb.parse_and_eval("*(uint64_t*)({}+6*8)".format(context))
        rsp = gdb.parse_and_eval("{}+8*8".format(context))
        gdb.parse_and_eval("$rip = {}".format(rip))
        gdb.parse_and_eval("$rsp = {}".format(rsp))
        gdb.parse_and_eval("$rbp = {}".format(rbp))

class FiberRegsCmd(gdb.Command):
    """fibers_regs <id>, print fiber registers"""
    def __init__(self):
        gdb.Command.__init__(self, "fibers_regs", gdb.COMMAND_STACK, gdb.COMPLETE_NONE)

    def invoke(self, arg, tty):
        global status
        global fibers
        if not status:
            print("Not in fiber debug mode")
            return
        if not arg:
            print("fibers_regs <id>, see 'fiber_list'")
            return
        fibers_id = int(arg)
        if fibers_id >= len(fibers):
            print("id {} exceeds max fiber nums {}".format(fibers_id, len(fibers)))
            return
        stack = fibers[fibers_id]["stack"]
        if str(stack) == "0x0":
            print("this fiber has no stack")
            return
        context = gdb.parse_and_eval("(*(('melon::fiber_internal::fiber_contextual_stack' *){})).context".format(stack))
        rip = int(gdb.parse_and_eval("*(uint64_t*)({}+7*8)".format(context)))
        rbp = int(gdb.parse_and_eval("*(uint64_t*)({}+6*8)".format(context)))
        rbx = int(gdb.parse_and_eval("*(uint64_t*)({}+5*8)".format(context)))
        r15 = int(gdb.parse_and_eval("*(uint64_t*)({}+4*8)".format(context)))
        r14 = int(gdb.parse_and_eval("*(uint64_t*)({}+3*8)".format(context)))
        r13 = int(gdb.parse_and_eval("*(uint64_t*)({}+2*8)".format(context)))
        r12 = int(gdb.parse_and_eval("*(uint64_t*)({}+1*8)".format(context)))
        rsp = int(gdb.parse_and_eval("{}+8*8".format(context)))
        print("rip: 0x{:x}\nrsp: 0x{:x}\nrbp: 0x{:x}\nrbx: 0x{:x}\nr15: 0x{:x}\nr14: 0x{:x}\nr13: 0x{:x}\nr12: 0x{:x}".format(rip, rsp, rbp, rbx, r15, r14, r13, r12))

class FiberMetaCmd(gdb.Command):
    """fibers_meta <id>, print task meta by id"""
    def __init__(self):
        gdb.Command.__init__(self, "fibers_meta", gdb.COMMAND_STACK, gdb.COMPLETE_NONE)

    def invoke(self, arg, tty):
        global status
        global fibers
        if not status:
            print("Not in fiber debug mode")
            return
        if not arg:
            print("fibers_meta <id>, see 'fiber_list'")
            return
        fibers_id = int(arg)
        if fibers_id >= len(fibers):
            print("id {} exceeds max fiber nums {}".format(fibers_id, len(fibers)))
            return
        print(fibers[fibers_id])

class FiberBeginCmd(gdb.Command):
    """enter fiber debug mode"""
    def __init__(self):
        gdb.Command.__init__(self, "fibers_begin", gdb.COMMAND_STACK, gdb.COMPLETE_NONE)

    def invoke(self, arg, tty):
        global status
        if status:
            print("Already in fiber debug mode, do not switch thread before exec 'fibers_end' !!!")
            return
        active_fibers = get_fiber_num()
        scanned_fibers = active_fibers
        if arg:
            num_arg = int(arg)
            if num_arg < active_fibers:
                scanned_fibers = num_arg
            else:
                print("requested fibers {} more than actived, will display {} fibers".format(num_arg, scanned_fibers))
        print("Active fibers: {}, will display {} fibers".format(active_fibers, scanned_fibers))
        get_all_fibers(scanned_fibers)
        gdb.parse_and_eval("$saved_rip = $rip")
        gdb.parse_and_eval("$saved_rsp = $rsp")
        gdb.parse_and_eval("$saved_rbp = $rbp")
        status = True
        print("Enter fiber debug mode, do not switch thread before exec 'fibers_end' !!!")

class FiberRegRestoreCmd(gdb.Command):
    """restore registers"""
    def __init__(self):
        gdb.Command.__init__(self, "fibers_reg_restore", gdb.COMMAND_STACK, gdb.COMPLETE_NONE)

    def invoke(self, arg, tty):
        global status
        if not status:
            print("Not in fiber debug mode")
            return
        gdb.parse_and_eval("$rip = $saved_rip")
        gdb.parse_and_eval("$rsp = $saved_rsp")
        gdb.parse_and_eval("$rbp = $saved_rbp")
        print("OK")

class FiberEndCmd(gdb.Command):
    """exit fiber debug mode"""
    def __init__(self):
        gdb.Command.__init__(self, "fibers_end", gdb.COMMAND_STACK, gdb.COMPLETE_NONE)

    def invoke(self, arg, tty):
        global status
        if not status:
            print("Not in fiber debug mode")
            return
        gdb.parse_and_eval("$rip = $saved_rip")
        gdb.parse_and_eval("$rsp = $saved_rsp")
        gdb.parse_and_eval("$rbp = $saved_rbp")
        status = False
        print("Exit fiber debug mode")

FiberListCmd()
FiberNumCmd()
FiberBeginCmd()
FiberEndCmd()
FiberFrameCmd()
FiberMetaCmd()
FiberRegRestoreCmd()
FiberRegsCmd()
