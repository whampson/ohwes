#include "fat.hpp"

static void ReadString(char *dst, const char *src, int n, bool allowSpaces);
static void WriteString(char *dst, const char *src, int n);

void InitBiosParamBlock(BiosParamBlock *bpb)
{
    memset(bpb, 0, sizeof(BiosParamBlock));
    SetLabel(bpb->Label, "");
    SetName(bpb->FsType, "");
    bpb->Signature = BPBSIG_DOS41;
    bpb->_Reserved = 0;
}

void InitBootSector(BootSector *bootSect, const BiosParamBlock *bpb)
{
    memset(bootSect, 0, sizeof(BootSector));
    bootSect->BootSignature = BOOTSIG;
    bootSect->BiosParams = *bpb;

    static const char JumpCode[] = {
    /* entry:      jmp     boot_code    */  '\xEB', '\x3C',
    /*             nop                  */  '\x90'
    };
    static const char BootCode[] = {
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

    static_assert(sizeof(JumpCode) <= sizeof(bootSect->JumpCode), "JumpCode is too large!");
    static_assert(sizeof(BootCode) <= sizeof(BootCode), "BootCode is too large!");

    memcpy(bootSect->BootCode, BootCode, sizeof(BootCode));
    memcpy(bootSect->JumpCode, JumpCode, sizeof(JumpCode));
    SetName(bootSect->OemName, PROG_NAME);
}

void InitFat12(char *fat, size_t fatSize, char mediaType)
{
    memset(fat, 0, fatSize);

    SetCluster12(fat, fatSize, 0, mediaType);
    SetCluster12(fat, fatSize, 1, CLUSTER_EOC);
}

void InitFat16(char *fat, size_t fatSize, char mediaType)
{
    memset(fat, 0, fatSize);

    SetCluster16(fat, fatSize, 0, mediaType);
    SetCluster16(fat, fatSize, 1, CLUSTER_EOC);
}

int GetCluster12(const char *fat, size_t fatSize, int index)
{
    size_t i = index + (index / 2);
    if ((i + 1) >= fatSize) {
        return -1;
    }

    //
    //     0        1        2      :: byte index
    // |........|++++....|++++++++| :: . = clust0, + = clust1
    // |76543210|3210ba98|ba987654| :: bit index
    //

    uint16_t pair = *((uint16_t *) &fat[i]);
    if (index & 1) {
        return pair >> 4;
    }
    return pair & 0x0FFF;
}

int GetCluster16(const char *fat, size_t fatSize, int index)
{
    size_t i = index * 2;
    if ((i + 1) >= fatSize) {
        return -1;
    }

    //
    //     0        1        2        3      :: byte index
    // |........|........|++++++++|++++++++| :: . = clust0, + = clust1
    // |76543210|fedcba98|76543210|fedcba98| :: bit index
    //

    return *((uint16_t *) &fat[i]);
}

int SetCluster12(char *fat, size_t fatSize, int index, int value)
{
    size_t i = index + (index / 2);
    if ((i + 1) >= fatSize) {
        return -1;
    }

    //
    //     0        1        2      :: byte index
    // |........|++++....|++++++++| :: . = clust0, + = clust1
    // |76543210|3210ba98|ba987654| :: bit index
    //

    uint16_t pair = *((uint16_t *) &fat[i]);
    if (index & 1) {
        value <<= 4;
        pair &= 0x000F;
    }
    else {
        value &= 0x0FFF;
        pair &= 0xF000;
    }

    *((uint16_t *) &fat[i]) = pair | value;
    return value;
}

int SetCluster16(char *fat, size_t fatSize, int index, int value)
{
    size_t i = index * 2;
    if ((i + 1) >= fatSize) {
        return -1;
    }

    //
    //     0        1        2        3      :: byte index
    // |........|........|++++++++|++++++++| :: . = clust0, + = clust1
    // |76543210|fedcba98|76543210|fedcba98| :: bit index
    //

    *((uint16_t *) &fat[i]) = value;
    return value;
}

void MakeVolumeLabel(DirEntry *dst, const char *label)
{
    memset(dst, 0, sizeof(DirEntry));

    time_t now = time(NULL);
    dst->Attributes = ATTR_LABEL;
    SetLabel(dst->Name, label);
    SetDate(&dst->CreationDate, &now);
    SetTime(&dst->CreationTime, &now);
    SetDate(&dst->ModifiedDate, &now);
    SetTime(&dst->ModifiedTime, &now);
    SetDate(&dst->LastAccessDate, &now);
}

void GetName(char dst[MAX_NAME], const char *src)
{
    ReadString(dst, src, NAME_LENGTH, false);
}

void GetExtension(char dst[MAX_EXTENSION], const char *src)
{
    ReadString(dst, src, EXTENSION_LENGTH, false);
}

void GetLabel(char dst[MAX_LABEL], const char *src)
{
    ReadString(dst, src, LABEL_LENGTH, true);
}

void GetDate(char dst[MAX_DATE], const FatDate *src)
{
    char month[4];
    switch (src->Month)
    {
        case 1: sprintf(month, "Jan"); break;
        case 2: sprintf(month, "Feb"); break;
        case 3: sprintf(month, "Mar"); break;
        case 4: sprintf(month, "Apr"); break;
        case 5: sprintf(month, "May"); break;
        case 6: sprintf(month, "Jun"); break;
        case 7: sprintf(month, "Jul"); break;
        case 8: sprintf(month, "Aug"); break;
        case 9: sprintf(month, "Sep"); break;
        case 10:sprintf(month, "Oct"); break;
        case 11:sprintf(month, "Nov"); break;
        case 12:sprintf(month, "Dec"); break;
        default:sprintf(month, "   "); break;
    }

    sprintf(dst, "%s%3d%5d",
        month,
        src->Day,
        src->Year + 1980);
}

void GetTime(char dst[MAX_TIME], const FatTime *src)
{
    sprintf(dst, "%02d:%02d:%02d",
        src->Hours,
        src->Minutes,
        src->Seconds << 1);
}

void SetName(char dst[NAME_LENGTH], const char *src)
{
    WriteString(dst, src, NAME_LENGTH);
}

void SetExtension(char dst[EXTENSION_LENGTH], const char *src)
{
    WriteString(dst, src, EXTENSION_LENGTH);
}

void SetLabel(char dst[LABEL_LENGTH], const char *src)
{
    WriteString(dst, src, LABEL_LENGTH);
}

void SetDate(FatDate *dst, time_t *t)
{
    if (t == NULL) {
        time(t);
    }
    tm *tm = localtime(t);
    dst->Year = tm->tm_year - 80;
    dst->Month = tm->tm_mon + 1;
    dst->Day = tm->tm_mday;
}

void SetTime(FatTime *dst, time_t *t)
{
    if (t == NULL) {
        time(t);
    }
    tm *tm = localtime(t);
    dst->Hours = tm->tm_hour;
    dst->Minutes = tm->tm_min;
    dst->Seconds = tm->tm_sec / 2;
}

static void ReadString(char *dst, const char *src, int n, bool allowSpaces)
{
    if (!allowSpaces)
    {
        int i = 0;
        for (; i < n; i++)
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

        int beg = 0;
        int end = 0;
        int i;

        for (i = 0; i < n; i++)
        {
            if (src[i] != ' ')
            {
                beg = i;
                break;
            }
        }

        for (i = n; i > 0; i--)
        {
            if (src[i - 1] != ' ')
            {
                end = i;
                break;
            }
        }

        for (i = 0; i < end - beg; i++)
        {
            dst[i] = src[beg + i];
        }
        dst[i] = '\0';
    }
}

static void WriteString(char *dst, const char *src, int n)
{
    int len = strnlen(src, n);
    strncpy(dst, src, n);

    while (n >= len && n - len > 0)
    {
        dst[len++] = ' ';
    }
}
