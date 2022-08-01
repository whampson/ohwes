TARGET = $(BIN_PATH)/fatfs
NO_OBJCOPY = 1

include $(_TOOLGOD)

CC = gcc
LD = gcc

# macOS only
# C_FLAGS += -fsanitize=address
# LD_FLAGS += -fsanitize=address

C_WARNINGS += \
	no-unused-function \
	no-unused-parameter \
	no-unused-variable \
	no-gnu-statement-expression \
	no-pedantic
