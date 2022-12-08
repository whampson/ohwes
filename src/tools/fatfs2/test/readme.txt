fat12_1440K.img
--------------------
$ mkdosfs -v -F 12 /dev/disk2
mkfs.fat 4.2 (2021-01-31)
/dev/disk2 has 2 heads and 18 sectors per track,
hidden sectors 0x0000;
logical sector size is 512,
using 0xf0 media descriptor, with 2880 sectors;
drive number 0x00;
filesystem has 2 12-bit FATs and 1 sector per cluster.
FAT size is 9 sectors, and provides 2847 clusters.
There is 1 reserved sector.
Root directory contains 224 slots and uses 14 sectors.
Volume ID is 634da36e, no volume label.

fat12_4M.img
--------------------
$ mkdosfs -v -F 12 /dev/disk3
mkfs.fat 4.2 (2021-01-31)
/dev/disk3 has 2 heads and 32 sectors per track,
hidden sectors 0x0000;
logical sector size is 512,
using 0xf8 media descriptor, with 8192 sectors;
drive number 0x80;
filesystem has 2 12-bit FATs and 4 sectors per cluster.
FAT size is 6 sectors, and provides 2036 clusters.
There is 1 reserved sector.
Root directory contains 512 slots and uses 32 sectors.
Volume ID is 6376310f, no volume label.

fat12_32M.img
--------------------
$ mkdosfs -v -F 12 /dev/disk4
mkfs.fat 4.2 (2021-01-31)
/dev/disk4 has 4 heads and 32 sectors per track,
hidden sectors 0x0000;
logical sector size is 512,
using 0xf8 media descriptor, with 65536 sectors;
drive number 0x80;
filesystem has 2 12-bit FATs and 32 sectors per cluster.
FAT size is 32 sectors, and provides 2044 clusters.
There are 32 reserved sectors.
Root directory contains 512 slots and uses 32 sectors.
Volume ID is 639361e6, no volume label.

fat12_64M.img
--------------------
$ mkdosfs -v -F 12 /dev/disk5
mkfs.fat 4.2 (2021-01-31)
/dev/disk5 has 8 heads and 32 sectors per track,
hidden sectors 0x0000;
logical sector size is 512,
using 0xf8 media descriptor, with 131072 sectors;
drive number 0x80;
filesystem has 2 12-bit FATs and 64 sectors per cluster.
FAT size is 64 sectors, and provides 2044 clusters.
There are 64 reserved sectors.
Root directory contains 1024 slots and uses 64 sectors.
Volume ID is 63ac6997, no volume label.

fat16_32M.img
--------------------
$ mkdosfs -v -F 16 /dev/disk4
mkfs.fat 4.2 (2021-01-31)
/dev/disk4 has 4 heads and 32 sectors per track,
hidden sectors 0x0000;
logical sector size is 512,
using 0xf8 media descriptor, with 65536 sectors;
drive number 0x80;
filesystem has 2 16-bit FATs and 4 sectors per cluster.
FAT size is 64 sectors, and provides 16343 clusters.
There are 4 reserved sectors.
Root directory contains 512 slots and uses 32 sectors.
Volume ID is 81ec34fb, no volume label.

fat16_64M.img
--------------------
$ mkdosfs -v -F 16 /dev/disk5
mkfs.fat 4.2 (2021-01-31)
/dev/disk5 has 8 heads and 32 sectors per track,
hidden sectors 0x0000;
logical sector size is 512,
using 0xf8 media descriptor, with 131072 sectors;
drive number 0x80;
filesystem has 2 16-bit FATs and 4 sectors per cluster.
FAT size is 128 sectors, and provides 32695 clusters.
There are 4 reserved sectors.
Root directory contains 512 slots and uses 32 sectors.
Volume ID is 820713ee, no volume label.
