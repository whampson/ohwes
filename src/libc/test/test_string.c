/* =============================================================================
 * Copyright (C) 2020-2026 Wes Hampson. All Rights Reserved.
 *
 * This file is part of the OH-WES Operating System.
 * OH-WES is free software; you may redistribute it and/or modify it under the
 * terms of the GNU GPLv2. See the LICENSE file in the root of this repository.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -----------------------------------------------------------------------------
 *         File: src/libc/test/test_string.c
 *      Created: May 24, 2026
 *       Author: Wes Hampson
 *
 * libc string.h function tests
 * =============================================================================
 */

#include "framework.h"

/* ========================================================================= */
/*  strlen tests                                                             */
/* ========================================================================= */

static void test_strlen_empty(void)
{
    TEST("strlen: empty string");
    ASSERT_EQ_INT(0, (int)strlen(""), "expected 0");
    PASS();
}

static void test_strlen_basic(void)
{
    TEST("strlen: basic string");
    ASSERT_EQ_INT(5, (int)strlen("hello"), "expected 5");
    PASS();
}

static void test_strlen_single_char(void)
{
    TEST("strlen: single character");
    ASSERT_EQ_INT(1, (int)strlen("a"), "expected 1");
    PASS();
}

static void test_strlen_stops_at_nul(void)
{
    TEST("strlen: stops at embedded NUL");
    ASSERT_EQ_INT(3, (int)strlen("abc\0def"), "expected 3");
    PASS();
}

/* ========================================================================= */
/*  strcpy tests                                                             */
/* ========================================================================= */

static void test_strcpy_basic(void)
{
    TEST("strcpy: basic copy");
    char dest[32];
    strcpy(dest, "hello");
    ASSERT_EQ_STR("hello", dest, "mismatch");
    PASS();
}

static void test_strcpy_empty(void)
{
    TEST("strcpy: empty string");
    char dest[32] = "old";
    strcpy(dest, "");
    ASSERT_EQ_STR("", dest, "expected empty");
    PASS();
}

static void test_strcpy_returns_dest(void)
{
    TEST("strcpy: returns dest pointer");
    char dest[32];
    char *ret = strcpy(dest, "test");
    ASSERT(ret == dest, "return value should be dest");
    PASS();
}

static void test_strcpy_stops_at_nul(void)
{
    TEST("strcpy: stops at first NUL");
    char src[] = "abc\0xyz";
    char dest[8];
    memset(dest, 'Z', sizeof(dest));
    strcpy(dest, src);
    ASSERT_EQ_STR("abc", dest, "should stop at NUL");
    ASSERT(dest[4] == 'Z', "bytes past NUL+1 should be untouched");
    PASS();
}

/* ========================================================================= */
/*  strncpy tests                                                            */
/* ========================================================================= */

static void test_strncpy_truncation(void)
{
    TEST("strncpy: truncation when n < src length");
    char dest[4];
    strncpy(dest, "hello", 3);
    dest[3] = '\0';
    ASSERT(dest[0] == 'h' && dest[1] == 'e' && dest[2] == 'l', "wrong chars");
    PASS();
}

static void test_strncpy_zero_pads(void)
{
    TEST("strncpy: zero-pads when n > src length");
    char dest[8];
    memset(dest, 'X', sizeof(dest));
    strncpy(dest, "hi", 8);
    ASSERT(dest[2] == '\0' && dest[7] == '\0', "should be zero-padded");
    PASS();
}

static void test_strncpy_exact_length_no_nul(void)
{
    TEST("strncpy: n == strlen(src) does NOT append NUL");
    char dest[8];
    memset(dest, 'X', sizeof(dest));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy(dest, "abc", 3);
#pragma GCC diagnostic pop
    ASSERT(dest[0] == 'a' && dest[1] == 'b' && dest[2] == 'c', "copied");
    ASSERT(dest[3] == 'X', "no NUL appended when n == strlen(src)");
    PASS();
}

static void test_strncpy_exact_plus_one(void)
{
    TEST("strncpy: n == strlen(src)+1 includes NUL");
    char dest[8];
    memset(dest, 'X', sizeof(dest));
    strncpy(dest, "abc", 4);
    ASSERT_EQ_STR("abc", dest, "should include terminator");
    ASSERT(dest[4] == 'X', "no extra padding beyond n");
    PASS();
}

static void test_strncpy_zero_n(void)
{
    TEST("strncpy: n=0 does nothing");
    char dest[4] = "old";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy(dest, "new", 0);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("old", dest, "should be unchanged");
    PASS();
}

static void test_strncpy_full_zero_pad_range(void)
{
    TEST("strncpy: verifies all bytes in zero-pad range");
    char dest[8];
    memset(dest, 'X', sizeof(dest));
    strncpy(dest, "ab", 8);
    ASSERT(dest[0] == 'a' && dest[1] == 'b', "copied chars");
    for (int i = 2; i < 8; i++)
        ASSERT(dest[i] == '\0', "should be zero-padded");
    PASS();
}

static void test_strncpy_embedded_nul_source(void)
{
    TEST("strncpy: stops at first NUL, zero-pads remainder");
    char dest[8];
    memset(dest, 'X', sizeof(dest));
    strncpy(dest, "a\0b", 5);
    ASSERT(dest[0] == 'a', "first char copied");
    ASSERT(dest[1] == '\0', "NUL from source");
    ASSERT(dest[2] == '\0', "zero-padded past source NUL");
    ASSERT(dest[3] == '\0', "zero-padded");
    ASSERT(dest[4] == '\0', "zero-padded to n");
    ASSERT(dest[5] == 'X', "beyond n untouched");
    PASS();
}

/* ========================================================================= */
/*  strcat tests                                                             */
/* ========================================================================= */

static void test_strcat_basic(void)
{
    TEST("strcat: basic concatenation");
    char dest[32] = "hello";
    strcat(dest, " world");
    ASSERT_EQ_STR("hello world", dest, "mismatch");
    PASS();
}

static void test_strcat_to_empty(void)
{
    TEST("strcat: append to empty string");
    char dest[32] = "";
    strcat(dest, "hello");
    ASSERT_EQ_STR("hello", dest, "mismatch");
    PASS();
}

static void test_strcat_empty_src(void)
{
    TEST("strcat: append empty string");
    char dest[32] = "hello";
    strcat(dest, "");
    ASSERT_EQ_STR("hello", dest, "should be unchanged");
    PASS();
}

/* ========================================================================= */
/*  strncat tests                                                            */
/* ========================================================================= */

static void test_strncat_basic(void)
{
    TEST("strncat: basic concatenation");
    char dest[32] = "hello";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
    strncat(dest, " world", 6);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("hello world", dest, "mismatch");
    PASS();
}

static void test_strncat_truncated(void)
{
    TEST("strncat: truncated append");
    char dest[32] = "hello";
    strncat(dest, " world", 3);
    ASSERT_EQ_STR("hello wo", dest, "mismatch");
    PASS();
}

static void test_strncat_zero_n(void)
{
    TEST("strncat: n=0 does nothing");
    char dest[32] = "hello";
    strncat(dest, " world", 0);
    ASSERT_EQ_STR("hello", dest, "should be unchanged");
    PASS();
}

static void test_strncat_n_greater_than_src(void)
{
    TEST("strncat: n >= strlen(src) appends full src");
    char dest[32] = "hello";
    strncat(dest, " hi", 10);
    ASSERT_EQ_STR("hello hi", dest, "full src appended");
    PASS();
}

static void test_strncat_post_terminator_untouched(void)
{
    TEST("strncat: bytes after NUL terminator untouched");
    char dest[16];
    memset(dest, 'Z', sizeof(dest));
    dest[0] = 'a';
    dest[1] = '\0';
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
    strncat(dest, "bc", 2);
#pragma GCC diagnostic pop
    ASSERT_EQ_STR("abc", dest, "appended");
    ASSERT(dest[4] == 'Z', "byte after NUL untouched");
    PASS();
}

/* ========================================================================= */
/*  strcmp tests                                                              */
/* ========================================================================= */

static void test_strcmp_equal(void)
{
    TEST("strcmp: equal strings");
    ASSERT_EQ_INT(0, strcmp("hello", "hello"), "should be 0");
    PASS();
}

static void test_strcmp_less(void)
{
    TEST("strcmp: first < second");
    ASSERT(strcmp("abc", "abd") < 0, "should be negative");
    PASS();
}

static void test_strcmp_greater(void)
{
    TEST("strcmp: first > second");
    ASSERT(strcmp("abd", "abc") > 0, "should be positive");
    PASS();
}

static void test_strcmp_empty(void)
{
    TEST("strcmp: both empty");
    ASSERT_EQ_INT(0, strcmp("", ""), "should be 0");
    PASS();
}

static void test_strcmp_prefix(void)
{
    TEST("strcmp: prefix comparison");
    ASSERT(strcmp("hello", "hello world") < 0, "shorter < longer");
    PASS();
}

static void test_strcmp_unsigned_chars(void)
{
    TEST("strcmp: compares as unsigned char");
    ASSERT(strcmp("\xFF", "\x01") > 0, "0xFF > 0x01 unsigned");
    PASS();
}

static void test_strcmp_stops_at_nul(void)
{
    TEST("strcmp: stops at embedded NUL");
    ASSERT_EQ_INT(0, strcmp("abc\0x", "abc\0y"), "should be equal");
    PASS();
}

/* ========================================================================= */
/*  strncmp tests                                                            */
/* ========================================================================= */

static void test_strncmp_equal_within_n(void)
{
    TEST("strncmp: equal within n");
    ASSERT_EQ_INT(0, strncmp("hello", "hello world", 5), "should be 0");
    PASS();
}

static void test_strncmp_diff_within_n(void)
{
    TEST("strncmp: differ within n");
    ASSERT(strncmp("abc", "abd", 3) < 0, "should be negative");
    PASS();
}

static void test_strncmp_zero_n(void)
{
    TEST("strncmp: n=0 always equal");
    ASSERT_EQ_INT(0, strncmp("abc", "xyz", 0), "should be 0");
    PASS();
}

static void test_strncmp_unsigned_chars(void)
{
    TEST("strncmp: compares as unsigned char");
    ASSERT(strncmp("\x80", "\x7F", 1) > 0, "0x80 > 0x7F unsigned");
    PASS();
}

static void test_strncmp_stops_at_nul_before_n(void)
{
    TEST("strncmp: stops at NUL before n is reached");
    ASSERT(strncmp("abc", "abcx", 10) < 0, "shorter string < longer");
    PASS();
}

static void test_strncmp_embedded_nul(void)
{
    TEST("strncmp: embedded NUL stops comparison");
    ASSERT_EQ_INT(0, strncmp("abc\0x", "abc\0y", 5), "equal up to NUL");
    PASS();
}

#if TEST_STRCHR
/* ========================================================================= */
/*  strchr tests                                                             */
/* ========================================================================= */

static void test_strchr_found(void)
{
    TEST("strchr: character found");
    const char *s = "hello";
    const char *p = strchr(s, 'l');
    ASSERT(p == s + 2, "should point to first 'l'");
    PASS();
}

static void test_strchr_not_found(void)
{
    TEST("strchr: character not found");
    ASSERT(strchr("hello", 'z') == NULL, "should be NULL");
    PASS();
}

static void test_strchr_null_terminator(void)
{
    TEST("strchr: finds null terminator");
    const char *s = "hello";
    const char *p = strchr(s, '\0');
    ASSERT(p == s + 5, "should point to '\\0'");
    PASS();
}

static void test_strchr_first_char(void)
{
    TEST("strchr: finds first character");
    const char *s = "hello";
    ASSERT(strchr(s, 'h') == s, "should point to start");
    PASS();
}

static void test_strchr_last_char(void)
{
    TEST("strchr: finds last character");
    const char *s = "abc";
    ASSERT(strchr(s, 'c') == s + 2, "should find at end");
    PASS();
}

static void test_strchr_high_byte(void)
{
    TEST("strchr: finds high-byte value (0xFF)");
    char buf[] = {(char)0x80, (char)0xFF, '\0'};
    char *p = strchr(buf, 0xFF);
    ASSERT(p == buf + 1, "should find 0xFF");
    PASS();
}

static void test_strchr_0x80(void)
{
    TEST("strchr: finds 0x80 byte");
    char buf[] = {(char)0x41, (char)0x80, '\0'};
    char *p = strchr(buf, 0x80);
    ASSERT(p == buf + 1, "should find 0x80");
    PASS();
}

/* ========================================================================= */
/*  strrchr tests                                                            */
/* ========================================================================= */

static void test_strrchr_found(void)
{
    TEST("strrchr: last occurrence");
    const char *s = "hello";
    const char *p = strrchr(s, 'l');
    ASSERT(p == s + 3, "should point to last 'l'");
    PASS();
}

static void test_strrchr_not_found(void)
{
    TEST("strrchr: character not found");
    ASSERT(strrchr("hello", 'z') == NULL, "should be NULL");
    PASS();
}

static void test_strrchr_null_terminator(void)
{
    TEST("strrchr: finds null terminator");
    const char *s = "hello";
    ASSERT(strrchr(s, '\0') == s + 5, "should point to NUL");
    PASS();
}

static void test_strrchr_high_byte(void)
{
    TEST("strrchr: finds high-byte value (0xFF)");
    char buf[] = {(char)0xFF, 'a', (char)0xFF, '\0'};
    char *p = strrchr(buf, 0xFF);
    ASSERT(p == buf + 2, "should find last 0xFF");
    PASS();
}

static void test_strrchr_0x80(void)
{
    TEST("strrchr: finds last 0x80 byte");
    char buf[] = {(char)0x80, (char)0x41, (char)0x80, '\0'};
    char *p = strrchr(buf, 0x80);
    ASSERT(p == buf + 2, "should find last 0x80");
    PASS();
}
#endif

#if TEST_STRSTR
/* ========================================================================= */
/*  strstr tests                                                             */
/* ========================================================================= */

static void test_strstr_found(void)
{
    TEST("strstr: substring found");
    const char *s = "hello world";
    const char *p = strstr(s, "world");
    ASSERT(p == s + 6, "should point to 'world'");
    PASS();
}

static void test_strstr_not_found(void)
{
    TEST("strstr: substring not found");
    ASSERT(strstr("hello", "xyz") == NULL, "should be NULL");
    PASS();
}

static void test_strstr_empty_needle(void)
{
    TEST("strstr: empty needle returns haystack");
    const char *s = "hello";
    ASSERT(strstr(s, "") == s, "should return haystack");
    PASS();
}

static void test_strstr_full_match(void)
{
    TEST("strstr: full string match");
    const char *s = "hello";
    ASSERT(strstr(s, "hello") == s, "should return start");
    PASS();
}

static void test_strstr_partial_overlap(void)
{
    TEST("strstr: partial match then real match");
    const char *s = "aab";
    ASSERT(strstr(s, "ab") == s + 1, "should find 'ab' at index 1");
    PASS();
}

static void test_strstr_repeated_prefix(void)
{
    TEST("strstr: repeated prefix overlap");
    const char *s = "abababc";
    const char *p = strstr(s, "ababc");
    ASSERT(p == s + 2, "should find at index 2");
    PASS();
}

static void test_strstr_needle_longer_than_haystack(void)
{
    TEST("strstr: needle longer than haystack");
    ASSERT(strstr("abc", "abcd") == NULL, "should be NULL");
    PASS();
}

static void test_strstr_empty_haystack(void)
{
    TEST("strstr: empty haystack, non-empty needle");
    ASSERT(strstr("", "a") == NULL, "should be NULL");
    PASS();
}

static void test_strstr_stops_at_nul(void)
{
    TEST("strstr: stops at NUL in haystack");
    ASSERT(strstr("abc\0def", "def") == NULL, "should not find past NUL");
    PASS();
}
#endif

/* ========================================================================= */
/*  strtok tests                                                             */
/* ========================================================================= */

static void test_strtok_basic(void)
{
    TEST("strtok: basic tokenization");
    char str[] = "hello world foo";
    char *tok = strtok(str, " ");
    ASSERT(tok != NULL && strcmp(tok, "hello") == 0, "first token");
    tok = strtok(NULL, " ");
    ASSERT(tok != NULL && strcmp(tok, "world") == 0, "second token");
    tok = strtok(NULL, " ");
    ASSERT(tok != NULL && strcmp(tok, "foo") == 0, "third token");
    tok = strtok(NULL, " ");
    ASSERT(tok == NULL, "should be NULL after last token");
    PASS();
}

static void test_strtok_multiple_delimiters(void)
{
    TEST("strtok: multiple delimiter characters");
    char str[] = "one,two;three";
    char *tok = strtok(str, ",;");
    ASSERT(tok != NULL && strcmp(tok, "one") == 0, "first token");
    tok = strtok(NULL, ",;");
    ASSERT(tok != NULL && strcmp(tok, "two") == 0, "second token");
    tok = strtok(NULL, ",;");
    ASSERT(tok != NULL && strcmp(tok, "three") == 0, "third token");
    PASS();
}

static void test_strtok_consecutive_delimiters(void)
{
    TEST("strtok: consecutive delimiters skipped");
    char str[] = "a,,b";
    char *tok = strtok(str, ",");
    ASSERT(tok != NULL && strcmp(tok, "a") == 0, "first token");
    tok = strtok(NULL, ",");
    ASSERT(tok != NULL && strcmp(tok, "b") == 0, "second token (skips empty)");
    PASS();
}

static void test_strtok_empty_string(void)
{
    TEST("strtok: empty string returns NULL");
    char str[] = "";
    ASSERT(strtok(str, ",") == NULL, "should be NULL");
    PASS();
}

static void test_strtok_all_delimiters(void)
{
    TEST("strtok: all-delimiter string returns NULL");
    char str[] = ",,,";
    ASSERT(strtok(str, ",") == NULL, "should be NULL");
    PASS();
}

static void test_strtok_leading_trailing_delimiters(void)
{
    TEST("strtok: leading and trailing delimiters");
    char str[] = ",a,b,";
    char *tok = strtok(str, ",");
    ASSERT(tok != NULL && strcmp(tok, "a") == 0, "first token");
    tok = strtok(NULL, ",");
    ASSERT(tok != NULL && strcmp(tok, "b") == 0, "second token");
    tok = strtok(NULL, ",");
    ASSERT(tok == NULL, "no more tokens");
    PASS();
}

static void test_strtok_no_delimiter(void)
{
    TEST("strtok: no delimiter in string");
    char str[] = "abc";
    char *tok = strtok(str, ",");
    ASSERT(tok != NULL && strcmp(tok, "abc") == 0, "whole string is token");
    tok = strtok(NULL, ",");
    ASSERT(tok == NULL, "no more tokens");
    PASS();
}

static void test_strtok_overwrites_delimiter(void)
{
    TEST("strtok: overwrites delimiter with NUL");
    char str[] = "a,b";
    strtok(str, ",");
    ASSERT(str[1] == '\0', "delimiter should be replaced with NUL");
    PASS();
}

static void test_strtok_change_delimiter(void)
{
    TEST("strtok: different delimiters between calls");
    char str[] = "a,b;c";
    char *tok = strtok(str, ",");
    ASSERT(tok != NULL && strcmp(tok, "a") == 0, "first token");
    tok = strtok(NULL, ";");
    ASSERT(tok != NULL && strcmp(tok, "b") == 0, "second token (comma delim)");
    tok = strtok(NULL, ";");
    ASSERT(tok != NULL && strcmp(tok, "c") == 0, "third token (semicolon delim)");
    PASS();
}

static void test_strtok_reset_mid_sequence(void)
{
    TEST("strtok: new string resets mid-sequence");
    char str1[] = "a,b,c";
    char str2[] = "x;y";
    strtok(str1, ",");
    /* Reset with new string before exhausting str1 */
    char *tok = strtok(str2, ";");
    ASSERT(tok != NULL && strcmp(tok, "x") == 0, "first from new string");
    tok = strtok(NULL, ";");
    ASSERT(tok != NULL && strcmp(tok, "y") == 0, "second from new string");
    tok = strtok(NULL, ";");
    ASSERT(tok == NULL, "exhausted");
    PASS();
}

static void test_strtok_reset_abandons_previous(void)
{
    TEST("strtok: reset abandons previous string completely");
    char str1[] = "a,b,c";
    char str2[] = "x;y";
    strtok(str1, ",");  /* returns "a", str1 state has more tokens */
    strtok(str2, ";");  /* reset to str2, abandons str1 */
    strtok(NULL, ";");  /* should get "y" from str2, NOT "b" from str1 */
    char *tok = strtok(NULL, ";");
    ASSERT(tok == NULL, "str2 exhausted, str1 not resumed");
    PASS();
}

static void test_strtok_empty_delimiter(void)
{
    TEST("strtok: empty delimiter string returns whole string");
    char str[] = "hello";
    char *tok = strtok(str, "");
    ASSERT(tok != NULL && strcmp(tok, "hello") == 0, "whole string is token");
    tok = strtok(NULL, "");
    ASSERT(tok == NULL, "no more tokens");
    PASS();
}

static void test_strtok_null_before_init(void)
{
    TEST("strtok: NULL arg after exhaustion of prior sequence");
    /* Exhaust a sequence first, then call NULL again — should stay NULL */
    char str[] = "x";
    strtok(str, ",");
    strtok(NULL, ","); /* exhausted */
    /* Now call with NULL — no active string, should return NULL */
    ASSERT(strtok(NULL, ",") == NULL, "NULL after exhaustion");
    PASS();
}

/* ========================================================================= */
/*  memcpy tests                                                             */
/* ========================================================================= */

static void test_memcpy_basic(void)
{
    TEST("memcpy: basic copy");
    char src[] = "hello";
    char dest[8];
    memcpy(dest, src, 6);
    ASSERT_EQ_STR("hello", dest, "mismatch");
    PASS();
}

static void test_memcpy_partial(void)
{
    TEST("memcpy: partial copy preserves trailing bytes");
    char src[] = "hello world";
    char dest[8];
    memset(dest, 'Z', sizeof(dest));
    memcpy(dest, src, 5);
    ASSERT(memcmp(dest, "hello", 5) == 0, "copied region mismatch");
    ASSERT(dest[5] == 'Z' && dest[6] == 'Z' && dest[7] == 'Z',
           "bytes after n should be untouched");
    PASS();
}

static void test_memcpy_returns_dest(void)
{
    TEST("memcpy: returns dest pointer");
    char src[] = "hi";
    char dest[4];
    void *ret = memcpy(dest, src, 3);
    ASSERT(ret == dest, "return value should be dest");
    PASS();
}

static void test_memcpy_zero_bytes(void)
{
    TEST("memcpy: zero bytes does nothing");
    char dest[4] = "abc";
    memcpy(dest, "xyz", 0);
    ASSERT_EQ_STR("abc", dest, "should be unchanged");
    PASS();
}

static void test_memcpy_binary_data(void)
{
    TEST("memcpy: copies binary data including NUL/0xFF");
    unsigned char src[] = {0x00, 0xFF, 0x80, 0x01};
    unsigned char dest[4];
    memcpy(dest, src, 4);
    ASSERT(dest[0] == 0x00 && dest[1] == 0xFF &&
           dest[2] == 0x80 && dest[3] == 0x01, "binary mismatch");
    PASS();
}

static void test_memcpy_large_n(void)
{
    TEST("memcpy: large count (1024 bytes)");
    unsigned char src[1024], dest[1024];
    for (int i = 0; i < 1024; i++)
        src[i] = (unsigned char)(i & 0xFF);
    memcpy(dest, src, 1024);
    ASSERT(memcmp(dest, src, 1024) == 0, "all 1024 bytes match");
    PASS();
}

/* ========================================================================= */
/*  memmove tests                                                            */
/* ========================================================================= */

static void test_memmove_nonoverlapping(void)
{
    TEST("memmove: non-overlapping");
    char src[] = "hello";
    char dest[8];
    memmove(dest, src, 6);
    ASSERT_EQ_STR("hello", dest, "mismatch");
    PASS();
}

static void test_memmove_overlap_forward(void)
{
    TEST("memmove: overlapping forward");
    char buf[] = "abcdef";
    memmove(buf + 2, buf, 4);  /* "ab" + "abcd" + trailing */
    ASSERT(buf[0] == 'a' && buf[1] == 'b', "prefix preserved");
    ASSERT(buf[2] == 'a' && buf[3] == 'b' && buf[4] == 'c' && buf[5] == 'd',
           "moved region correct");
    PASS();
}

static void test_memmove_overlap_backward(void)
{
    TEST("memmove: overlapping backward");
    char buf[] = "abcdef";
    memmove(buf, buf + 2, 4);  /* "cdef" overwrites beginning */
    ASSERT(buf[0] == 'c' && buf[1] == 'd' && buf[2] == 'e' && buf[3] == 'f',
           "moved region correct");
    PASS();
}

static void test_memmove_same_pointer(void)
{
    TEST("memmove: dest == src is a no-op, returns dest");
    char buf[] = "hello";
    void *ret = memmove(buf, buf, 5);
    ASSERT(ret == buf, "return value should be dest");
    ASSERT_EQ_STR("hello", buf, "should be unchanged");
    PASS();
}

static void test_memmove_binary_overlap(void)
{
    TEST("memmove: overlap with embedded zeros");
    unsigned char buf[] = {0x00, 0xFF, 0x80, 0x01, 0x00};
    memmove(buf + 1, buf, 4);
    ASSERT(buf[1] == 0x00 && buf[2] == 0xFF &&
           buf[3] == 0x80 && buf[4] == 0x01, "binary overlap");
    PASS();
}

static void test_memmove_zero_n(void)
{
    TEST("memmove: n=0 does nothing");
    char buf[] = "hello";
    void *ret = memmove(buf + 2, buf, 0);
    ASSERT(ret == buf + 2, "returns dest");
    ASSERT_EQ_STR("hello", buf, "unchanged");
    PASS();
}

static void test_memmove_overlap_forward_returns_dest(void)
{
    TEST("memmove: overlap forward returns dest");
    char buf[] = "abcdef";
    void *ret = memmove(buf + 2, buf, 4);
    ASSERT(ret == buf + 2, "returns dest on forward overlap");
    PASS();
}

static void test_memmove_large_overlap(void)
{
    TEST("memmove: large overlapping move (512 bytes)");
    unsigned char buf[1024];
    for (int i = 0; i < 1024; i++)
        buf[i] = (unsigned char)(i & 0xFF);
    memmove(buf + 256, buf, 512);
    for (int i = 0; i < 512; i++)
        ASSERT(buf[256 + i] == (unsigned char)(i & 0xFF), "overlap mismatch");
    PASS();
}

/* ========================================================================= */
/*  memset tests                                                             */
/* ========================================================================= */

static void test_memset_basic(void)
{
    TEST("memset: fill with character");
    char buf[8];
    memset(buf, 'A', 7);
    buf[7] = '\0';
    ASSERT_EQ_STR("AAAAAAA", buf, "mismatch");
    PASS();
}

static void test_memset_zero(void)
{
    TEST("memset: zero fill");
    char buf[4] = "abc";
    memset(buf, 0, 4);
    ASSERT(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0,
           "all bytes should be 0");
    PASS();
}

static void test_memset_returns_dest(void)
{
    TEST("memset: returns dest pointer");
    char buf[4];
    void *ret = memset(buf, 0, 4);
    ASSERT(ret == buf, "return value should be dest");
    PASS();
}

static void test_memset_value_truncation(void)
{
    TEST("memset: int value truncated to unsigned char");
    unsigned char buf[4];
    memset(buf, 0x123, 4);
    ASSERT(buf[0] == 0x23 && buf[1] == 0x23 &&
           buf[2] == 0x23 && buf[3] == 0x23, "should fill with low byte");
    PASS();
}

static void test_memset_0xff(void)
{
    TEST("memset: fill with 0xFF");
    unsigned char buf[4];
    memset(buf, 0xFF, 4);
    ASSERT(buf[0] == 0xFF && buf[1] == 0xFF &&
           buf[2] == 0xFF && buf[3] == 0xFF, "all 0xFF");
    PASS();
}

static void test_memset_zero_length(void)
{
    TEST("memset: n=0 does nothing");
    char buf[4] = "abc";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmemset-transposed-args"
    void *ret = memset(buf, 'X', 0);
#pragma GCC diagnostic pop
    ASSERT(ret == buf, "return value should be dest");
    ASSERT_EQ_STR("abc", buf, "should be unchanged");
    PASS();
}

static void test_memset_partial(void)
{
    TEST("memset: partial fill");
    char buf[8] = "abcdefg";
    memset(buf, 'X', 3);
    ASSERT(buf[0] == 'X' && buf[1] == 'X' && buf[2] == 'X', "filled region");
    ASSERT(buf[3] == 'd', "rest unchanged");
    PASS();
}

static void test_memset_large_n(void)
{
    TEST("memset: large count (1024 bytes)");
    unsigned char buf[1024];
    memset(buf, 0xAB, 1024);
    for (int i = 0; i < 1024; i++)
        ASSERT(buf[i] == 0xAB, "all bytes should be 0xAB");
    PASS();
}

/* ========================================================================= */
/*  memcmp tests                                                             */
/* ========================================================================= */

static void test_memcmp_equal(void)
{
    TEST("memcmp: equal regions");
    ASSERT_EQ_INT(0, memcmp("hello", "hello", 5), "should be 0");
    PASS();
}

static void test_memcmp_less(void)
{
    TEST("memcmp: first < second");
    ASSERT(memcmp("abc", "abd", 3) < 0, "should be negative");
    PASS();
}

static void test_memcmp_greater(void)
{
    TEST("memcmp: first > second");
    ASSERT(memcmp("abd", "abc", 3) > 0, "should be positive");
    PASS();
}

static void test_memcmp_partial(void)
{
    TEST("memcmp: partial comparison");
    ASSERT_EQ_INT(0, memcmp("hello world", "hello xyz", 5), "first 5 equal");
    PASS();
}

static void test_memcmp_unsigned_bytes(void)
{
    TEST("memcmp: compares as unsigned bytes");
    ASSERT(memcmp("\xFF", "\x00", 1) > 0, "0xFF > 0x00 unsigned");
    PASS();
}

static void test_memcmp_through_nuls(void)
{
    TEST("memcmp: compares through embedded NULs");
    unsigned char a[] = {1, 0, 3};
    unsigned char b[] = {1, 0, 4};
    ASSERT(memcmp(a, b, 3) < 0, "differs after embedded NUL");
    ASSERT_EQ_INT(0, memcmp(a, a, 3), "same through NULs");
    PASS();
}

static void test_memcmp_sign_boundary(void)
{
    TEST("memcmp: 0x7F vs 0x80 unsigned boundary");
    unsigned char a[] = {0x7F};
    unsigned char b[] = {0x80};
    ASSERT(memcmp(a, b, 1) < 0, "0x7F < 0x80 as unsigned");
    ASSERT(memcmp(b, a, 1) > 0, "0x80 > 0x7F as unsigned");
    PASS();
}

#if TEST_MEMCHR
/* ========================================================================= */
/*  memchr tests                                                             */
/* ========================================================================= */

static void test_memchr_found(void)
{
    TEST("memchr: byte found");
    const char *s = "hello";
    ASSERT(memchr(s, 'l', 5) == s + 2, "should find first 'l'");
    PASS();
}

static void test_memchr_not_found(void)
{
    TEST("memchr: byte not found");
    ASSERT(memchr("hello", 'z', 5) == NULL, "should be NULL");
    PASS();
}

static void test_memchr_finds_null(void)
{
    TEST("memchr: finds null byte");
    const char *s = "he\0lo";
    ASSERT(memchr(s, '\0', 5) == s + 2, "should find embedded null");
    PASS();
}

static void test_memchr_zero_n(void)
{
    TEST("memchr: n=0 returns NULL");
    ASSERT(memchr("hello", 'h', 0) == NULL, "should be NULL");
    PASS();
}

static void test_memchr_at_last_byte(void)
{
    TEST("memchr: finds byte at position n-1");
    const char *s = "abcd";
    ASSERT(memchr(s, 'd', 4) == s + 3, "should find at last position");
    PASS();
}

static void test_memchr_0xff(void)
{
    TEST("memchr: finds 0xFF byte");
    unsigned char buf[] = {0x00, 0x80, 0xFF, 0x01};
    ASSERT(memchr(buf, 0xFF, 4) == buf + 2, "should find 0xFF");
    PASS();
}

static void test_memchr_byte_truncation(void)
{
    TEST("memchr: search byte truncated to unsigned char");
    unsigned char buf[] = {0x23, 0x00};
    /* 0x123 should be truncated to 0x23 */
    ASSERT(memchr(buf, 0x123, 2) == buf, "truncated to low byte");
    PASS();
}

static void test_memchr_0x80(void)
{
    TEST("memchr: finds 0x80 byte");
    unsigned char buf[] = {0x41, 0x80, 0x00};
    ASSERT(memchr(buf, 0x80, 3) == buf + 1, "should find 0x80");
    PASS();
}
#endif

/* ========================================================================= */
/*  Return-value contract tests                                              */
/* ========================================================================= */

#if TEST_STRCHR
static void test_strchr_returns_into_source(void)
{
    TEST("strchr: returned pointer is within source string");
    char str[] = "abcdef";
    char *p = strchr(str, 'd');
    ASSERT(p == str + 3, "points to correct offset");
    *p = 'X';  /* modify through returned pointer */
    ASSERT_EQ_STR("abcXef", str, "modification through pointer works");
    PASS();
}

static void test_strrchr_returns_into_source(void)
{
    TEST("strrchr: returned pointer is within source string");
    char str[] = "abcabc";
    char *p = strrchr(str, 'a');
    ASSERT(p == str + 3, "points to last occurrence");
    *p = 'X';
    ASSERT_EQ_STR("abcXbc", str, "modification through pointer works");
    PASS();
}
#endif

#if TEST_STRSTR
static void test_strstr_returns_into_haystack(void)
{
    TEST("strstr: returned pointer is within haystack");
    char hay[] = "hello world";
    char *p = strstr(hay, "world");
    ASSERT(p == hay + 6, "points to correct offset");
    *p = 'W';
    ASSERT_EQ_STR("hello World", hay, "modification through pointer works");
    PASS();
}
#endif

#if TEST_MEMCHR
static void test_memchr_returns_into_buffer(void)
{
    TEST("memchr: returned pointer is within buffer");
    unsigned char buf[] = {10, 20, 30, 40, 50};
    unsigned char *p = memchr(buf, 30, 5);
    ASSERT(p == buf + 2, "points to correct offset");
    *p = 99;
    ASSERT(buf[2] == 99, "modification through pointer works");
    PASS();
}
#endif

static void test_strtok_returns_into_source(void)
{
    TEST("strtok: returned pointers are within source string");
    char str[] = "a,b,c";
    char *tok1 = strtok(str, ",");
    char *tok2 = strtok(NULL, ",");
    char *tok3 = strtok(NULL, ",");
    ASSERT(tok1 == str, "first token at start of string");
    ASSERT(tok2 == str + 2, "second token at offset 2");
    ASSERT(tok3 == str + 4, "third token at offset 4");
    PASS();
}

static void test_strncpy_returns_dest(void)
{
    TEST("strncpy: returns dest pointer");
    char src[] = "hello";
    char dest[10];
    char *ret = strncpy(dest, src, sizeof(dest));
    ASSERT(ret == dest, "returns dest");
    ASSERT_EQ_STR("hello", dest, "content correct");
    PASS();
}

static void test_strcat_returns_dest(void)
{
    TEST("strcat: returns dest pointer");
    char dest[20] = "hello";
    char *ret = strcat(dest, " world");
    ASSERT(ret == dest, "returns dest");
    ASSERT_EQ_STR("hello world", dest, "content correct");
    PASS();
}

static void test_strncat_returns_dest(void)
{
    TEST("strncat: returns dest pointer");
    char dest[20] = "hello";
    char *ret = strncat(dest, " world", 4);
    ASSERT(ret == dest, "returns dest");
    ASSERT_EQ_STR("hello wor", dest, "content correct");
    PASS();
}

#if TEST_STRSPN
/* ========================================================================= */
/*  strspn                                                                   */
/* ========================================================================= */

static void test_strspn_all_match(void)
{
    TEST("strspn: entire string matches accept set");
    ASSERT(strspn("aabba", "ab") == 5, "all chars in set");
    PASS();
}

static void test_strspn_partial(void)
{
    TEST("strspn: partial match");
    ASSERT(strspn("aabcba", "ab") == 3, "stops at 'c'");
    PASS();
}

static void test_strspn_no_match(void)
{
    TEST("strspn: no chars match");
    ASSERT(strspn("xyz", "ab") == 0, "none in set");
    PASS();
}

static void test_strspn_empty_string(void)
{
    TEST("strspn: empty string returns 0");
    ASSERT(strspn("", "abc") == 0, "empty string");
    PASS();
}

static void test_strspn_empty_accept(void)
{
    TEST("strspn: empty accept set returns 0");
    ASSERT(strspn("hello", "") == 0, "empty accept");
    PASS();
}

/* ========================================================================= */
/*  strcspn                                                                  */
/* ========================================================================= */

static void test_strcspn_no_reject(void)
{
    TEST("strcspn: no reject chars found");
    ASSERT(strcspn("hello", "xyz") == 5, "full length");
    PASS();
}

static void test_strcspn_immediate_reject(void)
{
    TEST("strcspn: first char rejected");
    ASSERT(strcspn("hello", "h") == 0, "immediate reject");
    PASS();
}

static void test_strcspn_partial(void)
{
    TEST("strcspn: reject in middle");
    ASSERT(strcspn("hello world", " ") == 5, "stops at space");
    PASS();
}

static void test_strcspn_empty_string(void)
{
    TEST("strcspn: empty string returns 0");
    ASSERT(strcspn("", "abc") == 0, "empty string");
    PASS();
}

static void test_strcspn_empty_reject(void)
{
    TEST("strcspn: empty reject set returns strlen");
    ASSERT(strcspn("hello", "") == 5, "empty reject = strlen");
    PASS();
}
#endif

#if TEST_STRPBRK
/* ========================================================================= */
/*  strpbrk                                                                  */
/* ========================================================================= */

static void test_strpbrk_found(void)
{
    TEST("strpbrk: finds first occurrence from accept set");
    const char *s = "hello world";
    char *p = strpbrk(s, "ow");
    ASSERT(p == s + 4, "first 'o' at index 4");
    PASS();
}

static void test_strpbrk_not_found(void)
{
    TEST("strpbrk: returns NULL when no match");
    ASSERT(strpbrk("hello", "xyz") == NULL, "no match");
    PASS();
}

static void test_strpbrk_first_char(void)
{
    TEST("strpbrk: match at first char");
    const char *s = "hello";
    ASSERT(strpbrk(s, "h") == s, "match at start");
    PASS();
}

static void test_strpbrk_empty_accept(void)
{
    TEST("strpbrk: empty accept set returns NULL");
    ASSERT(strpbrk("hello", "") == NULL, "empty accept");
    PASS();
}

static void test_strpbrk_returns_into_source(void)
{
    TEST("strpbrk: returned pointer is within source");
    char str[] = "hello world";
    char *p = strpbrk(str, "w");
    ASSERT(p == str + 6, "points into source");
    *p = 'W';
    ASSERT_EQ_STR("hello World", str, "modification works");
    PASS();
}
#endif

/* ========================================================================= */
/*  cross-function invariants                                                */
/* ========================================================================= */

static void test_strlen_sprintf_consistency(void)
{
    TEST("cross: strlen(s) == sprintf(buf, \"%s\", s)");
    const char *s = "hello world";
    char buf[64];
    int ret = sprintf(buf, "%s", s);
    ASSERT_EQ_INT((int)strlen(s), ret, "strlen matches sprintf return");
    ASSERT_EQ_STR(s, buf, "content matches");
    PASS();
}

static void test_memcmp_strncmp_agreement(void)
{
    TEST("cross: memcmp and strncmp agree on ASCII strings");
    const char *a = "abcde";
    const char *b = "abcfg";
    int mc = memcmp(a, b, 5);
    int sc = strncmp(a, b, 5);
    /* Both should have the same sign */
    ASSERT((mc < 0 && sc < 0) || (mc > 0 && sc > 0) || (mc == 0 && sc == 0),
           "memcmp and strncmp should agree in sign");
    PASS();
}

/* ========================================================================= */
/*  main                                                                     */
/* ========================================================================= */

TEST_SUITE(string, "string.h tests")
{
    printf(COLOR_YELLOW "[strlen]" COLOR_RESET "\n");
    test_strlen_empty();
    test_strlen_basic();
    test_strlen_single_char();
    test_strlen_stops_at_nul();

    printf(COLOR_YELLOW "[strcpy]" COLOR_RESET "\n");
    test_strcpy_basic();
    test_strcpy_empty();
    test_strcpy_stops_at_nul();

    printf(COLOR_YELLOW "[strncpy]" COLOR_RESET "\n");
    test_strncpy_truncation();
    test_strncpy_zero_pads();
    test_strncpy_exact_length_no_nul();
    test_strncpy_exact_plus_one();
    test_strncpy_zero_n();
    test_strncpy_full_zero_pad_range();
    test_strncpy_embedded_nul_source();

    printf(COLOR_YELLOW "[strcat]" COLOR_RESET "\n");
    test_strcat_basic();
    test_strcat_to_empty();
    test_strcat_empty_src();

    printf(COLOR_YELLOW "[strncat]" COLOR_RESET "\n");
    test_strncat_basic();
    test_strncat_truncated();
    test_strncat_zero_n();
    test_strncat_n_greater_than_src();
    test_strncat_post_terminator_untouched();

    printf(COLOR_YELLOW "[strcmp]" COLOR_RESET "\n");
    test_strcmp_equal();
    test_strcmp_less();
    test_strcmp_greater();
    test_strcmp_empty();
    test_strcmp_prefix();
    test_strcmp_unsigned_chars();
    test_strcmp_stops_at_nul();

    printf(COLOR_YELLOW "[strncmp]" COLOR_RESET "\n");
    test_strncmp_equal_within_n();
    test_strncmp_diff_within_n();
    test_strncmp_zero_n();
    test_strncmp_unsigned_chars();
    test_strncmp_stops_at_nul_before_n();
    test_strncmp_embedded_nul();

#if TEST_STRCHR
    printf(COLOR_YELLOW "[strchr]" COLOR_RESET "\n");
    test_strchr_found();
    test_strchr_not_found();
    test_strchr_null_terminator();
    test_strchr_first_char();
    test_strchr_last_char();
    test_strchr_high_byte();
    test_strchr_0x80();

    printf(COLOR_YELLOW "[strrchr]" COLOR_RESET "\n");
    test_strrchr_found();
    test_strrchr_not_found();
    test_strrchr_null_terminator();
    test_strrchr_high_byte();
    test_strrchr_0x80();
#endif

#if TEST_STRSTR
    printf(COLOR_YELLOW "[strstr]" COLOR_RESET "\n");
    test_strstr_found();
    test_strstr_not_found();
    test_strstr_empty_needle();
    test_strstr_full_match();
    test_strstr_partial_overlap();
    test_strstr_repeated_prefix();
    test_strstr_needle_longer_than_haystack();
    test_strstr_empty_haystack();
    test_strstr_stops_at_nul();
#endif

    printf(COLOR_YELLOW "[strtok]" COLOR_RESET "\n");
    test_strtok_basic();
    test_strtok_multiple_delimiters();
    test_strtok_consecutive_delimiters();
    test_strtok_empty_string();
    test_strtok_all_delimiters();
    test_strtok_leading_trailing_delimiters();
    test_strtok_no_delimiter();
    test_strtok_overwrites_delimiter();
    test_strtok_change_delimiter();
    test_strtok_reset_mid_sequence();
    test_strtok_reset_abandons_previous();
    test_strtok_empty_delimiter();
    test_strtok_null_before_init();

    printf(COLOR_YELLOW "[memcpy]" COLOR_RESET "\n");
    test_memcpy_basic();
    test_memcpy_partial();
    test_memcpy_returns_dest();
    test_memcpy_zero_bytes();
    test_memcpy_binary_data();
    test_memcpy_large_n();

    printf(COLOR_YELLOW "[memmove]" COLOR_RESET "\n");
    test_memmove_nonoverlapping();
    test_memmove_overlap_forward();
    test_memmove_overlap_backward();
    test_memmove_same_pointer();
    test_memmove_binary_overlap();
    test_memmove_zero_n();
    test_memmove_overlap_forward_returns_dest();
    test_memmove_large_overlap();

    printf(COLOR_YELLOW "[memset]" COLOR_RESET "\n");
    test_memset_basic();
    test_memset_zero();
    test_memset_returns_dest();
    test_memset_value_truncation();
    test_memset_0xff();
    test_memset_zero_length();
    test_memset_partial();
    test_memset_large_n();

    printf(COLOR_YELLOW "[memcmp]" COLOR_RESET "\n");
    test_memcmp_equal();
    test_memcmp_less();
    test_memcmp_greater();
    test_memcmp_partial();
    test_memcmp_unsigned_bytes();
    test_memcmp_through_nuls();
    test_memcmp_sign_boundary();

#if TEST_MEMCHR
    printf(COLOR_YELLOW "[memchr]" COLOR_RESET "\n");
    test_memchr_found();
    test_memchr_not_found();
    test_memchr_finds_null();
    test_memchr_zero_n();
    test_memchr_at_last_byte();
    test_memchr_0xff();
    test_memchr_byte_truncation();
    test_memchr_0x80();
#endif

    printf(COLOR_YELLOW "[return-value contracts]" COLOR_RESET "\n");
    test_strcpy_returns_dest();
    test_strncpy_returns_dest();
    test_strcat_returns_dest();
    test_strncat_returns_dest();
#if TEST_STRCHR
    test_strchr_returns_into_source();
    test_strrchr_returns_into_source();
#endif
#if TEST_STRSTR
    test_strstr_returns_into_haystack();
#endif
#if TEST_MEMCHR
    test_memchr_returns_into_buffer();
#endif
    test_strtok_returns_into_source();

#if TEST_STRSPAN
    printf(COLOR_YELLOW "[strspn]" COLOR_RESET "\n");
    test_strspn_all_match();
    test_strspn_partial();
    test_strspn_no_match();
    test_strspn_empty_string();
    test_strspn_empty_accept();

    printf(COLOR_YELLOW "[strcspn]" COLOR_RESET "\n");
    test_strcspn_no_reject();
    test_strcspn_immediate_reject();
    test_strcspn_partial();
    test_strcspn_empty_string();
    test_strcspn_empty_reject();
#endif

#if TEST_STRPBRK
    printf(COLOR_YELLOW "[strpbrk]" COLOR_RESET "\n");
    test_strpbrk_found();
    test_strpbrk_not_found();
    test_strpbrk_first_char();
    test_strpbrk_empty_accept();
    test_strpbrk_returns_into_source();
#endif

    printf(COLOR_YELLOW "[cross-function invariants]" COLOR_RESET "\n");
    test_strlen_sprintf_consistency();
    test_memcmp_strncmp_agreement();
}
