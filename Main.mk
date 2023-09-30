DEBUG      := 1
DEBUGFLAGS := -DDEBUG -g -Og

TARGET_DIR := bin
BUILD_DIR  := obj
SCRIPT_DIR := scripts

SUBMAKEFILES := src/ohwes.mk

ifndef OHWES_ENVIRONMENT_SET
  $(error "Environment not set! Please source $(SCRIPT_DIR)/env.sh.")
endif

.PHONY: all
all:

.PHONY: nuke
nuke:
	$(RM) -r $(TARGET_DIR) $(BUILD_DIR)
