TARGET = $(BIN_PATH)/fatfs
NO_OBJCOPY = 1

include $(_MAKEROOT_TOOLS)

CC = gcc
CXX = g++
LD = g++

# MinGW32
ifeq ($(BUILD_MSYS),1)
  C_WARNINGS += \
    no-address-of-packed-member
  CXX_WARNINGS += \
    no-format-zero-length
endif

# macOS only
ifeq ($(BUILD_OSX),1)
  C_FLAGS += -fsanitize=address
  CXX_FLAGS += -fsanitize=address
  LD_FLAGS += -fsanitize=address
  CXX_WARNINGS += \
    no-unused-function \
    no-gnu-statement-expression \
    no-gnu-anonymous-struct
endif
