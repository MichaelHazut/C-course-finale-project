cmake_minimum_required(VERSION 3.10)
project(FinaleProj C)

set(CMAKE_C_STANDARD 90)
add_compile_options(-Wall -pedantic -ansi)

add_executable(main main_test.c
        macro.c
        macro.h
        symbol.c
        symbol.h
        memory.c
        memory.h
        directive.c
        directive.h
        instruction.c
        instruction.h
        utils.c
        utils.h
        pass.c
        pass.h)