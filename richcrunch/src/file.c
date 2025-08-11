#include "file.h"
#include <stdio.h>


file_read_result_t file_read_binary(const char *filename, arena_t *arena) {
    assert(filename);
    assert(arena);
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return (file_read_result_t) {
            .error = {file_error_not_found}
        };
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return (file_read_result_t) {
            .error = {file_error_read}
        };
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return (file_read_result_t) {
            .error = {file_error_read}
        };
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return (file_read_result_t) {
            .error = {file_error_read}
        };
    }

    void *ptr = arena_alloc(arena, (uint32_t)size);
    size_t size_read = fread(ptr, 1, size, file);
    if (ferror(file) || size_read != (size_t)size) {
        arena_free(arena, ptr, (uint32_t)size);
        fclose(file);
        return (file_read_result_t) {
            .error = {file_error_read}
        };
    }

    fclose(file);
    return (file_read_result_t) {
        .contents = {
            .data = ptr,
            .num = (uint32_t)size
        }
    };
}


file_error_t file_write_binary(const char *filename, byte_array_view_t data) {
    assert(filename);
    assert(data.data);

    FILE *file = fopen(filename, "wb");
    if (!file) {
        return (file_error_t) {file_error_write};
    }

    size_t bytes_written = fwrite(data.data, 1, data.num, file);
    if (bytes_written != data.num) {
        return (file_error_t) {file_error_write};
    }

    if (fclose(file) != 0) {
        return (file_error_t) {file_error_write};
    }

    return (file_error_t) {file_error_none};
}
