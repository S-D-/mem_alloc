project(OCPLab2)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
aux_source_directory(mem_alloc SRC_LIST)
#file(GLOB aux_headers mem_alloc/mem_types.h)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=gnu11 -g -lm -Wall -fsanitize=address")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(${PROJECT_NAME} ${aux_headers} ${SRC_LIST})
 
