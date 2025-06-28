cmake_minimum_required(VERSION 3.10)
project(MyProject C)

set(CMAKE_C_STANDARD 90)
add_compile_options(-Wall -pedantic -ansi)

add_executable(main main.c)