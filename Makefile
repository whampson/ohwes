export AS       	:= i686-elf-as
export ASFLAGS  	:=
export CC       	:= i686-elf-gcc
export CFLAGS   	:= 
export LD       	:= i686-elf-ld
export LDFLAGS  	:=
#export MAKEFLAGS	:= --no-print-directory
export MKDIR		:= mkdir -p
export RM			:= rm -f

export BASEDIR		:= $(PWD)
export BINDIR     	:= $(BASEDIR)/bin
export OBJDIR      	:= $(BASEDIR)/obj

.PHONY: all dirs boot

all: dirs boot

clean:
	@$(RM) -r $(BINDIR)
	@$(RM) -r $(OBJDIR)

dirs:
	@$(MKDIR) $(BINDIR)
	@$(MKDIR) $(OBJDIR)

boot:
	@$(MAKE) -C boot all