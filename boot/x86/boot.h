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
 *        File: boot/x86/boot.h
 *      Created: Mar 21, 2023
 *       Author: Wes Hampson
 * =============================================================================
 */

#ifndef BOOT_H
#define BOOT_H

#define IDT_BASE                0x0000
#define GDT_BASE                0x0800
#define MEMMAP_BASE             0x1000
#define STAGE1_BASE             0x7C00
#define STAGE2_BASE             0x7E00

#define A20METHOD_KEYBOARD      1
#define A20METHOD_PORT          2
#define A20METHOD_BIOS          3

#ifndef __ASSEMBLER__

// #include <inttypes.h>

extern char g_A20Method;

extern short g_RamCapacityLo;         // contiguous RAM <1M in 1K blocks
extern short g_RamCapacityHi;         // contiguous RAM >1M in 1K blocks, up to 15M or 64M
extern short g_RamCapacityLo_e801;    // contiguous RAM >1M in 1K blocks, up to 16M
extern short g_RamCapacityHi_e801;    // contiguous RAM >16M in 64K blocks

extern char g_bHasMemoryMap;
extern void *g_pAcpiMemoryMap;

extern short g_EquipmentFlags;

struct EquipmentFlags {
    short DisketteDrive     : 1;
    short Coprocessor       : 1;
    short Ps2Mouse          : 1;
    short                   : 1;
    short VideoMode         : 2;    // 00 = unused, 01 = 40x25, 10 = 80x25, 11 = 80x25 mono
    short NumOtherDiskette  : 2;    // num diskette drives attached less 1
    short _Dma              : 1;
    short NumSerialPorts    : 3;
    short GamePort          : 1;
    short _PrinterOrModem   : 1;
    short NumParallelPorts  : 2;
};
_Static_assert(sizeof(struct EquipmentFlags) == 2, "sizeof(struct EquipmentFlags)");

struct MemoryMapEntry {
    unsigned long long int Base;
    unsigned long long int Length;
    unsigned int Type;
    unsigned int ExtendedAttributes;
};
_Static_assert(sizeof(struct MemoryMapEntry) == 24, "sizeof(struct MemoryMapEntry)");
// Put C-only stuff here!

#endif  // __ASSEMBLER__

#endif  // BOOT_H
