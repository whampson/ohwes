/* =============================================================================
 * Copyright (C) 2023 Wes Hampson. All Rights Reserved.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * -----------------------------------------------------------------------------
 *        File: boot/x86/boot_stage2.S
 *      Created: Mar 21, 2023
 *       Author: Wes Hampson
 *  Environment: 16-bit Real Mode (except for jump to Protected Mode)
 *
 * Stage 2 boot loader. Initializes devices and loads the kernel.
 * =============================================================================
 */

#ifndef BOOT_H
#define BOOT_H

#define STAGE2_BASE         0x7E00

#define RESETMODE           0x0472  // Soft reset mode address
#define RESETMODE_NOMEMTEST 0x1234  // Skip memory test on soft reset

// #define PORT_PS2KBD_CMD     0x64
// #define PORT_PS2KBD_DATA    0x60

// #define PORT_SYSCNTL_A      0x92
// #define SYSCNTLA_A20        0x02

#define DUMMY_CS            0x08
#define DUMMY_DS            0x10

#ifndef  __ASSEMBLER__

// Put C-only stuff here!

#endif  // __ASSEMBLER__

#endif  // BOOT_H
