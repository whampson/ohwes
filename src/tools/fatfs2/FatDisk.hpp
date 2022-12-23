#ifndef FATDISK_H
#define FATDISK_H

#include "fat.hpp"

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
    uint32_t GetFileSize(const DirEntry *f) const;

    uint32_t GetSectorSize() const;
    uint32_t GetSectorCount() const;
    uint32_t GetClusterSize() const;
    uint32_t GetClusterCount() const;
    uint32_t GetFatCapacity() const;
    uint32_t GetRootCapacity() const;

    uint32_t CountFreeClusters() const;
    uint32_t CountBadClusters() const;

    // Danger area!
    uint32_t MarkClusterBad(uint32_t index);
    uint32_t MarkClusterFree(uint32_t index);

    uint32_t GetCluster(uint32_t index) const;
    uint32_t SetCluster(uint32_t index, uint32_t value);

    bool ReadSector(char *dst, uint32_t index) const;
    bool ReadCluster(char *dst, uint32_t index) const;
    bool ReadFile(char *dst, const DirEntry *file) const;

    bool FindFile(DirEntry *dst, const char *path) const;

    ~FatDisk();

private:
    size_t      m_BaseAddr;     // offset in file of FAT filesystem
    BootSector  m_Boot;         // boot sector
    char        *m_Fat;         // file allocation table
    const char  *m_Path;        // file path
    FILE        *m_File;        // file pointer

    FatDisk();

    // Dummy DirEntry used to identify the root directory
    DirEntry GetRootToken() const;

    bool WalkPath(DirEntry *out, char *path, const DirEntry *base) const;
};

#endif // FATDISK_H
