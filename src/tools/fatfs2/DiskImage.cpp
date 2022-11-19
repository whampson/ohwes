#include "DiskImage.hpp"

DiskImage::DiskImage()
{
    InitBootSector(&m_BootSect);
}

DiskImage::~DiskImage()
{

}
