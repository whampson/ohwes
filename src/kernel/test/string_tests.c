#include <stdio.h>
#include <string.h>

#include "test_libc.h"

#ifdef TEST_BUILD

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

bool test_strings(void)
{
    memset_reference();
    memcmp_reference();
    strcmp_reference();
    strlen_reference();

    return true;
}

#endif
