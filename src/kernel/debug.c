#include <stdio.h>
#include <sys/boot.h>
#include "debug.h"


void PrintHardwareInfo()
{
    const HwFlags *pHwFlags = &g_pBootParams->m_HwFlags;
    printf("boot: diskette drive? %s\n", pHwFlags->HasDisketteDrive ? "yes" : "no");
    printf("boot: coprocessor? %s\n", pHwFlags->HasCoprocessor ? "yes" : "no");
    printf("boot: PS/2 mouse? %s\n", pHwFlags->HasPs2Mouse ? "yes" : "no");
    printf("boot: game port? %s\n", pHwFlags->HasGamePort ? "yes" : "no");
    printf("boot: num serial ports = %d\n", pHwFlags->NumSerialPorts);
    printf("boot: num parallel ports = %d\n", pHwFlags->NumParallelPorts);
    printf("boot: num secondary diskette drives = %d\n", pHwFlags->NumOtherDisketteDrives);
    printf("boot: video mode = ");
    switch (pHwFlags->VideoMode) {
        case HWFLAGS_VIDEOMODE_40x25:       printf("40x25\n"); break;
        case HWFLAGS_VIDEOMODE_80x25:       printf("80x25\n"); break;
        case HWFLAGS_VIDEOMODE_80x25_MONO:  printf("80x25 (monochrome)\n"); break;
        default:                            printf("(invalid)\n"); break;
    }
}

void PrintMemoryInfo()
{
    printf("boot: A20 ");
    switch (g_pBootParams->m_A20Method) {
        case A20_NONE:      printf("enabled\n"); break;
        case A20_KEYBOARD:  printf("enabled via PS/2 keyboard controller\n"); break;
        case A20_FAST:      printf("enabled via I/O port 92h\n"); break;
        case A20_BIOS:      printf("enabled via BIOS INT=15h,AX=2401h\n"); break;
        default:            printf("(invalid)"); break;
    }
    printf("boot: g_RamLo_Legacy = %d\n", g_pBootParams->m_RamLo_Legacy);
    printf("boot: g_RamHi_Legacy = %d\n", g_pBootParams->m_RamHi_Legacy);
    printf("boot: g_RamLo_E801h = %d\n", g_pBootParams->m_RamLo_E801h);
    printf("boot: g_RamHi_E801h = %d\n", g_pBootParams->m_RamHi_E801h << 6);    // 64K pages to 1K pages

    const AcpiMemoryMapEntry *memMap = g_pBootParams->m_pAcpiMemoryMap;
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