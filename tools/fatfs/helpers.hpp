/*============================================================================*
 * Copyright (C) 2020 Wes Hampson. All Rights Reserved.                       *
 *                                                                            *
 * This file is part of the Niobium Operating System.                         *
 * Niobium is free software; you may redistribute it and/or modify it under   *
 * the terms of the license agreement provided with this software.            *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
 *============================================================================*
 *    File: tools/fatfs/helpers.cpp                                           *
 * Created: December 1, 2020                                                  *
 *  Author: Wes Hampson                                                       *
 *============================================================================*/

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

// Trim leading spaces
inline std::string ltrim(const std::string &str)
{
    std::string s = str;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// Trim trailing spaces
inline std::string rtrim(const std::string &str)
{
    std::string s = str;
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// Trim leading and trailing spaces
inline std::string trim(const std::string &str)
{
    // why the fuck is there no string::trim() function in the standard library?
    return ltrim(rtrim(str));
}

// Extract filename from path
inline std::string get_filename(const std::string &path)
{
    return path.substr(path.find_last_of("/\\") + 1);
}

// Get filename without extension
inline std::string get_basename(const std::string &path)
{
    std::string s = get_filename(path);
    int dotpos = s.find_last_of('.');
    if (dotpos == -1) {
        return s;
    }
    return s.substr(0, dotpos);
}

// Get extension only
inline std::string get_extension(const std::string &path)
{
    std::string s = get_filename(path);
    int dotpos = s.find_last_of('.');
    if (dotpos == -1) {
        return "";
    }
    return s.substr(dotpos + 1);
}

}

#endif // __HELPERS_HPP
