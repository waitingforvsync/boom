#include "arena.h"
#include "byte_array.h"
#include "file.h"
#include "refs.h"
#include "test.h"
#include <stdio.h>


#define TEST_REQUIRE_TRUE(cond) do { if (!(cond)) { fprintf(stderr, "%s:%d: error: test failed: %s\n", __FILE__, __LINE__, #cond); return 1; }} while (false)
#define TEST_REQUIRE_EQUAL(cond, val) do { if ((cond) != (val)) { fprintf(stderr, "%s:%d: error: test failed: %s; expected %d, got %d\n", __FILE__, __LINE__, #cond, val, (cond)); return 1; }} while (false)


int test_simple(void) {
    arena_t arena = arena_make(0x800000);
    arena_t scratch = arena_make(0x800000);

    byte_view_t src = {
        //                        0123456789.123456789.1
        .data = (const uint8_t *)"the cat sat on the mat",
        .num = sizeof("the cat sat on the mat") - 1
    };

    refs_t refs = refs_make(src, &arena, scratch);

    {
        token_view_t tv = refs_get_tokens(&refs, 8);
        TEST_REQUIRE_EQUAL(tv.num, 1);
        TEST_REQUIRE_TRUE(token_is_literal(token_view_get(tv, 0)));
    }
    {
        token_view_t tv = refs_get_tokens(&refs, 15);
        TEST_REQUIRE_EQUAL(tv.num, 4);
        TEST_REQUIRE_TRUE(token_is_literal(token_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 1)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 2)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 3)));
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).length_minus_one, 1); // 'th'
        TEST_REQUIRE_EQUAL(token_view_get(tv, 2).length_minus_one, 2); // 'the'
        TEST_REQUIRE_EQUAL(token_view_get(tv, 3).length_minus_one, 3); // 'the '
    }
    {
        token_view_t tv = refs_get_tokens(&refs, 9);
        TEST_REQUIRE_EQUAL(tv.num, 3);
        TEST_REQUIRE_TRUE(token_is_literal(token_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 1)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 2)));
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).offset, 4);
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).length_minus_one, 1); // 'at'
        TEST_REQUIRE_EQUAL(token_view_get(tv, 2).length_minus_one, 2); // 'at '
    }
    {
        token_view_t tv = refs_get_tokens(&refs, 20);
        TEST_REQUIRE_EQUAL(tv.num, 2);
        TEST_REQUIRE_TRUE(token_is_literal(token_view_get(tv, 0)));
        TEST_REQUIRE_TRUE(!token_is_literal(token_view_get(tv, 1)));
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).offset, 11);
        TEST_REQUIRE_EQUAL(token_view_get(tv, 1).length_minus_one, 1); // 'at'
        // Note: the earlier matches are rejected for being no longer than the closer ones
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


int test_run(void) {
    return test_simple() ||
           test_file();
}
