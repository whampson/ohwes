TARGET := boot/boot.elf
TARGET_LDSCRIPT := boot.ld

SOURCES := \
    stage1.S \
    stage2.S \

$(eval $(call make-rawbin,boot/boot.bin,--only-section=.stage1))
$(eval $(call make-rawbin,boot/boot.sys,--only-section=.stage2))
