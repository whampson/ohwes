#include <string.h>

// TODO: scrutinize these

void * memmove(void *dest, const void *src, size_t n)
{
    /* TODO: aligned move */
    __asm__ volatile (
        "                               \n\
        cld                             \n\
        cmpl    %%edi, %%esi            \n\
        jae     _memmove_start%=        \n\
        leal    -1(%%esi, %%ecx), %%esi \n\
        leal    -1(%%edi, %%ecx), %%edi \n\
        std                             \n\
    _memmove_start%=:                   \n\
        rep     movsb                   \n\
        "
        :
        : "D"(dest), "S"(src), "c"(n)
        : "edx", "cc", "memory"
    );

    return dest;
}

void * memcpy(void *dest, const void *src, size_t n)
{
    return memmove(dest, src, n);
}

char * strcpy(char *dest, const char *src)
{
    __asm__ volatile (
        "                               \n\
        cld                             \n\
    _strcpy_top%=:                      \n\
        lodsb                           \n\
        stosb                           \n\
        testb   %%al, %%al              \n\
        jz      _strcpy_done%=          \n\
        jmp     _strcpy_top%=           \n\
    _strcpy_done%=:                     \n\
        "
        :
        : "D"(dest), "S"(src)
        : "eax", "edx", "memory", "cc"
    );

    return dest;
}

char * strncpy(char *dest, const char *src, size_t n)
{
    __asm__ volatile (
        "                               \n\
        cld                             \n\
    _strncpy_str%=:                     \n\
        lodsb                           \n\
        stosb                           \n\
        decl    %%ecx                   \n\
        jz      _strncpy_done%=         \n\
        testb   %%al, %%al              \n\
        jz      _strncpy_zero%=         \n\
        jmp     _strncpy_str%=          \n\
    _strncpy_zero%=:                    \n\
        xorb    %%al, %%al              \n\
    _strncpy_zero0%=:                   \n\
        stosb                           \n\
        decl    %%ecx                   \n\
        jz      _strncpy_done%=         \n\
        jmp     _strncpy_zero0%=        \n\
    _strncpy_done%=:                    \n\
        "
        :
        : "D"(dest), "S"(src), "c"(n)
        : "eax", "edx", "memory", "cc"
    );

    return dest;
}

// char * strcat(char *dst, const char *src) { }
// char * strncat(char *dst, const char *src, size_t n) { }

int memcmp(const void *ptr1, const void *ptr2, size_t n)
{
    int result;
    __asm__ volatile (
        "                               \n\
        xorl    %%edx, %%edx            \n\
        cld                             \n\
        rep     cmpsb                   \n\
        jz      _memcmp_done%=          \n\
        jns     _memcmp_less%=          \n\
        incl    %%edx                   \n\
        jmp     _memcmp_done%=          \n\
    _memcmp_less%=:                     \n\
        decl    %%edx                   \n\
    _memcmp_done%=:                     \n\
        "
        : "=&d"(result)
        : "D"(ptr1), "S"(ptr2), "c"(n)
        : "eax", "memory", "cc"
    );

    return result;
}

int strcmp(const char *str1, const char *str2)
{
    int result;
    __asm__ volatile (
        "                               \n\
        xorl    %%edx, %%edx            \n\
        movl    $1, %%eax               \n\
        cld                             \n\
    _strcmp_top%=:                      \n\
        testb   %%al, %%al              \n\
        jz      _strcmp_done%=          \n\
        lodsb                           \n\
        movb    %%al, %%bl              \n\
        xchgl   %%esi, %%edi            \n\
        lodsb                           \n\
        xchgl   %%esi, %%edi            \n\
        cmpb    %%al, %%bl              \n\
        je      _strcmp_top%=           \n\
        jns     _strcmp_less%=          \n\
        incl    %%edx                   \n\
        jmp     _strcmp_done%=          \n\
    _strcmp_less%=:                     \n\
        decl    %%edx                   \n\
    _strcmp_done%=:                     \n\
        "
        : "=&d"(result)
        : "D"(str1), "S"(str2)
        : "eax", "ebx", "memory", "cc"
    );

    return result;
}

int strncmp(const char *str1, const char *str2, size_t n)
{
    int result;
    __asm__ volatile (
        "                               \n\
        xorl    %%edx, %%edx            \n\
        movl    $1, %%eax               \n\
        cld                             \n\
    _strncmp_top%=:                     \n\
        testb   %%al, %%al              \n\
        jz      _strncmp_done%=         \n\
        decl    %%ecx                   \n\
        js      _strncmp_done%=         \n\
        lodsb                           \n\
        movb    %%al, %%bl              \n\
        xchgl   %%esi, %%edi            \n\
        lodsb                           \n\
        xchgl   %%esi, %%edi            \n\
        cmpb    %%al, %%bl              \n\
        je      _strncmp_top%=          \n\
        jns     _strncmp_less%=         \n\
        incl    %%edx                   \n\
        jmp     _strncmp_done%=         \n\
    _strncmp_less%=:                    \n\
        decl    %%edx                   \n\
    _strncmp_done%=:                    \n\
        "
        : "=&d"(result)
        : "D"(str1), "S"(str2), "c"(n)
        : "eax", "ebx", "memory", "cc"
    );

    return result;
}

// int strcoll(const char *str1, const char *str2) { }
// int strxfrm(char *dest, const char *src, size_t n) { }

// void * memchr(const void *ptr, int value, size_t n) { }
// char * strchr(const char *str, int ch) { }
// char * strrchr(const char *str, int ch) { }
// size_t strspn(const char *str1, const char *str2) { }
// size_t strcspn(const char *str1, const char *str2) { }
// char * strpbrk(const char *str1, const char *str2) { }
// char * strstr(const char *str1, const char *str2) { }
// char * strtok(char *str, const char *delim) { }

void * memset(void *dest, int c, size_t n)
{
    unsigned char ch;
    ch = (unsigned char) c;

    __asm__ volatile (
        "                               \n\
    _memset_top%=:                      \n\
        testl   %%ecx, %%ecx            \n\
        jz      _memset_done%=          \n\
        testl   $3, %%edi               \n\
        jz      _memset_aligned%=       \n\
        movb    %%al, (%%edi)           \n\
        incl    %%edi                   \n\
        decl    %%ecx                   \n\
    _memset_aligned%=:                  \n\
        movl    %%ecx, %%edx            \n\
        shrl    $2, %%ecx               \n\
        andl    $3, %%edx               \n\
        cld                             \n\
        rep     stosl                   \n\
    _memset_bottom%=:                   \n\
        testl   %%edx, %%edx            \n\
        jz      _memset_done%=          \n\
        movb    %%al, (%%edi)           \n\
        incl    %%edi                   \n\
        decl    %%ecx                   \n\
    _memset_done%=:                     \n\
        "
        :
        : "D"(dest), "a"(ch << 24 | ch << 16 | ch << 8 | ch), "c"(n)
        : "edx", "memory", "cc"
    );

    return dest;
}

size_t strlen(const char *str)
{
    size_t len;

    __asm__ volatile (
        "                               \n\
        cld                             \n\
        xorl    %%ecx, %%ecx            \n\
    _strlen_top%=:                      \n\
        lodsb                           \n\
        testb   %%al, %%al              \n\
        jz      _strlen_done%=          \n\
        incl    %%ecx                   \n\
        jmp     _strlen_top%=           \n\
    _strlen_done%=:                     \n\
        "
        : "=c"(len)
        : "S"(str)
        : "eax", "cc"
    );

    return len;
}