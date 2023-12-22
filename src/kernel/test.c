#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

void testprint(const char *msg)
{
    const char *a = msg;
    while (*a != '\0')
    {
        putchar(*a++);
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

    // not supported:
    // printf("Floating point:\n");
    // printf("\tRounding:\t%f %.0f %.32f\n", 1.5, 1.5, 1.3);
    // printf("\tPadding:\t%05.2f %.2f %5.2f\n", 1.5, 1.5, 1.5);
    // printf("\tScientific:\t%E %e\n", 1.5, 1.5);
    // printf("\tHexadecimal:\t%a %A\n", 1.5, 1.5);
    // printf("\tSpecial values:\t0/0=%g 1/0=%g\n", 0.0/0.0, 1.0/0.0);

    // printf("Fixed-width types:\n");
    // printf("\tLargest 32-bit value is %" PRIu32 " or %#" PRIx32 "\n",
    //                                     UINT32_MAX,     UINT32_MAX );
}

bool test_printf()
{
    bool pass = true;
    char buf[256];

    // Assumes terminal works to some extent (and puts())
    // TODO: puts() is supposed to emit a trailing newline...

    #define _TEST_FN(exp,...) \
        snprintf(buf, sizeof(buf), __VA_ARGS__); \
        bool _pass = (strcmp(buf, exp) == 0); \

    #define _TEST_CHECK(exp,...) \
        if (!_pass) { \
            testprint("!! PRINTF FAILED: " #__VA_ARGS__ "\n"); \
            testprint("!! \texp='" exp "'\n"); \
            testprint("!! \tgot='"); \
            testprint(buf); \
            testprint("'\n"); \
        } \
        pass &= _pass; \

    #define TEST(exp,...) \
    do { \
        _TEST_FN(exp, __VA_ARGS__) \
        _TEST_CHECK(exp, __VA_ARGS__) \
    } while (0)

    // printf_reference();

    // expectation,     format, args

    // string, char (%s, %c)
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

    // // signed integer (%d, %i)
    // TEST("0",           "%d", 0);           // zero
    // TEST("-1",          "%d", 0xFFFFFFFF);  // negative
    // TEST("2147483647",  "%d", 0x7FFFFFFF);  // max
    // TEST("-2147483648", "%d", 0x80000000);  // min
    // TEST("-1",          "%hhd", (char) 0xFF); // negative
    // TEST("127",         "%hhd", (char) 0x7F); // max
    // TEST("-128",        "%hhd", (char) 0x80); // min
    // TEST("-1",          "%hd", (short) 0xFFFF); // negative
    // TEST("32767",       "%hd", (short) 0x7FFF); // max
    // TEST("-32768",      "%hd", (short) 0x8000); // min
    // if (sizeof(long) == 4) {
    //     TEST("-1",          "%ld", (long) 0xFFFFFFFFL); // negative
    //     TEST("2147483647",  "%ld", (long) 0x7FFFFFFFL); // max
    //     TEST("-2147483648", "%ld", (long) 0x80000000L); // min
    // }
    // else {
    //     TEST("-1",          "%ld", (long) 0xFFFFFFFFFFFFFFFFL); // negative
    //     TEST("9223372036854775807",  "%ld", (long) 0x7FFFFFFFFFFFFFFFL); // max
    //     TEST("-9223372036854775808", "%ld", (long) 0x8000000000000000L); // min
    // }
    // TEST("-1",          "%lld", (long long) 0xFFFFFFFFFFFFFFFFLL); // negative
    // TEST("9223372036854775807",  "%lld", (long long) 0x7FFFFFFFFFFFFFFFLL); // max
    // TEST("-9223372036854775808", "%lld", (long long) 0x8000000000000000LL); // min
    // TEST("+123",        "%+d", 123);        // sign flag
    // TEST("-123",        "%+d", -123);       // sign flag w/ negative
    // TEST(" 123",        "% d", 123);        // space flag
    // TEST("-123",        "% d", -123);       // space flag w/ negative
    // TEST("+123",        "% +d", 123);       // space (ignored due to sign flag)
    // TEST("+123",        "%+ d", 123);       // space (ignored due to sign flag)
    // TEST("123",         "%0d", 123);        // zero-pad w/ no width specifier (ignored)
    // TEST("   123",      "%6d", 123);        // right justify
    // TEST("  -123",      "%6d", -123);       // right justify, negative
    // TEST("  +123",      "%+6d", 123);       // right justify, sign
    // TEST("   123",      "% 6d", 123);       // right justify, space
    // TEST("000123",      "%06d", 123);       // right justify, zero-pad
    // TEST("-00123",      "%06d", -123);      // right justify, zero-pad, negative
    // TEST("+00123",      "%+06d", 123);      // right justify, zero-pad, sign
    // TEST("+00123",      "%0+6d", 123);      // right justify, zero-pad, sign
    // TEST(" 00123",      "% 06d", 123);      // right justify, zero-pad, space
    // TEST(" 00123",      "%0 6d", 123);      // right justify, zero-pad, space
    // TEST("   123",      "%*d", 6, 123);     // right justify w/ arg
    // TEST("123   ",      "%-6d", 123);       // left justify
    // TEST("-123  ",      "%-6d", -123);      // left justify, negative
    // TEST("+123  ",      "%-+6d", 123);      // left justify, sign
    // TEST("+123  ",      "%+-6d", 123);      // left justify, sign
    // TEST(" 123  ",      "%- 6d", 123);      // left justify, space
    // TEST(" 123  ",      "% -6d", 123);      // left justify, space
    // TEST("123   ",      "%-06d", 123);      // left justify, zero-pad (ignored)
    // TEST("123   ",      "%0-6d", 123);      // left justify, zero-pad (ignored)
    // TEST("123   ",      "%-*d", 6, 123);    // left justify w/ arg
    // TEST("",            "%.0d", 0);         // zero precision on zero
    // TEST("123",         "%.0d", 123);       // zero precision on nonzero
    // TEST("",            "%.d", 0);          // implied zero precision
    // TEST("000123",      "%.6d", 123);       // precision
    // TEST("000123   ",   "%-9.6d", 123);     // precision, left justified
    // TEST("   000123",   "%9.6d", 123);      // precision, right justified
    // TEST("   000123",   "%09.6d", 123);     // precision, right justified, zero-pad (ignored due to explicit precision)
    // TEST("000123",      "%.*d", 6, 123);    // precision w/ arg
    // TEST("123",         "%.*d", -6, 123);   // negative precision (ignored)
    // TEST("123",         "%.3d", 123);       // equal precision
    // TEST("123",         "%.1d", 123);       // small precision
    // TEST("   000123",   "%*.*d", 9, 6, 123);// width & precision w/ arg
    // TEST("       +000009223372036854775807", "%+32.24lld", 0x7FFFFFFFFFFFFFFFLL); // big complicated format

    // // unsigned integer (%u)
    // TEST("0",           "%u", 0);           // zero
    // TEST("4294967295",  "%u", 0xFFFFFFFFU);  // negative
    // TEST("2147483647",  "%u", 0x7FFFFFFFU);  // max
    // TEST("2147483648",  "%u",  0x80000000U);  // min
    // TEST("255",         "%hhu", (unsigned char) 0xFFU); // negative
    // TEST("127",         "%hhu", (unsigned char) 0x7FU); // max
    // TEST("128",         "%hhu", (unsigned char) 0x80U); // min
    // TEST("65535",       "%hu", (unsigned short) 0xFFFFU); // negative
    // TEST("32767",       "%hu", (unsigned short) 0x7FFFU); // max
    // TEST("32768",       "%hu", (unsigned short) 0x8000U); // min
    // if (sizeof(unsigned long) == 4) {
    //     TEST("4294967295",  "%lu", (unsigned long) 0xFFFFFFFFUL); // negative
    //     TEST("2147483647",  "%lu", (unsigned long) 0x7FFFFFFFUL); // max
    //     TEST("2147483648",  "%lu", (unsigned long) 0x80000000UL); // min
    // }
    // else {
    //     TEST("18446744073709551615", "%lu", (unsigned long) 0xFFFFFFFFFFFFFFFFUL); // negative
    //     TEST("9223372036854775807", "%lu", (unsigned long) 0x7FFFFFFFFFFFFFFFUL); // max
    //     TEST("9223372036854775808", "%lu", (unsigned long) 0x8000000000000000UL); // min
    // }
    // TEST("18446744073709551615", "%llu", (unsigned long long) 0xFFFFFFFFFFFFFFFFULL); // negative
    // TEST("9223372036854775807", "%llu", (unsigned long long) 0x7FFFFFFFFFFFFFFFULL); // max
    // TEST("9223372036854775808", "%llu", (unsigned long long) 0x8000000000000000ULL); // min
    // TEST("123",         "%+u", 123);        // sign flag (ignored due to unsigned)
    // TEST("123",         "% u", 123);        // space flag (ignored due to unsigned)
    // TEST("123",         "% +u", 123);       // space (ignored due to sign flag)
    // TEST("123",         "%+ u", 123);       // space (ignored due to sign flag)
    // TEST("123",         "%0u", 123);        // zero-pad w/ no width specifier (ignored)
    // TEST("   123",      "%6u", 123);        // right justify
    // TEST("   123",      "%+6u", 123);       // right justify, sign (ignored due to unsigned)
    // TEST("   123",      "% 6u", 123);       // right justify, space
    // TEST("000123",      "%06u", 123);       // right justify, zero-pad
    // TEST("000123",      "%+06u", 123);      // right justify, zero-pad, sign (ignored due to unsigned)
    // TEST("000123",      "%0+6u", 123);      // right justify, zero-pad, sign (ignored due to unsigned)
    // TEST("000123",      "% 06u", 123);      // right justify, zero-pad, space (ignored due to unsigned)
    // TEST("000123",      "%0 6u", 123);      // right justify, zero-pad, space (ignored due to unsigned)
    // TEST("   123",      "%*u", 6, 123);     // right justify w/ arg
    // TEST("123   ",      "%-6u", 123);       // left justify
    // TEST("123   ",      "%-+6u", 123);      // left justify, sign (ignored due to unsigned)
    // TEST("123   ",      "%+-6u", 123);      // left justify, sign (ignored due to unsigned)
    // TEST("123   ",      "%- 6u", 123);      // left justify, space (ignored due to unsigned)
    // TEST("123   ",      "% -6u", 123);      // left justify, space (ignored due to unsigned)
    // TEST("123   ",      "%-06u", 123);      // left justify, zero-pad (ignored)
    // TEST("123   ",      "%0-6u", 123);      // left justify, zero-pad (ignored)
    // TEST("123   ",      "%-*u", 6, 123);    // left justify w/ arg
    // TEST("",            "%.0u", 0);         // zero precision on zero
    // TEST("123",         "%.0u", 123);       // zero precision on nonzero
    // TEST("",            "%.u", 0);          // implied zero precision
    // TEST("000123",      "%.6u", 123);       // precision
    // TEST("000123   ",   "%-9.6u", 123);     // precision, left justified
    // TEST("   000123",   "%9.6u", 123);      // precision, right justified
    // TEST("   000123",   "%09.6u", 123);     // precision, right justified, zero-pad (ignored due to explicit precision)
    // TEST("000123",      "%.*u", 6, 123);    // precision w/ arg
    // TEST("123",         "%.*u", -6, 123);   // negative precision (ignored)
    // TEST("123",         "%.3u", 123);       // equal precision
    // TEST("123",         "%.1u", 123);       // small precision
    // TEST("   000123",   "%*.*u", 9, 6, 123);// width & precision w/ arg
    // TEST("        000018446744073709551615", "%+32.24llu", 0xFFFFFFFFFFFFFFFFULL); // big complicated format

    // // octal (%o)
    // TEST("0",           "%o", 0);           // zero
    // TEST("37777777777", "%o", 0xFFFFFFFFU);  // negative
    // TEST("17777777777", "%o", 0x7FFFFFFFU);  // max
    // TEST("20000000000", "%o",  0x80000000U);  // min
    // TEST("377",         "%hho", (unsigned char) 0xFFU); // negative
    // TEST("177",         "%hho", (unsigned char) 0x7FU); // max
    // TEST("200",         "%hho", (unsigned char) 0x80U); // min
    // TEST("177777",       "%ho", (unsigned short) 0xFFFFU); // negative
    // TEST("77777",       "%ho", (unsigned short) 0x7FFFU); // max
    // TEST("100000",       "%ho", (unsigned short) 0x8000U); // min
    // if (sizeof(unsigned long) == 4) {
    //     TEST("37777777777",  "%lo", (unsigned long) 0xFFFFFFFFUL); // negative
    //     TEST("17777777777",  "%lo", (unsigned long) 0x7FFFFFFFUL); // max
    //     TEST("20000000000",  "%lo", (unsigned long) 0x80000000UL); // min
    // }
    // else {
    //     TEST("1777777777777777777777", "%lo", (unsigned long) 0xFFFFFFFFFFFFFFFFUL); // negative
    //     TEST("777777777777777777777", "%lo", (unsigned long) 0x7FFFFFFFFFFFFFFFUL); // max
    //     TEST("1000000000000000000000", "%lo", (unsigned long) 0x8000000000000000UL); // min
    // }
    // TEST("1777777777777777777777", "%llo", (unsigned long long) 0xFFFFFFFFFFFFFFFFULL); // negative
    // TEST("777777777777777777777", "%llo", (unsigned long long) 0x7FFFFFFFFFFFFFFFULL); // max
    // TEST("1000000000000000000000", "%llo", (unsigned long long) 0x8000000000000000ULL); // min
    // TEST("123",         "%+o", 0123);       // sign flag (ignored due to unsigned)
    // TEST("123",         "% o", 0123);       // space flag (ignored due to unsigned)
    // TEST("123",         "% +o", 0123);      // space (ignored due to sign flag)
    // TEST("123",         "%+ o", 0123);      // space (ignored due to sign flag)
    // TEST("123",         "%0o", 0123);       // zero-pad w/ no width specifier (ignored)
    // TEST("   123",      "%6o", 0123);       // right justify
    // TEST("   123",      "%+6o", 0123);      // right justify, sign (ignored due to unsigned)
    // TEST("   123",      "% 6o", 0123);      // right justify, space
    // TEST("000123",      "%06o", 0123);      // right justify, zero-pad
    // TEST("000123",      "%+06o", 0123);     // right justify, zero-pad, sign (ignored due to unsigned)
    // TEST("000123",      "%0+6o", 0123);     // right justify, zero-pad, sign (ignored due to unsigned)
    // TEST("000123",      "% 06o", 0123);     // right justify, zero-pad, space (ignored due to unsigned)
    // TEST("000123",      "%0 6o", 0123);     // right justify, zero-pad, space (ignored due to unsigned)
    // TEST("   123",      "%*o", 6, 0123);    // right justify w/ arg
    // TEST("  0123",      "%#6o", 0123);      // right justify w/ alternative representation
    // TEST("123   ",      "%-6o", 0123);      // left justify
    // TEST("123   ",      "%-+6o", 0123);     // left justify, sign (ignored due to unsigned)
    // TEST("123   ",      "%+-6o", 0123);     // left justify, sign (ignored due to unsigned)
    // TEST("123   ",      "%- 6o", 0123);     // left justify, space (ignored due to unsigned)
    // TEST("123   ",      "% -6o", 0123);     // left justify, space (ignored due to unsigned)
    // TEST("123   ",      "%-06o", 0123);     // left justify, zero-pad (ignored)
    // TEST("123   ",      "%0-6o", 0123);     // left justify, zero-pad (ignored)
    // TEST("123   ",      "%-*o", 6, 0123);   // left justify w/ arg
    // TEST("0123  ",      "%#-6o", 0123);     // left justify w/ alternative representation
    // TEST("",            "%.0o", 0);         // zero precision on zero
    // TEST("123",         "%.0o", 0123);      // zero precision on nonzero
    // TEST("",            "%.o", 0);          // implied zero precision
    // TEST("000123",      "%.6o", 0123);      // precision
    // TEST("000123   ",   "%-9.6o", 0123);    // precision, left justified
    // TEST("   000123",   "%9.6o", 0123);     // precision, right justified
    // TEST("   000123",   "%09.6o", 0123);    // precision, right justified, zero-pad (ignored due to explicit precision)
    // TEST("000123",      "%.*o", 6, 0123);   // precision w/ arg
    // TEST("123",         "%.*o", -6, 0123);  // negative precision (ignored)
    // TEST("123",         "%.3o", 0123);      // equal precision
    // TEST("123",         "%.1o", 0123);      // small precision
    // TEST("   000123",   "%*.*o", 9, 6, 0123);// width & precision w/ arg
    // TEST("0123",        "%#o", 0123);       // alternative representation
    // TEST("0",           "%#o", 0);          // alternative representation w/ zero
    // TEST("0",           "%#.o", 0);         // alternative representation w/ zero precision on zero
    // TEST("000123",      "%#.6o", 0123);     // alternative representation w/ large precision
    // TEST("0123",        "%#.3o", 0123);     // alternative representation w/ equal precision
    // TEST("0123",        "%#.1o", 0123);     // alternative representation w/ small precision
    // TEST("        001777777777777777777777", "%+#32.24llo", 0xFFFFFFFFFFFFFFFFULL); // big complicated format

    // // hexadecimal, lowercase (%x)
    // TEST("0",           "%x", 0);           // zero
    // TEST("ffffffff",    "%x", 0xFFFFFFFFU);  // negative
    // TEST("7fffffff",    "%x", 0x7FFFFFFFU);  // max
    // TEST("80000000",    "%x",  0x80000000U);  // min
    // TEST("ff",          "%hhx", (unsigned char) 0xFFU); // negative
    // TEST("7f",          "%hhx", (unsigned char) 0x7FU); // max
    // TEST("80",          "%hhx", (unsigned char) 0x80U); // min
    // TEST("ffff",        "%hx", (unsigned short) 0xFFFFU); // negative
    // TEST("7fff",        "%hx", (unsigned short) 0x7FFFU); // max
    // TEST("8000",        "%hx", (unsigned short) 0x8000U); // min
    // if (sizeof(unsigned long) == 4) {
    //     TEST("ffffffff",    "%lx", (unsigned long) 0xFFFFFFFFUL); // negative
    //     TEST("7fffffff",    "%lx", (unsigned long) 0x7FFFFFFFUL); // max
    //     TEST("80000000",    "%lx", (unsigned long) 0x80000000UL); // min
    // }
    // else {
    //     TEST("ffffffffffffffff", "%lx", (unsigned long) 0xFFFFFFFFFFFFFFFFUL); // negative
    //     TEST("7fffffffffffffff", "%lx", (unsigned long) 0x7FFFFFFFFFFFFFFFUL); // max
    //     TEST("8000000000000000", "%lx", (unsigned long) 0x8000000000000000UL); // min
    // }
    // TEST("ffffffffffffffff", "%llx", (unsigned long long) 0xFFFFFFFFFFFFFFFFULL); // negative
    // TEST("7fffffffffffffff", "%llx", (unsigned long long) 0x7FFFFFFFFFFFFFFFULL); // max
    // TEST("8000000000000000", "%llx", (unsigned long long) 0x8000000000000000ULL); // min
    // TEST("123",         "%+x", 0x123);      // sign flag (ignored due to unsigned)
    // TEST("123",         "% x", 0x123);      // space flag (ignored due to unsigned)
    // TEST("123",         "% +x", 0x123);     // space (ignored due to sign flag)
    // TEST("123",         "%+ x", 0x123);     // space (ignored due to sign flag)
    // TEST("123",         "%0x", 0x123);      // zero-pad w/ no width specifier (ignored)
    // TEST("   123",      "%6x", 0x123);      // right justify
    // TEST("   123",      "%+6x", 0x123);     // right justify, sign (ignored due to unsigned)
    // TEST("   123",      "% 6x", 0x123);     // right justify, space
    // TEST("000123",      "%06x", 0x123);     // right justify, zero-pad
    // TEST("000123",      "%+06x", 0x123);    // right justify, zero-pad, sign (ignored due to unsigned)
    // TEST("000123",      "%0+6x", 0x123);    // right justify, zero-pad, sign (ignored due to unsigned)
    // TEST("000123",      "% 06x", 0x123);    // right justify, zero-pad, space (ignored due to unsigned)
    // TEST("000123",      "%0 6x", 0x123);    // right justify, zero-pad, space (ignored due to unsigned)
    // TEST("   123",      "%*x", 6, 0x123);   // right justify w/ arg
    // TEST(" 0x123",      "%#6x", 0x123);     // right justify w/ alternative representation
    // TEST("123   ",      "%-6x", 0x123);     // left justify
    // TEST("123   ",      "%-+6x", 0x123);    // left justify, sign (ignored due to unsigned)
    // TEST("123   ",      "%+-6x", 0x123);    // left justify, sign (ignored due to unsigned)
    // TEST("123   ",      "%- 6x", 0x123);    // left justify, space (ignored due to unsigned)
    // TEST("123   ",      "% -6x", 0x123);    // left justify, space (ignored due to unsigned)
    // TEST("123   ",      "%-06x", 0x123);    // left justify, zero-pad (ignored)
    // TEST("123   ",      "%0-6x", 0x123);    // left justify, zero-pad (ignored)
    // TEST("123   ",      "%-*x", 6, 0x123);  // left justify w/ arg
    // TEST("0x123 ",      "%#-6x", 0x123);    // left justify w/ alternative representation
    // TEST("",            "%.0x", 0);         // zero precision on zero
    // TEST("123",         "%.0x", 0x123);     // zero precision on nonzero
    // TEST("",            "%.x", 0);          // implied zero precision
    // TEST("000123",      "%.6x", 0x123);     // precision
    // TEST("000123   ",   "%-9.6x", 0x123);   // precision, left justified
    // TEST("   000123",   "%9.6x", 0x123);    // precision, right justified
    // TEST("   000123",   "%09.6x", 0x123);   // precision, right justified, zero-pad (ignored due to explicit precision)
    // TEST("000123",      "%.*x", 6, 0x123);  // precision w/ arg
    // TEST("123",         "%.*x", -6, 0x123); // negative precision (ignored)
    // TEST("123",         "%.3x", 0x123);     // equal precision
    // TEST("123",         "%.1x", 0x123);     // small precision
    // TEST("   000123",   "%*.*x", 9, 6, 0x123);// width & precision w/ arg
    // TEST("0x123",       "%#x", 0x123);      // alternative representation
    // TEST("0",           "%#x", 0);          // alternative representation w/ zero
    // TEST("",            "%#.x", 0);         // alternative representation w/ zero precision on zero
    // TEST("0x000123",    "%#.6x", 0x123);    // alternative representation w/ large precision
    // TEST("0x123",       "%#.3x", 0x123);    // alternative representation w/ equal precision
    // TEST("0x123",       "%#.1x", 0x123);    // alternative representation w/ small precision
    // TEST("      0x000000000123456789abcdef", "%+#32.24llx", 0x123456789ABCDEFULL); // big complicated format

    // // hexadecimal, uppercase (%X)
    // TEST("0",           "%X", 0);           // zero
    // TEST("FFFFFFFF",    "%X", 0xFFFFFFFFU);  // negative
    // TEST("7FFFFFFF",    "%X", 0x7FFFFFFFU);  // max
    // TEST("80000000",    "%X",  0x80000000U);  // min
    // TEST("FF",          "%hhX", (unsigned char) 0xFFU); // negative
    // TEST("7F",          "%hhX", (unsigned char) 0x7FU); // max
    // TEST("80",          "%hhX", (unsigned char) 0x80U); // min
    // TEST("FFFF",        "%hX", (unsigned short) 0xFFFFU); // negative
    // TEST("7FFF",        "%hX", (unsigned short) 0x7FFFU); // max
    // TEST("8000",        "%hX", (unsigned short) 0x8000U); // min
    // if (sizeof(unsigned long) == 4) {
    //     TEST("FFFFFFFF",    "%lX", (unsigned long) 0xFFFFFFFFUL); // negative
    //     TEST("7FFFFFFF",    "%lX", (unsigned long) 0x7FFFFFFFUL); // max
    //     TEST("80000000",    "%lX", (unsigned long) 0x80000000UL); // min
    // }
    // else {
    //     TEST("FFFFFFFFFFFFFFFF", "%lX", (unsigned long) 0xFFFFFFFFFFFFFFFFUL); // negative
    //     TEST("7FFFFFFFFFFFFFFF", "%lX", (unsigned long) 0x7FFFFFFFFFFFFFFFUL); // max
    //     TEST("8000000000000000", "%lX", (unsigned long) 0x8000000000000000UL); // min
    // }
    // TEST("FFFFFFFFFFFFFFFF", "%llX", (unsigned long long) 0xFFFFFFFFFFFFFFFFULL); // negative
    // TEST("7FFFFFFFFFFFFFFF", "%llX", (unsigned long long) 0x7FFFFFFFFFFFFFFFULL); // max
    // TEST("8000000000000000", "%llX", (unsigned long long) 0x8000000000000000ULL); // min
    // TEST("123",         "%+X", 0x123);      // sign flag (ignored due to unsigned)
    // TEST("123",         "% X", 0x123);      // space flag (ignored due to unsigned)
    // TEST("123",         "% +X", 0x123);     // space (ignored due to sign flag)
    // TEST("123",         "%+ X", 0x123);     // space (ignored due to sign flag)
    // TEST("123",         "%0X", 0x123);      // zero-pad w/ no width specifier (ignored)
    // TEST("   123",      "%6X", 0x123);      // right justify
    // TEST("   123",      "%+6X", 0x123);     // right justify, sign (ignored due to unsigned)
    // TEST("   123",      "% 6X", 0x123);     // right justify, space
    // TEST("000123",      "%06X", 0x123);     // right justify, zero-pad
    // TEST("000123",      "%+06X", 0x123);    // right justify, zero-pad, sign (ignored due to unsigned)
    // TEST("000123",      "%0+6X", 0x123);    // right justify, zero-pad, sign (ignored due to unsigned)
    // TEST("000123",      "% 06X", 0x123);    // right justify, zero-pad, space (ignored due to unsigned)
    // TEST("000123",      "%0 6X", 0x123);    // right justify, zero-pad, space (ignored due to unsigned)
    // TEST("   123",      "%*X", 6, 0x123);   // right justify w/ arg
    // TEST(" 0X123",      "%#6X", 0x123);     // right justify w/ alternative representation
    // TEST("123   ",      "%-6X", 0x123);     // left justify
    // TEST("123   ",      "%-+6X", 0x123);    // left justify, sign (ignored due to unsigned)
    // TEST("123   ",      "%+-6X", 0x123);    // left justify, sign (ignored due to unsigned)
    // TEST("123   ",      "%- 6X", 0x123);    // left justify, space (ignored due to unsigned)
    // TEST("123   ",      "% -6X", 0x123);    // left justify, space (ignored due to unsigned)
    // TEST("123   ",      "%-06X", 0x123);    // left justify, zero-pad (ignored)
    // TEST("123   ",      "%0-6X", 0x123);    // left justify, zero-pad (ignored)
    // TEST("123   ",      "%-*X", 6, 0x123);  // left justify w/ arg
    // TEST("0X123 ",      "%#-6X", 0x123);    // left justify w/ alternative representation
    // TEST("",            "%.0X", 0);         // zero precision on zero
    // TEST("123",         "%.0X", 0x123);     // zero precision on nonzero
    // TEST("",            "%.X", 0);          // implied zero precision
    // TEST("000123",      "%.6X", 0x123);     // precision
    // TEST("000123   ",   "%-9.6X", 0x123);   // precision, left justified
    // TEST("   000123",   "%9.6X", 0x123);    // precision, right justified
    // TEST("   000123",   "%09.6X", 0x123);   // precision, right justified, zero-pad (ignored due to explicit precision)
    // TEST("000123",      "%.*X", 6, 0x123);  // precision w/ arg
    // TEST("123",         "%.*X", -6, 0x123); // negative precision (ignored)
    // TEST("123",         "%.3X", 0x123);     // equal precision
    // TEST("123",         "%.1X", 0x123);     // small precision
    // TEST("   000123",   "%*.*X", 9, 6, 0x123);// width & precision w/ arg
    // TEST("0X123",       "%#X", 0x123);      // alternative representation
    // TEST("0",           "%#X", 0);          // alternative representation w/ zero
    // TEST("",            "%#.X", 0);         // alternative representation w/ zero precision on zero
    // TEST("0X000123",    "%#.6X", 0x123);    // alternative representation w/ large precision
    // TEST("0X123",       "%#.3X", 0x123);    // alternative representation w/ equal precision
    // TEST("0X123",       "%#.1X", 0x123);    // alternative representation w/ small precision
    // TEST("      0X000000000123456789ABCDEF", "%+#32.24llX", 0x123456789ABCDEFULL); // big complicated format
    #undef TEST

    return pass;
}

bool run_tests(void)
{
    bool pass = true;
    pass &= test_printf();
    return pass;
}

#ifdef MAIN
int main(int argc, char **argv)
{
    bool pass= run_tests();
    return (pass) ? 0 : 1;
}
#endif