#ifndef __DISKIMAGE_H
#define __DISKIMAGE_H

#include "fatfs.h"
#include "fat12.h"

bool OpenImage(const char *path);
void CloseImage();

const DirectoryEntry * FindFile(const char *path);

bool ReadFile(const DirectoryEntry *entry, char *buf);

void PrintDiskInfo();

#endif  // __DISKIMAGE_H
