
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <syscall.h>
#include "test.h"

#define _console_write      _wr

void _wr(const char *msg)
{
    const char *a = msg;
    while (*a != '\0')
    {
        write(stdout_fd, a++, 1);
    }
}

void printf_reference()
{
    // from https://en.cppreference.com/w/c/io/fprintf

    const char* s = "Hello";
    printf("Strings:\n"); // same as puts("Strings");
    printf(" padding:\n");
    printf("\t[%10s]\n", s);
    printf("\t[%-10s]\n", s);
    printf("\t[%*s]\n", 10, s);
    printf(" truncating:\n");
    printf("\t%.4s\n", s);
    printf("\t%.*s\n", 3, s);

    printf("Characters:\t%c %%\n", 'A');

    printf("Integers:\n");
    printf("\tDecimal:\t%i %d %.6i %i %.0i %+i %i\n",
                        1, 2,   3, 0,   0,  4,-4);
    printf("\tHexadecimal:\t%x %x %X %#x\n", 5, 10, 10, 6);
    printf("\tOctal:\t\t%o %#o %#o\n", 10, 10, 4);

    //
    // not supported:
    //
    // printf("Floating point:\n");
    // printf("\tRounding:\t%f %.0f %.32f\n", 1.5, 1.5, 1.3);
    // printf("\tPadding:\t%05.2f %.2f %5.2f\n", 1.5, 1.5, 1.5);
    // printf("\tScientific:\t%E %e\n", 1.5, 1.5);
    // printf("\tHexadecimal:\t%a %A\n", 1.5, 1.5);
    // printf("\tSpecial values:\t0/0=%g 1/0=%g\n", 0.0/0.0, 1.0/0.0);

    printf("Fixed-width types:\n");
    printf("\tLargest 32-bit value is %" PRIu32 " or %#" PRIx32 "\n",
                                     UINT32_MAX,     UINT32_MAX );
}

void test_printf(void)
{
    // printf_reference();

    char buf[256];

    // ------------------------------------------------------------------------
    //
    // s(n)printf tests - assumes strcmp works and terminal works to some extent
    //
    // ------------------------------------------------------------------------

    //
    // ugg-lee macros :)
    //
    {
        #define _TEST_SPRINTF(exp_ret,exp_buf,...) \
            buf[0] = '\0'; \
            int _ret = sprintf(buf, __VA_ARGS__); \
            bool _pass = (_ret == (exp_ret)) && (strcmp(buf, exp_buf) == 0); \

        #define _TEST_SNPRINTF(exp_ret,exp_buf,n,...) \
            buf[0] = '\0'; \
            int _ret = snprintf(buf, n, __VA_ARGS__); \
            bool _pass = (_ret == (exp_ret)) && (exp_buf == NULL || strcmp(buf, exp_buf) == 0); \

        #define _TEST_CHECK(exp_ret,exp_buf,...) \
            if (!_pass) { \
                _console_write("!! sprintf SANITY CHECK FAILED: " #__VA_ARGS__ "\n"); \
                if (_ret != (exp_ret)) { \
                    _console_write("!! return value does not match expected value of " #exp_ret "\n"); \
                } \
                _console_write("!! \texp_buf='" exp_buf "'\n"); \
                _console_write("!! \tgot_buf='"); \
                _console_write(buf); \
                _console_write("'\n"); \
            } \

        #define TEST_SPRINTF(exp_ret,exp_buf,...) \
        do { \
            _TEST_SPRINTF(exp_ret,exp_buf,__VA_ARGS__) \
            _TEST_CHECK(exp_ret,exp_buf,__VA_ARGS__) \
            VERIFY_IS_TRUE(_pass); \
        } while (0)

        #define TEST_SNPRINTF(exp_ret,exp_buf,n,...) \
        do { \
            _TEST_SNPRINTF(exp_ret,exp_buf,n,__VA_ARGS__) \
            _TEST_CHECK(exp_ret,exp_buf,__VA_ARGS__) \
            VERIFY_IS_TRUE(_pass); \
        } while (0)
    }

    //
    // quick sprintf and snprintf buffer sanity checks - assumes strcmp works
    //

    // sprintf return value is num chars printed
    TEST_SPRINTF(0, "", "");
    TEST_SPRINTF(14, "Hello, world!\n", "Hello, world!\n");

    // snprintf return value is num chars that would've been printed had limit
    //   not been reached
    TEST_SNPRINTF(-EINVAL, "", 0, NULL);  // NULL ok if n==0
    TEST_SNPRINTF(3, "a", 1, "abc");
    TEST_SNPRINTF(3, "abc", 3, "abc");
    TEST_SNPRINTF(3, "", 0, "abc");
    TEST_SNPRINTF(3, "a", 1, "abc");

    #undef TEST_SPRINTF
    #undef TEST_SNPRINTF
    #undef _TEST_SPRINTF
    #undef _TEST_SNPRINTF
    #undef _TEST_CHECK


    // ------------------------------------------------------------------------
    //
    // printf tests - assumes snprintf works and terminal works to some extent
    //
    // ------------------------------------------------------------------------

    #define _TEST_FN(expected,...) \
        snprintf(buf, sizeof(buf), __VA_ARGS__); \
        bool _pass = (strcmp(buf, expected) == 0); \

    #define _TEST_CHECK(expected,...) \
        if (!_pass) { \
            _console_write("!! printf FAILED: " #__VA_ARGS__ "\n"); \
            _console_write("!! \texp='" expected "'\n"); \
            _console_write("!! \tgot='"); \
            _console_write(buf); \
            _console_write("'\n"); \
        } \

    #define TEST(expected,...) \
    do { \
        _TEST_FN(expected, __VA_ARGS__) \
        _TEST_CHECK(expected, __VA_ARGS__) \
        VERIFY_IS_TRUE(_pass); \
    } while (0)

    //
    // invalid format specifiers
    //
    {
        TEST("%q","%q");                        // unknown format char
        TEST("%q widgets","%q widgets", 35);    // unknown format char w/ arg
        TEST("%q widgets made in 35 days", "%q widgets made in %d days", 35, 17);   // unknown and known format char
        TEST("%&d", "%&d");                     // known format char w/ unknown flag
        TEST("%0&d", "%0&d");                   // known format char w/ known and unknown flags
        TEST("%0&#.d", "%0&#.d");               // ""
        TEST("%.-8d", "%.-8d");                 // known format char w/ invalid numeric param
        TEST("%.-8d", "%%.-8d");                // oops! you typed a double percent
        TEST("A %#045.123q B", "A %#045.123q B");// unknown format char, complicated
        TEST("dfs%qwerty%,l;'p", "dfs%qwerty%,l;'p");   // straight gibberish
    }

    //
    // string, char (%s, %c)
    //
    {
        TEST("",            "");                // empty
        TEST("",            "%s", "");          // empty string using format
        TEST("A",           "A");               // nonempty string string
        TEST("A",           "%s", "A");         // nonempty string w/ format
        // TEST(L"A",          "%ls", L"A");       // wide string
        TEST("%",           "%%");              // percent signs
        TEST("\n",          "\n");              // newline
        TEST("A",           "%c", 'A');         // char
        TEST("%",           "%c", '%');         // char w/ percent
        TEST("\n",          "%c", '\n');        // char w/ newline
        // TEST(L"A",          "%lc", L'A');       // wide char
        TEST("a%",          "a%%");             // string w/ percent, prepend
        TEST("%a",          "%%a");             // string w/ percent, append
        TEST("a%",          "%c%%", 'a');       // mixed format and percent, prepend
        TEST("%a",          "%%%c", 'a');       // mixed format and percent, append
        TEST("ABC   ",      "%-6s", "ABC");     // left justify
        TEST("   ABC",      "%6s", "ABC");      // right justify
        TEST("ABC   ",      "%*s", -6, "ABC");  // left justify w/ * arg
        TEST("   ABC",      "%*s", 6, "ABC");   // right justify w/ * arg
        TEST("ABCDEFG",     "%3s", "ABCDEFG");  // width < length
        TEST("abcdefghijlklmnopqrstuvwxyzABCDEFGHIJLKLMNOPQRSTUVWXYZ0123456789/*-+,./;'[]\\-=`<>?:\"{}|_+~", "abcdefghijlklmnopqrstuvwxyzABCDEFGHIJLKLMNOPQRSTUVWXYZ0123456789/*-+,./;'[]\\-=`<>?:\"{}|_+~");   // big unformatted string
        TEST("abcdefghijlklmnopqrstuvwxyzABCDEFGHIJLKLMNOPQRSTUVWXYZ0123456789/*-+,./;'[]\\-=`<>?:\"{}|_+~", "%s", "abcdefghijlklmnopqrstuvwxyzABCDEFGHIJLKLMNOPQRSTUVWXYZ0123456789/*-+,./;'[]\\-=`<>?:\"{}|_+~");   // big formatted string
        TEST("",            "%.s", "ABCDEFG");  // zero precision (implied)
        TEST("",            "%.0s", "ABCDEFG"); // zero precision (explicit)
        TEST("ABC",         "%.3s", "ABCDEFG"); // nonzero precision
        TEST("ABCDEFG",     "%.10s", "ABCDEFG");// precision exceeds length
        TEST("ABC",         "%.*s",3,"ABCDEFG");// precision w/ arg
        TEST("   ABCDEFG",  "%10.*s", -3, "ABCDEFG");// negative precision (ignored)
        TEST("   ABC",      "%*.*s", 6, 3, "ABCDEFG");  // precision & width w/ args
        TEST("ABCDEFGHIJKLMN","%-13.14s", "ABCDEFGHIJKLMNOP");  // complicated format
    }

    //
    // numeric limits (%d, %o, %u, %x, %X)
    //
    {
        // signed integer (%d, %i)
        TEST("0",           "%d", 0);           // zero
        TEST("-1",          "%d", 0xFFFFFFFF);  // negative
        TEST("2147483647",  "%d", 0x7FFFFFFF);  // max
        TEST("-2147483648", "%d", 0x80000000);  // min
        TEST("-1",          "%hhd", (char) 0xFF); // negative
        TEST("127",         "%hhd", (char) 0x7F); // max
        TEST("-128",        "%hhd", (char) 0x80); // min
        TEST("-1",          "%hd", (short) 0xFFFF); // negative
        TEST("32767",       "%hd", (short) 0x7FFF); // max
        TEST("-32768",      "%hd", (short) 0x8000); // min
        if (sizeof(long) == 4) {
            TEST("-1",          "%ld", (long) 0xFFFFFFFFL); // negative
            TEST("2147483647",  "%ld", (long) 0x7FFFFFFFL); // max
            TEST("-2147483648", "%ld", (long) 0x80000000L); // min
        }
        else {
            TEST("-1",          "%ld", (long) 0xFFFFFFFFFFFFFFFFL); // negative
            TEST("9223372036854775807",  "%ld", (long) 0x7FFFFFFFFFFFFFFFL); // max
            TEST("-9223372036854775808", "%ld", (long) 0x8000000000000000L); // min
        }
        TEST("-1",          "%lld", (long long) 0xFFFFFFFFFFFFFFFFLL); // negative
        TEST("9223372036854775807",  "%lld", (long long) 0x7FFFFFFFFFFFFFFFLL); // max
        TEST("-9223372036854775808", "%lld", (long long) 0x8000000000000000LL); // min
    }
    {
        // unsigned integer (%u)
        TEST("0",           "%u", 0);           // zero
        TEST("4294967295",  "%u", 0xFFFFFFFFU);  // negative
        TEST("2147483647",  "%u", 0x7FFFFFFFU);  // max
        TEST("2147483648",  "%u",  0x80000000U);  // min
        TEST("255",         "%hhu", (unsigned char) 0xFFU); // negative
        TEST("127",         "%hhu", (unsigned char) 0x7FU); // max
        TEST("128",         "%hhu", (unsigned char) 0x80U); // min
        TEST("65535",       "%hu", (unsigned short) 0xFFFFU); // negative
        TEST("32767",       "%hu", (unsigned short) 0x7FFFU); // max
        TEST("32768",       "%hu", (unsigned short) 0x8000U); // min
        if (sizeof(unsigned long) == 4) {
            TEST("4294967295",  "%lu", (unsigned long) 0xFFFFFFFFUL); // negative
            TEST("2147483647",  "%lu", (unsigned long) 0x7FFFFFFFUL); // max
            TEST("2147483648",  "%lu", (unsigned long) 0x80000000UL); // min
        }
        else {
            TEST("18446744073709551615", "%lu", (unsigned long) 0xFFFFFFFFFFFFFFFFUL); // negative
            TEST("9223372036854775807", "%lu", (unsigned long) 0x7FFFFFFFFFFFFFFFUL); // max
            TEST("9223372036854775808", "%lu", (unsigned long) 0x8000000000000000UL); // min
        }
        TEST("18446744073709551615", "%llu", (unsigned long long) 0xFFFFFFFFFFFFFFFFULL); // negative
        TEST("9223372036854775807", "%llu", (unsigned long long) 0x7FFFFFFFFFFFFFFFULL); // max
        TEST("9223372036854775808", "%llu", (unsigned long long) 0x8000000000000000ULL); // min
    }
    {
        // octal (%o)
        TEST("0",           "%o", 0);           // zero
        TEST("37777777777", "%o", 0xFFFFFFFFU);  // negative
        TEST("17777777777", "%o", 0x7FFFFFFFU);  // max
        TEST("20000000000", "%o",  0x80000000U);  // min
        TEST("377",         "%hho", (unsigned char) 0xFFU); // negative
        TEST("177",         "%hho", (unsigned char) 0x7FU); // max
        TEST("200",         "%hho", (unsigned char) 0x80U); // min
        TEST("177777",       "%ho", (unsigned short) 0xFFFFU); // negative
        TEST("77777",       "%ho", (unsigned short) 0x7FFFU); // max
        TEST("100000",       "%ho", (unsigned short) 0x8000U); // min
        if (sizeof(unsigned long) == 4) {
            TEST("37777777777",  "%lo", (unsigned long) 0xFFFFFFFFUL); // negative
            TEST("17777777777",  "%lo", (unsigned long) 0x7FFFFFFFUL); // max
            TEST("20000000000",  "%lo", (unsigned long) 0x80000000UL); // min
        }
        else {
            TEST("1777777777777777777777", "%lo", (unsigned long) 0xFFFFFFFFFFFFFFFFUL); // negative
            TEST("777777777777777777777", "%lo", (unsigned long) 0x7FFFFFFFFFFFFFFFUL); // max
            TEST("1000000000000000000000", "%lo", (unsigned long) 0x8000000000000000UL); // min
        }
        TEST("1777777777777777777777", "%llo", (unsigned long long) 0xFFFFFFFFFFFFFFFFULL); // negative
        TEST("777777777777777777777", "%llo", (unsigned long long) 0x7FFFFFFFFFFFFFFFULL); // max
        TEST("1000000000000000000000", "%llo", (unsigned long long) 0x8000000000000000ULL); // min
    }
    {
        // hexadecimal, lowercase (%x)
        TEST("0",           "%x", 0);           // zero
        TEST("ffffffff",    "%x", 0xFFFFFFFFU);  // negative
        TEST("7fffffff",    "%x", 0x7FFFFFFFU);  // max
        TEST("80000000",    "%x",  0x80000000U);  // min
        TEST("ff",          "%hhx", (unsigned char) 0xFFU); // negative
        TEST("7f",          "%hhx", (unsigned char) 0x7FU); // max
        TEST("80",          "%hhx", (unsigned char) 0x80U); // min
        TEST("ffff",        "%hx", (unsigned short) 0xFFFFU); // negative
        TEST("7fff",        "%hx", (unsigned short) 0x7FFFU); // max
        TEST("8000",        "%hx", (unsigned short) 0x8000U); // min
        if (sizeof(unsigned long) == 4) {
            TEST("ffffffff",    "%lx", (unsigned long) 0xFFFFFFFFUL); // negative
            TEST("7fffffff",    "%lx", (unsigned long) 0x7FFFFFFFUL); // max
            TEST("80000000",    "%lx", (unsigned long) 0x80000000UL); // min
        }
        else {
            TEST("ffffffffffffffff", "%lx", (unsigned long) 0xFFFFFFFFFFFFFFFFUL); // negative
            TEST("7fffffffffffffff", "%lx", (unsigned long) 0x7FFFFFFFFFFFFFFFUL); // max
            TEST("8000000000000000", "%lx", (unsigned long) 0x8000000000000000UL); // min
        }
        TEST("ffffffffffffffff", "%llx", (unsigned long long) 0xFFFFFFFFFFFFFFFFULL); // negative
        TEST("7fffffffffffffff", "%llx", (unsigned long long) 0x7FFFFFFFFFFFFFFFULL); // max
        TEST("8000000000000000", "%llx", (unsigned long long) 0x8000000000000000ULL); // min
    }
    {
        // hexadecimal, uppercase (%X)
        TEST("0",           "%X", 0);           // zero
        TEST("FFFFFFFF",    "%X", 0xFFFFFFFFU);  // negative
        TEST("7FFFFFFF",    "%X", 0x7FFFFFFFU);  // max
        TEST("80000000",    "%X",  0x80000000U);  // min
        TEST("FF",          "%hhX", (unsigned char) 0xFFU); // negative
        TEST("7F",          "%hhX", (unsigned char) 0x7FU); // max
        TEST("80",          "%hhX", (unsigned char) 0x80U); // min
        TEST("FFFF",        "%hX", (unsigned short) 0xFFFFU); // negative
        TEST("7FFF",        "%hX", (unsigned short) 0x7FFFU); // max
        TEST("8000",        "%hX", (unsigned short) 0x8000U); // min
        if (sizeof(unsigned long) == 4) {
            TEST("FFFFFFFF",    "%lX", (unsigned long) 0xFFFFFFFFUL); // negative
            TEST("7FFFFFFF",    "%lX", (unsigned long) 0x7FFFFFFFUL); // max
            TEST("80000000",    "%lX", (unsigned long) 0x80000000UL); // min
        }
        else {
            TEST("FFFFFFFFFFFFFFFF", "%lX", (unsigned long) 0xFFFFFFFFFFFFFFFFUL); // negative
            TEST("7FFFFFFFFFFFFFFF", "%lX", (unsigned long) 0x7FFFFFFFFFFFFFFFUL); // max
            TEST("8000000000000000", "%lX", (unsigned long) 0x8000000000000000UL); // min
        }
        TEST("FFFFFFFFFFFFFFFF", "%llX", (unsigned long long) 0xFFFFFFFFFFFFFFFFULL); // negative
        TEST("7FFFFFFFFFFFFFFF", "%llX", (unsigned long long) 0x7FFFFFFFFFFFFFFFULL); // max
        TEST("8000000000000000", "%llX", (unsigned long long) 0x8000000000000000ULL); // min
    }

    //
    // flags on numerics (- + 0 # space)
    //
    {
        //
        // general flag behavior on numerics
        //
        TEST("+123",        "%+d", 123);        // sign flag
        TEST("-123",        "%+d", -123);       // sign flag w/ negative
        TEST("+0",           "%+d", 0);         // sign flag w/ zero
        TEST(" 123",        "% d", 123);        // space flag
        TEST("-123",        "% d", -123);       // space flag w/ negative
        TEST(" 0",          "% d", 0);          // space flag w/ zero
        TEST("+123",        "% +d", 123);       // space flag (ignored due to sign flag)
        TEST("+123",        "%+ d", 123);       // space flag (ignored due to sign flag)
        TEST("123",         "%0d", 123);        // zero-pad w/ no width (ignored)
        TEST("123     ",    "%-8d", 123);       // left justify
        TEST("-123    ",    "%-8d", -123);      // left justify, negative
        TEST("0       ",    "%-8d", 0);         // left justify, zero
        TEST("+123    ",    "%-+8d", 123);      // left justify, sign
        TEST("+123    ",    "%+-8d", 123);      // left justify, sign
        TEST(" 123    ",    "%- 8d", 123);      // left justify, space
        TEST(" 123    ",    "% -8d", 123);      // left justify, space
        TEST("123     ",    "%-08d", 123);      // left justify, zero-pad (ignored)
        TEST("123     ",    "%0-8d", 123);      // left justify, zero-pad (ignored)
        TEST("123     ",    "%-*d", 8, 123);    // left justify w/ arg
        TEST("123     ",    "%-*d", -8, 123);   // left justify w/ arg, negative
        TEST("123",         "%-*d", 0, 123);    // left justify w/ arg, zero
        TEST("     123",    "%8d", 123);        // right justify
        TEST("    -123",    "%8d", -123);       // right justify, negative
        TEST("       0",    "%8d", 0);          // right justify, zero
        TEST("    +123",    "%+8d", 123);       // right justify, sign
        TEST("     123",    "% 8d", 123);       // right justify, space
        TEST("00000123",    "%08d", 123);       // right justify, zero-pad
        TEST("-0000123",    "%08d", -123);      // right justify, zero-pad, negative
        TEST("00000000",    "%08d", 0);         // right justify, zero-pad, zero
        TEST("+0000123",    "%+08d", 123);      // right justify, zero-pad, sign
        TEST("+0000123",    "%0+8d", 123);      // right justify, zero-pad, sign
        TEST(" 0000123",    "% 08d", 123);      // right justify, zero-pad, space
        TEST(" 0000123",    "%0 8d", 123);      // right justify, zero-pad, space
        TEST("     123",    "%*d", 8, 123);     // right justify w/ arg
        TEST("123     ",    "%*d", -8, 123);    // right justify w/ arg, negative (left justify)
        TEST("123",         "%*d", 0, 123);     // right justify w/ arg, zero
        TEST("00000123",    "%.8d", 123);       // precision
        TEST("00000123  ",  "%-10.8d", 123);    // precision, left justified
        TEST("  00000123",  "%10.8d", 123);     // precision, right justified
        TEST("  00000123",  "%010.8d", 123);    // precision, right justified, zero-pad (ignored due to explicit precision)
        TEST("0000000123",  "%8.10d", 123);     // precision greater than width
        TEST("00000123",    "%.*d", 8, 123);    // precision w/ arg
        TEST("123",         "%.*d", -8, 123);   // precision w/ arg, negative (ignored)
        TEST("0",           "%.*d", -8, 0);     // precision w/ arg, negative on zero (ignored)
        TEST("123",         "%.*d", 0, 123);    // precision w/ arg, zero
        TEST("",            "%.*d", 0, 0);      // precision w/ arg, zero on zero
        TEST("",            "%.d", 0);          // implied zero precision
        TEST("",            "%.0d", 0);         // zero precision on zero
        TEST("123",         "%.0d", 123);       // zero precision on nonzero
        TEST("        ",    "%8.0d", 0);        // zero precision on zero w/ width
        TEST("        ",    "%08.0d", 0);       // zero precision on zero w/ width, zero-pad (ignored)
        TEST("123",         "%.3d", 123);       // equal precision
        TEST("123",         "%.1d", 123);       // small precision
        TEST("00000000",    "%.8d", 0);         // precision on zero
        TEST("  00000123",  "%*.*d",10,8,123);  // width & precision w/ arg
        TEST("123",         "%#d", 123);        // alternative representation (ignored on decimal integers)
        TEST("       +000009223372036854775807", "%+# 032.24lld", 0x7FFFFFFFFFFFFFFFLL); // big complicated format
    }
    {
        //
        // unsigned
        //
        TEST("4294967173",  "%u", -123);        // negative number
        TEST("123",         "%+u", 123);        // sign flag (ignored due to unsigned)
        TEST("123",         "% u", 123);        // space flag (ignored due to unsigned)
        TEST("     123",    "%+8u", 123);       // right justify, sign (ignored due to unsigned)
        TEST("00000123",    "%+08u", 123);      // right justify, zero-pad, sign (ignored due to unsigned)
        TEST("00000123",    "%0+8u", 123);      // right justify, zero-pad, sign (ignored due to unsigned)
        TEST("00000123",    "% 08u", 123);      // right justify, zero-pad, space (ignored due to unsigned)
        TEST("00000123",    "%0 8u", 123);      // right justify, zero-pad, space (ignored due to unsigned)
        TEST("123     ",    "%-+8u", 123);      // left justify, sign (ignored due to unsigned)
        TEST("123     ",    "%+-8u", 123);      // left justify, sign (ignored due to unsigned)
        TEST("123     ",    "%- 8u", 123);      // left justify, space (ignored due to unsigned)
        TEST("123     ",    "% -8u", 123);      // left justify, space (ignored due to unsigned)

    }
    {
        //
        // octal
        //
        TEST("0123",        "%#o", 0123);       // alternative representation
        TEST("0",           "%#o", 0);          // alternative representation w/ zero
        TEST("0123",        "%#.o", 0123);      // alternative representation w/ zero precision
        TEST("0",           "%#.o", 0);         // alternative representation w/ zero precision on zero
        TEST("00000123",    "%#.8o", 0123);     // alternative representation w/ large precision
        TEST("0123",        "%#.4o", 0123);     // alternative representation w/ equal precision
        TEST("0123",        "%#.1o", 0123);     // alternative representation w/ small precision
        TEST("    0123",    "%#8o", 0123);      // alternative representation w/ width
        TEST("00000123",    "%#08o", 0123);     // alternative representation w/ width, zero-pad
        TEST("  000123",    "%#8.6o", 0123);    // alternative representation w/ width and precision
    }
    {
        //
        // hexadeciaml
        //
        TEST("0xa55",       "%#x", 0xa55);      // alternative representation
        TEST("0",           "%#x", 0);          // alternative representation w/ zero
        TEST("0xa55",       "%#.x", 0xa55);     // alternative representation w/ zero precision
        TEST("",            "%#.x", 0);         // alternative representation w/ zero precision on zero
        TEST("0x00000a55",  "%#.8x", 0xa55);    // alternative representation w/ large precision
        TEST("0x00a55",     "%#.5x", 0xa55);    // alternative representation w/ equal precision
        TEST("0xa55",       "%#.1x", 0xa55);    // alternative representation w/ small precision
        TEST("       0",    "%#8x", 0);         // alternative representation w/ width
        TEST("00000000",    "%#08x", 0);        // alternative representation w/ width, zero-pad
        TEST("A55",         "%X", 0xa55);       // uppercase
        TEST("0XA55",       "%#X", 0xa55);      // uppercase, alternative representation
    }

    #undef TEST
    #undef _TEST_CHECK
    #undef _TEST_FN
}
