TARGET = $(BIN_PATH)/fatfs
NO_OBJCOPY = 1

include $(_TOOLMAKEROOT)

CC = gcc
CXX = g++
LD = g++

# macOS only
ifeq ($(OSX),1)
  C_FLAGS += -fsanitize=address
  CXX_FLAGS += -fsanitize=address
  LD_FLAGS += -fsanitize=address
endif

CXX_WARNINGS += \
  no-unused-function \
  no-gnu-statement-expression \
  no-gnu-anonymous-struct
