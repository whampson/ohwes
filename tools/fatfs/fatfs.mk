
TARGET = fatfs

SOURCES = \
  Command_Add.cpp \
  Command_Attr.cpp \
  Command_Create.cpp \
  Command_Extract.cpp \
  Command_Info.cpp \
  Command_List.cpp \
  Command_Mkdir.cpp \
  Command_Test.cpp \
  Command_Touch.cpp \
  Command_Type.cpp \
  Command.cpp \
  FatDisk.cpp \
  fat.c \
  main.cpp

TARGET_CXXFLAGS += -Wno-address-of-packed-member -Wno-format-zero-length
TARGET_CFLAGS   += ${TARGET_CXXFLAGS}

# ifeq ($(OSX),1)
#   CFLAGS   += -fsanitize=address
#   CXXFLAGS += -fsanitize=address
#   LDFLAGS  += -fsanitize=address
#   CXX_WARNINGS += \
#     no-unused-function \
#     no-gnu-statement-expression \
#     no-gnu-anonymous-struct
# endif
