#ifndef __HELPERS_HPP
#define __HELPERS_HPP

#include <algorithm>
#include <string>
#include <stdio.h>

// Printing
#define ERROR(msg)                                      \
do {                                                    \
    fprintf(stderr, "fatfs: error: " msg);              \
} while (0)
#define ERRORF(fmt, ...)                                \
do {                                                    \
    fprintf(stderr, "fatfs: error: " fmt, __VA_ARGS__); \
} while (0)

// Return If False
#define RIF(x)          do { if (!(x)) return false } while (0)
#define RIF_M(x,m)      do { if (!(x)) { ERROR(m); return false; } } while (0)
#define RIF_MF(x,m,...) do { if (!(x)) { ERRORF(m, __VA_ARGS__); return false; } } while (0)

// Return If True
#define RIT(x)          do { if (x) return false } while (0)
#define RIT_M(x,m)      do { if (x) { ERROR(m); return false; } } while (0)
#define RIT_MF(x,m,...) do { if (x) { ERRORF(m, __VA_ARGS__); return false; } } while (0)

namespace
{

// Gets the ceiling of x / y
inline int ceil(int x, int y)
{
    return (x + y - 1) / y;
}

// Get uppercase string
inline std::string upper(const std::string &str)
{
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// Trim trailing spaces
inline std::string rtrim(const std::string &str)
{
    // why the fuck is there no string::trim() function in the standard library?
    std::string s = str;
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// Extract filename from path
inline std::string get_filename(const std::string &path)
{
    return path.substr(path.find_last_of("/\\") + 1);
}

}

#endif // __HELPERS_HPP
