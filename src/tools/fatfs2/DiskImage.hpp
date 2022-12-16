#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include "fat.hpp"

class DiskImage {
public:
    static DiskImage * CreateNew(const char *path, const BiosParamBlock *bpb);
    static DiskImage * Open(const char *path);

    void PrintDiskInfo() const { }

    ~DiskImage() { LogInfo("DTOR()\n"); }
    DiskImage() { LogInfo("CTOR()\n"); }

private:
};

#endif // DISKIMAGE_H
