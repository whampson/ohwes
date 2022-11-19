#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include "fat12.hpp"

class DiskImage
{
public:
    DiskImage();
    ~DiskImage();

private:
    BootSector m_BootSect;
    // uint32_t m_NumClusters;
    // uint32_t *m_pClusterMap;
    // FILE *m_FilePtr;
    // char m_ImagePath[MAX_PATH];
};

#endif // DISKIMAGE_H
