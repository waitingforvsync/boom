#include "arena.h"
#include "bitreader.h"
#include "bitwriter.h"
#include "byte_array.h"
#include "file.h"
#include "refs.h"
#include "test.h"
#include <stdio.h>



int test_simple(void) {
    arena_t arena = arena_make(0x800000);
    arena_t scratch = arena_make(0x800000);

    byte_view_t src = {
        //                        0123456789.123456789.123456789.12
        .data = (const uint8_t *)"the cat sat on the mat singinging",
        .num = sizeof("the cat sat on the mat singinging") - 1
    };

    refs_t refs = refs_make(src, &arena, scratch);

    {
        token_view_t tv = refs_get_tokens(&refs, 8);
        TEST_REQUIRE_EQUAL(tv.num, 1);
        TEST_REQUIRE_TRUE(token_is_literal(token_view_get(tv, 0)));
    }
    {
        token_view_t tv = refs_get_tokens(&refs, 15);
        TEST_REQUIRE_EQUAL(tv.num, 2);
        TEST_REQUIRE_TRUE(token_is_literal(token_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 1)));
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).offset, 15);
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).length_minus_one, 3); // 'the '
    }
    {
        token_view_t tv = refs_get_tokens(&refs, 9);
        TEST_REQUIRE_EQUAL(tv.num, 2);
        TEST_REQUIRE_TRUE(token_is_literal(token_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 1)));
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).offset, 4);
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).length_minus_one, 2); // 'at '
    }
    {
        token_view_t tv = refs_get_tokens(&refs, 20);
        TEST_REQUIRE_EQUAL(tv.num, 3);
        TEST_REQUIRE_TRUE(token_is_literal(token_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 1)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 2)));
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).offset, 11);
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).length_minus_one, 2); // 'at '
        TEST_REQUIRE_EQUAL(token_view_get(tv, 2).offset, 15);
        TEST_REQUIRE_EQUAL(token_view_get(tv, 2).length_minus_one, 3); // 'at s'
    }
    {
        token_view_t tv = refs_get_tokens(&refs, 27);
        TEST_REQUIRE_EQUAL(tv.num, 2);
        TEST_REQUIRE_TRUE(token_is_literal(token_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 1)));
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).offset, 3);
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).length_minus_one, 5); // 'inging'
    }
    {
        token_view_t tv = refs_get_tokens(&refs, 31);
        TEST_REQUIRE_EQUAL(tv.num, 2);
        TEST_REQUIRE_TRUE(token_is_literal(token_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 1)));
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).offset, 3);
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).length_minus_one, 1); // 'ng'
        // Note the earlier 'ng' is ignored for being further away than another of the same length
    }

    arena_deinit(&scratch);
    arena_deinit(&arena);

    return 0;
}


int test_file(void) {
    arena_t arena = arena_make(0x800000);
    arena_t scratch = arena_make(0x800000);

    file_read_result_t file_result = file_read("titlescreen.bin", &arena);
    TEST_REQUIRE_EQUAL(file_result.error.type, file_error_none);
    TEST_REQUIRE_EQUAL(file_result.data.num, 8320);

    refs_t refs = refs_make(file_result.data, &arena, scratch);

    arena_deinit(&scratch);
    arena_deinit(&arena);

    return 0;
}


int test_bitstream(void) {
    arena_t arena = arena_make(0x800000);

    bitwriter_t writer = bitwriter_make(&arena, 1000);
    bitwriter_add_value(&writer, 42, 6, &arena);
    bitwriter_add_aligned_byte(&writer, 123, &arena);
    bitwriter_add_elias_gamma_value(&writer, 13, &arena);
    bitwriter_add_hybrid_value(&writer, 1234, 5, &arena);
    bitwriter_add_hybrid_value(&writer, 0xFFF, 4, &arena);
    bitwriter_add_elias_gamma_value(&writer, 256, &arena);

    bitreader_t reader = bitreader_make(byte_view_make(&writer.data));
    TEST_REQUIRE_EQUAL(bitreader_get_value(&reader, 6), 42);
    TEST_REQUIRE_EQUAL(bitreader_get_aligned_byte(&reader), 123);
    TEST_REQUIRE_EQUAL(bitreader_get_elias_gamma_value(&reader), 13);
    TEST_REQUIRE_EQUAL(bitreader_get_hybrid_value(&reader, 5), 1234);
    TEST_REQUIRE_EQUAL(bitreader_get_hybrid_value(&reader, 4), 0xFFF);
    TEST_REQUIRE_EQUAL(bitreader_get_elias_gamma_value(&reader), 0);

    arena_deinit(&arena);
    return 0;
}


int test_run(void) {
    return test_simple()
        || test_file()
        || test_bitstream();
}
