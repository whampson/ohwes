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
 *    File: include/stdio.h                                                   *
 * Created: December 14, 2020                                                 *
 *  Author: Wes Hampson                                                       *
 *                                                                            *
 * Implementation of stdio.h from the C Standard Library.                     *
 *============================================================================*/

/* Completion Status: INCOMPLETE */

#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>
#include <stdint.h>

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
typedef uint32_t size_t;
#endif

#ifndef __NULL_DEFINED
#define __NULL_DEFINED
#define NULL ((void *) 0)
#endif

typedef uint64_t fpos_t;

// typedef struct
// {
//     /* TODO */
// } FILE;

#define BUFSIZ          1024    /* arbitrary for now... */
#define EOF             (-1)
#define FILENAME_MAX    255     /* arbitrary for now... */
#define FOPEN_MAX       8       /* arbitrary for now... */
#define TMP_MAX         256     /* arbitrary for now... */
#define L_tmpnam        8       /* arbitrary for now... */

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

// #define stdin
// #define stdout
// #define stderr

// #define _IOFBF          0
// #define _IOLBF          1
// #define _IONBF          2

// #define SEEK_SET        0
// #define SEEK_CUR        1
// #define SEEK_END        2

// int remove(const char *filename);
// int rename(const char *oldname, const char *newname);
// FILE * tmpfile(void);
// char *tmpnam(char *str);

// int fclose(FILE *stream);
// int fflush(FILE *stream);
// FILE * fopen(const char *filename, const char *mode);
// FILE * freopen(const char *filename, const char *mode, FILE *stream);
// void setbuf(FILE *stream, char *buf);
// int setvbuf(FILE *stream, char *buf, int mode, size_t size);

int printf(const char *fmt, ...);
// int scanf(const char *fmt, ...);
// int sscanf(const char *str, const char *fmt, ...);
int sprintf(char *str, const char *fmt, ...);
int snprintf(char *str, size_t n, const char *fmt, ...);
// int fprintf(FILE *stream, const char *fmt, ...);
// int fscanf(FILE *stream, const char *fmt, ...);
int vprintf(const char *fmt, va_list args);
// int vscanf(const char *fmt, va_list args);
// int vsscanf(const char *str, const char *fmt, va_list args);
int vsprintf(char *str, const char *fmt, va_list args);
// int vsnprintf(char *str, size_t n, const char *fmt, va_list args);
// int vfprintf(FILE *stream, const char *fmt, va_list args);
// int vfscanf(FILE *stream, const char *fmt, va_list args);

// int getchar(void);
// int getc(FILE *stream);
// char * gets(char *str);
// int fgetc(FILE *stream);
// char * fgets(char *str, int num, FILE *stream);
int putchar(int ch);
// int putc(int ch, FILE *stream);
int puts(const char *str);
// int fputc(int ch, FILE *stream);
// int fputs(const char *str, FILE *stream);
// int ungetc(int ch, FILE *stream);

// size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
// size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);

// int fgetpos(FILE *stream, fpos_t *pos);
// int fseek(FILE *stream, int64_t offset, int origin);
// int fsetpos(FILE *stream, const fpos_t *pos);
// int64_t ftell(FILE *stream);
// void rewind(FILE *stream);

// void clearerr(FILE *stream);
// int feof(FILE *stream);
// int ferror(FILE *stream);
// void perror(const char *str);

#endif /* __STDIO_H */
