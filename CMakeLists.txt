cmake_minimum_required(VERSION 3.10)
project(order-book)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

include_directories(include)

# enable_testing()

add_executable(strategy src/main.cpp)
add_executable(viewer src/viewer.cpp)
add_executable(test src/test.cpp)
add_executable(benchmark src/benchmark.cpp)
# add_executable(strategy-test test/test.cpp)

#TEST:
# add_test(NAME test COMMAND strategy-test)
