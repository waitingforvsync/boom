cmake_minimum_required(VERSION 3.31)
set(CMAKE_C_STANDARD 17)
project("crunch")

add_executable("crunch")

if(MSVC)
	target_compile_options("crunch" PRIVATE /W4 /WX /wd4100 /wd4820 /wd4189 /wd4206 /wd4101)
else()
	target_compile_options("crunch" PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function)
endif()

# set_target_properties("crunch" PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
# set_target_properties("crunch" PROPERTIES UNITY_BUILD ON)

target_include_directories("crunch" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_sources("crunch" PRIVATE
    "arena.c"
    "arena.h"
    "bitreader.c"
    "bitreader.h"
    "bitwriter.c"
    "bitwriter.h"
    "byte_array.c"
    "byte_array.h"
    "file.c"
    "file.h"
    "lz.c"
    "lz.h"
    "main.c"
    "refs.c"
    "refs.h"
    "token.c"
    "token.h"
    "utils.c"
    "utils.h"
)

set(TESTS_ENABLED ON CACHE BOOL "Whether tests are enabled on this build")

if(TESTS_ENABLED)
    target_compile_definitions("crunch" PRIVATE TESTS_ENABLED)
    add_subdirectory("test")
    add_custom_command(
        TARGET "crunch"
        POST_BUILD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test"
        COMMAND echo "Run tests"
        COMMAND crunch --test
    )
endif()

