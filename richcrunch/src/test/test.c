#include "arena.h"
#include "bitreader.h"
#include "bitwriter.h"
#include "byte_array.h"
#include "file.h"
#include "huffman.h"
#include "lz.h"
#include "refs.h"
#include "test.h"
#include <stdio.h>



int test_refs(void) {
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

    arena_deinit(&scratch);
    arena_deinit(&arena);

    return 0;
}


int test_lz_simple(void) {
    arena_t arena = arena_make(0x800000);
    arena_t scratch = arena_make(0x800000);

    byte_array_view_t src = {
        //                        0123456789.123456789.123456789.12
        .data = (const uint8_t *)"the cat sat on the mat singinging",
        .num = sizeof("the cat sat on the mat singinging") - 1
    };

    lz_parse_result_t lz = lz_parse(src, &arena, scratch);

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

    // Serialise it
    byte_array_view_t compressed = lz_serialise(&lz, &arena);

    // Deserialise it
    byte_array_view_t expanded = lz_deserialise(compressed, &arena);

    // Compare it
    bool same = (memcmp(src.data, expanded.data, src.num) == 0);
    TEST_REQUIRE_TRUE(same);

    arena_deinit(&scratch);
    arena_deinit(&arena);

    return 0;
}


int test_lz_file(void) {
    arena_t arena = arena_make(0x800000);
    arena_t scratch = arena_make(0x800000);

    file_read_result_t file_result = file_read_binary("titlescreen.bin", &arena);
    TEST_REQUIRE_EQUAL(file_result.error.type, file_error_none);
    TEST_REQUIRE_EQUAL(file_result.contents.num, 8320);

    lz_parse_result_t lz = lz_parse(file_result.contents, &arena, scratch);
    lz_dump(&lz, "titlescreen.txt");
    byte_array_view_t compressed = lz_serialise(&lz, &arena);
    byte_array_view_t expanded = lz_deserialise(compressed, &arena);
    bool same = (memcmp(file_result.contents.data, expanded.data, file_result.contents.num) == 0);
    TEST_REQUIRE_TRUE(same);

    // printf("lz compressed: %d / 8320 (%d%%), %d fixed bits\n",
    //     compressed.num,
    //     compressed.num * 100 / file_result.contents.num,
    //     lz.num_fixed_bits
    // );

    arena_deinit(&scratch);
    arena_deinit(&arena);

    return 0;
}


int test_bitstream(void) {
    arena_t arena = arena_make(0x800000);

    bitwriter_t writer = bitwriter_make(1000, &arena);
    bitwriter_add_value(&writer, 42, 6, &arena);            // 6 bits
    TEST_REQUIRE_EQUAL(writer.data.num, 1);
    TEST_REQUIRE_EQUAL(writer.bit, 6);
    bitwriter_add_aligned_byte(&writer, 123, &arena);       
    TEST_REQUIRE_EQUAL(writer.data.num, 2);
    TEST_REQUIRE_EQUAL(writer.bit, 6);
    bitwriter_add_elias_gamma_value(&writer, 13, &arena);   // 7 bits
    TEST_REQUIRE_EQUAL(writer.data.num, 3);
    TEST_REQUIRE_EQUAL(writer.bit, 5);
    bitwriter_add_hybrid_value(&writer, 1234, 5, &arena);   // 5+11=16 bits
    TEST_REQUIRE_EQUAL(writer.data.num, 5);
    TEST_REQUIRE_EQUAL(writer.bit, 5);
    bitwriter_add_hybrid_value(&writer, 0xFFF, 4, &arena);  // 4+17=21 bits
    TEST_REQUIRE_EQUAL(writer.data.num, 8);
    TEST_REQUIRE_EQUAL(writer.bit, 2);
    bitwriter_add_elias_gamma_value(&writer, 256, &arena);  // 17 bits
    TEST_REQUIRE_EQUAL(writer.data.num, 10);
    TEST_REQUIRE_EQUAL(writer.bit, 3);

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


#define TEMPLATE_ARRAY_NAME uint32_array
#define TEMPLATE_ARRAY_TYPE uint32_t
#include "array.template.h"

#define TEMPLATE_SORT_NAME uint32
#include "sort.template.h"

int test_sort(void) {
    uint32_t data[1000];
    uint32_array_span_t span = SPAN(data);

    uint32_t val = 372621;
    for (uint32_t i = 0; i < span.num; i++) {
        uint32_array_span_set(span, i, val % 256);
        val = (val * 100009 + 12356237);
    }
    
    sort_uint32(span);

    bool is_sorted = true;
    for (uint32_t i = 1; is_sorted && i < span.num; i++) {
        is_sorted &= (uint32_array_span_get(span, i - 1) <= uint32_array_span_get(span, i));
    }
    TEST_REQUIRE_TRUE(is_sorted);

    return 0;
}


int test_huffman_simple(void) {
    arena_t arena = arena_make(0x800000);
    arena_t scratch = arena_make(0x800000);

    byte_array_view_t src = {
        .data = (const uint8_t *)"the cat sat on the mat",
        .num = sizeof("the cat sat on the mat") - 1
    };

    // Symbol frequencies:
    //  ' ' = 5
    //  't' = 5
    //  'a' = 3
    //  'e' = 2
    //  'h' = 2
    //  'c' = 1
    //  'm' = 1
    //  'n' = 1
    //  'o' = 1
    //  's' = 1

    // When building the Huffman tree, if we prioritise leaf nodes over tree nodes of the same frequency,
    // we get a more balanced tree (left) compared to prioritising tree nodes (right).
    // The more balanced tree is preferable, and is what we are expecting.
    //  ' ' = 00                ' ' = 00
    //  't' = 01                't' = 01
    //  'a' = 100               'a' = 100
    //  'e' = 1010              'h' = 101
    //  'h' = 1011              'e' = 1100
    //  'n' = 1100              's' = 1101
    //  'o' = 1101              'c' = 11100
    //  's' = 1110              'm' = 11101
    //  'c' = 11110             'n' = 11110
    //  'm' = 11111             'o' = 11111

    uint16_t counts[256] = {0};
    for (uint32_t i = 0; i < src.num; i++) {
        counts[byte_array_view_get(src, i)]++;
    }

    // Test that length limiting does something sensible
    uint8_array_view_t huff_limited = huffman_build_code_lengths((uint16_array_view_t) VIEW(counts), 4, &arena, scratch);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, 't'), 2);   // 00
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, ' '), 3);   // 010
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, 'a'), 3);   // 011
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, 'e'), 4);   // 1000
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, 'h'), 4);   // 1001
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, 'n'), 4);   // 1010
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, 'o'), 4);   // 1011
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, 's'), 4);   // 1100
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, 'c'), 4);   // 1101
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, 'm'), 4);   // 1110
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff_limited, 'x'), 0);

    // Generate an new huffman encoding without length limiting
    uint8_array_view_t huff = huffman_build_code_lengths((uint16_array_view_t) VIEW(counts), 0, &arena, scratch);
    TEST_REQUIRE_EQUAL(huff.num, 256);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, ' '), 2);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, 't'), 2);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, 'a'), 3);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, 'e'), 4);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, 'h'), 4);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, 'n'), 4);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, 'o'), 4);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, 's'), 4);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, 'c'), 5);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, 'm'), 5);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(huff, 'x'), 0);

    uint16_array_view_t codes = huffman_get_canonical_encoding(huff, &arena);
    TEST_REQUIRE_EQUAL(codes.num, 256);
    TEST_REQUIRE_EQUAL(uint16_array_view_get(codes, ' '), 4);   // [1]00
    TEST_REQUIRE_EQUAL(uint16_array_view_get(codes, 't'), 5);   // [1]01
    TEST_REQUIRE_EQUAL(uint16_array_view_get(codes, 'a'), 12);  // [1]100
    TEST_REQUIRE_EQUAL(uint16_array_view_get(codes, 'e'), 26);  // [1]1010
    TEST_REQUIRE_EQUAL(uint16_array_view_get(codes, 'h'), 27);  // [1]1011
    TEST_REQUIRE_EQUAL(uint16_array_view_get(codes, 'n'), 28);  // [1]1100
    TEST_REQUIRE_EQUAL(uint16_array_view_get(codes, 'o'), 29);  // [1]1101
    TEST_REQUIRE_EQUAL(uint16_array_view_get(codes, 's'), 30);  // [1]1110
    TEST_REQUIRE_EQUAL(uint16_array_view_get(codes, 'c'), 62);  // [1]11110
    TEST_REQUIRE_EQUAL(uint16_array_view_get(codes, 'm'), 63);  // [1]11111

    huffman_decoder_t decoder = huffman_decoder_make(huff, &arena);
    TEST_REQUIRE_EQUAL(decoder.num_codes_of_length.num, 5);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(decoder.num_codes_of_length, 0), 0);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(decoder.num_codes_of_length, 1), 2);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(decoder.num_codes_of_length, 2), 1);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(decoder.num_codes_of_length, 3), 5);
    TEST_REQUIRE_EQUAL(uint8_array_view_get(decoder.num_codes_of_length, 4), 2);

    TEST_REQUIRE_EQUAL(decoder.dictionary.num, 10);
    TEST_REQUIRE_EQUAL(uint16_array_view_get(decoder.dictionary, 0), ' ');
    TEST_REQUIRE_EQUAL(uint16_array_view_get(decoder.dictionary, 1), 't');
    TEST_REQUIRE_EQUAL(uint16_array_view_get(decoder.dictionary, 2), 'a');
    TEST_REQUIRE_EQUAL(uint16_array_view_get(decoder.dictionary, 3), 'e');
    TEST_REQUIRE_EQUAL(uint16_array_view_get(decoder.dictionary, 4), 'h');
    TEST_REQUIRE_EQUAL(uint16_array_view_get(decoder.dictionary, 5), 'n');
    TEST_REQUIRE_EQUAL(uint16_array_view_get(decoder.dictionary, 6), 'o');
    TEST_REQUIRE_EQUAL(uint16_array_view_get(decoder.dictionary, 7), 's');
    TEST_REQUIRE_EQUAL(uint16_array_view_get(decoder.dictionary, 8), 'c');
    TEST_REQUIRE_EQUAL(uint16_array_view_get(decoder.dictionary, 9), 'm');

    bitwriter_t writer = bitwriter_make(1000, &arena);
    bitwriter_add_huffman_code(&writer, codes, 'a', &arena);
    bitwriter_add_huffman_code(&writer, codes, 'n', &arena);
    bitwriter_add_huffman_code(&writer, codes, 't', &arena);

    bitreader_t reader = bitreader_make(writer.data.view);
    TEST_REQUIRE_EQUAL(bitreader_get_huffman_code(&reader, decoder), 'a');
    TEST_REQUIRE_EQUAL(bitreader_get_huffman_code(&reader, decoder), 'n');
    TEST_REQUIRE_EQUAL(bitreader_get_huffman_code(&reader, decoder), 't');

    arena_deinit(&scratch);
    arena_deinit(&arena);

    return 0;
}


int test_huffman_file(void) {
    arena_t arena = arena_make(0x800000);
    arena_t scratch = arena_make(0x800000);

    file_read_result_t file_result = file_read_binary("titlescreen.bin", &arena);
    TEST_REQUIRE_EQUAL(file_result.error.type, file_error_none);
    TEST_REQUIRE_EQUAL(file_result.contents.num, 8320);

    uint16_t counts[256] = {0};
    for (uint32_t i = 0; i < file_result.contents.num; i++) {
        counts[byte_array_view_get(file_result.contents, i)]++;
    }

    uint8_array_view_t huff = huffman_build_code_lengths(
        (uint16_array_view_t) VIEW(counts),
        0,
        &arena,
        scratch
    );

    uint16_array_view_t codes = huffman_get_canonical_encoding(huff, &arena);

    bitwriter_t writer = bitwriter_make(file_result.contents.num, &arena);
    for (uint32_t i = 0; i < file_result.contents.num; i++) {
        bitwriter_add_huffman_code(
            &writer,
            codes,
            byte_array_view_get(file_result.contents, i),
            &arena
        );
    }

    huffman_decoder_t decoder = huffman_decoder_make(huff, &arena);

    bitreader_t reader = bitreader_make(writer.data.view);
    for (uint32_t i = 0; i < file_result.contents.num; i++) {
        uint16_t symbol = bitreader_get_huffman_code(&reader, decoder);
        TEST_REQUIRE_EQUAL(byte_array_view_get(file_result.contents, i), symbol);
    }

    // printf("huffman compressed: %d / 8320 (%d%%)\n",
    //     writer.data.num,
    //     writer.data.num * 100 / file_result.contents.num
    // );

    arena_deinit(&scratch);
    arena_deinit(&arena);

    return 0;
}


int test_lzhuff_simple(void) {
    return 0;
}


int test_compare_methods(void) {
    arena_t arena = arena_make(0x800000);
    arena_t scratch = arena_make(0x800000);

    // Load test file
    file_read_result_t file_result = file_read_binary("test_0.bin", &arena);
    TEST_REQUIRE_EQUAL(file_result.error.type, file_error_none);
    TEST_REQUIRE_EQUAL(file_result.contents.num, 10240);

    // Do lz compression
    {
        lz_parse_result_t lz = lz_parse(file_result.contents, &arena, scratch);
        lz_dump(&lz, "test_0.txt");
        byte_array_view_t compressed = lz_serialise(&lz, &arena);
        byte_array_view_t expanded = lz_deserialise(compressed, &arena);
        bool same = (memcmp(file_result.contents.data, expanded.data, file_result.contents.num) == 0);
        TEST_REQUIRE_TRUE(same);

        printf("lz compressed: %d / %d (%d%%), %d fixed bits\n",
            compressed.num,
            file_result.contents.num,
            compressed.num * 100 / file_result.contents.num,
            lz.num_fixed_bits
        );
    }

    // Do huffman compression
    {
        byte_array_view_t compressed = huffman_serialise(file_result.contents, &arena, scratch);
        byte_array_view_t expanded = huffman_deserialise(compressed, &arena, scratch);
        bool same = (memcmp(file_result.contents.data, expanded.data, file_result.contents.num) == 0);
        TEST_REQUIRE_TRUE(same);

        printf("huffman compressed: %d / %d (%d%%)\n",
            compressed.num,
            file_result.contents.num,
            compressed.num * 100 / file_result.contents.num
        );
    }


    arena_deinit(&scratch);
    arena_deinit(&arena);

    return 0;

}


int test_run(void) {
    return test_refs()
        || test_lz_simple()
        || test_lz_file()
        || test_bitstream()
        || test_sort()
        || test_huffman_simple()
        || test_huffman_file()
        || test_lzhuff_simple()
        || test_compare_methods();
}
