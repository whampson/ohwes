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
 *         File: src/libc/test/framework.c
 *      Created: May 22, 2026
 *       Author: Wes Hampson
 * =============================================================================
 */

#include "framework.h"

int tests_run    = 0;
int tests_passed = 0;
int tests_failed = 0;

struct failure_record failure_log[MAX_FAIL];

void record_failure(const char *msg, const char *file, int line)
{
    if (tests_failed < MAX_FAIL) {
        struct failure_record *entry = &failure_log[tests_failed];
        memset(entry, 0, sizeof(struct failure_record));
        strncpy(entry->msg,  msg,  MAX_FAIL_MSG_LEN - 1);
        strncpy(entry->file, file, MAX_FAIL_FILE_LEN - 1);
        entry->line = line;
    }
    tests_failed++;
}

void print_failure_summary(void)
{
    if (tests_failed == 0)
        return;

    int logged = tests_failed < MAX_FAIL ? tests_failed : MAX_FAIL;
    printf("\n" COLOR_BOLD COLOR_RED "--- Failed Tests ---" COLOR_RESET "\n\n");
    for (int i = 0; i < logged; i++) {
        printf("  %d) " COLOR_RED "%s" COLOR_RESET " (%s:%d)\n",
               i + 1, failure_log[i].msg, failure_log[i].file, failure_log[i].line);
    }
    if (tests_failed > MAX_FAIL) {
        int hidden = tests_failed - MAX_FAIL;
        printf(COLOR_YELLOW "(log exceeded, %d failure%s not shown)" COLOR_RESET "\n",
               hidden, hidden != 1 ? "s" : "");
    }
}
