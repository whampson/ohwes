include $(_OSROOT)/build/Common.mk

export _BINROOT         := $(CURDIR)/bin
export _OBJROOT         := $(CURDIR)/obj
export CXX              := g++
export CXXFLAGS         := -g
#export CXXFLAGS         += -Wall -Werror -Wpedantic
export LD               := g++
export LDFLAGS          :=
export INC              := .