#ifndef TEST_H_
#define TEST_H_

#define TEST_REQUIRE_TRUE(cond) \
    do { if (!(cond)) { fprintf(stderr, "%s:%d: error: test failed: %s\n", __FILE__, __LINE__, #cond); return 1; }} while (false)

#define TEST_REQUIRE_FALSE(cond) \
    do { if (cond) { fprintf(stderr, "%s:%d: error: test failed: %s\n", __FILE__, __LINE__, #cond); return 1; }} while (false)

#define TEST_REQUIRE_EQUAL(cond, val) \
    do { if ((cond) != (val)) { fprintf(stderr, "%s:%d: error: test failed: %s; expected %d, got %d\n", __FILE__, __LINE__, #cond, val, (cond)); return 1; }} while (false)


int test_run(void);


#endif // ifndef TEST_H_
