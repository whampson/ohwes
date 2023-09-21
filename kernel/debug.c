#include "init.h"
#include "debug.h"

#include <stdio.h>

void PrintHardwareInfo()
{
    const HwFlags hwFlags = g_pBootInfo->m_hwFlags;
    printf("boot: diskette drive? %s\n", hwFlags.HasDisketteDrive ? "yes" : "no");
    printf("boot: coprocessor? %s\n", hwFlags.HasCoprocessor ? "yes" : "no");
    printf("boot: PS/2 mouse? %s\n", hwFlags.HasPs2Mouse ? "yes" : "no");
    printf("boot: game port? %s\n", hwFlags.HasGamePort ? "yes" : "no");
    printf("boot: num serial ports = %d\n", hwFlags.NumSerialPorts);
    printf("boot: num parallel ports = %d\n", hwFlags.NumParallelPorts);
    printf("boot: num secondary diskette drives = %d\n", hwFlags.NumOtherDisketteDrives);
    printf("boot: video mode = ");
    switch (hwFlags.VideoMode) {
        case HWFLAGS_VIDEOMODE_40x25:       printf("40x25\n"); break;
        case HWFLAGS_VIDEOMODE_80x25:       printf("80x25\n"); break;
        case HWFLAGS_VIDEOMODE_80x25_MONO:  printf("80x25 (monochrome)\n"); break;
        default:                            printf("(invalid)\n"); break;
    }
}

void PrintMemoryInfo()
{
    printf("boot: A20 ");
    switch (g_pBootInfo->m_a20Method) {
        case A20METHOD_NONE:        printf("enabled\n"); break;
        case A20METHOD_KEYBOARD:    printf("enabled via PS/2 keyboard controller\n"); break;
        case A20METHOD_PORT92h:     printf("enabled via I/O port 92h\n"); break;
        case A20METHOD_BIOS:        printf("enabled via BIOS INT=15h,AX=2401h\n"); break;
        default:                    printf("(invalid)"); break;
    }
    printf("boot: g_RamLo_Legacy = %d\n", g_pBootInfo->m_ramCapacity_LoLegacy);
    printf("boot: g_RamHi_Legacy = %d\n", g_pBootInfo->m_ramCapacity_HiLegacy);
    printf("boot: g_RamLo_E801h = %d\n", g_pBootInfo->m_ramCapacity_LoE801h);
    printf("boot: g_RamHi_E801h = %d\n", g_pBootInfo->m_ramCapacity_HiE801h << 6);    // 64K pages to 1K pages

    const AcpiMemoryMapEntry *memMap = g_pBootInfo->m_pMemoryMap;
    if (memMap) {
        do {
            if (memMap->Length > 0) {
                printf("boot: BIOS-E820h: %08x-%08x ",
                    (uint32_t) memMap->Base,
                    (uint32_t) memMap->Base + memMap->Length - 1);

                switch (memMap->Type) {
                    case ACPI_MMAP_TYPE_USABLE:     printf("usable\n"); break;
                    case ACPI_MMAP_TYPE_RESERVED:   printf("reserved\n"); break;
                    case ACPI_MMAP_TYPE_ACPI:       printf("ACPI\n"); break;
                    case ACPI_MMAP_TYPE_ACPI_NVS:   printf("ACPI NV\n"); break;
                    case ACPI_MMAP_TYPE_BAD:        printf("bad\n"); break;
                    default:                        printf("reserved (%d)\n", memMap->Type); break;
                }
            }
        } while ((memMap++)->Type != ACPI_MMAP_TYPE_INVALID);
    }
}