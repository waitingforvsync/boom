cmake_minimum_required(VERSION 3.31)
set(CMAKE_C_STANDARD 17)
project("richcrunch")

add_executable("richcrunch")

if(MSVC)
	target_compile_options("richcrunch" PRIVATE /W4 /WX /wd4100 /wd4820 /wd4189 /wd4206 /wd4101)
else()
	target_compile_options("richcrunch" PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function)
endif()

set_target_properties("richcrunch" PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
# set_target_properties("richcrunch" PROPERTIES UNITY_BUILD ON)

target_include_directories("richcrunch" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources("richcrunch" PRIVATE
    "arena.c"
    "arena.h"
    "array.template.h"
    "bitreader.c"
    "bitreader.h"
    "bitwriter.c"
    "bitwriter.h"
    "byte_array.h"
    "file.c"
    "file.h"
    "huffman.c"
    "huffman.h"
    "lz.c"
    "lz.h"
    "lzhuff.c"
    "lzhuff.h"
    "main.c"
    "refs.c"
    "refs.h"
    "token.h"
    "uint16_array.h"
    "uint8_array.h"
    "utils.c"
    "utils.h"
)

set(TESTS_ENABLED ON CACHE BOOL "Whether tests are enabled on this build")

if(TESTS_ENABLED)
    target_compile_definitions("richcrunch" PRIVATE TESTS_ENABLED)
    add_subdirectory("test")
    add_custom_command(
        TARGET "richcrunch"
        POST_BUILD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
        COMMAND echo "Run tests"
        COMMAND richcrunch --test
    )
endif()

