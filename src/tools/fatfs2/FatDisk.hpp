#ifndef FATDISK_HPP
#define FATDISK_HPP

#include "fatfs.hpp"

class FatDisk {
public:
    static bool CreateNew(const char *path, const BiosParamBlock *bpb);
    static bool CreateNew(const char *path, const BiosParamBlock *bpb,
        uint32_t sector);

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

    uint32_t FindNextFreeCluster() const;

    bool IsClusterNumberEOC(uint32_t clustNum) const;
    bool IsClusterNumberBad(uint32_t clustNum) const;

    uint32_t GetClusterNumberEOC() const;
    uint32_t GetClusterNumberBad() const;

    bool IsClusterBad(uint32_t index) const;
    bool IsClusterFree(uint32_t index) const;

    uint32_t MarkClusterBad(uint32_t index);
    uint32_t MarkClusterFree(uint32_t index);

    uint32_t GetCluster(uint32_t index) const;                  // TODO: rename GetClusterNumber?
    uint32_t SetCluster(uint32_t index, uint32_t value);        // TODO: rename SetClusterNumber?

    bool ReadSector(char *pBuf, uint32_t index) const;          // TODO: private?
    bool ReadCluster(char *pBuf, uint32_t index) const;         // TODO: private?
    bool ReadFile(char *pBuf, const DirEntry *pFile) const;

    bool WriteSector(uint32_t index, const char *pBuf) const;   // TODO: private?
    bool WriteCluster(uint32_t index, const char *pBuf) const;  // TODO: private?
    bool WriteFile(const DirEntry *pFile, const char *pBuf);

    bool FindFile(DirEntry *pFile, DirEntry *pParent, const char *path) const;
    bool FindFileInDir(DirEntry **ppFile, const DirEntry *pDirTable,
        uint32_t sizeBytes, const char *name) const;

    bool CreateDirectory(DirEntry *pDir, DirEntry *pParent, const char *name);
    // TODO: bool CreateFile(DirEntry *pFile, DirEntry *pParent, const char *name);

    ~FatDisk();

private:
    const char  *m_path;        // file path
    FILE        *m_file;        // file pointer
    size_t      m_base;         // byte offset in disk of FAT filesystem
    BootSector  m_boot;         // boot sector
    char        *m_fat;         // file allocation table
    bool        m_bDirty;       // disk has pending writes (or was not dismounted properly if set on open)
    bool        m_bHardError;   // I/O errors or bad clusters encountered

    FatDisk(const char *path, FILE *file, size_t base, BootSector *boot, char *fat);

    DirEntry GetRootDirEntry() const;

    bool ReadRoot(char *pBuf) const;
    bool WriteRoot(const char *pBuf) const;

    // bool ReadFAT();
    bool WriteFAT() const;

    bool WalkPath(DirEntry *pFile, DirEntry *pParent, char *path,
        const DirEntry *pBase) const;
};

#endif // FATDISK_HPP
