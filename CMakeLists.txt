cmake_minimum_required(VERSION 3.15)
project(icpc_management)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
set(src_files
        main.cpp
)
add_executable(code ${src_files})
