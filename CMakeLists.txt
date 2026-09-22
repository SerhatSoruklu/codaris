cmake_minimum_required(VERSION 3.27)

project(
    codaris
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PostgreSQL REQUIRED)

add_executable(
    codaris_server
    src/server/main.c
    src/server/db.c
)

target_include_directories(
    codaris_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(
    codaris_server
    PRIVATE
        PostgreSQL::PostgreSQL
)

if(MSVC)
    target_compile_options(codaris_server PRIVATE /W4 /permissive-)
else()
    target_compile_options(codaris_server PRIVATE -Wall -Wextra -Wpedantic)
endif()
