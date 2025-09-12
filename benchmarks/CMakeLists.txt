add_executable(bench_spsc bench_spsc.cpp)
target_link_libraries(bench_spsc PRIVATE syslib::syslib benchmark::benchmark)
syslib_apply_sanitizers(bench_spsc)

add_executable(bench_mpmc bench_mpmc.cpp)
target_link_libraries(bench_mpmc PRIVATE syslib::syslib benchmark::benchmark)
syslib_apply_sanitizers(bench_mpmc)

