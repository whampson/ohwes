#ifndef __IMAGE_H
#define __IMAGE_H

#include "fatfs.h"
#include "fat12.h"

/**
 * Opens a disk image for reading and writing.
 *
 * @param path  the path to the disk image file
 * @return      true if the file contains a valid FAT12 disk image
 *              and was loaded properly
 * @warning     allocates memory on success,
 *              use CloseImage() to de-allocate memory when done
 */
bool OpenImage(const char *path);

/**
 * Closes the current disk image.
 */
void CloseImage(void);

/**
 * Returns true if a disk image is currently loaded.
 */
bool IsImageOpen(void);

/**
 * Returns the file path of the currently-loaded disk image.
 */
const char * GetImagePath(void);

/**
 * Returns a pointer to the boot sector of the currently-loaded disk image.
 */
BootSector * GetBootSector(void);

/**
 * Returns a pointer to the BIOS parameter block (BPB) of the currently-loaded
 * disk image.
 */
BiosParamBlock * GetBiosParams(void);

/**
 * Returns a pointer to a DirEntry representing the root directory.
 */
const DirEntry * GetRootDir(void);

/**
 * Returns the cluster map (File Allocation Table).
 */
const uint32_t * GetClusterMap(void);

/**
 * Returns the size of a data cluster in bytes.
 */
size_t GetClusterSize(void);

/**
 * Returns the next cluster index in the chain.
 *
 * @return  the index of the next cluster in the chain according to the cluster
 *          map (File Allocation Table); CLUSTER_END marks the end of the chain
 */
uint32_t GetNextCluster(uint32_t current);

/**
 * Returns the number of bytes required to store a file on disk.
 * NOTE: for directories, this returns the size of the directory tree structure,
 * not the total size of the contents of the directory.
 *
 * @param file  a DirEntry pointing to the file whose size on disk to calculate
 * @return      the size in bytes of the file
 */
uint32_t GetFileSize(const DirEntry *file);

/**
 * Calculates the number of bytes required to store a file on disk.
 *
 * @param file  a DirEntry pointing to the file whose size on disk to calculate
 * @return      the number of bytes this file takes up on disk
 */
uint32_t GetFileSizeOnDisk(const DirEntry *file);

/**
 * Attempts to locate a file on disk using the specified absolute path.
 *
 * @param file  a pointer to the DirEntry to store found file information
 * @param path  the absolute path to the desired file
 * @return      true if the file was found
 */
bool FindFile(DirEntry *file, const char *path);

/**
 * Attempts to locate a file on disk using the specified relative path.
 *
 * @param file  a pointer to the DirEntry to store found file information
 * @param path  the relative path to the desired file
 * @param dir   the directory in which to begin searching
 * @return      true if the file was found
 */
bool FindFileInDir(DirEntry *file, const char *path, const DirEntry *dir);

/**
 * Reads a file from disk into the destination buffer.
 *
 * @param dst   a pre-allocated destination buffer
 * @param file  a DirEntry pointing to the file to read
 * @return      true if successful
 */
bool ReadFile(char *dst, const DirEntry *file);

/**
 * Reads a single cluster from disk.
 *
 * @param dst   a pre-allocated destination buffer,
 *              must be at least the size of one cluster
 * @param index the cluster to read
 * @return      true if successful
 */
bool ReadCluster(char *dst, uint32_t index);

#endif  // __IMAGE_H
