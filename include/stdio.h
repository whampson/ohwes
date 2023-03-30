/* =============================================================================
 * Copyright (C) 2020-2023 Wes Hampson. All Rights Reserved.
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
 *         File: include/stdio.h
 *      Created: December 14, 2020
 *       Author: Wes Hampson
 *       Module: C Standard Library (C99)
 * =============================================================================
 */


/* Status: INCOMPLETE */

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
#define L_tmpnam        8       /* arbitrary for now... */  // what the fuck is this?

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

/**
 * printf()-family functions.
 *
 *     %[flags][width][.precision][length]specifier
 *
 * Specifier    Output                                      Support
 * -----------------------------------------------------------------------------
 * d or i       signed decimal integer                      SUPPORTED
 * u            unsigned decimal integer                    SUPPORTED
 * o            unsigned octal                              SUPPORTED
 * x            unsigned hexadecimal integer                SUPPORTED
 * X            unsigned hexadecimal integer, uppercase     SUPPORTED
 * f            decimal floating-point, lowercase           NOT IMPLEMENTED
 * F            decimal floating-point, uppercase           NOT IMPLEMENTED
 * e            scientific notation, lowercase              NOT SUPPORTED
 * E            scientific notation, uppercase              NOT SUPPORTED
 * g            use the shortest representation: %e or %f   NOT SUPPORTED
 * G            use the shortest representation: %E or %F   NOT SUPPORTED
 * a            hexadecimal floating-point, lowercase       NOT SUPPORTED
 * A            hexadecimal floating-point, uppercase       NOT SUPPORTED
 * c            character                                   SUPPORTED
 * s            string of characters                        SUPPORTED
 * p            pointer address                             SUPPORTED
 * n            write chars printed to address              NOT SUPPORTED
 * %            writes a '%' character                      SUPPORTED
 *
 *
 * Flag         Description                                 Support
 * -----------------------------------------------------------------------------
 * -            left-justify within the given width         SUPPORTED
 * +            always print a sign                         SUPPORTED
 * (space)      write blank space if no sign to be printed  SUPPORTED
 * #            always print hex prefix or decimal dot      SUPPORTED
 * 0            left-pad with zeros                         SUPPORTED
 *
 *
 * Width        Description                                 Support
 * -----------------------------------------------------------------------------
 * (number)     minimum number of characters                SUPPORTED
 * *            width specified in argument list            SUPPORTED
 *
 *
 * Precision    Description                                 Support
 * -----------------------------------------------------------------------------
 * .(number)    diouxX: minimum number of digits            SUPPORTED
 *              fF:     minumum number of decimal places    NOT IMPLEMENTED
 *              s:      maximum number of characters        SUPPORTED
 * .*           precision specified in argument list        SUPPORTED
 *
 *
 * Length   di          uoxX        fFeEgGaAa   c           s           p
 * -----------------------------------------------------------------------------
 * (none)   int         u int       not impl    int         char*       void*
 * hh       char        u char      n/a         n/a         n/a         n/a
 * h        s int       us int      n/a         n/a         n/a         n/a
 * l        l int       ul int      n/a         no support  no support  n/a
 * ll       not impl    not impl    n/a         n/a         n/a         n/a
 * j        not impl    not impl    n/a         n/a         n/a         n/a
 * z        size_t      size_t      n/a         n/a         n/a         n/a
 * t        ptrdiff_t   ptrdiff_t   n/a         n/a         n/a         n/a
 * L        n/a         n/a         no support  n/a         n/a         n/a
 */

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
int vsnprintf(char *str, size_t n, const char *fmt, va_list args);
// int vfprintf(FILE *stream, const char *fmt, va_list args);
// int vfscanf(FILE *stream, const char *fmt, va_list args);

int getchar(void);
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
void perror(const char *msg);

#endif /* __STDIO_H */
