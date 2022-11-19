TARGET = $(BIN_PATH)/fatfs
NO_OBJCOPY = 1

include $(_TOOLMAKEROOT)

CXX = g++
LD = g++

# macOS only
ifeq ($(OSX),1)
  CXX_FLAGS += -fsanitize=address
  LD_FLAGS += -fsanitize=address
endif

CXX_WARNINGS += no-unused-function