cmake_minimum_required(VERSION 3.16)
project(truenorth C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(truenorth
        chip.c
        core.h
        coresram.c
        main.c
        neuronblock.c
        queue.c
        queue.h
        router.c
        scheduler.c
        tokencontroller.c)
