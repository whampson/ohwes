MODULES := \
    shell

SUBMAKEFILES := $(addsuffix /Module.mk,${MODULES})
