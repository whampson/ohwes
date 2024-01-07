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
 *         File: include/inttypes.h
 *      Created: January 4, 2024
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef __INTTYPES_H
#define __INTTYPES_H

#include <stdint.h>

#define PRId8       "d"
#define PRId16      "d"
#define PRId32      "d"
#define PRId64      "lld"
#define PRIdLEAST8  PRId8
#define PRIdLEAST16 PRId16
#define PRIdLEAST32 PRId32
#define PRIdLEAST64 PRId64
#define PRIdFAST8   PRId32
#define PRIdFAST16  PRId32
#define PRIdFAST32  PRId32
#define PRIdFAST64  PRId64
#define PRIdMAX     "jd"
#define PRIdPTR     "td"

#define PRIi8       "i"
#define PRIi16      "i"
#define PRIi32      "i"
#define PRIi64      "lli"
#define PRIiLEAST8  PRIi8
#define PRIiLEAST16 PRIi16
#define PRIiLEAST32 PRIi32
#define PRIiLEAST64 PRIi64
#define PRIiFAST8   PRIi32
#define PRIiFAST16  PRIi32
#define PRIiFAST32  PRIi32
#define PRIiFAST64  PRIi64
#define PRIiMAX     "ji"
#define PRIiPTR     "ti"

#define PRIu8       "u"
#define PRIu16      "u"
#define PRIu32      "u"
#define PRIu64      "llu"
#define PRIuLEAST8  PRIu8
#define PRIuLEAST16 PRIu16
#define PRIuLEAST32 PRIu32
#define PRIuLEAST64 PRIu64
#define PRIuFAST8   PRIu32
#define PRIuFAST16  PRIu32
#define PRIuFAST32  PRIu32
#define PRIuFAST64  PRIu64
#define PRIuMAX     "ju"
#define PRIuPTR     "tu"

#define PRIo8       "o"
#define PRIo16      "o"
#define PRIo32      "o"
#define PRIo64      "llo"
#define PRIoLEAST8  PRIo8
#define PRIoLEAST16 PRIo16
#define PRIoLEAST32 PRIo32
#define PRIoLEAST64 PRIo64
#define PRIoFAST8   PRIo32
#define PRIoFAST16  PRIo32
#define PRIoFAST32  PRIo32
#define PRIoFAST64  PRIo64
#define PRIoMAX     "jo"
#define PRIoPTR     "to"

#define PRIx8       "x"
#define PRIx16      "x"
#define PRIx32      "x"
#define PRIx64      "llx"
#define PRIxLEAST8  PRIx8
#define PRIxLEAST16 PRIx16
#define PRIxLEAST32 PRIx32
#define PRIxLEAST64 PRIx64
#define PRIxFAST8   PRIx32
#define PRIxFAST16  PRIx32
#define PRIxFAST32  PRIx32
#define PRIxFAST64  PRIx64
#define PRIxMAX     "jx"
#define PRIxPTR     "tx"

#define PRIX8       "X"
#define PRIX16      "X"
#define PRIX32      "X"
#define PRIX64      "llX"
#define PRIXLEAST8  PRIX8
#define PRIXLEAST16 PRIX16
#define PRIXLEAST32 PRIX32
#define PRIXLEAST64 PRIX64
#define PRIXFAST8   PRIX32
#define PRIXFAST16  PRIX32
#define PRIXFAST32  PRIX32
#define PRIXFAST64  PRIX64
#define PRIXMAX     "jX"
#define PRIXPTR     "tX"

#endif // __INTTYPES_H
