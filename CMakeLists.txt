cmake_minimum_required(VERSION 3.27)

project(
    codaris
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PostgreSQL REQUIRED)
find_package(CURL REQUIRED)
find_path(MHD_INCLUDE_DIR microhttpd.h REQUIRED)
find_library(MHD_LIBRARY NAMES microhttpd REQUIRED)
find_path(SODIUM_INCLUDE_DIR sodium.h REQUIRED)
find_library(SODIUM_LIBRARY NAMES sodium libsodium REQUIRED)
find_path(JSONC_INCLUDE_DIR json-c/json.h REQUIRED)
find_library(JSONC_LIBRARY NAMES json-c REQUIRED)

add_executable(
    codaris_server
    src/server/main.c
    src/server/db.c
    src/server/config.c
    src/server/api.c
    src/server/mail.c
)

target_include_directories(
    codaris_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${MHD_INCLUDE_DIR} ${SODIUM_INCLUDE_DIR} ${JSONC_INCLUDE_DIR}
)

target_link_libraries(
    codaris_server
    PRIVATE
        PostgreSQL::PostgreSQL CURL::libcurl ${MHD_LIBRARY} ${SODIUM_LIBRARY} ${JSONC_LIBRARY}
)

if(MSVC)
    target_compile_options(codaris_server PRIVATE /W4 /permissive-)
else()
    target_compile_options(codaris_server PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(NOT WIN32)
    target_compile_definitions(codaris_server PRIVATE _POSIX_C_SOURCE=200809L)
else()
    target_link_libraries(codaris_server PRIVATE ws2_32)
endif()
