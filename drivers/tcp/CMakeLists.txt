# stop if cmake version below 3.5
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(helion-tcp-driver C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DBUILD_MODE=Debug -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DBUILD_MODE=Debug -fno-omit-frame-pointer")

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -DBUILD_MODE=Release")
set(CMAKE_C_FLAGS_RELEASE   "-O3 -flto -DBUILD_MODE=Release")

add_library(tcpdriver SHARED
	tcpdriver.cpp
)


add_executable(tcphost
	tcphost.c
)
target_link_libraries (tcphost -pthread)
