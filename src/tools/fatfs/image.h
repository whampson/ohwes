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
DirEntry * GetRootDir(void);

size_t GetClusterSize(void);
size_t GetRootDirSize(void);
size_t GetTableSize(void);

uint32_t GetNextCluster(uint32_t current);

const DirEntry * FindFile(const char *path);
// const DirEntry * FindFileInDir(const DirEntry *dir, const char *fileName);

bool ReadFile(const DirEntry *file, char *dst);
bool ReadCluster(uint32_t index, char *dst);

#endif  // __IMAGE_H
