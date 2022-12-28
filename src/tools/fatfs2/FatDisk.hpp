#ifndef FATDISK_HPP
#define FATDISK_HPP

#include "fatfs.hpp"

class FatDisk {
public:
    static bool CreateNew(const char *path, const BiosParamBlock *bpb);
    static bool CreateNew(const char *path, const BiosParamBlock *bpb, uint32_t sector);

    static FatDisk * Open(const char *path);
    static FatDisk * Open(const char *path, uint32_t sector);

    bool IsFat12() const;
    bool IsFat16() const;

    const BiosParamBlock * GetBPB() const;

    uint32_t GetDiskSize() const;
    uint32_t GetFileSize(const DirEntry *pFile) const;
    uint32_t GetFileAllocSize(const DirEntry *pFile) const;

    uint32_t GetSectorSize() const;
    uint32_t GetSectorCount() const;
    uint32_t GetClusterSize() const;
    uint32_t GetClusterCount() const;
    uint32_t GetFatCapacity() const;
    uint32_t GetRootCapacity() const;

    uint32_t CountFreeClusters() const;
    uint32_t CountBadClusters() const;
    uint32_t CountClusters(const DirEntry *pFile) const;

    bool IsEOC(int clustNum) const;

    bool IsClusterBad(uint32_t index) const;
    bool IsClusterFree(uint32_t index) const;

    // Danger area!
    uint32_t MarkClusterBad(uint32_t index);
    uint32_t MarkClusterFree(uint32_t index);

    uint32_t GetCluster(uint32_t index) const;
    uint32_t SetCluster(uint32_t index, uint32_t value);

    bool ReadSector(char *pBuf, uint32_t index) const;
    bool ReadCluster(char *pBuf, uint32_t index) const;
    bool ReadFile(char *pBuf, const DirEntry *pFile) const;

    bool FindFile(DirEntry *pFile, DirEntry *pParent, const char *path) const;
    bool FindFileInDir(const DirEntry **ppFile, const DirEntry *pDirTable,
        uint32_t sizeBytes, const char *path) const;

    ~FatDisk();

private:
    const char  *m_Path;        // file path
    FILE        *m_File;        // file pointer
    size_t      m_Base;         // byte offset in disk of FAT filesystem
    BootSector  m_Boot;         // boot sector
    char        *m_Fat;         // file allocation table

    // TODO: use EOC mark from FAT
    // TODO: bits [15:14] of FAT[1] on FAT16 are flags
    //   0x8000 - CleanShut  - 0 is "dirty", disk not dismounted properly
    //   0x4000 - HardError  - 0 is "error", I/O errors or bad clusters encountered

    FatDisk(const char *path, FILE *file, size_t base, BootSector *boot, char *fat);

    uint32_t GetClusterEocNumber() const;
    uint32_t GetClusterBadNumber() const;
    DirEntry GetRootDirEntry() const;

    bool WalkPath(DirEntry *pFile, DirEntry *pParent, char *path,
        const DirEntry *pBase) const;
};

#endif // FATDISK_HPP
