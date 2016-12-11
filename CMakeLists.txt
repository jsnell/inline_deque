cmake_minimum_required(VERSION 2.8)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -g3 -Wall -Werror -fno-strict-aliasing -Wno-sign-compare -fprofile-arcs -ftest-coverage")

add_executable(queue_benchmark
  src/queue_benchmark.cc)

enable_testing()
# Usual workaround for the broken test build dependency handling in CMake.
# Every test must depend on this dummy target.
add_test(build_test_code "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target all)

macro(define_test name)
  add_executable(${name}.testbin
    src/test/${name}.cc)
  add_test(${name} bin/${name}.testbin)
  set_tests_properties(${name} PROPERTIES DEPENDS
    build_test_code)
endmacro()

define_test(test_construct)
define_test(test_move)
define_test(test_erase)
define_test(test_insert)
define_test(test_random_ops)
