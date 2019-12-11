cmake_minimum_required(VERSION 3.15)
project(HW_2)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS -pthread)

add_executable(HW_2 main.cpp thread_joiner.hpp thread_safe_queue.hpp threadpool.cpp threadpool.h)