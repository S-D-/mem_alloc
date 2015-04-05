project(OCPLab2)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
aux_source_directory(mem_alloc SRC_LIST)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=gnu11 -g -lm -Wall")
add_executable(${PROJECT_NAME} ${SRC_LIST})

