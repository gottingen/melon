
SET(TEST_BUTIL_SOURCES
        ${PROJECT_SOURCE_DIR}/test/util/recordio_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/popen_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/bounded_queue_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/at_exit_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/atomicops_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/base64_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/base64url_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/big_endian_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/bits_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/hash_tables_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/linked_list_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/mru_cache_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/small_map_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/stack_container_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/mpsc_queue_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/cpu_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/crash_logging_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/leak_tracker_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/stack_trace_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/environment_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/file_util_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/dir_reader_posix_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/file_path_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/file_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/scoped_temp_dir_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/guid_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/hash_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/lazy_instance_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/aligned_memory_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/linked_ptr_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/ref_counted_memory_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/ref_counted_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/scoped_ptr_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/scoped_vector_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/singleton_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/weak_ptr_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/observer_list_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/file_descriptor_shuffle_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/rand_util_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/safe_numerics_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/scoped_clear_errno_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/scoped_generic_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/security_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/sha1_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/stl_util_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/nullable_string16_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/safe_sprintf_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/string16_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/stringprintf_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/string_number_conversions_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/string_piece_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/string_split_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/string_tokenizer_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/string_util_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/stringize_macros_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/sys_string_conversions_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/utf_offset_string_conversions_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/utf_string_conversions_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/cancellation_flag_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/condition_variable_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/lock_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/waitable_event_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/type_traits_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/non_thread_safe_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/platform_thread_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/simple_thread_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/thread_checker_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/thread_collision_warner_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/thread_id_name_manager_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/thread_local_storage_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/thread_local_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/watchdog_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/time_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/version_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/logging_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/cacheline_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/class_name_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/endpoint_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/unique_ptr_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/errno_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/fd_guard_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/file_watcher_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/find_cstr_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/scoped_lock_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/status_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/string_printf_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/string_splitter_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/synchronous_event_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/temp_file_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/baidu_thread_local_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/thread_key_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/baidu_time_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/flat_map_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/crc32c_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/iobuf_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/object_pool_unittest.cc
        ${PROJECT_SOURCE_DIR}/test/util/test_switches.cc
        ${PROJECT_SOURCE_DIR}/test/util/scoped_locale.cc
        ${PROJECT_SOURCE_DIR}/test/util/butil_unittest_main.cc
        ${PROJECT_SOURCE_DIR}/test/util/resource_pool_unittest.cc
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET(TEST_BUTIL_SOURCES ${TEST_BUTIL_SOURCES}
            ${PROJECT_SOURCE_DIR}/test/util/proc_maps_linux_unittest.cc
            ${PROJECT_SOURCE_DIR}/test/util/test_file_util_linux.cc)
endif()

# bthread_* functions are used in logging.cc, and they need to be marked as
# weak symbols explicitly in Darwin system.
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(DYNAMIC_LIB ${DYNAMIC_LIB}
            "-Wl,-U,_bthread_getspecific"
            "-Wl,-U,_bthread_setspecific"
            "-Wl,-U,_bthread_key_create")
endif()

add_library(BUTIL_DEBUG_LIB OBJECT ${BUTIL_SOURCES})
add_library(SOURCES_DEBUG_LIB OBJECT ${SOURCES})
add_dependencies(SOURCES_DEBUG_LIB PROTO_LIB)

# shared library needs POSITION_INDEPENDENT_CODE
set_property(TARGET ${BUTIL_DEBUG_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${SOURCES_DEBUG_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(melon-shared-debug SHARED $<TARGET_OBJECTS:BUTIL_DEBUG_LIB>
        $<TARGET_OBJECTS:SOURCES_DEBUG_LIB>
        $<TARGET_OBJECTS:PROTO_LIB>)
# change the debug lib output dir to be different from the release output
set_target_properties(melon-shared-debug PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/test)

target_link_libraries(melon-shared-debug ${DYNAMIC_LIB})
if(BRPC_WITH_GLOG)
    target_link_libraries(melon-shared-debug ${GLOG_LIB})
endif()

# test_butil
add_executable(test_butil ${TEST_BUTIL_SOURCES}
        $<TARGET_OBJECTS:TEST_PROTO_LIB>)
target_link_libraries(test_butil melon-shared-debug
        gtest
        ${GPERFTOOLS_LIBRARIES})

add_test(NAME test_butil COMMAND test_butil)