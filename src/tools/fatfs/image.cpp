#include "image.hpp"
#include <cstdio>

FatImage::FatImage()
{

}

FatImage * FatImage::Load(const std::string &path)
{
    FatImage *img = new FatImage();
    if (img == nullptr) {
        return nullptr;
    }

    auto mode = std::ios_base::in | std::ios_base::out | std::ios_base::binary;
    img->m_file.open(path, mode);
    if (!img->m_file.is_open()) {
        return nullptr;
    }

    char sector[512];
    img->Read(sector, 1);
    BootSector *boot = (BootSector *) sector;

    // TODO: detect issues
    printf("            Volume Label: %.11s\n", boot->BiosParams.Label);
    printf("        File System Type: %.8s\n", boot->BiosParams.FileSystemType);
    printf("                OEM Name: %.8s\n", boot->OemName);
    printf("               Volume ID: %#x\n", boot->BiosParams.VolumeId);
    printf("            Drive Number: %d\n", boot->BiosParams.DriveNumber);
    printf("              Media Type: %#x\n", boot->BiosParams.MediaType);
    printf("              Head Count: %d\n", boot->BiosParams.HeadCount);
    printf("            Sector Count: %d\n", boot->BiosParams.SectorCount);
    printf("             Sector Size: %d\n", boot->BiosParams.SectorSize);
    printf("     Sectors Per Cluster: %d\n", boot->BiosParams.SectorsPerCluster);
    printf("       Sectors Per Track: %d\n", boot->BiosParams.SectorsPerTrack);
    printf("       Sectors Per Table: %d\n", boot->BiosParams.SectorsPerTable);
    printf("      Large Sector Count: %d\n", boot->BiosParams.LargeSectorCount);
    printf("     Hidden Sector Count: %d\n", boot->BiosParams.HiddenSectorCount);
    printf("   Reserved Sector Count: %d\n", boot->BiosParams.ReservedSectorCount);
    printf("             Table Count: %d\n", boot->BiosParams.TableCount);
    printf("Max Root Dir Entry Count: %d\n", boot->BiosParams.MaxRootDirEntryCount);
    printf(" Extended Boot Signature: %#x\n", boot->BiosParams.ExtendedBootSignature);
    printf("              (reserved): %d\n", boot->BiosParams._Reserved);

    return img;
}

FatImage::~FatImage()
{

}

BiosParameterBlock * FatImage::GetBPB()
{
    return nullptr;
}

void FatImage::Seek(int pos)
{
    m_file.seekg(pos * 512, std::ios_base::beg);
}

void FatImage::Read(void *data, int numSectors)
{
    m_file.read((char *) data, numSectors * 512);
}

void FatImage::Write(void *data)
{
    (void) data;
}
