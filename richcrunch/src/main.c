#include "arena.h"
#include "file.h"
#include "huffman.h"
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
    puts("Usage: richcrunch TYPE [OPTIONS]... <input> <output>");
    puts("A tool for compressing small binary files.");
    puts("");
    puts("Possible types:");
    puts("  lz           Use lz style back-reference compression");
    puts("  huffman      Use huffman tree compression");
    puts("  lzhuff       Use huffman combined with lz compression");
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

    typedef enum compression_type_t {
        compression_type_none,
        compression_type_lz,
        compression_type_huffman,
        compression_type_lzhuff
    } compression_type_t;

    compression_type_t type = compression_type_none;
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
        else if (type == compression_type_none && strcmp(argv[i], "lz") == 0) {
            type = compression_type_lz;
        }
        else if (type == compression_type_none && strcmp(argv[i], "huffman") == 0) {
            type = compression_type_huffman;
        }
        else if (type == compression_type_none && strcmp(argv[i], "lzhuff") == 0) {
            type = compression_type_lzhuff;
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

    if (type == compression_type_none) {
        fprintf(stderr, "Compression type not specified: richcrunch --help to see options\n");
        return 1;
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

    byte_array_view_t compressed = {0};

    if (type == compression_type_lz) {

        // Perform lz compression
        lz_parse_result_t lz = lz_parse(src_file.contents, &arena, scratch);
        if (log_filename) {
            lz_dump(&lz, log_filename);
        }
        compressed = lz_serialise(&lz, &arena);
        if (verify) {
            byte_array_view_t expanded = lz_deserialise(compressed, &arena);
            bool same = (src_file.contents.num == expanded.num &&
                memcmp(src_file.contents.data, expanded.data, src_file.contents.num) == 0);
            if (!same) {
                fprintf(stderr, "Unknown error attempting to compress file\n");
            }
        }
    }
    else if (type == compression_type_huffman) {

        // Perform huffman compression
        compressed = huffman_serialise(src_file.contents, &arena, scratch);
        if (verify) {
            byte_array_view_t expanded = huffman_deserialise(compressed, &arena, scratch);
            bool same = (src_file.contents.num == expanded.num &&
                memcmp(src_file.contents.data, expanded.data, src_file.contents.num) == 0);
            if (!same) {
                fprintf(stderr, "Unknown error attempting to compress file\n");
            }
        }
    }
    else if (type == compression_type_lzhuff) {

        // Perform lzhuff compression
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
