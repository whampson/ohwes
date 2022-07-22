#ifndef __DISKIMAGE_H
#define __DISKIMAGE_H

#include "fatfs.h"

bool OpenImage(const char *path);
void CloseImage();

#endif  // __DISKIMAGE_H
