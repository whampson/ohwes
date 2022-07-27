#ifndef __IMAGE_H
#define __IMAGE_H

#include "fatfs.h"
#include "fat12.h"

bool OpenImage(const char *path);
void CloseImage(void);

bool IsImageOpen(void);

const char * GetImagePath(void);
BootSector * GetBootSector(void);
BiosParamBlock * GetBiosParams(void);

uint32_t GetNextCluster(uint32_t cluster);

const DirectoryEntry * FindFile(const char *path);

bool ReadFile(const DirectoryEntry *entry, char *buf);

#endif  // __IMAGE_H
