#include "arena.h"
#include "file.h"
#include "lz.h"
#ifdef TESTS_ENABLED
#include "test/test.h"
#endif
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


#define VERSION "0.1"


static void display_version(void) {
    puts("richcrunch " VERSION);
    puts("Developed and maintained by Rich Talbot-Watkins");
    puts("github.com/waitingforvsync/richcrunch");
}


static void display_help(void) {
    puts("Usage: richcrunch [OPTIONS]... <input> <output>");
    puts("A tool for compressing small binary files.");
    puts("");
    puts("Possible options:");
    puts("  -d <file>    Output beebasm includeable file with details");
    puts("  -log <file>  Output verbose listing with compression details");
    puts("  --verify     Verifies that the compressed data is correct");
    puts("");
    puts("  --version to display version and author information");
    puts("  --help to display this help again");
}


int main(int argc, char *argv[]) {
#ifdef TESTS_ENABLED
    if (argc == 2 && strcmp(argv[1], "--test") == 0) {
        return test_run();
    }
#endif

    if (argc == 1) {
        display_help();
        return 0;
    }

    const char *input_filename = 0;
    const char *output_filename = 0;
    const char *log_filename = 0;
    bool verify = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            display_help();
            return 0;
        }
        else if (strcmp(argv[i], "--version") == 0) {
            display_version();
            return 0;
        }
        else if (strcmp(argv[i], "--verify") == 0) {
            verify = true;
        }
        else if (strcmp(argv[i], "-log") == 0) {
            if (++i < argc) {
                log_filename = argv[i];
            }
            else {
                fprintf(stderr, "Missing verbose log filename (-log <filename>)\n");
                return 1;
            }
        }
        else if (!input_filename) {
            input_filename = argv[i];
        }
        else if (!output_filename) {
            output_filename = argv[i];
        }
        else {
            fprintf(stderr, "Unknown parameter: %s\n", argv[i]);
            return 1;
        }
    }

    if (!input_filename) {
        fprintf(stderr, "Missing input filename\n");
        return 1;
    }
    else if (!output_filename) {
        fprintf(stderr, "Warning: missing output filename\n");
    }

    arena_t arena = arena_make(0x1000000);
    arena_t scratch = arena_make(0x1000000);

    file_read_result_t src_file = file_read_binary(input_filename, &arena);
    if (src_file.error.type != file_error_none) {
        fprintf(stderr, "Error reading file '%s'\n", input_filename);
        return 1;
    }

    lz_parse_result_t lz = lz_parse(src_file.contents, &arena, scratch);
    if (log_filename) {
        lz_dump(&lz, log_filename);
    }
    byte_array_view_t compressed = lz_serialise(&lz, &arena);
    if (verify) {
        byte_array_view_t expanded = lz_deserialise(compressed, &arena);
        bool same = (memcmp(src_file.contents.data, expanded.data, src_file.contents.num) == 0);
        if (!same) {
            fprintf(stderr, "Unknown error attempting to compress file\n");
        }
    }

    if (output_filename) {
        file_error_t dest_file = file_write_binary(output_filename, compressed);
        if (dest_file.type != file_error_none) {
            fprintf(stderr, "Error writing file '%s'\n", output_filename);
        }
    }

    arena_deinit(&scratch);
    arena_deinit(&arena);

    return 0;
}
