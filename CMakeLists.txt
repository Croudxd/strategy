cmake_minimum_required(VERSION 3.10)
project(order-book)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

include_directories(include)

# enable_testing()

add_executable(strategy src/main.cpp)
# add_executable(strategy-test test/test.cpp)

#TEST:
# add_test(NAME test COMMAND strategy-test)
