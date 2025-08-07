#include "arena.h"
#include "file.h"
#include "refs.h"
#ifdef TESTS_ENABLED
#include "test/test.h"
#endif
#include <stdio.h>
#include <string.h>


int main(int argc, char *argv[]) {

#ifdef TESTS_ENABLED
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--test") == 0)) {
        return test_run();
    }
#endif

    if (argc != 3) {
        fprintf(stderr, "Syntax: crunch <input> <output>\n");
        return 1;
    }

    arena_t arena = arena_make(0x800000);
    arena_t scratch = arena_make(0x800000);

    file_read_result_t src_file = file_read(argv[1], &arena);
    if (src_file.error.type != file_error_none) {
        fprintf(stderr, "Error reading file '%s'\n", argv[1]);
        return 1;
    }

    refs_t refs = refs_make(src_file.contents, &arena, scratch);

    arena_deinit(&scratch);
    arena_deinit(&arena);

    return 0;
}
