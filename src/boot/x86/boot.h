#ifndef __BOOT_H
#define __BOOT_H

#define STAGE1_SEGMENT      0x0000
#define STAGE2_SEGMENT      0x0900
#define KERNEL_SEGMENT      0x1000

#define STAGE1_BASE         0x7C00
#define STAGE2_BASE         (STAGE2_SEGMENT<<4)
#define KERNEL_BASE_EARLY   (KERNEL_SEGMENT<<4)

#define STAGE1_ENTRY        (STAGE1_BASE)
#define STAGE2_ENTRY        (STAGE2_BASE)

#define BOOT_STACK          0x7C00

#endif /* __BOOT_H */
