#include "arena.h"
#include "bitreader.h"
#include "bitwriter.h"
#include "byte_array.h"
#include "file.h"
#include "lz.h"
#include "refs.h"
#include "test.h"
#include <stdio.h>



int test_simple(void) {
    arena_t arena = arena_make(0x800000);
    arena_t scratch = arena_make(0x800000);

    byte_array_view_t src = {
        //                        0123456789.123456789.123456789.12
        .data = (const uint8_t *)"the cat sat on the mat singinging",
        .num = sizeof("the cat sat on the mat singinging") - 1
    };

    refs_t refs = refs_make(src, &arena, scratch);

    {
        token_array_view_t tv = refs_get_tokens(&refs, 8);
        TEST_REQUIRE_EQUAL(tv.num, 1);
        TEST_REQUIRE_TRUE(token_is_literal(token_array_view_get(tv, 0)));
    }
    {
        token_array_view_t tv = refs_get_tokens(&refs, 15);
        TEST_REQUIRE_EQUAL(tv.num, 2);
        TEST_REQUIRE_TRUE(token_is_literal(token_array_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_array_view_get(tv, 1)));
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 1).offset, 15);
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 1).length_minus_one, 3); // 'the '
    }
    {
        token_array_view_t tv = refs_get_tokens(&refs, 9);
        TEST_REQUIRE_EQUAL(tv.num, 2);
        TEST_REQUIRE_TRUE(token_is_literal(token_array_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_array_view_get(tv, 1)));
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 1).offset, 4);
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 1).length_minus_one, 2); // 'at '
    }
    {
        token_array_view_t tv = refs_get_tokens(&refs, 20);
        TEST_REQUIRE_EQUAL(tv.num, 3);
        TEST_REQUIRE_TRUE(token_is_literal(token_array_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_array_view_get(tv, 1)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_array_view_get(tv, 2)));
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 1).offset, 11);
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 1).length_minus_one, 2); // 'at '
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 2).offset, 15);
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 2).length_minus_one, 3); // 'at s'
    }
    {
        token_array_view_t tv = refs_get_tokens(&refs, 27);
        TEST_REQUIRE_EQUAL(tv.num, 2);
        TEST_REQUIRE_TRUE(token_is_literal(token_array_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_array_view_get(tv, 1)));
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 1).offset, 3);
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 1).length_minus_one, 5); // 'inging'
    }
    {
        token_array_view_t tv = refs_get_tokens(&refs, 31);
        TEST_REQUIRE_EQUAL(tv.num, 2);
        TEST_REQUIRE_TRUE(token_is_literal(token_array_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_array_view_get(tv, 1)));
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 1).offset, 3);
        TEST_REQUIRE_EQUAL(token_array_view_get(tv, 1).length_minus_one, 1); // 'ng'
        // Note the earlier 'ng' is ignored for being further away than another of the same length
    }

    {
        lz_result_t lz = lz_parse(&refs, 4, &arena, scratch);

        //  0123456789.123456789.123456789.12
        // "the cat sat on the mat singinging"
        // 't'      - literal
        // 'h'      - literal 
        // 'e'      - literal
        // ' '      - literal
        // 'c'      - literal 
        // 'a'      - literal
        // 't'      - literal
        // ' '      - literal
        // 's'      - literal
        // 'at '    - reference (offset 4, length_minus_one 2)
        // 'o'      - literal
        // 'n'      - literal
        // ' '      - literal
        // 'the '   - reference (offset 15, length_minus_one 3)
        // 'm'      - literal
        // 'at s'   - reference (offset 15, length_minus_one 3)
        // 'i'      - literal
        // 'n'      - literal
        // 'g'      - literal
        // 'inging' - reference (offset 3, length_minus_one 5)

        TEST_REQUIRE_EQUAL(lz.items.num, 20);

        // Check initial run of literals is correct
        for (uint32_t i = 0; i < 9; i++) {
            const lz_item_t *lit = lz_item_array_view_at(lz.items, i);
            TEST_REQUIRE_TRUE(token_is_literal(lit->token));
            TEST_REQUIRE_EQUAL(lit->token.value, "the cat s"[i]);
            TEST_REQUIRE_EQUAL(lit->tally, 9 - i);
        }

        // Check first reference is correct
        const lz_item_t *ref1 = lz_item_array_view_at(lz.items, 9);
        TEST_REQUIRE_FALSE(token_is_literal(ref1->token));
        TEST_REQUIRE_EQUAL(ref1->tally, 1);
        TEST_REQUIRE_EQUAL(ref1->token.offset, 4);
        TEST_REQUIRE_EQUAL(ref1->token.length_minus_one, 2);

        // Check next run of literals is correct
        for (uint32_t i = 0; i < 3; i++) {
            const lz_item_t *lit = lz_item_array_view_at(lz.items, i + 10);
            TEST_REQUIRE_TRUE(token_is_literal(lit->token));
            TEST_REQUIRE_EQUAL(lit->token.value, "on "[i]);
            TEST_REQUIRE_EQUAL(lit->tally, 3 - i);
        }

        // Check next reference is correct
        const lz_item_t *ref2 = lz_item_array_view_at(lz.items, 13);
        TEST_REQUIRE_FALSE(token_is_literal(ref2->token));
        TEST_REQUIRE_EQUAL(ref2->tally, 1);
        TEST_REQUIRE_EQUAL(ref2->token.offset, 15);
        TEST_REQUIRE_EQUAL(ref2->token.length_minus_one, 3);

        // Check next literal is correct
        const lz_item_t *lit1 = lz_item_array_view_at(lz.items, 14);
        TEST_REQUIRE_TRUE(token_is_literal(lit1->token));
        TEST_REQUIRE_EQUAL(lit1->tally, 1);
        TEST_REQUIRE_EQUAL(lit1->token.value, 'm');

        // Check next reference is correct
        const lz_item_t *ref3 = lz_item_array_view_at(lz.items, 15);
        TEST_REQUIRE_FALSE(token_is_literal(ref3->token));
        TEST_REQUIRE_EQUAL(ref3->tally, 1);
        TEST_REQUIRE_EQUAL(ref3->token.offset, 15);
        TEST_REQUIRE_EQUAL(ref3->token.length_minus_one, 3);

        // Check next run of literals is correct
        for (uint32_t i = 0; i < 3; i++) {
            const lz_item_t *lit = lz_item_array_view_at(lz.items, i + 16);
            TEST_REQUIRE_TRUE(token_is_literal(lit->token));
            TEST_REQUIRE_EQUAL(lit->token.value, "ing"[i]);
            TEST_REQUIRE_EQUAL(lit->tally, 3 - i);
        }

        // Check final reference is correct
        const lz_item_t *ref4 = lz_item_array_view_at(lz.items, 19);
        TEST_REQUIRE_FALSE(token_is_literal(ref4->token));
        TEST_REQUIRE_EQUAL(ref4->tally, 1);
        TEST_REQUIRE_EQUAL(ref4->token.offset, 3);
        TEST_REQUIRE_EQUAL(ref4->token.length_minus_one, 5);
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

    bitwriter_t writer = bitwriter_make(1000, &arena);
    bitwriter_add_value(&writer, 42, 6, &arena);
    bitwriter_add_aligned_byte(&writer, 123, &arena);
    bitwriter_add_elias_gamma_value(&writer, 13, &arena);
    bitwriter_add_hybrid_value(&writer, 1234, 5, &arena);
    bitwriter_add_hybrid_value(&writer, 0xFFF, 4, &arena);
    bitwriter_add_elias_gamma_value(&writer, 256, &arena);

    bitreader_t reader = bitreader_make(writer.data.view);
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
