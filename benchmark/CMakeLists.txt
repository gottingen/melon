
find_package(benchmark REQUIRED)

carbin_check_target(benchmark::benchmark)
carbin_check_target(benchmark::benchmark_main)

include_directories(${benchmark_INCLUDE_DIRS})

add_subdirectory(algorithm)
add_subdirectory(base)
add_subdirectory(container)
add_subdirectory(functional)
add_subdirectory(numeric)
add_subdirectory(stats)
add_subdirectory(strings)
add_subdirectory(synchronization)
add_subdirectory(time)
add_subdirectory(types)