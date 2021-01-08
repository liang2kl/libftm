cmake_minimum_required(VERSION 3.13)
project(ftm)

set(CMAKE_CXX_STANDARD 20)
include_directories(/usr/include/libnl3)
file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*.cpp")

add_executable(ftm ${SOURCES}) 
#set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(ftm nl-genl-3 nl-3 Threads::Threads)

