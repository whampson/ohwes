#
# NOTE: using native toolchain!!!
#       do not call from ../Main.mk!
#
DEBUG      := 1
DEBUGFLAGS := -DDEBUG -g -Og

TARGET_DIR := ../bin/tools
BUILD_DIR  := ../obj/tools

SUBMAKEFILES := \
    fatfs/fatfs.mk \

CC  := gcc
CXX := g++
LD  := g++

CXXFLAGS   :=# -Wall -Werror
CFLAGS     := ${CXXFLAGS}

ifeq "${DEBUG}" "1"
  CXXFLAGS += ${DEBUGFLAGS}
  CFLAGS += ${DEBUGFLAGS}
endif

.PHONY: all nuke

all:

# destroy everything!!!
nuke:
	${RM} -r ${TARGET_DIR} ${BUILD_DIR}
