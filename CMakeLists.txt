cmake_minimum_required(VERSION 3.16)

project(guild VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GUILD_BUILD_TESTS "Build unit tests" ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(CMAKE_AUTOMOC ON)

if(GUILD_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(libs)

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE GUILD_FORMAT_FILES
        ${CMAKE_SOURCE_DIR}/libs/*.h
        ${CMAKE_SOURCE_DIR}/libs/*.cpp
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${GUILD_FORMAT_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()
