#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include "fat12.hpp"

class DiskImage
{
public:
    static bool Create(const char *path);

    DiskImage(const char *path);
    ~DiskImage();

private:
    BootSector m_BootSect;
    // uint32_t m_NumClusters;
    // uint32_t *m_pClusterMap;
    FILE *m_FilePtr;
    char m_ImagePath[MAX_PATH];
    bool m_IsValid;
};

#endif // DISKIMAGE_H
