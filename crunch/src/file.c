#include "file.h"
#include <stdio.h>


file_read_result_t file_read(const char *filename, arena_t *arena) {
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
        .data = {
            .data = ptr,
            .num = (uint32_t)size
        }
    };
}


file_error_t file_write(const char *filename, byte_array_view_t data) {
    return (file_error_t) {file_error_write};
}
