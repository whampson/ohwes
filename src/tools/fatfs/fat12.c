#include "fat12.h"

static void GetString(char *dst, const char *src, int count);

void InitBPB(BiosParamBlock *bpb)
{
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
    strcpy(bootsect->OemName, OEM_NAME);

    bootsect->Signature = BOOT_SECTOR_ID;

    InitBPB(&bootsect->BiosParams);
}

void GetLabel(char dst[LABEL_LENGTH+1], const char *src)
{
    GetString(dst, src, LABEL_LENGTH);
}

void GetName(char dst[NAME_LENGTH+1], const char *src)
{
    GetString(dst, src, NAME_LENGTH);
}

void GetExt(char dst[EXT_LENGTH+1], const char *src)
{
    GetString(dst, src, EXT_LENGTH);
}

static void GetString(char *dst, const char *src, int count)
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

void GetShortName(char dst[MAX_SHORTNAME], const DirEntry *file)
{
    char nameBuf[NAME_LENGTH+1] = { 0 };
    char extBuf[EXT_LENGTH+1] = { 0 };

    GetName(nameBuf, file->Name);
    GetExt(extBuf, file->Extension);
    size_t nameLen = strlen(nameBuf);
    size_t extLen = strlen(extBuf);

    snprintf(dst, nameLen + 1, "%s", nameBuf);
    if (extLen != 0)
    {
        snprintf(dst + nameLen, extLen + 2, ".%s", extBuf);
    }
}

char GetShortNameChecksum(const DirEntry *file)
{
    int i;
    unsigned char sum = 0;

    const char *name = file->Name;

    for (i = 11; i > 0; i--)
    {
        sum = ((sum & 1) << 7) + (sum >> 1) + ((unsigned char) (*name++));
    }
    return (char) sum;
}

void GetDate(char dst[MAX_DATE], const FatDate *date)
{
    char month[10];

    switch (date->Month)
    {
        case 1:  sprintf(month, "January"); break;
        case 2:  sprintf(month, "February"); break;
        case 3:  sprintf(month, "March"); break;
        case 4:  sprintf(month, "April"); break;
        case 5:  sprintf(month, "May"); break;
        case 6:  sprintf(month, "June"); break;
        case 7:  sprintf(month, "July"); break;
        case 8:  sprintf(month, "August"); break;
        case 9:  sprintf(month, "September"); break;
        case 10: sprintf(month, "October"); break;
        case 11: sprintf(month, "November"); break;
        case 12: sprintf(month, "December"); break;
        default: sprintf(month, "(%d)", date->Month); break;
    }

    snprintf(dst, MAX_DATE, "%s %d, %d",
        month, date->Day, YEAR_BASE + date->Year);

}

void GetTime(char dst[MAX_TIME], const FatTime *time)
{
    int h = time->Hours;
    int m = time->Minutes;
    int s = time->Seconds * 2;

    snprintf(dst, MAX_TIME, "%d:%02d:%02d %s",
        h % 12, m, s,
        (h < 12) ? "AM" : "PM");
}
