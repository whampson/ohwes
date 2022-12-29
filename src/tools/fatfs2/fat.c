#include "fat.h"

#include <ctype.h>
#include <stdio.h>
#include <wchar.h>

// -----------------------------------------------------------------------------
// String Functions
// -----------------------------------------------------------------------------

int ReadFatString(char *dst, const char *src, int n)
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
    for (i = 0; i < end - beg; i++)
    {
        dst[i] = src[beg + i];
    }

    assert(i <= n);

    dst[i] = '\0';
    return i;
}

int WriteFatString(char *dst, const char *src, int n)
{
    int len = strnlen(src, n);
    strncpy(dst, src, n);

    // pad with trailing spaces
    while (n >= len && n - len > 0)
    {
        dst[len++] = ' ';
    }

    return len;
}

// -----------------------------------------------------------------------------
// BIOS Parameter Block
// -----------------------------------------------------------------------------

void InitBiosParamBlock(BiosParamBlock *bpb)
{
    memset(bpb, 0, sizeof(BiosParamBlock));

    WriteFatString(bpb->Label, "", LABEL_LENGTH);
    WriteFatString(bpb->FsType, "", NAME_LENGTH);
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
    WriteFatString(bootSect->OemName, oemName, NAME_LENGTH);
}

// -----------------------------------------------------------------------------
// File Allocation Table
// -----------------------------------------------------------------------------

void InitFat12(char *fat, size_t sizeBytes, uint8_t mediaType, uint32_t eoc)
{
    memset(fat, 0, sizeBytes);

    assert((eoc & 0xFFF) >= CLUSTER_EOC_12_LO);
    assert((eoc & 0xFFF) <= CLUSTER_EOC_12_HI);

    SetCluster12(fat, 0, 0x0F00 | mediaType);
    SetCluster12(fat, 1, eoc);
}

void InitFat16(char *fat, size_t sizeBytes, uint8_t mediaType, uint32_t eoc)
{
    memset(fat, 0, sizeBytes);

    assert((eoc & 0xFFFF) >= CLUSTER_EOC_16_LO);
    assert((eoc & 0xFFFF) <= CLUSTER_EOC_16_HI);

    SetCluster16(fat, 0, 0xFF00 | mediaType);
    SetCluster16(fat, 1, eoc);  // TODO: flags in bits [15:14]
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
    uint32_t oldValue = GetCluster12(fat, index);

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

    return oldValue;
}

uint32_t SetCluster16(char *fat, uint32_t index, uint32_t value)
{
    uint32_t oldValue = GetCluster16(fat, index);

    size_t i = index * 2;
    *((uint16_t *) &fat[i]) = value;

    return oldValue;
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
    // need to find a way to get ms (platform independent)

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
    // TODO: include fine creation time
    // need to find a way to get ms (platform independent)
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

char * GetShortName(char dst[MAX_SHORTNAME], const DirEntry *src)
{
    char name[MAX_NAME];
    char ext[MAX_EXTENSION];

    assert(MAX_SHORTNAME >= NAME_LENGTH + EXTENSION_LENGTH + 1);

    ReadFatString(name, src->Label, NAME_LENGTH);
    ReadFatString(ext, &src->Label[NAME_LENGTH], EXTENSION_LENGTH);

    // 0xE5 is a valid KANJI lead byte, but it's been replaced with
    // 0x05 to distinguish it from the 'deleted' marker. Let's fix that!
    if (name[0] == 0x05) {
        name[0] = 0xE5;
    }

    if (ext[0] != '\0') {
        snprintf(dst, MAX_SHORTNAME, "%s.%s", name, ext);
    }
    else {
        snprintf(dst, MAX_SHORTNAME, "%s", name);
    }

    return dst;
}

bool SetShortName(DirEntry *dst, const char *src)
{
    char srcCopy[MAX_SHORTNAME];
    memset(srcCopy, '\0', sizeof(srcCopy));

    char *name = srcCopy;
    char *ext = "";
    bool dotSeen = false;
    size_t nameLen = 0;
    size_t extLen = 0;

    // trim leading spaces
    while (isspace(*src)) {
        src++;
    }

    // trim trailing spaces
    const char *end = src + strlen(src) - 1;
    while(end > src && isspace(*end)) {
        end--;
    }

    size_t len = end - src + 1;
    if (len == 0 || len > SHORTNAME_LENGTH) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        unsigned char c = src[i];
        if (c == '\0') break;

        // Not Allowed: " * / : < > ? \ | + , . ; = [ ]

        bool valid = isalnum(c);
        valid |= (c > 0x7F);
        valid |= (c == '$' || c == '%' || c == '\'' || c == '-' ||
                  c == '_' || c == '@' || c == '~' || c == '`' ||
                  c == '!' || c == '(' || c == ')' || c == '{' ||
                  c == '}' || c == '^' || c == '#' || c == '&');
        valid |= (c == '.' && !dotSeen);
        valid |= (c == ' ' && i != 0);

        if (!valid) return false;

        if (c == '.') {
            dotSeen = true;
            srcCopy[i] = '\0';
            ext = &srcCopy[i + 1];
            continue;
        }

        srcCopy[i] = toupper(c);

        if (!dotSeen)
            nameLen++;
        else
            extLen++;
    }

    if (nameLen == 0 || nameLen > NAME_LENGTH || extLen > EXTENSION_LENGTH) {
        return false;
    }

    assert(strlen(name) == nameLen);
    assert(strlen(ext) == extLen);

    // 0xE5 is a valid KANJI lead byte, but it needs to be replaced with
    // 0x05 to distinguish it from the 'deleted' marker.
    if (((unsigned char) name[0]) == 0xE5) {
        name[0] = 0x05;
    }

    WriteFatString(dst->Label, name, NAME_LENGTH);
    WriteFatString(&dst->Label[NAME_LENGTH], ext, EXTENSION_LENGTH);

    return true;
}

// -----------------------------------------------------------------------------
// Long File Name
// -----------------------------------------------------------------------------

const int ChunkLength = 13;
const int MaxChainLength = 20;
const int Name1Length = 5;
const int Name2Length = 6;
const int Name3Length = 2;

static void InitLongFileNameLink(LongFileName *lfnLink, int seq)
{
    memset(lfnLink, 0, sizeof(LongFileName));
    memset(&lfnLink->Name1, 0xFF, sizeof(lfnLink->Name1));
    memset(&lfnLink->Name1, 0xFF, sizeof(lfnLink->Name2));
    memset(&lfnLink->Name1, 0xFF, sizeof(lfnLink->Name3));
    lfnLink->Attributes = ATTR_LFN;
    lfnLink->SequenceNumber = seq;
}

const DirEntry * GetLongName(wchar_t dst[MAX_LONGNAME], const DirEntry *srcTable)
{
    const LongFileName *lfn = (const LongFileName *) srcTable;
    if (IsDeleted(srcTable) || !IsLongFileName(srcTable) || !lfn->FirstInChain) {
        return srcTable;
    }

    int len = 0;
    int count = 0;

    uint8_t key = (uint8_t) (time(NULL) & 0xFF);
    uint8_t cksum = key;
    uint8_t lastsum = 0;
    uint8_t compsum = 0;

    do {
        // MS spec limits the chain length to 20, but theoretically we could
        // have 63 links in the chain given the number of bits available.
        // TODO: should we allow 63 links (total 819 chars)?
        assert(lfn->SequenceNumber <= MaxChainLength);

        int bucket = (lfn->SequenceNumber - 1) * ChunkLength;
        wchar_t *pChar = &dst[bucket];
        bool nulSeen = false;

    #define YankChars(buf, n)                                               \
        for (int i = 0; i < (n); i++, pChar++) {                            \
            if (nulSeen) break;                                             \
            *pChar = (wchar_t) (buf)[i];                                    \
            if (*pChar == 0) {                                              \
                nulSeen = true;                                             \
                break;                                                      \
            }                                                               \
            len++;                                                          \
        }                                                                   \

        YankChars(lfn->Name1, Name1Length);
        YankChars(lfn->Name2, Name2Length);
        YankChars(lfn->Name3, Name3Length);

    #undef YankChars

        // The checksum byte in every LFN chain entry should match the computed
        // checksum of the shortname entry that follows the chain. To check this
        // efficiently, we're going to keep a rolling XOR of all the checksum
        // bytes beginning with a "random" key. If the number of chain links is
        // odd, we XOR the last checksum byte again. At the end, we should end
        // up with the initial key if all the checksum bytes matched. We then
        // re-compute the checksum of the shortname entry and compare this
        // against the last checksum byte we saw (which should be valid given
        // that our XOR test passed).

        cksum ^= lfn->Checksum;
        lastsum = lfn->Checksum;
        count++;

    } while ((lfn++)->SequenceNumber > 1);

    if ((count & 1)) {
        cksum ^= lastsum;
    }

    // Grab the shortname entry and compute the checksum.
    const DirEntry *e = (const DirEntry *) lfn;
    compsum = GetShortNameChecksum(e);

    if (cksum != key || (lastsum != compsum)) {
        // Mismatch! Return empty string
        len = 0;
    }

    dst[len] = L'\0';
    return e;
}

bool SetLongName(DirEntry **ppDstTable, const wchar_t *src, const DirEntry *sfnEntry)
{
    wchar_t srcCopy[MAX_LONGNAME];
    memset(srcCopy, 0, sizeof(srcCopy));

    // trim leading spaces
    while (iswspace(*src)) {
        src++;
    }

    // trim trailing spaces and dots
    const wchar_t *end = src + wcslen(src) - 1;
    while(end > src && (iswspace(*end) || *end == L'.')) {
        end--;
    }

    size_t len = end - src + 1;
    if (len == 0 || len > LONGNAME_LENGTH) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        wchar_t c = src[i];
        if (c == L'\0') break;

        // Not Allowed: " * / : < > ? \ |

        bool valid = isalnum(c);
        valid |= (c == L' ');
        valid |= (c > 0x7F);
        valid |= (c != L'"' && c != L'*' && c != L'/' &&
                  c != L':' && c != L'<' && c != L'>' &&
                  c != L'?' && c != L'`' && c != L'|');

        if (!valid) return false;
        srcCopy[i] = c;
    }

    uint8_t cksum = GetShortNameChecksum(sfnEntry);

    int linkCount = (len + ChunkLength - 1) / ChunkLength;
    assert(linkCount <= MaxChainLength);

    for (int seq = linkCount; seq > 0; seq--, (*ppDstTable)++) {
        LongFileName *lfn = (LongFileName *) *ppDstTable;
        InitLongFileNameLink(lfn, seq);
        int bucket = (seq - 1) * ChunkLength;
        if (seq == linkCount) {
            lfn->FirstInChain = 1;
        }
        lfn->Checksum = cksum;

        const wchar_t *pChar = &srcCopy[bucket];
        bool nulSeen = false;

    #define PutChars(buf, n)                                                \
        for (int i = 0; i < (n); i++, pChar++) {                            \
            if (nulSeen) break;                                             \
            buf[i] = (uint16_t) *pChar;                                     \
            if (*pChar == 0) break;                                         \
        }                                                                   \

        PutChars(lfn->Name1, Name1Length);
        PutChars(lfn->Name2, Name2Length);
        PutChars(lfn->Name3, Name3Length);

    #undef PutChars

    }

    *((DirEntry *) *ppDstTable) = *sfnEntry;
    return true;
}

uint8_t GetShortNameChecksum(const DirEntry *src)
{
    uint8_t sum = 0;
    uint8_t *pChar = (uint8_t *) &src->Label;

    for (int len = LABEL_LENGTH; len != 0; len--) {
        // just a bit rotate right
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *pChar++;
    }

    return sum;
}
