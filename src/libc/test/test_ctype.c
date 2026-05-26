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
 *         File: src/libc/test/test_ctype.c
 *      Created: May 23, 2026
 *       Author: Wes Hampson
 *
 * libc ctype.h function tests
 *
 * Covers: positive/negative spot checks, full ASCII sweep (0-127),
 *         EOF handling, boundary characters, non-letter passthrough.
 * =============================================================================
 */

#include <ctype.h>
#include "framework.h"

/* ========================================================================= */
/*  isalpha                                                                  */
/* ========================================================================= */

static void test_isalpha_sweep(void)
{
    TEST("isalpha: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        ASSERT(!!isalpha(c) == expected, "isalpha sweep mismatch");
    }
    PASS();
}

static void test_isalpha_eof(void)
{
    TEST("isalpha: EOF returns false");
    ASSERT(!isalpha(EOF), "EOF");
    PASS();
}

/* ========================================================================= */
/*  isdigit                                                                  */
/* ========================================================================= */

static void test_isdigit_sweep(void)
{
    TEST("isdigit: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c >= '0' && c <= '9');
        ASSERT(!!isdigit(c) == expected, "isdigit sweep mismatch");
    }
    PASS();
}

static void test_isdigit_eof(void)
{
    TEST("isdigit: EOF returns false");
    ASSERT(!isdigit(EOF), "EOF");
    PASS();
}

/* ========================================================================= */
/*  isalnum                                                                  */
/* ========================================================================= */

static void test_isalnum_sweep(void)
{
    TEST("isalnum: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c >= 'A' && c <= 'Z') ||
                       (c >= 'a' && c <= 'z') ||
                       (c >= '0' && c <= '9');
        ASSERT(!!isalnum(c) == expected, "isalnum sweep mismatch");
    }
    PASS();
}

/* ========================================================================= */
/*  isspace                                                                  */
/* ========================================================================= */

static void test_isspace_sweep(void)
{
    TEST("isspace: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c == ' ' || c == '\t' || c == '\n' ||
                        c == '\r' || c == '\f' || c == '\v');
        ASSERT(!!isspace(c) == expected, "isspace sweep mismatch");
    }
    PASS();
}

/* ========================================================================= */
/*  isupper / islower                                                        */
/* ========================================================================= */

static void test_isupper_sweep(void)
{
    TEST("isupper: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c >= 'A' && c <= 'Z');
        ASSERT((!!isupper(c)) == expected, "isupper sweep mismatch");
    }
    PASS();
}

static void test_islower_sweep(void)
{
    TEST("islower: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c >= 'a' && c <= 'z');
        ASSERT((!!islower(c)) == expected, "islower sweep mismatch");
    }
    PASS();
}

/* ========================================================================= */
/*  isxdigit                                                                 */
/* ========================================================================= */

static void test_isxdigit_sweep(void)
{
    TEST("isxdigit: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c >= '0' && c <= '9') ||
                       (c >= 'A' && c <= 'F') ||
                       (c >= 'a' && c <= 'f');
        ASSERT(!!isxdigit(c) == expected, "isxdigit sweep mismatch");
    }
    PASS();
}

/* ========================================================================= */
/*  ispunct                                                                  */
/* ========================================================================= */

static void test_ispunct_sweep(void)
{
    TEST("ispunct: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        /* Punctuation: printable (0x21-0x7E) but not letter or digit */
        int expected = (c >= 0x21 && c <= 0x7E) &&
                       !(c >= 'A' && c <= 'Z') &&
                       !(c >= 'a' && c <= 'z') &&
                       !(c >= '0' && c <= '9');
        ASSERT(!!ispunct(c) == expected, "ispunct sweep mismatch");
    }
    PASS();
}

/* ========================================================================= */
/*  isprint / isgraph                                                        */
/* ========================================================================= */

static void test_isprint_sweep(void)
{
    TEST("isprint: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c >= 0x20 && c <= 0x7E);
        ASSERT(!!isprint(c) == expected, "isprint sweep mismatch");
    }
    PASS();
}

static void test_isgraph_sweep(void)
{
    TEST("isgraph: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c >= 0x21 && c <= 0x7E);
        ASSERT(!!isgraph(c) == expected, "isgraph sweep mismatch");
    }
    PASS();
}

/* ========================================================================= */
/*  iscntrl                                                                  */
/* ========================================================================= */

static void test_iscntrl_sweep(void)
{
    TEST("iscntrl: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c < 0x20 || c == 127);
        ASSERT(!!iscntrl(c) == expected, "iscntrl sweep mismatch");
    }
    PASS();
}

/* ========================================================================= */
/*  toupper / tolower                                                        */
/* ========================================================================= */

static void test_toupper_basic(void)
{
    TEST("toupper: converts lowercase to uppercase");
    ASSERT_EQ_INT('A', toupper('a'), "a -> A");
    ASSERT_EQ_INT('Z', toupper('z'), "z -> Z");
    PASS();
}

static void test_toupper_sweep(void)
{
    TEST("toupper: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        if (c >= 'a' && c <= 'z')
            ASSERT_EQ_INT(c - 32, toupper(c), "toupper lowercase");
        else
            ASSERT_EQ_INT(c, toupper(c), "toupper non-lowercase unchanged");
    }
    PASS();
}

static void test_tolower_basic(void)
{
    TEST("tolower: converts uppercase to lowercase");
    ASSERT_EQ_INT('a', tolower('A'), "A -> a");
    ASSERT_EQ_INT('z', tolower('Z'), "Z -> z");
    PASS();
}

static void test_tolower_sweep(void)
{
    TEST("tolower: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        if (c >= 'A' && c <= 'Z')
            ASSERT_EQ_INT(c + 32, tolower(c), "tolower uppercase");
        else
            ASSERT_EQ_INT(c, tolower(c), "tolower non-uppercase unchanged");
    }
    PASS();
}

static void test_isalnum_eof(void)
{
    TEST("isalnum: EOF returns false");
    ASSERT(!isalnum(EOF), "EOF");
    PASS();
}

static void test_isspace_eof(void)
{
    TEST("isspace: EOF returns false");
    ASSERT(!isspace(EOF), "EOF");
    PASS();
}

static void test_isupper_eof(void)
{
    TEST("isupper: EOF returns false");
    ASSERT(!isupper(EOF), "EOF");
    PASS();
}

static void test_islower_eof(void)
{
    TEST("islower: EOF returns false");
    ASSERT(!islower(EOF), "EOF");
    PASS();
}

static void test_isxdigit_eof(void)
{
    TEST("isxdigit: EOF returns false");
    ASSERT(!isxdigit(EOF), "EOF");
    PASS();
}

static void test_ispunct_eof(void)
{
    TEST("ispunct: EOF returns false");
    ASSERT(!ispunct(EOF), "EOF");
    PASS();
}

static void test_isprint_eof(void)
{
    TEST("isprint: EOF returns false");
    ASSERT(!isprint(EOF), "EOF");
    PASS();
}

static void test_isgraph_eof(void)
{
    TEST("isgraph: EOF returns false");
    ASSERT(!isgraph(EOF), "EOF");
    PASS();
}

static void test_iscntrl_eof(void)
{
    TEST("iscntrl: EOF returns false");
    ASSERT(!iscntrl(EOF), "EOF");
    PASS();
}

static void test_toupper_eof(void)
{
    TEST("toupper: EOF returns EOF");
    ASSERT_EQ_INT(EOF, toupper(EOF), "EOF unchanged");
    PASS();
}

static void test_tolower_eof(void)
{
    TEST("tolower: EOF returns EOF");
    ASSERT_EQ_INT(EOF, tolower(EOF), "EOF unchanged");
    PASS();
}

static void test_toupper_tolower_roundtrip(void)
{
    TEST("toupper/tolower: round-trip for ASCII letters");
    for (int c = 'a'; c <= 'z'; c++)
    {
        ASSERT_EQ_INT(c, tolower(toupper(c)), "lower(upper(c)) == c");
    }
    for (int c = 'A'; c <= 'Z'; c++)
    {
        ASSERT_EQ_INT(c, toupper(tolower(c)), "upper(lower(c)) == c");
    }
    PASS();
}

static void test_isalpha_high_bytes(void)
{
    TEST("isalpha: high bytes 128-255 return false (C locale)");
    for (int c = 128; c <= 255; c++)
        ASSERT(!isalpha(c), "isalpha should be false for high bytes in C locale");
    PASS();
}

static void test_isdigit_high_bytes(void)
{
    TEST("isdigit: high bytes 128-255 return false (C locale)");
    for (int c = 128; c <= 255; c++)
        ASSERT(!isdigit(c), "isdigit should be false for high bytes");
    PASS();
}

static void test_ctype_high_bytes_false(void)
{
    TEST("ctype: high bytes (128-255) are false in C locale");
    for (int c = 128; c <= 255; c++) {
        ASSERT(isalnum(c) == 0, "isalnum high byte");
        ASSERT(isspace(c) == 0, "isspace high byte");
        ASSERT(isupper(c) == 0, "isupper high byte");
        ASSERT(islower(c) == 0, "islower high byte");
        ASSERT(isxdigit(c) == 0, "isxdigit high byte");
        ASSERT(ispunct(c) == 0, "ispunct high byte");
        ASSERT(isgraph(c) == 0, "isgraph high byte");
        ASSERT(iscntrl(c) == 0, "iscntrl high byte");
        ASSERT(isprint(c) == 0, "isprint high byte");
    }
    PASS();
}

static void test_toupper_high_bytes(void)
{
    TEST("toupper: high bytes 128-255 unchanged");
    for (int c = 128; c <= 255; c++)
        ASSERT_EQ_INT(c, toupper(c), "high byte should pass through");
    PASS();
}

static void test_tolower_high_bytes(void)
{
    TEST("tolower: high bytes 128-255 unchanged");
    for (int c = 128; c <= 255; c++)
        ASSERT_EQ_INT(c, tolower(c), "high byte should pass through");
    PASS();
}

/* ========================================================================= */
/*  isblank                                                                  */
/* ========================================================================= */

static void test_isblank_blanks(void)
{
    TEST("isblank: space and tab return true");
    ASSERT(isblank(' '), "space");
    ASSERT(isblank('\t'), "tab");
    PASS();
}

static void test_isblank_non_blanks(void)
{
    TEST("isblank: non-blank whitespace and others return false");
    ASSERT(!isblank('\n'), "newline");
    ASSERT(!isblank('\r'), "carriage return");
    ASSERT(!isblank('\f'), "form feed");
    ASSERT(!isblank('\v'), "vertical tab");
    ASSERT(!isblank('a'), "letter");
    ASSERT(!isblank('0'), "digit");
    ASSERT(!isblank('\0'), "NUL");
    PASS();
}

static void test_isblank_sweep(void)
{
    TEST("isblank: ASCII sweep 0-127");
    for (int c = 0; c < 128; c++)
    {
        int expected = (c == ' ' || c == '\t');
        ASSERT(!!isblank(c) == expected, "isblank sweep mismatch");
    }
    PASS();
}

static void test_isblank_eof(void)
{
    TEST("isblank: EOF returns false");
    ASSERT(!isblank(EOF), "EOF");
    PASS();
}

static void test_isblank_high_bytes(void)
{
    TEST("isblank: high bytes 128-255 return false (C locale)");
    for (int c = 128; c <= 255; c++)
        ASSERT(!isblank(c), "isblank should be false for high bytes in C locale");
    PASS();
}

/* ========================================================================= */
/*  Suite runner                                                             */
/* ========================================================================= */

TEST_SUITE(ctype, "ctype.h tests")
{
    printf(COLOR_YELLOW "[isalpha]" COLOR_RESET "\n");
    test_isalpha_sweep();
    test_isalpha_eof();

    printf(COLOR_YELLOW "[isdigit]" COLOR_RESET "\n");
    test_isdigit_sweep();
    test_isdigit_eof();

    printf(COLOR_YELLOW "[isalnum]" COLOR_RESET "\n");
    test_isalnum_sweep();
    test_isalnum_eof();

    printf(COLOR_YELLOW "[isspace]" COLOR_RESET "\n");
    test_isspace_sweep();
    test_isspace_eof();

    printf(COLOR_YELLOW "[isupper / islower]" COLOR_RESET "\n");
    test_isupper_sweep();
    test_islower_sweep();
    test_isupper_eof();
    test_islower_eof();

    printf(COLOR_YELLOW "[isxdigit]" COLOR_RESET "\n");
    test_isxdigit_sweep();
    test_isxdigit_eof();

    printf(COLOR_YELLOW "[ispunct]" COLOR_RESET "\n");
    test_ispunct_sweep();
    test_ispunct_eof();

    printf(COLOR_YELLOW "[isprint / isgraph]" COLOR_RESET "\n");
    test_isprint_sweep();
    test_isprint_eof();
    test_isgraph_sweep();
    test_isgraph_eof();

    printf(COLOR_YELLOW "[iscntrl]" COLOR_RESET "\n");
    test_iscntrl_sweep();
    test_iscntrl_eof();

    printf(COLOR_YELLOW "[toupper / tolower]" COLOR_RESET "\n");
    test_toupper_basic();
    test_toupper_sweep();
    test_tolower_basic();
    test_tolower_sweep();
    test_toupper_eof();
    test_tolower_eof();
    test_toupper_tolower_roundtrip();

    printf(COLOR_YELLOW "[isblank]" COLOR_RESET "\n");
    test_isblank_blanks();
    test_isblank_non_blanks();
    test_isblank_sweep();
    test_isblank_eof();

    printf(COLOR_YELLOW "[high byte range 128-255]" COLOR_RESET "\n");
    test_isalpha_high_bytes();
    test_isdigit_high_bytes();
    test_ctype_high_bytes_false();
    test_toupper_high_bytes();
    test_tolower_high_bytes();
    test_isblank_high_bytes();
}
