# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# NIOBIUM NIOBIUM NIOBIUM NIOBIUM NIOBIUM NIOBIUM NIOBIUM NIOBIUM 
# NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS NbOS
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

include build/Common.mk

.PHONY: all tools src

all: tools src
src:
	@$(MAKE) -C $(_SRCROOT)

tools:
	@$(MAKE) -C $(_TOOLSRC)