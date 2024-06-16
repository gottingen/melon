#
# Copyright 2024 The Carbin Authors.
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
SET(TEST_UTILITY_SOURCES
        popen_unittest
        bounded_queue_unittest
        at_exit_unittest
        atomicops_unittest
        hash_tables_unittest
        linked_list_unittest
        dir_reader_posix_unittest
        file_path_unittest
        file_unittest
        scoped_temp_dir_unittest
        lazy_instance_unittest
        aligned_memory_unittest
        ref_counted_memory_unittest
        ref_counted_unittest
        scoped_ptr_unittest
        rand_util_unittest
        scoped_clear_errno_unittest
        scoped_generic_unittest
        security_unittest
        stl_util_unittest
        nullable_string16_unittest
        string16_unittest
        stringprintf_unittest
        string_split_unittest
        string_tokenizer_unittest
        string_util_unittest
        stringize_macros_unittest
        waitable_event_unittest
        platform_thread_unittest
        simple_thread_unittest
        thread_id_name_manager_unittest
        time_unittest
        cacheline_unittest
        class_name_unittest
        endpoint_unittest
        unique_ptr_unittest
        errno_unittest
        fd_guard_unittest
        file_watcher_unittest
        find_cstr_unittest
        scoped_lock_unittest
        status_unittest
        string_splitter_unittest
        temp_file_unittest
        melon_thread_local_unittest
        melon_time_unittest
        flat_map_unittest
        iobuf_unittest
        object_pool_unittest
        test_switches
        scoped_locale
        resource_pool_unittest
)


foreach (test ${TEST_UTILITY_SOURCES})
    carbin_cc_test(
            NAME ${test}
            MODULE utility
            SOURCES ${test}.cc
            DEFINES ${MELON_TEST_DEFINES}
            LINKS melon::melon_test test_proto_static ${MELON_DEPS_LINK} ${GPERFTOOLS_LIBRARIES} ${GTEST_LIB} ${GTEST_MAIN_LIB}
            CXXOPTS ${MELON_TEST_CXX_OPTIONS}
    )
endforeach ()