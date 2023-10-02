
SUBMAKEFILES := \
    fatfs/fatfs.mk \

TGT_CC  := gcc
TGT_CXX := g++
TGT_LD  := g++

TGT_CXXFLAGS   := -Wall -Werror
TGT_CFLAGS     := ${TGT_CXXFLAGS}

SRC_INCDIRS :=
# INCDIRS :=

ifeq "${DEBUG}" "1"
  TGT_CFLAGS += ${DEBUGFLAGS}
  TGT_CXXFLAGS += ${DEBUGFLAGS}
endif
