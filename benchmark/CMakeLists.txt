

find_package(benchmark REQUIRED)
carbin_check_target(benchmark::benchmark_main)
carbin_check_target(benchmark::benchmark)
include_directories(${BENCHMARK_INCLUDE_DIRS})

set(BENCHMARK_LINKS)
set(BENCHMARK_LINKS "benchmark::benchmark" "benchmark::benchmark_main" "abel::testing" "abel::abel")

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