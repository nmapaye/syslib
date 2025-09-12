cmake_minimum_required(VERSION 3.20)

project(syslib LANGUAGES CXX)

option(SYSLIB_BUILD_TESTS "Build tests" ON)
option(SYSLIB_BUILD_BENCHMARKS "Build benchmarks" ON)
option(SYSLIB_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(SYSLIB_ENABLE_UBSAN "Enable UBSan" OFF)
option(SYSLIB_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

add_library(syslib INTERFACE)
add_library(syslib::syslib ALIAS syslib)
target_compile_features(syslib INTERFACE cxx_std_20)
target_include_directories(syslib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

function(syslib_apply_sanitizers target)
  if (NOT TARGET ${target})
    message(FATAL_ERROR "Unknown target: ${target}")
  endif()
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    if (SYSLIB_ENABLE_ASAN)
      target_compile_options(${target} PRIVATE -fsanitize=address)
      target_link_options(${target} PRIVATE -fsanitize=address)
    endif()
    if (SYSLIB_ENABLE_UBSAN)
      target_compile_options(${target} PRIVATE -fsanitize=undefined)
      target_link_options(${target} PRIVATE -fsanitize=undefined)
    endif()
    if (SYSLIB_ENABLE_TSAN)
      target_compile_options(${target} PRIVATE -fsanitize=thread)
      target_link_options(${target} PRIVATE -fsanitize=thread)
    endif()
  endif()
endfunction()

if (SYSLIB_BUILD_TESTS)
  enable_testing()
  include(FetchContent)
  set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_subdirectory(tests)
endif()

if (SYSLIB_BUILD_BENCHMARKS)
  include(FetchContent)
  set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

  FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
  )
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googlebenchmark)

  add_subdirectory(benchmarks)
endif()

