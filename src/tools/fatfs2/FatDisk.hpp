#ifndef FATDISK_H
#define FATDISK_H

#include "fat.hpp"

class FatDisk {       // FatDisk?
public:

    static bool CreateNew(const char *path, const BiosParamBlock *bpb);
    static bool CreateNew(const char *path, const BiosParamBlock *bpb, int sector);

    static FatDisk * Open(const char *path);
    static FatDisk * Open(const char *path, int sector);

    bool IsFat12() const;

    const BiosParamBlock * GetBPB() const;

    int GetSectorSize() const;
    int GetSectorCount() const;
    int GetClusterSize() const;
    int GetClusterCount() const;
    int GetFatCapacity() const;

    int CountFreeClusters() const;
    int CountBadClusters() const;

    int GetCluster(int index) const;

    // Danger area!
    int MarkClusterBad(int index);
    int MarkClusterFree(int index);
    int SetCluster(int index, int value);

    ~FatDisk();

private:
    BootSector  m_Boot;
    char        *m_Fat;
    DirEntry    *m_Root;
    const char  *m_Path;
    FILE        *m_File;

    FatDisk();
};

#endif // FATDISK_H
