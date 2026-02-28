# ============================================================
# 6502 ASSEMBLER STUDIO — PRO MAKEFILE
# Includes Versioning, Dependency Tracking, and Cross-Platform I/O
# ============================================================

# ------------------------------------------------------------
# COMPILER SETTINGS
# ------------------------------------------------------------
CXX       := g++
# -MMD -MP generates dependency (.d) files automatically!
CXXFLAGS  := -std=c++17 -Wall -Wextra -Wno-unused-parameter -MMD -MP

INCLUDES := \
    -I./src \
    -I./src/ui \
    -I./vendor/imgui \
    -I./vendor/imgui/backends \
    -I./vendor/GLFW/include \
    -I./vendor/stb_image

# ------------------------------------------------------------
# PROJECT DIRECTORIES
# ------------------------------------------------------------
SRC_DIR      := src
UI_DIR       := src/ui
VENDOR_DIR   := vendor/imgui
BACKEND_DIR  := vendor/imgui/backends
LIB_DIR      := libs
ASSETS_DIR   := assets

BUILD_DIR    := build
DEBUG_DIR    := $(BUILD_DIR)/debug
RELEASE_DIR  := $(BUILD_DIR)/release

# Phony targets don't represent actual files
.PHONY: all debug release clean clean-all info copy-libs copy-assets bump_build

# ============================================================
# VERSIONING SYSTEM
# ============================================================

# Defining the base semantic versioning
VERSION_MAJOR	:= 0
VERSION_MINOR	:= 1

GIT_HASH       := $(shell git rev-parse --short HEAD 2>/dev/null || echo "nogit")
BUILD_DATE     := $(shell date +"%Y-%m-%d %H:%M:%S")
BUILD_FILE     := build_number.txt
BUILD_NUMBER   := $(shell [ -f $(BUILD_FILE) ] && cat $(BUILD_FILE) || echo 0)

# Calculate the Patch Version (integer division by 100)
VERSION_PATCH	:= $(shell expr $(BUILD_NUMBER) / 100)

# Combine into standard Semantic Versioning (Major.Minor.Patch)
VERSION			:= $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

# Calculate the next number safely, avoiding Windows shell math bugs
NEXT_BUILD_NUMBER := $(shell expr $(BUILD_NUMBER) + 1)
VERSION_HEADER := $(SRC_DIR)/version.h

# Generate version.h if it doesn't exist
$(VERSION_HEADER): $(wildcard .git/HEAD) $(wildcard .git/index) $(BUILD_FILE)
	@echo "Generating initial version.h..."
	@echo "#pragma once" > $(VERSION_HEADER)
	@echo "#define APP_VERSION \"$(VERSION)\"" >> $(VERSION_HEADER)
	@echo "#define APP_GIT_HASH \"$(GIT_HASH)\"" >> $(VERSION_HEADER)
	@echo "#define APP_BUILD_DATE \"$(BUILD_DATE)\"" >> $(VERSION_HEADER)
	@echo "#define APP_BUILD_NUMBER \"$(BUILD_NUMBER)\"" >> $(VERSION_HEADER)

# Run `make bump_build` manually when you want to increment the build number
bump_build:
	@echo $(NEXT_BUILD_NUMBER) > $(BUILD_FILE)
	@rm -f $(VERSION_HEADER)
	@$(MAKE) $(VERSION_HEADER) --no-print-directory

# ============================================================
# PLATFORM‑SPECIFIC SETTINGS
# ============================================================

ifeq ($(OS),Windows_NT)
    LIBS       := -L./libs -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32
    TARGET_EXT := .exe
    
    # Check for Windows Resource file for app icons
    ifneq ("$(wildcard $(SRC_DIR)/*.rc)","")
        RES_OBJ_D := $(DEBUG_DIR)/obj/resource.res
        RES_OBJ_R := $(RELEASE_DIR)/obj/resource.res
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        LIBS       := -lglfw -lGL -ldl -lpthread -lm
        TARGET_EXT :=
    endif
    ifeq ($(UNAME_S),Darwin)
        LIBS       := -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
        TARGET_EXT :=
    endif
endif

WINDRES := windres

# ============================================================
# SOURCE & OBJECT FILE DISCOVERY
# ============================================================

SRCS := \
    $(wildcard $(SRC_DIR)/*.cpp) \
    $(wildcard $(UI_DIR)/*.cpp) \
    $(wildcard $(VENDOR_DIR)/*.cpp) \
    $(wildcard $(BACKEND_DIR)/*.cpp)

DEBUG_OBJS   := $(patsubst %.cpp, $(DEBUG_DIR)/obj/%.o, $(SRCS))
RELEASE_OBJS := $(patsubst %.cpp, $(RELEASE_DIR)/obj/%.o, $(SRCS))

# Collect the generated .d dependency files
DEPS := $(DEBUG_OBJS:.o=.d) $(RELEASE_OBJS:.o=.d)

APP_NAME    := assembler
DEBUG_BIN   := $(DEBUG_DIR)/$(APP_NAME)_d$(TARGET_EXT)
RELEASE_BIN := $(RELEASE_DIR)/$(APP_NAME)$(TARGET_EXT)

# ============================================================
# BUILD TARGETS
# ============================================================

all: debug				#bump_build $(VERSION_HEADER) $(BUILD_DIR)/$(APP_NAME) copy-assets copy-libs

debug: CXXFLAGS += -g -O0 -DDEBUG
debug: bump_build $(VERSION_HEADER) copy-libs copy-assets $(DEBUG_BIN)
	@echo "Debug build complete → $(DEBUG_BIN)"

$(DEBUG_BIN): $(DEBUG_OBJS) $(RES_OBJ_D)
	@mkdir -p $(DEBUG_DIR)
	$(CXX) $(CXXFLAGS) $(DEBUG_OBJS) $(RES_OBJ_D) -o $@ $(LIBS)

release: CXXFLAGS += -O3 -DNDEBUG
release: bump_build $(VERSION_HEADER) copy-libs copy-assets $(RELEASE_BIN)
	@echo "Release build complete → $(RELEASE_BIN)"

$(RELEASE_BIN): $(RELEASE_OBJS) $(RES_OBJ_R)
	@mkdir -p $(RELEASE_DIR)
	$(CXX) $(CXXFLAGS) $(RELEASE_OBJS) $(RES_OBJ_R) -o $@ $(LIBS)

# ============================================================
# COMPILATION RULES
# ============================================================

$(DEBUG_DIR)/obj/%.o: %.cpp $(VERSION_HEADER)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(RELEASE_DIR)/obj/%.o: %.cpp $(VERSION_HEADER)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(DEBUG_DIR)/obj/%.res: $(SRC_DIR)/%.rc
	@mkdir -p $(dir $@)
	$(WINDRES) $< -O coff -o $@

$(RELEASE_DIR)/obj/%.res: $(SRC_DIR)/%.rc
	@mkdir -p $(dir $@)
	$(WINDRES) $< -O coff -o $@

# ============================================================
# ASSET & LIBRARY COPYING (Cross-Platform Fixes)
# ============================================================
# Make's wildcard evaluates to empty if the folder doesn't exist, preventing shell crashes.

HAS_ASSETS := $(wildcard $(ASSETS_DIR)/*)
HAS_LIBS   := $(wildcard $(LIB_DIR)/*)

copy-assets:
ifneq ($(strip $(HAS_ASSETS)),)
	@mkdir -p $(DEBUG_DIR)/assets $(RELEASE_DIR)/assets
	@cp -r $(ASSETS_DIR)/* $(DEBUG_DIR)/assets/ 2>/dev/null || true
	@cp -r $(ASSETS_DIR)/* $(RELEASE_DIR)/assets/ 2>/dev/null || true
endif

copy-libs:
ifneq ($(strip $(HAS_LIBS)),)
	@mkdir -p $(DEBUG_DIR) $(RELEASE_DIR)
	@cp -r $(LIB_DIR)/* $(DEBUG_DIR)/ 2>/dev/null || true
	@cp -r $(LIB_DIR)/* $(RELEASE_DIR)/ 2>/dev/null || true
endif

# ============================================================
# CLEAN & INFO
# ============================================================

clean:
	rm -rf $(DEBUG_DIR)/obj $(RELEASE_DIR)/obj
	@echo "Object files and dependencies cleaned."

clean-all:
	rm -rf $(BUILD_DIR)
	@echo "Entire build directory wiped."

info:
	@echo "--- Project Info ---"
	@echo "Compiler:      $(CXX)"
	@echo "Flags:         $(CXXFLAGS)"
	@echo "Libraries:     $(LIBS)"
	@echo "Target Ext:    $(TARGET_EXT)"
	@echo "Version:       $(VERSION) (Build $(BUILD_NUMBER))"
	@echo "--------------------"

# ============================================================
# INCLUDE DEPENDENCY FILES
# ============================================================
# This tells Make to read the .d files generated by GCC so it knows 
# exactly which .h files trigger which .cpp files to recompile.
-include $(DEPS)