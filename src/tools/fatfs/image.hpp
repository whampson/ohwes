#pragma once

#include <cstdio>
#include <string>
#include "fat.hpp"

class FatImage
{
public:
    FatImage();
    ~FatImage();

    static FatImage * Load(const std::string &path);

    BiosParameterBlock * GetBPB();

private:
    FILE *m_pFile;
    int m_Cursor;

    void SeekSector(int pos);
    void ReadSector(void *data, int numSectors);
    void WriteSector(void *data);

};
