include $(_OSROOT)/Makefile

export DEFINES          =

export AS               = g++
export ASFLAGS          =
export CC               = g++
export CFLAGS           =
export LD               = g++
export LDFLAGS          =
export INC              = .

ifdef DEBUG
  export CFLAGS         += -ggdb
endif

# export BINDIR           = $(_BINROOT)/tools

.PHONY: all fatfs

all: fatfs

fatfs:
	@$(MAKE) -C fatfs
