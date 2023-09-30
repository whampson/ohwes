TARGET_DIR := bin
BUILD_DIR  := obj

SUBMAKEFILES := src/ohwes.mk

.PHONY: all
all:

.PHONY: nuke
nuke:
	$(RM) -r $(TARGET_DIR) $(BUILD_DIR)
