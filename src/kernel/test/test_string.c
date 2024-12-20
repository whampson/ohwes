#include <stdio.h>
#include <string.h>
#include <test.h>

void memcmp_demo(const char* lhs, const char* rhs, size_t sz)
{
    for(size_t n = 0; n < sz; ++n)
        putchar(lhs[n]);

    int rc = memcmp(lhs, rhs, sz);
    const char *rel = rc < 0 ? " precedes " : rc > 0 ? " follows " : " compares equal to ";
    // fputs(rel, stdout);
    printf(rel);

    for(size_t n = 0; n < sz; ++n)
        putchar(rhs[n]);
    puts(" in lexicographical order");
}
void memcmp_reference(void)
{
    // from https://en.cppreference.com/w/c/string/byte/memcmp

    char a1[] = {'a','b','c'};
    char a2[sizeof a1] = {'a','b','d'};

    memcmp_demo(a1, a2, sizeof a1);
    memcmp_demo(a2, a1, sizeof a1);
    memcmp_demo(a1, a1, sizeof a1);
}

void memset_reference(void)
{
    // from https://en.cppreference.com/w/c/string/byte/memset

    char str[] = "ghghghghghghghghghghgh";
    puts(str);
    memset(str,'a',5);
    puts(str);
}

void strcmp_demo(const char* lhs, const char* rhs)
{
    int rc = strcmp(lhs, rhs);
    const char *rel = rc < 0 ? "precedes" : rc > 0 ? "follows" : "equals";
    printf("[%s] %s [%s]\n", lhs, rel, rhs);
}
void strcmp_reference(void)
{
    // from https://en.cppreference.com/w/c/string/byte/strcmp

    const char* string = "Hello World!";
    strcmp_demo(string, "Hello!");
    strcmp_demo(string, "Hello");
    strcmp_demo(string, "Hello there");
    strcmp_demo("Hello, everybody!" + 12, "Hello, somebody!" + 11);
}

void strlen_reference(void)
{
    // from https://en.cppreference.com/w/c/string/byte/strlen
    const char str[] = "How many characters does this string contain?";

    printf("without null character: %zu\n", strlen(str));
    printf("with null character:    %zu\n", sizeof str);
}

void test_memset(void)
{
    //
    // test writing a single byte value to every slot in a buffer
    //

    char buf[64] = {};
    void *ret;

    ret = memset(buf, 'A', sizeof(buf));
    VERIFY_ARE_EQUAL(buf, ret);

    for (int i = 0; i < sizeof(buf); i++) {
        char c = buf[i];
        VERIFY_ARE_EQUAL('A', c);
    }
}

void test_memcpy(void)
{
    //
    // test copying bytes between non-overlapping buffers
    // assumes memset works
    //

    char src[64];
    char dst[64];
    void *ret;

    //
    // test copy entire src buffer to dst
    //
    memset(src, 'A', sizeof(src));
    memset(dst, 'B', sizeof(dst));

    ret = memcpy(dst, src, sizeof(src));
    VERIFY_ARE_EQUAL(dst, ret);

    for (int i = 0; i < sizeof(dst); i++) {
        VERIFY_ARE_EQUAL(dst[i], src[i]);
    }

    //
    // test count == 0
    //
    memset(src, 'X', sizeof(src));
    memcpy(dst, src, 0);

    for (int i = 0; i < sizeof(dst); i++) {
        VERIFY_ARE_EQUAL('A', dst[i]);
    }
}

void test_memmove(void)
{
    //
    // test copying bytes between potentially overlapping buffers
    // assumes memset works
    //

    char buf[64];
    char tmpbuf[32];
    char *src;
    char *dst;
    const int count = 32;

    //
    // initialize buffer to descending ASCII chars;
    // initialize source and temp buffer to ascending ascii chars;
    // memmove src to test, buffers potentially overlap;
    // verify that dest buffer matches temp buffer
    //
    #define TEST(d,s) \
    do { \
        dst = &buf[d]; \
        src = &buf[s]; \
        for (int i = 0; i < count; i++) { \
            dst[i] = ' ' + (count - i - 1); \
        } \
        for (int i = 0; i < count; i++) { \
            src[i] = tmpbuf[i] = ' ' + i; \
        } \
        VERIFY_ARE_EQUAL(dst, memmove(dst, src, count)); \
        for (int i = 0; i < count; i++) { \
            VERIFY_ARE_EQUAL(dst[i], tmpbuf[i]); \
        } \
    } while (0)

    //
    // non-overlapping buffers (memcpy)
    // dst:         --------
    // src: ++++++++
    //
    TEST(32, 0);

    //
    // non-overlapping buffers (memcpy)
    // dst: --------
    // src:         ++++++++
    //
    TEST(0, 32);

    //
    // overlap from right
    // dst:   --------
    // src: ++++++++
    //
    TEST(8, 0);

    //
    // overlap from left
    // dst: --------
    // src:   ++++++++
    //
    TEST(0, 8);

    #undef TEST
}

void test_memcmp(void)
{
    char a[4];
    char b[4];
    const int count = 4;

    #define SETUP(x,y) \
    do {\
        memset(a,x,count); \
        memset(b,y,count); \
    } while (0)

    SETUP(1, 2);
    VERIFY_IS_TRUE(memcmp(a, b, 0) == 0);
    VERIFY_IS_TRUE(memcmp(a, b, count) < 0);
    SETUP(2, 1);
    VERIFY_IS_TRUE(memcmp(a, b, count) > 0);
    SETUP(2, 2);
    VERIFY_IS_TRUE(memcmp(a, b, count) == 0);

    #undef SETUP
}

void test_strcmp(void)
{
    char *a;
    char *b;

    #define SETUP(x,y) \
    do {\
        a = x; \
        b = y; \
    } while (0)

    SETUP("", "");
    VERIFY_IS_TRUE(strcmp(a, b) == 0);
    SETUP("", "a");
    VERIFY_IS_TRUE(strcmp(a, b) < 0);
    SETUP("a", "");
    VERIFY_IS_TRUE(strcmp(a, b) > 0);
    SETUP("a", "a");
    VERIFY_IS_TRUE(strcmp(a, b) == 0);
    SETUP("a", "abc");
    VERIFY_IS_TRUE(strcmp(a, b) < 0);
    SETUP("abc", "a");
    VERIFY_IS_TRUE(strcmp(a, b) > 0);
    SETUP("abc", "abc");
    VERIFY_IS_TRUE(strcmp(a, b) == 0);

    #undef SETUP
}

void test_strncmp(void)
{
    char *a;
    char *b;

    #define SETUP(x,y) \
    do {\
        a = x; \
        b = y; \
    } while (0)

    SETUP("", "");
    VERIFY_IS_TRUE(strncmp(a, b, 0) == 0);
    SETUP("", "a");
    VERIFY_IS_TRUE(strncmp(a, b, 1) < 0);       // TODO: verify behavior
    SETUP("a", "");
    VERIFY_IS_TRUE(strncmp(a, b, 1) > 0);
    SETUP("a", "a");
    VERIFY_IS_TRUE(strncmp(a, b, 1) == 0);
    SETUP("a", "a");
    VERIFY_IS_TRUE(strncmp(a, b, 2) == 0);
    SETUP("abc", "abc");
    VERIFY_IS_TRUE(strncmp(a, b, 1) == 0);
    SETUP("abc", "ayz");
    VERIFY_IS_TRUE(strncmp(a, b, 1) == 0);
    SETUP("abc", "abc");
    VERIFY_IS_TRUE(strncmp(a, b, 3) == 0);
    SETUP("abc", "ayz");
    VERIFY_IS_TRUE(strncmp(a, b, 3) < 0);
    SETUP("abc", "ayz");
    VERIFY_IS_TRUE(strncmp(a, b, 10) < 0);

    #undef SETUP
}

void test_strlen(void)
{
    VERIFY_ARE_EQUAL(0, strlen(""));
    VERIFY_ARE_EQUAL(13, strlen("Hello, world!"));
}

void test_strcpy(void)
{
    //
    // assumes memset, strlen, and strcmp work
    //

    char dst[64];
    void *ret;

    memset(dst, 'A', sizeof(dst));

    ret = strcpy(dst, "");
    VERIFY_ARE_EQUAL(dst, ret);
    VERIFY_ARE_EQUAL(0, strlen(ret));

    ret = strcpy(dst, "Test");
    VERIFY_ARE_EQUAL(dst, ret);
    VERIFY_ARE_EQUAL(4, strlen(ret));
    VERIFY_IS_ZERO(strcmp(dst, "Test"));
}

void test_strncpy(void)
{
    //
    // assumes memset, strlen, and strcmp work
    //

    char dst[64];
    void *ret;

    memset(dst, 'A', sizeof(dst));

    ret = strncpy(dst, "Test", 0);
    VERIFY_ARE_EQUAL(dst, ret);
    VERIFY_ARE_EQUAL(0, strlen(ret));

    ret = strncpy(dst, "Test", 2);
    VERIFY_ARE_EQUAL(dst, ret);
    VERIFY_ARE_EQUAL(2, strlen(ret));
    VERIFY_IS_ZERO(strcmp(dst, "Te"));

    ret = strncpy(dst, "Test", 6);
    VERIFY_ARE_EQUAL(dst, ret);
    VERIFY_ARE_EQUAL(4, strlen(ret));
    VERIFY_IS_ZERO(strcmp(dst, "Test"));
}


void test_string(void)
{
    // memset_reference();
    // memcmp_reference();
    // strcmp_reference();
    // strlen_reference();

    DECLARE_TEST("string.h");

    test_memset();
    test_memcpy();
    test_memmove();
    test_memcmp();
    test_strcmp();
    test_strncmp();
    test_strlen();
    test_strcpy();
    test_strncpy();
}
