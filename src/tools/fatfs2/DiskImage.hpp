#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include "fat.hpp"

class DiskImage {
public:

    static bool CreateNew(const char *path, const BiosParamBlock *bpb);
    static DiskImage * Open(const char *path);

    const BiosParamBlock * GetBPB() const;
    int GetSectorSize() const;
    int GetSectorCount() const;
    int GetClusterSize() const;
    int GetClusterCount() const;

    bool IsFat12() const;

    ~DiskImage();

private:
    BootSector  m_Boot;
    void        *m_Fat;
    DirEntry    *m_Root;
    const char  *m_Path;
    FILE        *m_File;

    DiskImage();
};

#endif // DISKIMAGE_H
