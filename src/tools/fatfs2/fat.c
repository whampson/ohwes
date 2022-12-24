#include "fat.h"

// -----------------------------------------------------------------------------
// String Functions
// -----------------------------------------------------------------------------

static int ReadString(char *dst, const char *src, int n)
{
    int beg = 0;
    int end = 0;
    int i;

    // trim leading spaces
    for (i = 0; i < n; i++)
    {
        if (src[i] != ' ')
        {
            beg = i;
            break;
        }
    }

    // trim trailing spaces
    for (i = n; i > 0; i--)
    {
        if (src[i - 1] != ' ')
        {
            end = i;
            break;
        }
    }

    // copy trimmed string
    strncpy(dst, &src[beg], end - beg);

    dst[i] = '\0';
    return i;
}

static int WriteString(char *dst, const char *src, int n)
{
    int len = strnlen(src, n);
    strncpy(dst, src, n);

    while (n >= len && n - len > 0)
    {
        dst[len++] = ' ';
    }

    return len;
}

int ReadName(char dst[MAX_NAME], const char *src)
{
    return ReadString(dst, src, NAME_LENGTH);
}

int ReadExt(char dst[MAX_EXT], const char *src)
{
    return ReadString(dst, src, EXT_LENGTH);
}

int ReadLabel(char dst[MAX_LABEL], const char *src)
{
    return ReadString(dst, src, LABEL_LENGTH);
}

int WriteName(char dst[MAX_NAME], const char *src)
{
    return WriteString(dst, src, NAME_LENGTH);
}

int WriteExt(char dst[MAX_EXT], const char *src)
{
    return WriteString(dst, src, EXT_LENGTH);
}

int WriteLabel(char dst[MAX_LABEL], const char *src)
{
    return WriteString(dst, src, LABEL_LENGTH);
}

// -----------------------------------------------------------------------------
// BIOS Parameter Block
// -----------------------------------------------------------------------------

void InitBiosParamBlock(BiosParamBlock *bpb)
{
    memset(bpb, 0, sizeof(BiosParamBlock));
    WriteLabel(bpb->Label, "");
    WriteName(bpb->FsType, "");
    bpb->Signature = BPBSIG_DOS41;
    bpb->_Reserved = 0;
}

// -----------------------------------------------------------------------------
// Boot Sector
// -----------------------------------------------------------------------------

inline void InitBootSector(
    BootSector *bootSect,
    const BiosParamBlock *bpb,
    const char *oemName)
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
    WriteName(bootSect->OemName, oemName);
}

// -----------------------------------------------------------------------------
// File Allocation Table
// -----------------------------------------------------------------------------

void InitFat12(char *fat, size_t sizeBytes, uint8_t mediaType)
{
    memset(fat, 0, sizeBytes);

    SetCluster12(fat, 0, mediaType);
    SetCluster12(fat, 1, CLUSTER_EOC);
}

void InitFat16(char *fat, size_t sizeBytes, uint8_t mediaType)
{
    memset(fat, 0, sizeBytes);

    SetCluster16(fat, 0, mediaType);
    SetCluster16(fat, 1, CLUSTER_EOC);
}

uint32_t GetCluster12(const char *fat, uint32_t index)
{
    //
    //     0        1        2      :: byte index
    // |........|++++....|++++++++| :: . = clust0, + = clust1
    // |76543210|3210ba98|ba987654| :: bit index
    //

    size_t i = index + (index / 2);
    uint16_t pair = *((uint16_t *) &fat[i]);
    if (index & 1) {
        return pair >> 4;
    }
    return pair & 0x0FFF;
}

uint32_t GetCluster16(const char *fat, uint32_t index)
{
    //
    //     0        1        2        3      :: byte index
    // |........|........|++++++++|++++++++| :: . = clust0, + = clust1
    // |76543210|fedcba98|76543210|fedcba98| :: bit index
    //

    size_t i = index * 2;
    return *((uint16_t *) &fat[i]);
}

uint32_t SetCluster12(char *fat, uint32_t index, uint32_t value)
{
    size_t i = index + (index / 2);

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

uint32_t SetCluster16(char *fat, uint32_t index, uint32_t value)
{
    size_t i = index * 2;
    *((uint16_t *) &fat[i]) = value;
    return value;
}

// -----------------------------------------------------------------------------
// Date/Time
// -----------------------------------------------------------------------------

void GetDate(struct tm *dst, const FatDate *src)
{
    dst->tm_year = src->Year + 80;
    dst->tm_mon = src->Month - 1;
    dst->tm_mday = src->Day;
}

void GetTime(struct tm *dst, const FatTime *src)
{
    dst->tm_hour = src->Hours;
    dst->tm_min = src->Minutes;
    dst->tm_sec = src->Seconds << 1;
}

void SetDate(FatDate *dst, const struct tm *src)
{
    dst->Year = src->tm_year - 80;
    dst->Month = src->tm_mon + 1;
    dst->Day = src->tm_mday;
}

void SetTime(FatTime *dst, const struct tm *src)
{
    dst->Hours = src->tm_hour;
    dst->Minutes = src->tm_min;
    dst->Seconds = src->tm_sec >> 1;
}

// -----------------------------------------------------------------------------
// Directory Entry
// -----------------------------------------------------------------------------

void InitDirEntry(DirEntry *e)
{
    memset(e, 0, sizeof(DirEntry));

    struct tm tm;
    time_t now = time(NULL);
    localtime_r(&now, &tm);

    SetCreationTime(e, &tm);
    SetModifiedTime(e, &tm);
    SetAccessedTime(e, &tm);
}

time_t GetCreationTime(struct tm *dst, const DirEntry *src)
{
    GetDate(dst, &src->CreationDate);
    GetTime(dst, &src->CreationTime);
    // TODO: include fine creation time

    return mktime(dst);
}

time_t GetModifiedTime(struct tm *dst, const DirEntry *src)
{
    GetDate(dst, &src->ModifiedDate);
    GetTime(dst, &src->ModifiedTime);

    return mktime(dst);
}

time_t GetAccessedTime(struct tm *dst, const DirEntry *src)
{
    GetDate(dst, &src->AccessedDate);

    return mktime(dst);
}

void SetCreationTime(DirEntry *dst, const struct tm *src)
{
    SetDate(&dst->CreationDate, src);
    SetTime(&dst->CreationTime, src);
}

void SetModifiedTime(DirEntry *dst, const struct tm *src)
{
    SetDate(&dst->ModifiedDate, src);
    SetTime(&dst->ModifiedTime, src);
}

void SetAccessedTime(DirEntry *dst, const struct tm *src)
{
    SetDate(&dst->AccessedDate, src);
}

void GetShortFileName(char dst[MAX_SFN], const DirEntry *src)
{
    char name[MAX_NAME];
    char ext[MAX_EXT];

    ReadName(name, src->Name);
    ReadExt(ext, src->Extension);

    // 0xE5 is a valid KANJI lead byte, but it's been replaced with
    // 0x05 to distinguish it from the 'deleted' marker. Let's fix that!
    if (name[0] == 0x05) {
        name[0] = 0xE5;
    }

    if (ext[0] != '\0') {
        snprintf(dst, MAX_SFN, "%s.%s", name, ext);
    }
    else {
        snprintf(dst, MAX_SFN, "%s", name);
    }

    assert(MAX_SFN >= MAX_NAME + MAX_EXT - 1);
}

void SetShortFileName(DirEntry *dst, const char *src)
{
    // TODO: uppercase
    // TODO: filter valid chars?

    char srcCopy[MAX_SFN];
    strncpy(srcCopy, src, MAX_SFN);

    char *name = strtok(srcCopy, ".");
    char *ext = strtok(NULL, ".");
    if (ext == NULL) {
        ext = "";
    }

    // 0xE5 is a valid KANJI lead byte, but it needs to be replaced with
    // 0x05 to distinguish it from the 'deleted' marker.
    if (name[0] == 0xE5) {
        name[0] = 0x05;
    }

    WriteName(dst->Name, name);
    WriteExt(dst->Extension, ext);
}

// -----------------------------------------------------------------------------
// Long File Name
// -----------------------------------------------------------------------------

const DirEntry * GetLongFileName(wchar_t dst[MAX_LFN], const DirEntry *srcTable)
{
    const LongFileName *lfn = (const LongFileName *) srcTable;
    if (IsDeleted(srcTable) || !IsLongFileName(srcTable) || !lfn->FirstInChain) {
        return srcTable;
    }

    int count = 0;
    do {
        const int ChunkLength = 13;
        const int MaxChainLength = 20;

        // MS spec limits the chain length to 20, but theoretically we could
        // have 63 links in the chain given the number of bits available.
        // TODO: should we allow 63 links (total 819 chars)?
        assert(lfn->SequenceNumber <= MaxChainLength);

        int bucket = (lfn->SequenceNumber - 1) * ChunkLength;
        wchar_t *pChar = &dst[bucket];

    #define YankChars(buf, n)                                               \
        for (int i = 0; i < ((n) << 1); i++, pChar++) {                     \
            *pChar = (wchar_t) (buf)[i];                                    \
            if (*pChar != 0 && *pChar != 0xFFFF) count++;                   \
        }                                                                   \

        YankChars(lfn->Name1, 5);
        YankChars(lfn->Name2, 6);
        YankChars(lfn->Name3, 2);

    #undef YankChars

    } while ((lfn++)->SequenceNumber > 1);

    // TODO: checksum

    dst[count] = L'\0';
    return (const DirEntry *) lfn;
}

const DirEntry * SetLongName(DirEntry *dstTable, wchar_t *src)
{
    (void) dstTable;
    (void) src;

    assert(false);  // TODO
    return NULL;
}

