#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include "fat.hpp"

class DiskImage {       // FatDisk?
public:

    static bool CreateNew(const char *path, const BiosParamBlock *bpb);
    static bool CreateNew(const char *path, const BiosParamBlock *bpb, int sector);

    static DiskImage * Open(const char *path);
    static DiskImage * Open(const char *path, int sector);

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
    int MarkCluster(int index, int value);

    ~DiskImage();

private:
    BootSector  m_Boot;
    char        *m_Fat;
    DirEntry    *m_Root;
    const char  *m_Path;
    FILE        *m_File;

    DiskImage();
};

#endif // DISKIMAGE_H
