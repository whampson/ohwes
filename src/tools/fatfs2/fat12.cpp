#include "fat12.hpp"

static void GetString(char *dst, const char *src, int count, bool allowSpaces);

void InitBootSector(BootSector *bootsect)
{
    static const char BootCode[] =
    {
    /* boot_code:  pushw   %cs          */  "\x0E"
    /*             popw    %ds          */  "\x1F"
    /*             leaw    message, %si */  "\x8D\x36\x1C\x00"
    /* print_loop: movb    $0x0e, %ah   */  "\xB4\x0E"
    /*             movw    $0x07, %bx   */  "\xBB\x07\x00"
    /*             lodsb                */  "\xAC"
    /*             andb    %al, %al     */  "\x20\xC0"
    /*             jz      key_press    */  "\x74\x04"
    /*             int     $0x10        */  "\xCD\x10"
    /*             jmp     print_loop   */  "\xEB\xF2"
    /* key_press:  xorb    %ah, %ah     */  "\x30\xE4"
    /*             int     $0x16        */  "\xCD\x16"
    /*             int     $0x19        */  "\xCD\x19"
    /* halt:       jmp     halt         */  "\xEB\xFE"
    /* message:    .ascii               */  "\r\nThis disk is not bootable!"
    /*             .asciz               */  "\r\nInsert a bootable disk and press any key to try again..."
    };

    static const char JumpCode[] =
    {
    /* entry:       jmp     boot_code   */  '\xEB', '\x3C',
    /*              nop                 */  '\x90'
    };

    static_assert(sizeof(BootCode) <= sizeof(BootCode), "BootCode is too large!");
    static_assert(sizeof(JumpCode) <= sizeof(bootsect->JumpCode), "JumpCode is too large!");

    memcpy(bootsect->BootCode, BootCode, sizeof(BootCode));
    memcpy(bootsect->JumpCode, JumpCode, sizeof(JumpCode));
    strcpy(bootsect->OemName, DEFAULT_OEM_NAME);

    bootsect->Signature = BOOT_SECTOR_ID;

    InitBiosParamBlock(&bootsect->BiosParams);
}

void InitBiosParamBlock(BiosParamBlock *bpb)
{
    // FAT12 defaults
    bpb->MediaType = MEDIA_TYPE_1440K;
    bpb->SectorSize = 512;
    bpb->SectorCount = 2880;
    bpb->ReservedSectorCount = 1;
    bpb->HiddenSectorCount = 0;
    bpb->LargeSectorCount = 0;
    bpb->SectorsPerCluster = 1;
    bpb->SectorsPerTable = 9;
    bpb->SectorsPerTrack = 18;
    bpb->TableCount = 2;
    bpb->MaxRootDirEntryCount = 224;
    bpb->HeadCount = 2;
    bpb->DriveNumber = 0;
    bpb->_Reserved = 0;
    bpb->ExtendedBootSignature = 0x29;
    bpb->VolumeId = time(NULL);
    strcpy(bpb->Label, DEFAULT_LABEL);
    strcpy(bpb->FileSystemType, DEFAULT_FS_TYPE);
}

void GetName(char dst[MAX_NAME], const char *src)
{
    GetString(dst, src, NAME_LENGTH, false);
}

void GetExtension(char dst[MAX_EXTENSION], const char *src)
{
    GetString(dst, src, EXTENSION_LENGTH, false);
}

void GetLabel(char dst[MAX_LABEL], const char *src)
{
    GetString(dst, src, LABEL_LENGTH, true);
}

static void GetString(char *dst, const char *src, int count, bool allowSpaces)
{
    if (!allowSpaces)
    {
        int i = 0;
        for (; i < count; i++)
        {
            if (src[i] == ' ' || src[i] == '\0')
            {
                break;
            }
            dst[i] = src[i];
        }
        dst[i] = '\0';
    }
    else
    {
        // Trim leading/trailing space only

        int beg = -1;
        int end = 0;
        int i;

        for (i = 0; i < count; i++)
        {
            if (src[i] != ' ')
            {
                beg = i;
                break;
            }
        }

        for (i = count - 1; i >= 0; i--)
        {
            if (src[i] != ' ')
            {
                end = i;
                break;
            }
        }

        for (i = 0; i < end - beg + 1; i++)
        {
            dst[i] = src[beg + i];
        }
        dst[i] = '\0';
    }
}