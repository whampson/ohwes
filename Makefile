# boilermake: A reusable, but flexible, boilerplate Makefile.
#
# Copyright 2008, 2009, 2010 Dan Moulding, Alan T. DeKok
# Copyright 2023, 2024 Wes Hampson
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# boilermake:
# Modifications made by Wes Hampson.
#
# 29 Sep 23:
#  - Added support for compiling ASM files.
#
# 17 Jul 24:
#  - Cleaned up variable, macro, and function names.
#  - Items in TARGET_LDLIBS are now relative to TARGET_DIR.
#  - Added function `make-rawbin` to create raw executables with no relocation,
#    symbol, or debugging information.
#  - Added TARGET_LDSCRIPT so targets can specify an optional linker script
#    which is tracked by the build system and will thus trigger a relink if
#    modified.
#  - Build system is now aware of Makefile changes. If this file or a
#    submakefile are modified, the affected objects and targets will be rebuilt
#    to ensure the Makefile changes are applied properly.
#

# ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !
# Caution: Don't edit this Makefile! Create your own main.mk and other
#          submakefiles, which will be included by this Makefile.
#          Only edit this if you need to modify boilermake's behavior (fix
#          bugs, add features, etc).
# ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !

# Note: Parameterized "functions" in this makefile that are marked with
#       "USE WITH EVAL" are only useful in conjuction with eval. This is
#       because those functions result in a block of Makefile syntax that must
#       be evaluated after expansion. Since they must be used with eval, most
#       instances of "$" within them need to be escaped with a second "$" to
#       accomodate the double expansion that occurs when eval is invoked.

# add-clean - Parameterized "function" that adds a new rule and phony
#   target for cleaning the specified target (removing its build-generated
#   files).
#
#   USE WITH EVAL
#
define add-clean
    clean: clean_${1}
    .PHONY: clean_${1}
    clean_${1}:
	$$(strip rm -f ${TARGET_DIR}/${1} $${${1}_OBJECTS:%.o=%.[doP]})
	$${${1}_POSTCLEAN}
endef

# add-object - Parameterized "function" that adds a pattern rule for
#   building object files from source files with the filename extension
#   specified in the second argument. The first argument must be the name of the
#   base directory where the object files should reside (such that the portion
#   of the path after the base directory will match the path to corresponding
#   source files). The third argument must contain the rules used to compile the
#   source files into object code form.
#
#   USE WITH EVAL
#
define add-object
$${BUILD_DIR}/$$(call CANONICAL_PATH,${1})/%.o: ${2} $${${1}_MAKEFILES}
	${3}
endef

# add-target - Parameterized "function" that adds a new target to the
#   Makefile. The target may be an executable or a library. The two allowable
#   types of targets are distinguished based on the name: library targets must
#   end with the traditional ".a" extension.
#
#   USE WITH EVAL
#
define add-target
    ifeq "$$(suffix ${1})" ".a"
        # Add a target for creating a static library.
        $${TARGET_DIR}/${1}: $${${1}_OBJECTS} $${${1}_MAKEFILES}
	    @mkdir -p $$(dir $$@)
	    $$(strip $${AR} $${ARFLAGS} $${${1}_ARFLAGS} $$@ $${${1}_OBJECTS})
	    $${${1}_POSTMAKE}
    else
        # Add a target for linking an executable. First, attempt to select the
        # appropriate front-end to use for linking. This might not choose the
        # right one (e.g. if linking with a C++ static library, but all other
        # sources are C sources), so the user makefile is allowed to specify a
        # linker to be used for each target.
        ifeq "$$(strip $${${1}_LINKER})" ""
            # No linker was explicitly specified to be used for this target. If
            # there are any C++ sources for this target, use the C++ compiler.
            # For all other targets, default to using the C compiler.
            ifneq "$$(strip $$(filter $${CXX_SRC_EXTS},$${${1}_SOURCES}))" ""
                ${1}_LINKER = $${CXX}
            else
                ${1}_LINKER = $${CC}
            endif
        endif

        $${TARGET_DIR}/${1}: $${${1}_OBJECTS} $${${1}_PREREQS} $${${1}_LDLIBS} $${${1}_LDSCRIPT} $${${1}_MAKEFILES}
	    @mkdir -p $$(dir $$@)
	    $$(strip $${${1}_LINKER} -o $$@ $${${1}_OBJECTS} $${LDLIBS} \
	        $${${1}_LDLIBS} $${LDFLAGS} $${${1}_LDFLAGS} $$(addprefix -T,$${${1}_LDSCRIPT}))
	    $${${1}_POSTMAKE}
    endif
endef

# CANONICAL_PATH - Given one or more paths, converts the paths to the canonical
#   form. The canonical form is the path, relative to the project's top-level
#   directory (the directory from which "make" is run), and without
#   any "./" or "../" sequences. For paths that are not  located below the
#   top-level directory, the canonical form is the absolute path (i.e. from
#   the root of the filesystem) also without "./" or "../" sequences.
define CANONICAL_PATH
$(patsubst ${CURDIR}/%,%,$(abspath ${1}))
endef

# COMPILE_C_CMDS - Commands for compiling C source code.
define COMPILE_C_CMDS
	@mkdir -p $(dir $@)
	$(strip ${CC} -o $@ -c -MD ${CFLAGS} ${SOURCE_CFLAGS} ${DEFINES} \
	    ${SOURCE_DEFINES} ${INCLUDES} ${SOURCE_INCLUDES} $<)
	@cp ${@:%$(suffix $@)=%.d} ${@:%$(suffix $@)=%.P}; \
	 sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	     -e '/^$$/ d' -e 's/$$/ :/' < ${@:%$(suffix $@)=%.d} \
	     >> ${@:%$(suffix $@)=%.P}; \
	 rm -f ${@:%$(suffix $@)=%.d}
endef

# COMPILE_CXX_CMDS - Commands for compiling C++ source code.
define COMPILE_CXX_CMDS
	@mkdir -p $(dir $@)
	$(strip ${CXX} -o $@ -c -MD ${CXXFLAGS} ${SOURCE_CXXFLAGS} ${DEFINES} \
	    ${SOURCE_DEFINES}${INCLUDES} ${SOURCE_INCLUDES} $<)
	@cp ${@:%$(suffix $@)=%.d} ${@:%$(suffix $@)=%.P}; \
	 sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	     -e '/^$$/ d' -e 's/$$/ :/' < ${@:%$(suffix $@)=%.d} \
	     >> ${@:%$(suffix $@)=%.P}; \
	 rm -f ${@:%$(suffix $@)=%.d}
endef

# COMPILE_ASM_CMDS - Commands for compiling ASM source code.
define COMPILE_ASM_CMDS
	@mkdir -p $(dir $@)
	$(strip ${AS} -o $@ -c -MD ${ASFLAGS} ${SOURCE_ASFLAGS} ${DEFINES} \
	    ${SOURCE_DEFINES} ${INCLUDES} ${SOURCE_INCLUDES} $<)
	@cp ${@:%$(suffix $@)=%.d} ${@:%$(suffix $@)=%.P}; \
	 sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	     -e '/^$$/ d' -e 's/$$/ :/' < ${@:%$(suffix $@)=%.d} \
	     >> ${@:%$(suffix $@)=%.P}; \
	 rm -f ${@:%$(suffix $@)=%.d}
endef

# -----------------------------------------------------------------------------

# make-rawbin - Create a raw binary executable using objcopy. Raw binary
#   executables contain no relocation, symbol, or debugging information.
#     1 - target stripped binary
#     2 - flags to pass to objcopy
#
#   USE WITH EVAL
#
define make-rawbin
all: $${TARGET_DIR}/${1}
$${TARGET_DIR}/${1}: $${TARGET_DIR}/${TARGET}
	@mkdir -p $$(dir $$@)
	$$(strip $${OBJCOPY} -Obinary ${2} $$< $$@)
        $$(eval $$(call add-clean,${1}))
endef

# -----------------------------------------------------------------------------

# include-submakefile - Parameterized "function" that includes a new
#   "submakefile" fragment into the overall Makefile. It also recursively
#   includes all submakefiles of the specified submakefile fragment.
#
#   USE WITH EVAL
#
define include-submakefile
    # Initialize variables that apply to the current target.
    TARGET              :=
    TARGET_CFLAGS       :=
    TARGET_CXXFLAGS     :=
    TARGET_ASFLAGS      :=
    TARGET_DEFINES      :=
    TARGET_INCLUDES     :=
    TARGET_LDFLAGS      :=
    TARGET_LDLIBS       :=
    TARGET_LDSCRIPT     :=
    TARGET_LINKER       :=
    TARGET_POSTCLEAN    :=
    TARGET_POSTMAKE     :=
    TARGET_PREREQS      :=

    # Initialize variables that apply to the current set of source files.
    SOURCES             :=
    SOURCE_CFLAGS       :=
    SOURCE_CXXFLAGS     :=
    SOURCE_ASFLAGS      :=
    SOURCE_DEFINES      :=
    SOURCE_INCLUDES     :=

    # Initialize other pre-include variables.
    SUBMAKEFILES  :=

    # A directory stack is maintained so that the correct paths are used as we
    # recursively include all submakefiles. Get the makefile's directory and
    # push it onto the stack.
    DIR := $(call CANONICAL_PATH,$(dir ${1}))
    _DIR_STACK := $$(call PUSH,$${_DIR_STACK},$${DIR})

    include ${1}

    # Push the current "executing" Makefile onto the Makefile stack.
    CURRENT_MAKEFILE := $$(call CANONICAL_PATH,$$(abspath $$(lastword $${MAKEFILE_LIST})))
    _MAKEFILE_STACK := $$(call PUSH,$${_MAKEFILE_STACK},$${CURRENT_MAKEFILE})

    # Initialize post-include local variables.
    OBJECTS :=

    # Ensure that valid values are set for BUILD_DIR and TARGET_DIR.
    ifeq "$$(strip $${BUILD_DIR})" ""
        BUILD_DIR := build
    endif
    ifeq "$$(strip $${TARGET_DIR})" ""
        TARGET_DIR := .
    endif

    # Determine which target this makefile's variables apply to. A stack is
    # used to keep track of which target is the "current" target as we
    # recursively include other submakefiles.
    ifneq "$$(strip $${TARGET})" ""
        # This makefile defined a new target. Target variables defined by this
        # makefile apply to this new target. Initialize the target's variables.
        TARGET := $$(strip $${TARGET})
        ALL_TARGETS += $${TARGET}
        $${TARGET}_CFLAGS       := $${TARGET_CFLAGS}
        $${TARGET}_CXXFLAGS     := $${TARGET_CXXFLAGS}
        $${TARGET}_ASFLAGS      := $${TARGET_ASFLAGS}
        $${TARGET}_DEFINES      := $${TARGET_DEFINES}
        $${TARGET}_DEPENDS      :=
        TARGET_INCLUDES         := $$(call QUALIFY_PATH,$${DIR},$${TARGET_INCLUDES})
        TARGET_INCLUDES         := $$(call CANONICAL_PATH,$${TARGET_INCLUDES})
        $${TARGET}_INCLUDES     := $${TARGET_INCLUDES}
        $${TARGET}_LDFLAGS      := $${TARGET_LDFLAGS}
        $${TARGET}_LDLIBS       := $$(addprefix $${TARGET_DIR}/,$${TARGET_LDLIBS})
        TARGET_LDSCRIPT         := $$(call QUALIFY_PATH,$${DIR},$${TARGET_LDSCRIPT})
        TARGET_LDSCRIPT         := $$(call CANONICAL_PATH,$${TARGET_LDSCRIPT})
        $${TARGET}_LDSCRIPT     := $${TARGET_LDSCRIPT}
        $${TARGET}_LINKER       := $${TARGET_LINKER}
        $${TARGET}_OBJECTS      :=
        $${TARGET}_POSTCLEAN    := $${TARGET_POSTCLEAN}
        $${TARGET}_POSTMAKE     := $${TARGET_POSTMAKE}
        $${TARGET}_PREREQS      := $$(addprefix $${TARGET_DIR}/,$${TARGET_PREREQS})
        $${TARGET}_SOURCES      :=
        $${TARGET}_MAKEFILES    := $$(subst :, ,$${_MAKEFILE_STACK})
    else
        # The values defined by this makefile apply to the the "current" target
        # as determined by which target is at the top of the stack.
        TARGET := $$(strip $$(call PEEK,$${_TARGET_STACK}))
        $${TARGET}_CFLAGS       += $${TARGET_CFLAGS}
        $${TARGET}_CXXFLAGS     += $${TARGET_CXXFLAGS}
        $${TARGET}_ASFLAGS      += $${TARGET_ASFLAGS}
        $${TARGET}_DEFINES      += $${TARGET_DEFINES}
        TARGET_INCLUDES         := $$(call QUALIFY_PATH,$${DIR},$${TARGET_INCLUDES})
        TARGET_INCLUDES         := $$(call CANONICAL_PATH,$${TARGET_INCLUDES})
        $${TARGET}_INCLUDES     += $${TARGET_INCLUDES}
        $${TARGET}_LDFLAGS      += $${TARGET_LDFLAGS}
        $${TARGET}_LDLIBS       += $${TARGET_LDLIBS}
        $${TARGET}_POSTCLEAN    += $${TARGET_POSTCLEAN}
        $${TARGET}_POSTMAKE     += $${TARGET_POSTMAKE}
        $${TARGET}_PREREQS      += $${TARGET_PREREQS}
    endif

    # Push the current target onto the target stack.
    _TARGET_STACK := $$(call PUSH,$${_TARGET_STACK},$${TARGET})

    ifneq "$$(strip $${SOURCES})" ""
        # This makefile builds one or more objects from source. Validate the
        # specified sources against the supported source file types.
        BAD_SRCS := $$(strip $$(filter-out $${ALL_SRC_EXTS},$${SOURCES}))
        ifneq "$${BAD_SRCS}" ""
            $$(error Unsupported source file(s) found in ${1} [$${BAD_SRCS}])
        endif

        # Qualify and canonicalize paths.
        SOURCES         := $$(call QUALIFY_PATH,$${DIR},$${SOURCES})
        SOURCES         := $$(call CANONICAL_PATH,$${SOURCES})
        SOURCE_INCLUDES := $$(call QUALIFY_PATH,$${DIR},$${SOURCE_INCLUDES})
        SOURCE_INCLUDES := $$(call CANONICAL_PATH,$${SOURCE_INCLUDES})

        # Save the list of source files for this target.
        $${TARGET}_SOURCES += $${SOURCES}

        # Convert the source file names to their corresponding object file
        # names.
        OBJECTS := $$(addprefix $${BUILD_DIR}/$$(call CANONICAL_PATH,$${TARGET})/,\
                   $$(addsuffix .o,$$(basename $${SOURCES})))

        # Add the objects to the current target's list of objects, and create
        # target-specific variables for the objects based on any source
        # variables that were defined.
        $${TARGET}_OBJECTS += $${OBJECTS}
        $${TARGET}_DEPENDS += $${OBJECTS:%.o=%.P}
        $${OBJECTS}: SOURCE_CFLAGS      := $${$${TARGET}_CFLAGS} $${SOURCE_CFLAGS}
        $${OBJECTS}: SOURCE_CXXFLAGS    := $${$${TARGET}_CXXFLAGS} $${SOURCE_CXXFLAGS}
        $${OBJECTS}: SOURCE_ASFLAGS     := $${$${TARGET}_ASFLAGS} $${SOURCE_ASFLAGS}
        $${OBJECTS}: SOURCE_DEFINES     := $$(addprefix -D,$${$${TARGET}_DEFINES} $${SOURCE_DEFINES})
        $${OBJECTS}: SOURCE_INCLUDES    := $$(addprefix -I,$${$${TARGET}_INCLUDES} $${SOURCE_INCLUDES})
    endif

    ifneq "$$(strip $${SUBMAKEFILES})" ""
        # This makefile has submakefiles. Recursively include them.
        $$(foreach MK,$${SUBMAKEFILES},\
           $$(eval $$(call include-submakefile,\
                      $$(call CANONICAL_PATH,\
                         $$(call QUALIFY_PATH,$${DIR},$${MK})))))
    endif

    # Reset the "current" target to it's previous value.
    _TARGET_STACK := $$(call POP,$${_TARGET_STACK})
    TARGET := $$(call PEEK,$${_TARGET_STACK})

    # Reset the "current" directory to it's previous value.
    _DIR_STACK := $$(call POP,$${_DIR_STACK})
    DIR := $$(call PEEK,$${_DIR_STACK})

    # Reset the "current" Makefile to it's previous value.
    _MAKEFILE_STACK := $$(call POP,$${_MAKEFILE_STACK})
    CURRENT_MAKEFILE := $$(call PEEK,$${_MAKEFILE_STACK})
endef

# MIN - Parameterized "function" that results in the minimum lexical value of
#   the two values given.
define MIN
$(firstword $(sort ${1} ${2}))
endef

# PEEK - Parameterized "function" that results in the value at the top of the
#   specified colon-delimited stack.
define PEEK
$(lastword $(subst :, ,${1}))
endef

# POP - Parameterized "function" that pops the top value off of the specified
#   colon-delimited stack, and results in the new value of the stack. Note that
#   the popped value cannot be obtained using this function; use peek for that.
define POP
${1:%:$(lastword $(subst :, ,${1}))=%}
endef

# PUSH - Parameterized "function" that pushes a value onto the specified colon-
#   delimited stack, and results in the new value of the stack.
define PUSH
${2:%=${1}:%}
endef

# QUALIFY_PATH - Given a "root" directory and one or more paths, qualifies the
#   paths using the "root" directory (i.e. appends the root directory name to
#   the paths) except for paths that are absolute.
define QUALIFY_PATH
$(addprefix ${1}/,$(filter-out /%,${2})) $(filter /%,${2})
endef

###############################################################################
#
# Start of Makefile Evaluation
#
###############################################################################

# Older versions of GNU Make lack capabilities needed by boilermake.
# With older versions, "make" may simply output "nothing to do", likely leading
# to confusion. To avoid this, check the version of GNU make up-front and
# inform the user if their version of make doesn't meet the minimum required.
MIN_MAKE_VERSION := 3.81
MIN_MAKE_VER_MSG := boilermake requires GNU Make ${MIN_MAKE_VERSION} or greater
ifeq "${MAKE_VERSION}" ""
    $(info GNU Make not detected)
    $(error ${MIN_MAKE_VER_MSG})
endif
ifneq "${MIN_MAKE_VERSION}" "$(call MIN,${MIN_MAKE_VERSION},${MAKE_VERSION})"
    $(info This is GNU Make version ${MAKE_VERSION})
    $(error ${MIN_MAKE_VER_MSG})
endif

# Define the source file extensions that we know how to handle.
C_SRC_EXTS := %.c
CXX_SRC_EXTS := %.C %.cc %.cp %.cpp %.CPP %.cxx %.c++
ASM_SRC_EXTS := %.S
ALL_SRC_EXTS := ${C_SRC_EXTS} ${CXX_SRC_EXTS} ${ASM_SRC_EXTS}

# Initialize global variables.
ALL_TARGETS     :=
DEFINES         :=
INCLUDES        :=

_DIR_STACK       :=
_TARGET_STACK    :=
_MAKEFILE_STACK  := Makefile    # Initialize stack with this file

# Include the main user-supplied submakefile. This also recursively includes
# all other user-supplied submakefiles.
$(eval $(call include-submakefile,main.mk))

# Perform post-processing on global variables as needed.
DEFINES := $(addprefix -D,${DEFINES})
INCLUDES := $(addprefix -I,$(call CANONICAL_PATH,${INCLUDES}))

# Define the "all" target (which simply builds all user-defined targets) as the
# default goal.
.PHONY: all
all: $(addprefix ${TARGET_DIR}/,${ALL_TARGETS})

# Add a new target rule for each user-defined target.
$(foreach TGT,${ALL_TARGETS},\
  $(eval $(call add-target,${TGT})))

$(foreach TGT,${ALL_TARGETS},\
  $(foreach EXT,${C_SRC_EXTS},\
    $(eval $(call add-object,${TGT},${EXT},$${COMPILE_C_CMDS}))))

# Add pattern rule(s) for creating compiled object code from C++ source.
$(foreach TGT,${ALL_TARGETS},\
  $(foreach EXT,${CXX_SRC_EXTS},\
    $(eval $(call add-object,${TGT},${EXT},$${COMPILE_CXX_CMDS}))))

# Add pattern rule(s) for creating compiled object code from ASM source.
$(foreach TGT,${ALL_TARGETS},\
  $(foreach EXT,${ASM_SRC_EXTS},\
    $(eval $(call add-object,${TGT},${EXT},$${COMPILE_ASM_CMDS}))))

# Add "clean" rules to remove all build-generated files.
.PHONY: clean
$(foreach TGT,${ALL_TARGETS},\
  $(eval $(call add-clean,${TGT})))

# Include generated rules that define additional (header) dependencies.
$(foreach TGT,${ALL_TARGETS},\
  $(eval -include ${${TGT}_DEPENDS}))
