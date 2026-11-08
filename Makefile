
APP := bullethell

CC := clang
PKG_CONFIG := pkg-config
SLANGC := slangc

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin
ASSET_DIR := assets

SHADER_DIR := $(ASSET_DIR)/shaders
SHADER_BUILD_DIR := $(BUILD_DIR)/shaders
SHADER_STAGE_DIR := $(BIN_DIR)/assets/shaders
ASSET_STAGE_DIR := $(BIN_DIR)/assets

TARGET := $(BIN_DIR)/$(APP)

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

SHADER_SRCS := $(shell find $(SHADER_DIR) -name '*.hlsl')
SHADER_SPVS := $(patsubst $(SHADER_DIR)/%.hlsl,$(SHADER_BUILD_DIR)/%.spv,$(SHADER_SRCS))
SHADER_DEPS := $(SHADER_SPVS:.spv=.d)
STAGED_SPVS := $(patsubst $(SHADER_DIR)/%.hlsl,$(SHADER_STAGE_DIR)/%.spv,$(SHADER_SRCS))

ASSET_SRCS := $(shell find $(ASSET_DIR) -type f ! -name '*.hlsl' ! -name '*.spv')
STAGED_ASSETS := $(patsubst $(ASSET_DIR)/%,$(ASSET_STAGE_DIR)/%,$(ASSET_SRCS))

PKGS := sdl3 sdl3-image

CPPFLAGS := -Iinclude -Isrc $(shell $(PKG_CONFIG) --cflags $(PKGS))
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -MMD -MP
LDLIBS := $(shell $(PKG_CONFIG) --libs $(PKGS)) -lm

SLANGC_FLAGS := -target spirv -profile spirv_1_0 -entry main

SPIRV_VAL := $(shell command -v spirv-val 2>/dev/null)

shader_stage = $(if $(findstring .vert.,$(notdir $1)),vertex,\
               $(if $(findstring .frag.,$(notdir $1)),fragment,\
               $(if $(findstring .comp.,$(notdir $1)),compute,\
               $(if $(findstring .geom.,$(notdir $1)),geometry,\
               $(if $(findstring .tesc.,$(notdir $1)),hull,\
               $(if $(findstring .tese.,$(notdir $1)),domain,\
               $(if $(findstring .mesh.,$(notdir $1)),mesh,\
               $(if $(findstring .task.,$(notdir $1)),amplification,))))))))

MODE ?= debug

ifeq ($(MODE),release)
    CFLAGS += -O2 -DNDEBUG
else
    CFLAGS += -O0 -g3
endif

.DEFAULT_GOAL := all

.PHONY: all clean run release shaders assets

all: $(TARGET) shaders assets

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(SHADER_BUILD_DIR)/%.spv: $(SHADER_DIR)/%.hlsl
	@mkdir -p $(@D)
	$(SLANGC) $(SLANGC_FLAGS) \
	    $(if $(call shader_stage,$<),-stage $(call shader_stage,$<),) \
	    -depfile $(@:.spv=.d) \
	    $< -o $@

ifdef SPIRV_VAL
	$(SPIRV_VAL) $@ --target-env vulkan1.0
endif

$(SHADER_STAGE_DIR)/%.spv: $(SHADER_BUILD_DIR)/%.spv
	@mkdir -p $(@D)
	cp $< $@

$(ASSET_STAGE_DIR)/%: $(ASSET_DIR)/%
	@mkdir -p $(@D)
	cp $< $@

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

shaders: $(STAGED_SPVS)

assets: $(STAGED_ASSETS)

run: all
	./$(TARGET)

release:
	$(MAKE) MODE=release all

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

-include $(DEPS)
-include $(SHADER_DEPS)
