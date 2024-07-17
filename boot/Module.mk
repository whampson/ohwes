###
#  boot.elf = combined stage1 and stage2 executable w/ symbols
#  boot.bin = stripped boot.elf
# bsect.bin = bytes 0-511 of boot.bin; boot sector, stage1
#  boot.sys = bytes 512-end of boot.bin; stage2
###

TARGET := boot.elf

# SOURCES = \
# 	stage1.S \
# 	stage2.S \
# 	entry32.S \

# TGT_LDFLAGS := -T boot/boot.ld

# STAGE1_ARGS = --only-section=.stage1
# STAGE2_ARGS = --only-section=.stage2

# define boot-postmake
#   $(call raw-bin,${TARGET_DIR}/${TARGET},${TARGET_DIR}/${TARGET_STAGE1},${STAGE1_ARGS})
#   $(call raw-bin,${TARGET_DIR}/${TARGET},${TARGET_DIR}/${TARGET_STAGE2},${STAGE2_ARGS})
# endef

# define boot-postclean
# 	${RM} ${TARGET_DIR}/${TARGET_STAGE1}
# 	${RM} ${TARGET_DIR}/${TARGET_STAGE2}
# endef

# TGT_POSTMAKE  := $(call boot-postmake)
# TGT_POSTCLEAN := $(call boot-postclean)
# NOTE: stage1.S must come before stage2.S,
#       or else entrypoint will end up in the wrong spot!
SOURCES := \
    stage1.S \
    stage2.S \

TARGET_LDFLAGS := -Ttext 0x7C00 -e Entry

TARGET_BIN := boot.bin
$(eval $(call make-rawbin,${TARGET_BIN}))

define make-bootsect
all: $${TARGET_DIR}/${1}
$${TARGET_DIR}/${1}: $${TARGET_DIR}/${TARGET_BIN}
	@mkdir -p $$(dir $$@)
	head -c 512 $$< > $$@
        $$(eval $$(call add-clean,${1}))
endef

define make-stage2
all: $${TARGET_DIR}/${1}
$${TARGET_DIR}/${1}: $${TARGET_DIR}/${TARGET_BIN}
	@mkdir -p $$(dir $$@)
	tail -c +513 $$< > $$@
        $$(eval $$(call add-clean,${1}))
endef

$(eval $(call make-bootsect,bsect.bin))
$(eval $(call make-stage2,boot.sys))
