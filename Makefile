OS ?= linux # linux/Linux | windows/Windows_NT
PLATFORM ?= native  # native | web

ifeq ($(OS),Windows_NT)
	OS := windows
endif

TARGET_BASE := HeavenlySilvercross
SO_NAME := magical.so
TARGET := $(TARGET_BASE)

SRC_DIR := ./src
INC_DIR := ./include
BUILD_DIR := ./out

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))
DEP := $(OBJ:.o=.d)

CC ?= gcc
CFLAGS := -std=c23 -I$(INC_DIR) -Wall -Wextra -MMD -MP

LIB_DIR := ./lib

RAYLIB_SRC := $(LIB_DIR)/raylib/src
RAYLIB_PLATFORM := PLATFORM_DESKTOP
RAYLIB_LIB_FILE := $(RAYLIB_SRC)/libraylib.a
RLOS := Linux
AR ?= ar

LDFLAGS := $(RAYLIB_LIB_FILE) -lm -ldl
CFLAGS += -I$(RAYLIB_SRC)

ifeq ($(PLATFORM),web)
	CC := emcc
	TARGET := index.html
	RAYLIB_PLATFORM := PLATFORM_WEB
	AR := emar
	CFLAGS += -DPLATFORM_WEB
	EMFLAGS := \
			-sUSE_GLFW=3 \
			-sFULL_ES2=1 \
			-sMIN_WEBGL_VERSION=1 \
			-sMAX_WEBGL_VERSION=1 \
			-sALLOW_MEMORY_GROWTH=1 \
			-sASYNCIFY \
			--shell-file shell.html \
			--preload-file res@/res
	RAYLIB_LIB_FILE := $(RAYLIB_SRC)/libraylib.web.a
	LDFLAGS := $(RAYLIB_LIB_FILE) -lm $(EMFLAGS)
# TODO: exe icon and details
else ifeq ($(OS),windows)
	CC := x86_64-w64-mingw32-gcc
	TARGET := $(TARGET_BASE).exe
	LDFLAGS += -lopengl32 -lgdi32 -lwinmm -lbcrypt
	RLOS := Windows_NT
else
	LDFLAGS += -lGL -lpthread -ldl -lX11
endif

ifeq ($(DEBUG),1)
	SANITIZER_FLAGS := -fsanitize=address,undefined
	CFLAGS += -save-temps -ggdb -O0 -DDEBUG $(SANITIZER_FLAGS)
	LDFLAGS += $(SANITIZER_FLAGS)
else
	CFLAGS += -O2 -DRELEASE
endif

.PHONY: all clean debug raylib full-clean

all: $(BUILD_DIR)/$(TARGET)

# Compile raylib library
$(RAYLIB_LIB_FILE):
	$(MAKE) -C $(RAYLIB_SRC) \
		PLATFORM=$(RAYLIB_PLATFORM) \
		RAYLIB_LIBTYPE=STATIC \
		CC="$(CC)" \
		OS="$(RLOS)" \
		AR="$(AR)"

# Link object files and create the binary.
$(BUILD_DIR)/$(TARGET): $(OBJ) $(RAYLIB_LIB_FILE) 
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

# Compile object files from .c files.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

debug:
	$(MAKE) DEBUG=1

clean:
	rm -rf $(BUILD_DIR)

# Compile just raylib.
raylib: $(RAYLIB_LIB_FILE)

# Run raylib's make clean as well as cleaning game files.
full-clean:
	rm -rf $(BUILD_DIR) & \
	$(MAKE) -C $(RAYLIB_SRC) clean

-include $(DEP)
