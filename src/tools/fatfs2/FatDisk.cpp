#include "FatDisk.hpp"

#include <wchar.h>

bool FatDisk::CreateNew(const char *path, const BiosParamBlock *bpb)
{
    return CreateNew(path, bpb, 0);
}

bool FatDisk::CreateNew(const char *path, const BiosParamBlock *bpb, uint32_t sector)
{
    FILE *fp = NULL;
    void *sectorBuf = NULL;
    void *clusterBuf = NULL;
    char *fat = NULL;
    size_t bytesWritten = 0;
    bool success = true;

    if (bpb->SectorCount == 0 && bpb->SectorCountLarge == 0) {
        LogError("invalid BPB - sector count cannot be zero\n");
        return false;
    }
    if (bpb->SectorCount != 0 && bpb->SectorCountLarge != 0) {
        LogError("invalid BPB - only one 'SectorCount' field may be set\n");
        return false;
    }
    if (bpb->SectorsPerTable == 0) {
        LogError("invalid BPB - need at least one sector per FAT\n");
        // TODO: this field can be zero on FAT32, as long as the other one is set.
        return false;
    }
    if (bpb->SectorSize < MIN_SECTOR_SIZE) {
        LogError("invalid BPB - sector size must be at least 512\n");
        return false;
    }
    if (!IsPow2(bpb->SectorSize)) {
        LogError("invalid BPB - sector size must be a power of 2\n");
        return false;
    }
    if (!IsPow2(bpb->SectorsPerCluster)) {
        LogError("invalid BPB - sectors per cluster must be a power of 2\n");
        return false;
    }

    // Assume 512-byte sectors until the BPB tells us otherwise
    size_t baseAddr = sector * 512;

    uint32_t sectorSize = bpb->SectorSize;
    uint32_t sectorCount = (bpb->SectorCount) ? bpb->SectorCount : bpb->SectorCountLarge;
    uint32_t sectorsPerCluster = bpb->SectorsPerCluster;
    uint32_t clusterSize = sectorSize * sectorsPerCluster;
    size_t diskSize = (bpb->SectorCount * sectorSize) + baseAddr;
    uint32_t resSectorCount = bpb->ReservedSectorCount;
    uint32_t fatSectorCount = bpb->SectorsPerTable;
    uint32_t fatSize = fatSectorCount * bpb->SectorSize;
    uint32_t fatCount = bpb->TableCount;
    uint32_t rootSize = bpb->RootDirCapacity * sizeof(DirEntry);
    uint32_t rootSectorCount = CeilDiv(rootSize, sectorSize);
    uint32_t dataSectors = sectorCount - (resSectorCount + (fatSectorCount * fatCount) + rootSectorCount);
    uint32_t clusters = dataSectors / sectorsPerCluster;
    uint32_t extraSectors = dataSectors - (clusters * sectorsPerCluster);
    if (extraSectors) {
        LogWarning("disk has %d %s unreachable by FAT\n",
            PluralForPrintf(extraSectors, "sector"));
    }
    bool fat12 = clusters <= MAX_CLUSTERS_12;
    bool hasCustomLabel = (bpb->Label[0] != ' ');

    fp = SafeOpen(path, "wb", NULL);
    sectorBuf = (char *) SafeAlloc(sectorSize);
    clusterBuf = (char *) SafeAlloc(clusterSize);
    fat = (char *) SafeAlloc(fatSize);

    if (baseAddr != 0) {
        LogVerbose("creating FAT file system at sector %d (address = 0x%zX)\n",
            sector, baseAddr);
        fseek(fp, baseAddr, SEEK_SET);
        bytesWritten = ftell(fp);
    }

    InitBootSector((BootSector *) sectorBuf, bpb, PROG_NAME);
    bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);

    for (uint32_t i = 1; i < resSectorCount; i++) {
        bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
    }

    assert(bytesWritten == baseAddr + resSectorCount * sectorSize);

    if (fat12)
        InitFat12(fat, fatSize, bpb->MediaType, CLUSTER_EOC_12);
    else
        InitFat16(fat, fatSize, bpb->MediaType, CLUSTER_EOC_16);

    for (uint32_t i = 0; i < bpb->TableCount; i++) {
        bytesWritten += SafeWrite(fp, fat, fatSize);
    }

    assert(bytesWritten % sectorSize == 0);
    assert(bytesWritten ==
        baseAddr +
        (resSectorCount
            + (fatSectorCount * fatCount))
        * sectorSize);


    memset(sectorBuf, 0, sectorSize);
    for (uint32_t i = 0; i < rootSectorCount; i++) {
        if (hasCustomLabel && i == 0) {
            DirEntry volLabel;
            InitDirEntry(&volLabel);
            volLabel.Attributes = ATTR_LABEL;
            memcpy(sectorBuf, &volLabel, sizeof(DirEntry));
            bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
            memset(sectorBuf, 0, sectorSize);
        }
        else {
            bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
        }
    }

    assert(bytesWritten ==
        baseAddr +
        (resSectorCount
            + (fatSectorCount * fatCount)
            + rootSectorCount)
        * sectorSize);

    memset(clusterBuf, 0, clusterSize);
    for (uint32_t i = 0; i < clusters; i++) {
        bytesWritten += SafeWrite(fp, clusterBuf, clusterSize);
    }

    memset(sectorBuf, 0, sectorSize);
    for (uint32_t i = 0; i < extraSectors; i++) {
        bytesWritten += SafeWrite(fp, sectorBuf, sectorSize);
    }

    assert(bytesWritten % sectorSize == 0);
    assert(bytesWritten == diskSize);

    assert(bytesWritten ==
        baseAddr +
        (resSectorCount
            + (fatSectorCount * fatCount)
            + rootSectorCount
            + (clusters * sectorsPerCluster)
            + extraSectors)
        * sectorSize);

Cleanup:
    SafeClose(fp);
    SafeFree(clusterBuf);
    SafeFree(sectorBuf);
    SafeFree(fat);

    return success;
}

FatDisk * FatDisk::Open(const char *path)
{
    return Open(path, 0);
}

FatDisk * FatDisk::Open(const char *path, uint32_t sector)
{
    // TODO: handle partitioned disks?

    BootSector bootSect;
    BiosParamBlock *bpb;

    FILE *fp = NULL;
    FatDisk *disk = NULL;

    size_t pos;
    size_t size;
    size_t fsDataSize;
    uint32_t sectorSize;
    uint32_t rootSectorCount;

    void *sectorBuf = NULL;
    char *fat = NULL;
    char *root = NULL;

    // Assume 512-byte sectors until the BPB tells us otherwise
    size_t baseAddr = sector * 512;

    bool success = true;
    fp = SafeOpen(path, "rb+", &size);
    SafeRIF(size + baseAddr >= 4096, "disk is too small\n");

    if (baseAddr != 0) {
        LogVerbose("looking for FAT file system at sector %d (address = 0x%zX)\n",
            sector, baseAddr);
        fseek(fp, baseAddr, SEEK_SET);
    }

    pos = 0;
    pos += SafeRead(fp, &bootSect, sizeof(BootSector));
    bpb = &bootSect.BiosParams;

    sectorSize = bpb->SectorSize;

    SafeRIF(IsPow2(sectorSize),
        "BPB is corrupt (sector size = %d)\n", sectorSize);
    SafeRIF(sectorSize >= MIN_SECTOR_SIZE,
        "BPB is corrupt (sector size = %d)\n", sectorSize);
    SafeRIF(sectorSize <= MAX_SECTOR_SIZE,
        "BPB is corrupt (sector size = %d)\n", sectorSize);
    SafeRIF(bpb->SectorCount != 0 || (bpb->SectorCount == 0 && bpb->SectorCountLarge != 0),
        "BPB is corrupt (sector count = %d\n", bpb->SectorCount);
    SafeRIF(bpb->ReservedSectorCount > 0,
        "BPB is corrupt (reserved sector count = %d)\n", bpb->ReservedSectorCount);
    SafeRIF(bpb->RootDirCapacity > 0,
        "BPB is corrupt (root directory capacity = %d)\n", bpb->RootDirCapacity);
    SafeRIF(bpb->SectorsPerTable > 0,
        "BPB is corrupt (FAT sector count = %d)\n", bpb->SectorsPerTable);
    SafeRIF(bpb->TableCount > 0,
        "BPB is corrupt (FAT count = %d)\n", bpb->TableCount);

    rootSectorCount = CeilDiv(bpb->RootDirCapacity * sizeof(DirEntry), sectorSize);

    sectorBuf = SafeAlloc(sectorSize);
    fat = (char *) SafeAlloc(bpb->SectorsPerTable * sectorSize);
    root = (char *) SafeAlloc(rootSectorCount * sectorSize);

    if (sectorSize - pos > 0) {
        // skip the rest of the boot sector
        pos += SafeRead(fp, sectorBuf, sectorSize - pos);
    }

    fsDataSize = sectorSize *
        (bpb->ReservedSectorCount +
        (bpb->SectorsPerTable * bpb->TableCount) +
        rootSectorCount);

    SafeRIF(fsDataSize < size, "disk is too small\n");

    for (int i = 1; i < bpb->ReservedSectorCount; i++) {
        pos += SafeRead(fp, sectorBuf, sectorSize);
    }

    for (int i = 0; i < bpb->SectorsPerTable; i++) {
        pos += SafeRead(fp, fat + i * sectorSize, sectorSize);
    }

    // skip remaining FATs
    pos += (bpb->TableCount - 1) * bpb->SectorsPerTable * sectorSize;
    fseek(fp, pos, SEEK_SET);

    if (((uint8_t) fat[0]) != bpb->MediaType) {
        LogWarning("media type ID mismatch (FAT = 0x%02X, BPB = 0x%02X)\n",
            fat[0], bpb->MediaType);
    }

    for (uint32_t i = 0; i < rootSectorCount; i++) {
        pos += SafeRead(fp, root + i * sectorSize, sectorSize);
    }

    disk = new FatDisk(path, fp, baseAddr, &bootSect, fat);

    LogVerbose("opened FAT%d disk '%s' at offset 0x%zx; media type = 0x%02X, EOC = %X\n",
        disk->IsFat12() ? 12 : 16, path, baseAddr, (uint8_t) fat[0], disk->GetClusterNumberEOC());

Cleanup:
    SafeFree(sectorBuf);
    SafeFree(root);
    if (!success) {
        SafeFree(fat);
        SafeClose(fp);
    }

    return (success) ? disk : NULL;
}

FatDisk::FatDisk(const char *path, FILE *file, size_t base, BootSector *boot, char *fat)
{
    m_path = path;
    m_file = file;
    m_base = base;
    m_boot = *boot;
    m_fat = fat;

    if (IsFat16()) {
        uint32_t eoc = GetCluster(1);
        m_bHardError = !(eoc & 0x4000);
        m_bDirty     = !(eoc & 0x8000);

        if (m_bHardError) {
            LogWarning("disk is marked as having bad clusters!\n");
        }
        if (m_bDirty) {
            LogWarning("disk was not dismounted properly!\n");
        }
    }
}

FatDisk::~FatDisk()
{
    SafeFree(m_fat);
    SafeClose(m_file);
}

bool FatDisk::IsFat12() const
{
    return GetClusterCount() <= MAX_CLUSTERS_12;
}

bool FatDisk::IsFat16() const
{
    return GetClusterCount() >= MIN_CLUSTERS_16
        && GetClusterCount() <= MAX_CLUSTERS_16;
}

const BiosParamBlock * FatDisk::GetBPB() const
{
    return &m_boot.BiosParams;
}

uint32_t FatDisk::GetDiskSize() const
{
    return GetSectorCount() * GetSectorSize();
}

uint32_t FatDisk::GetFileSize(const DirEntry *pFile) const
{
    return IsDirectory(pFile)
        ? GetFileAllocSize(pFile)
        : pFile->FileSize;
}

uint32_t FatDisk::GetFileAllocSize(const DirEntry *pFile) const
{
    if (IsRoot(pFile)) {
        return GetRootCapacity() * sizeof(DirEntry);
    }

    if (!IsValidFile(pFile) || pFile->FirstCluster == 0) {
        return 0;
    }

    return CountClusters(pFile) * GetClusterSize();
}

uint32_t FatDisk::GetSectorSize() const
{
    return GetBPB()->SectorSize;
}

uint32_t FatDisk::GetSectorCount() const
{
    const BiosParamBlock *bpb = GetBPB();
    uint32_t count = bpb->SectorCount;
    if (count == 0) {
        count = bpb->SectorCountLarge;
    }

    assert(count != 0);
    return count;
}

uint32_t FatDisk::GetClusterSize() const
{
    const BiosParamBlock *bpb = GetBPB();

    return bpb->SectorSize * bpb->SectorsPerCluster;
}

uint32_t FatDisk::GetClusterCount() const
{
    const BiosParamBlock *bpb = GetBPB();
    uint32_t fsDataSectors = bpb->ReservedSectorCount +
        (bpb->SectorsPerTable * bpb->TableCount) +
        CeilDiv(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize);

    return (GetSectorCount() - fsDataSectors) / bpb->SectorsPerCluster;
}

uint32_t FatDisk::GetFatCapacity() const
{
    const BiosParamBlock *bpb = GetBPB();
    uint32_t fatSize = bpb->SectorsPerTable * bpb->SectorSize;
    uint32_t fatCapacity = IsFat12()
        ? ((fatSize / 3) * 2) - CLUSTER_FIRST
        : (fatSize / 2) - CLUSTER_FIRST;

    return fatCapacity;
}

uint32_t FatDisk::GetRootCapacity() const
{
    return GetBPB()->RootDirCapacity;
}

DirEntry FatDisk::GetRootDirEntry() const
{
    DirEntry e;
    memset(&e, 0, sizeof(DirEntry));

    e.Attributes = ATTR_DIRECTORY;
    e.FirstCluster = 0;

    return e;
}

uint32_t FatDisk::CountFreeClusters() const
{
    uint32_t free = 0;
    for (uint32_t i = 0; i < GetClusterCount(); i++) {
        if (IsClusterFree(CLUSTER_FIRST + i)) {
            free++;
        }
    }

    return free;
}

uint32_t FatDisk::CountBadClusters() const
{
    uint32_t bad = 0;
    for (uint32_t i = 0; i < GetClusterCount(); i++) {
        if (IsClusterBad(CLUSTER_FIRST + i)) {
            bad++;
        }
    }

    return bad;
}

uint32_t FatDisk::CountClusters(const DirEntry *pFile) const
{
    if (IsRoot(pFile)) {
        // root directory does not take up clusters
        return 0;
    }

    if (pFile->FirstCluster == CLUSTER_FREE) {
        // file is not on disk
        return 0;
    }

    uint32_t count = 0;
    uint32_t cluster = pFile->FirstCluster;

    do {
        count++;
        cluster = GetCluster(cluster);
    } while (!IsClusterNumberEOC(cluster));

    return count;
}

uint32_t FatDisk::FindNextFreeCluster() const
{
    for (uint32_t i = 0; i < GetClusterCount(); i++) {
        uint32_t clustNum = CLUSTER_FIRST + i;
        if (IsClusterFree(clustNum)) {
            return clustNum;
        }
    }

    return GetClusterNumberEOC();
}

bool FatDisk::IsClusterBad(uint32_t index) const
{
    return GetCluster(index) == GetClusterNumberBad();
}

bool FatDisk::IsClusterFree(uint32_t index) const
{
    return GetCluster(index) == CLUSTER_FREE;
}

uint32_t FatDisk::MarkClusterBad(uint32_t index)
{
    return SetCluster(index, GetClusterNumberBad());
}

uint32_t FatDisk::MarkClusterFree(uint32_t index)
{
    return SetCluster(index, CLUSTER_FREE);
}

uint32_t FatDisk::GetClusterNumberEOC() const
{
    return IsFat12()
        ? GetCluster12(m_fat, 1)
        : GetCluster16(m_fat, 1) | 0xC000;
}

uint32_t FatDisk::GetClusterNumberBad() const
{
    return IsFat12()
        ? CLUSTER_BAD_12
        : CLUSTER_BAD_16;
}

uint32_t FatDisk::GetCluster(uint32_t index) const
{
    if (index >= CLUSTER_FIRST && (index - CLUSTER_FIRST) > GetClusterCount()) {
        // ok to read clusters 0 and 1
        LogError("attempt to read out-of-bounds cluster in FAT (index = %04X)\n", index);
        return GetClusterNumberEOC();
    }

    return IsFat12()
        ? GetCluster12(m_fat, index)
        : GetCluster16(m_fat, index);
}

uint32_t FatDisk::SetCluster(uint32_t index, uint32_t value)
{
    if (index < CLUSTER_FIRST || (index - CLUSTER_FIRST) > GetClusterCount()) {
        // not ok to write clusters 0 and 1
        LogError("attempt to write out-of-bounds cluster in FAT (index = %04X)\n", index);
        return GetClusterNumberEOC();
    }

    return IsFat12()
        ? SetCluster12(m_fat, index, value)
        : SetCluster16(m_fat, index, value);
}

bool FatDisk::IsClusterNumberEOC(uint32_t clustNum) const
{
    if (clustNum < CLUSTER_FIRST || clustNum == GetClusterNumberBad()) {
        // treat clusters 0 and 1, and (BAD) as EOC
        return true;
    }

    return IsFat12()
            ? (clustNum >= CLUSTER_EOC_12_LO && clustNum <= CLUSTER_EOC_12_HI)
            : (clustNum >= CLUSTER_EOC_16_LO && clustNum <= CLUSTER_EOC_16_HI);
}

bool FatDisk::IsClusterNumberBad(uint32_t clustNum) const
{
    return IsFat12()
            ? clustNum == CLUSTER_BAD_12
            : clustNum == CLUSTER_BAD_16;
}

bool FatDisk::ReadSector(char *pBuf, uint32_t index) const
{
    const uint32_t SectorSize = GetSectorSize();
    const uint32_t DiskSize = GetDiskSize();

    uint32_t seekAddr = m_base + (index * SectorSize);
    if (seekAddr + SectorSize > DiskSize) {
        LogError("attempt to read out-of-bounds sector (index = %u)\n", index);
        return false;
    }

    bool success = true;
    fseek(m_file, seekAddr, SEEK_SET);
    SafeRead(m_file, pBuf, SectorSize);

Cleanup:
    return success;
}

bool FatDisk::ReadCluster(char *pBuf, uint32_t index) const
{
    const uint32_t ClusterSize = GetClusterSize();
    const uint32_t DiskSize = GetDiskSize();

    if ((index - CLUSTER_FIRST) > GetClusterCount()) {
        LogError("attempt to read out-of-bounds cluster (index = %04X)\n",
            index);
        return false;
    }

    bool success = true;
    const BiosParamBlock *bpb = GetBPB();
    uint32_t fsDataSectors = bpb->ReservedSectorCount +
        (bpb->SectorsPerTable * bpb->TableCount) +
        CeilDiv(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize);

    uint32_t seekAddr = m_base +
        (fsDataSectors * bpb->SectorSize) +
        ((index - CLUSTER_FIRST) * ClusterSize);

    assert(seekAddr + ClusterSize <= DiskSize);

    LogVeryVerbose("reading cluster %04X...\n", index);
    fseek(m_file, seekAddr, SEEK_SET);
    SafeRead(m_file, pBuf, ClusterSize);

Cleanup:
    return success;
}

bool FatDisk::ReadRoot(char *pBuf) const
{
    bool success = true;
    const BiosParamBlock *bpb = GetBPB();
    int count = CeilDiv(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize);
    int base = bpb->ReservedSectorCount + (bpb->SectorsPerTable * bpb->TableCount);
    int i = 0;

    LogVeryVerbose("reading root directory...\n");
    while (success && i < count) {
        success = ReadSector(pBuf + (i * GetSectorSize()), base + i);
        i++;
    }

    return success;
}

bool FatDisk::ReadFile(char *pBuf, const DirEntry *pFile) const
{
    if (IsRoot(pFile)) {
        return ReadRoot(pBuf);
    }

    char sfn[MAX_SHORTNAME];
    GetShortName(sfn, pFile);
    LogVeryVerbose("reading file '%s'...\n", sfn);

    if (!IsValidFile(pFile) || (pFile->FirstCluster == 0 && pFile->FileSize != 0)) {
        LogError("attempt to read a deleted or invalid file, device, or volume label\n");
        return false;
    }

    int i = 0;
    bool success = true;

    uint32_t cluster = pFile->FirstCluster;
    while (success && !IsClusterNumberEOC(cluster)) {
        success = ReadCluster(pBuf + (i * GetClusterSize()), cluster);
        cluster = GetCluster(cluster);
        i++;
    }

    return success;
}

bool FatDisk::WriteSector(uint32_t index, const char *pBuf) const
{
    const uint32_t SectorSize = GetSectorSize();
    const uint32_t DiskSize = GetDiskSize();

    uint32_t seekAddr = m_base + (index * SectorSize);
    if (seekAddr + SectorSize > DiskSize) {
        LogError("attempt to write out-of-bounds sector (index = %u)\n", index);
        return false;
    }

    bool success = true;
    fseek(m_file, seekAddr, SEEK_SET);
    SafeWrite(m_file, pBuf, SectorSize);

Cleanup:
    return success;
}

bool FatDisk::WriteCluster(uint32_t index, const char *pBuf) const
{
    const uint32_t ClusterSize = GetClusterSize();
    const uint32_t DiskSize = GetDiskSize();

    if ((index - CLUSTER_FIRST) > GetClusterCount()) {
        LogError("attempt to write out-of-bounds cluster (index = %04X)\n",
            index);
        return false;
    }

    bool success = true;
    const BiosParamBlock *bpb = GetBPB();
    uint32_t fsDataSectors = bpb->ReservedSectorCount +
        (bpb->SectorsPerTable * bpb->TableCount) +
        CeilDiv(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize);

    uint32_t seekAddr = m_base +
        (fsDataSectors * bpb->SectorSize) +
        ((index - CLUSTER_FIRST) * ClusterSize);

    assert(seekAddr + ClusterSize <= DiskSize);

    LogVeryVerbose("writing cluster %04X...\n", index);
    fseek(m_file, seekAddr, SEEK_SET);
    SafeWrite(m_file, pBuf, ClusterSize);

Cleanup:
    return success;
}

bool FatDisk::WriteRoot(const char *pBuf) const
{
    bool success = true;
    const BiosParamBlock *bpb = GetBPB();
    int count = CeilDiv(bpb->RootDirCapacity * sizeof(DirEntry), bpb->SectorSize);
    int base = bpb->ReservedSectorCount + (bpb->SectorsPerTable * bpb->TableCount);
    int i = 0;

    LogVeryVerbose("writing root directory...\n");
    while (success && i < count) {
        success = WriteSector(base + i, pBuf + (i * GetSectorSize()));
        i++;
    }

    return success;
}

bool FatDisk::WriteFAT() const
{
    bool success = true;
    const BiosParamBlock *bpb = GetBPB();
    int base = bpb->ReservedSectorCount;
    int sectorCount = bpb->SectorsPerTable;
    int tableCount = bpb->TableCount;

    int n = 0;
    while (success && n < tableCount) {
        int i = 0;
        LogVeryVerbose("writing file allocation table...\n");
        while (success && i < sectorCount) {
            success = WriteSector(base + i, m_fat + (i * GetSectorSize()));
            i++;
        }
        base += sectorCount;
        n++;
    }
    return success;
}

bool FatDisk::WriteFile(DirEntry *pFile, const char *pBuf, uint32_t sizeBytes)
{
    if (IsRoot(pFile)) {
        return WriteRoot(pBuf);
    }

    char sfn[MAX_SHORTNAME];
    GetShortName(sfn, pFile);
    LogVeryVerbose("writing file '%s'...\n", sfn);

    if (!IsValidFile(pFile)) {
        LogError("attempt to write a label, device, deleted, or invalid file\n");
        return false;
    }

    uint32_t newCount = CeilDiv(sizeBytes, GetClusterSize());
    uint32_t existingCount = CountClusters(pFile);
    uint32_t totalCount = Max(newCount, existingCount);

    if (newCount > CountFreeClusters()) {
        LogError("not enough space on disk!\n");
        return false;
    }

    bool newFile = (existingCount == 0);
    uint32_t firstCluster = pFile->FirstCluster;
    if (newFile) {
        firstCluster = FindNextFreeCluster();
    }

    bool success = true;
    char *clusterBuf = NULL;
    uint32_t cluster = firstCluster;
    uint32_t bytesWritten = 0;

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);

    clusterBuf = (char *) SafeAlloc(GetClusterSize());

    for (uint32_t i = 0; i < totalCount; i++) {
        uint32_t oldValue = GetCluster(cluster);
        // TODO: ensure old value is not bad or reserved

        if (i >= newCount) {
            // File size reduced, mark old clusters as free
            SetCluster(cluster, CLUSTER_FREE);
            cluster = oldValue;
            continue;
        }

        assert(!IsClusterNumberEOC(cluster));

        // Indicate that the cluster is currently being written
        uint32_t next = SetCluster(cluster, CLUSTER_RESERVED);

        // Grab the existing cluster data
        SafeRIF(ReadCluster(clusterBuf, cluster),
            "failed to read cluster %04X\n", cluster);

        // Figure out how may bytes we're writing into the cluster buffer
        uint32_t size = Min(sizeBytes - bytesWritten, GetClusterSize());

        // Copy file data into the cluster buffer
        memcpy(clusterBuf, pBuf, size);

        // Write the cluster
        SafeRIF(WriteCluster(cluster, clusterBuf),
            "failed to write cluster %04X\n", cluster);

        // Advance the file pointer
        pBuf += size;
        bytesWritten += size;

        // Figure out what the next cluster should be
        if (i == newCount - 1) {
            // This is the last cluster, mark it as EOC and preserve the old
            // value so we can free the clusters no longer in use
            SetCluster(cluster, GetClusterNumberEOC());
            cluster = oldValue;
        }
        else if (IsClusterNumberEOC(next) || next == CLUSTER_FREE) {
            // Cluster was already EOC or free, but we have more to write,
            // so we need to find the next free cluster
            next = FindNextFreeCluster();
            SetCluster(cluster, next);
            cluster = next;
        }
        else {
            // Cluster already in-use for the file we're overwriting,
            // so just keep it as is
            SetCluster(cluster, next);
            cluster = next;
        }
    }

    // Update the dir entry
    pFile->FirstCluster = firstCluster;
    pFile->FileSize = sizeBytes;
    SetModifiedTime(pFile, tm);
    SetAccessedTime(pFile, tm);

    // Write the FAT
    SafeRIF(WriteFAT(), "failed to write FAT\n");

Cleanup:
    SafeFree(clusterBuf);
    return success;
}

bool FatDisk::FindFile(DirEntry *pFile, DirEntry *pParent, const char *path) const
{
    char pathCopy[MAX_PATH];
    strncpy(pathCopy, path, MAX_PATH);

    DirEntry root = GetRootDirEntry();
    return WalkPath(pFile, pParent, pathCopy, &root);
}

bool FatDisk::FindFileInDir(DirEntry **ppFile, const DirEntry *pDirTable,
    uint32_t sizeBytes, const char *name) const
{
    wchar_t wName[MAX_PATH];
    mbstowcs(wName, name, MAX_PATH);

    const DirEntry *e = pDirTable;
    int count = sizeBytes / sizeof(DirEntry);

    for (int i = 0; i < count; i++, e++) {
        if (IsFree(e)) {
            continue;
        }

        char shortName[MAX_SHORTNAME];
        wchar_t longName[MAX_LONGNAME];
        longName[0] = L'\0';

        if (IsLongFileName(e)) {
            const DirEntry *sfnEntry = GetLongName(longName, e);
            i += (int) (sfnEntry - e);
            e = sfnEntry;
        }

        bool hasLfn = (longName[0] != L'\0');
        GetShortName(shortName, e);

        if (strncasecmp(shortName, name, MAX_SHORTNAME) == 0) {
            *ppFile = (DirEntry *) e;
            return true;
        }

        if (hasLfn && wcsncmp(longName, wName, MAX_LONGNAME) == 0) {
            *ppFile = (DirEntry *) e;
            return true;
        }
    }

    return false;
}

bool FatDisk::WalkPath(DirEntry *pFile, DirEntry *pParent,
    char *path, const DirEntry *pBase) const
{
    char *nameToFind = strtok(path, "/\\");

    if (nameToFind == NULL) {
        if (pFile != NULL) {
            memcpy(pFile, pBase, sizeof(DirEntry));
        }
        return true;
    }

    if (pBase == NULL || !IsDirectory(pBase)) {
        return false;
    }

    if (pParent != NULL) {
        memcpy(pParent, pBase, sizeof(DirEntry));
    }

    bool success = false;
    DirEntry *found = NULL;
    DirEntry *dirTable = NULL;
    uint32_t sizeBytes;

    sizeBytes = GetFileAllocSize(pBase);
    dirTable = (DirEntry *) SafeAlloc(sizeBytes);
    SafeRIF(ReadFile((char *) dirTable, pBase), "failed to read directory\n");

    success = FindFileInDir(&found, dirTable, sizeBytes, nameToFind);
    if (success) {
        success = WalkPath(pFile, pParent, NULL, found);  // NULL for recursive call
    }

Cleanup:
    SafeFree(dirTable);
    return success;
}
