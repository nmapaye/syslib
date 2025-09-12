add_executable(syslib_tests
  test_raii.cpp
  test_spsc.cpp
  test_mpmc.cpp
)
target_link_libraries(syslib_tests PRIVATE syslib::syslib GTest::gtest_main)
syslib_apply_sanitizers(syslib_tests)

include(GoogleTest)
gtest_discover_tests(syslib_tests)

