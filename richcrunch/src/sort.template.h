#ifndef SORT_TEMPLATE_H_
#define SORT_TEMPLATE_H_

#include <stdbool.h>
#include <stdint.h>

// Macros for concatenating multiple macros
#ifndef CONCAT
#define CONCAT(a, b) CONCAT2_(a, b)
#define CONCAT2_(a, b) a##b
#endif

#endif // ifndef SORT_TEMPLATE_H_


#ifndef TEMPLATE_SORT_NAME
#define TEMPLATE_SORT_NAME uint32
#error Must define TEMPLATE_SORT_NAME with the element type to be sorted
#endif

#ifndef TEMPLATE_SORT_TYPE
#define TEMPLATE_SORT_TYPE              CONCAT(TEMPLATE_SORT_NAME, _t)
#endif

#ifndef TEMPLATE_SORT_SPAN_NAME
#define TEMPLATE_SORT_SPAN_NAME         CONCAT(TEMPLATE_SORT_NAME, _array_span)
#endif


#define SORT_TYPE_t                     TEMPLATE_SORT_TYPE
#define SORT_SPAN_t                     CONCAT(TEMPLATE_SORT_SPAN_NAME, _t)

#define SORT_SPAN_get                   CONCAT(TEMPLATE_SORT_SPAN_NAME, _get)
#define SORT_SPAN_set                   CONCAT(TEMPLATE_SORT_SPAN_NAME, _set)
#define SORT_SPAN_at                    CONCAT(TEMPLATE_SORT_SPAN_NAME, _at)
#define SORT_SPAN_make_subspan          CONCAT(TEMPLATE_SORT_SPAN_NAME, _make_subspan)

#define SORT_NAME                       CONCAT(sort_, TEMPLATE_SORT_NAME)
#define SORT_NAME_partial_quicksort     CONCAT(SORT_NAME, _partial_quicksort)
#define SORT_NAME_insertion             CONCAT(SORT_NAME, _insertion)
#define SORT_NAME_swap                  CONCAT(SORT_NAME, _swap)
#define SORT_NAME_median_of_three       CONCAT(SORT_NAME, _median_of_three)
#define SORT_NAME_less_or_equal         CONCAT(SORT_NAME, _less_or_equal)

#ifndef TEMPLATE_SORT_LESS_FN
#define TEMPLATE_SORT_LESS_FN CONCAT(SORT_NAME, _default_less)
static inline bool TEMPLATE_SORT_LESS_FN(const SORT_TYPE_t *a, const SORT_TYPE_t *b) {
    return *a < *b;
}
#endif

#define SORT_less_fn                    TEMPLATE_SORT_LESS_FN


static inline void SORT_NAME_swap(SORT_TYPE_t *a, SORT_TYPE_t *b) {
    SORT_TYPE_t temp = *a;
    *a = *b;
    *b = temp;
}


static inline uint32_t SORT_NAME_median_of_three(SORT_SPAN_t span, uint32_t i, uint32_t j, uint32_t k) {
    SORT_TYPE_t *elem_i = SORT_SPAN_at(span, i);
    SORT_TYPE_t *elem_j = SORT_SPAN_at(span, j);
    SORT_TYPE_t *elem_k = SORT_SPAN_at(span, k);

    return SORT_less_fn(elem_i, elem_j) ?
        (SORT_less_fn(elem_j, elem_k) ? j : (SORT_less_fn(elem_i, elem_k) ? k : i)) :
        (SORT_less_fn(elem_i, elem_k) ? i : (SORT_less_fn(elem_j, elem_k) ? k : j));
}


static inline bool SORT_NAME_less_or_equal(const SORT_TYPE_t *a, const SORT_TYPE_t *b) {
    return SORT_less_fn(a, b) || !SORT_less_fn(b, a);
}


static inline void SORT_NAME_partial_quicksort(SORT_SPAN_t span) {
    while (span.num > 16) {     // threshold for performing insertion sort on a partition
        uint32_t mid_index = span.num / 2;
        uint32_t pivot_index = SORT_NAME_median_of_three(span, 0, mid_index, span.num - 1);
        SORT_TYPE_t *pivot_elem = SORT_SPAN_at(span, pivot_index);
        SORT_NAME_swap(pivot_elem, SORT_SPAN_at(span, span.num - 1));

        // Hoare partition
        uint32_t i = 0;
        uint32_t j = span.num - 1;
        while (i < j) {
            while (i < j && SORT_NAME_less_or_equal(SORT_SPAN_at(span, i), pivot_elem)) i++;
            while (i < j && SORT_NAME_less_or_equal(pivot_elem, SORT_SPAN_at(span, j))) j--;
            if (i < j) SORT_NAME_swap(SORT_SPAN_at(span, i), SORT_SPAN_at(span, j));
        }
        SORT_NAME_swap(SORT_SPAN_at(span, i), SORT_SPAN_at(span, span.num - 1));

        // Recurse
        if (i < span.num - i) {
            SORT_NAME_partial_quicksort(SORT_SPAN_make_subspan(span, 0, i));
            span = SORT_SPAN_make_subspan(span, i + 1, span.num);
        }
        else {
            SORT_NAME_partial_quicksort(SORT_SPAN_make_subspan(span, i + 1, span.num));
            span = SORT_SPAN_make_subspan(span, 0, i);
        }
    }
}


static inline void SORT_NAME_insertion(SORT_SPAN_t span) {
    for (uint32_t i = 1; i < span.num; i++) {
        SORT_TYPE_t key = SORT_SPAN_get(span, i);
        uint32_t j = i;
        // Rotate current element down to its sorted position
        while (j > 0 && SORT_less_fn(&key, SORT_SPAN_at(span, j - 1))) {
            SORT_SPAN_set(span, j, SORT_SPAN_get(span, j - 1));
            j--;
        }
        SORT_SPAN_set(span, j, key);
    }
}


static inline void SORT_NAME(SORT_SPAN_t span) {
    if (span.num > 1) {
        // Perform a partial quicksort to a granularity of partitions of <16 elements
        SORT_NAME_partial_quicksort(span);

        // Insertion sort to finish off
        SORT_NAME_insertion(span);
    }
}


// Undefine everything

#undef SORT_TYPE_t
#undef SORT_ARRAY_t
#undef SORT_SPAN_t
#undef SORT_SPAN_get
#undef SORT_SPAN_set
#undef SORT_SPAN_at
#undef SORT_SPAN_make_subspan
#undef SORT_NAME
#undef SORT_NAME_partial_quicksort
#undef SORT_NAME_insertion
#undef SORT_NAME_swap
#undef SORT_NAME_median_of_three
#undef SORT_NAME_less_or_equal
#undef SORT_less_fn

#undef TEMPLATE_SORT_NAME
#undef TEMPLATE_SORT_TYPE
#undef TEMPLATE_SORT_SPAN_NAME
#undef TEMPLATE_SORT_LESS_FN
