# --- Compiler Settings ---
CXX 	:= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter
INCLUDES = -I./src -I./vendor/imgui -I./vendor/imgui/backends

# --- OS Detection & Cross-Platform Libraries ---
ifeq ($(OS),Windows_NT)
    # WINDOWS
    # Ensure you have glfw3.dll in your lib/ folder!
    LIBS = -L./libs -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32
    TARGET_EXT = .exe
    # Command to copy DLL on Windows using cmd
    COPY_CMD = xcopy /Y /I "lib\*.dll"

	# Resource object
	RES_OBJ_D = $(DEBUG_DIR)/obj/resource.res
	RES_OBJ_R = $(RELEASE_DIR)/obj/resource.res
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        # LINUX
        LIBS = -lglfw -lGL -ldl -lpthread -lm
        TARGET_EXT = 
        COPY_CMD = cp -n lib/*.so*
    endif
    ifeq ($(UNAME_S),Darwin)
        # macOS
        LIBS = -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
        TARGET_EXT = 
        COPY_CMD = cp -n lib/*.dylib
    endif
endif

# --- RESOURCE COMPILER (For the .exe icon) ---
WINDRES       := windres

# --- Directories ---
SRC_DIR 	:= src
VENDOR_DIR 	:= vendor/imgui
LIB_DIR 	:= libs
ASSETS_DIR 	:= assets

# Output Directories
BUILD_DIR 	:= build
DEBUG_DIR 	:= $(BUILD_DIR)/debug
RELEASE_DIR := $(BUILD_DIR)/release

# --- Source Files ---
# Find all .cpp files in src/ and vendor/imgui/
SRCS = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/ui/*.cpp) $(wildcard $(VENDOR_DIR)/*.cpp) $(wildcard $(VENDOR_DIR)/backends/*.cpp)

# Generate object file paths for both Debug and Release
DEBUG_OBJS 		:= $(patsubst %.cpp, $(DEBUG_DIR)/obj/%.o, $(notdir $(SRCS)))
RELEASE_OBJS 	:= $(patsubst %.cpp, $(RELEASE_DIR)/obj/%.o, $(notdir $(SRCS)))


# Target Executable Names
APP_NAME 	:= assembler
DEBUG_BIN 	:= $(DEBUG_DIR)/$(APP_NAME)_d
RELEASE_BIN := $(RELEASE_DIR)/$(APP_NAME)

# --- Phony Targets (Commands that don't represent files) ---
.PHONY: all debug release clean clean-all info copy-libs-debug copy-libs-release

# Default target when you just type 'make'
all: release

# --- Debug Build ---
debug: CXXFLAGS += -g -O0 -DDEBUG
debug: copy-libs-debug copy-assets-debug $(DEBUG_BIN)
	@echo "Debug build complete! Executable is in $(DEBUG_DIR)"

$(DEBUG_BIN): $(DEBUG_OBJS) $(RES_OBJ_D)
	@mkdir -p $(DEBUG_DIR)
	$(CXX) $(CXXFLAGS) $(DEBUG_OBJS) $(RES_OBJ_D) -o $@ $(LIBS)

# --- Release Build ---
release: CXXFLAGS += -O3 -DNDEBUG
release: copy-libs-release copy-assets-release $(RELEASE_BIN)
	@echo "Release build complete! Executable is in $(RELEASE_DIR)"

$(RELEASE_BIN): $(RELEASE_OBJS) $(RES_OBJ_R)
	@mkdir -p $(RELEASE_DIR)
	$(CXX) $(CXXFLAGS) $(RELEASE_OBJS) $(RES_OBJ_R) -o $@ $(LIBS)

# --- Object File Compilation Rules ---
# We use VPATH to tell make where to look for .cpp files
VPATH = $(SRC_DIR):$(SRC_DIR)/ui:$(VENDOR_DIR):$(VENDOR_DIR)/backends

$(DEBUG_DIR)/obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(RELEASE_DIR)/obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# --- Resource File Compilation Rules (Windows) ---
$(DEBUG_DIR)/obj/%.res: $(SRC_DIR)/%.rc
	@mkdir -p $(dir $@)
	$(WINDRES) $< -O coff -o $@

$(RELEASE_DIR)/obj/%.res: $(SRC_DIR)/%.rc
	@mkdir -p $(dir $@)
	$(WINDRES) $< -O coff -o $@

# --- Copying Dynamic Libraries (.dll, .so) ---
copy-assets-debug:
	@mkdir -p $(DEBUG_DIR)/assets
	@cp -r $(ASSETS_DIR)/* $(DEBUG_DIR)/assets/

copy-assets-release:
	@mkdir -p $(RELEASE_DIR)/assets
	@cp -r $(ASSETS_DIR)/* $(RELEASE_DIR)/assets/

copy-libs-debug:
	@mkdir -p $(DEBUG_DIR)
	@if [ -d "$(LIB_DIR)" ] && [ "$$(ls -A $(LIB_DIR))" ]; then cp $(LIB_DIR)/* $(DEBUG_DIR)/; fi

copy-libs-release:
	@mkdir -p $(RELEASE_DIR)
	@if [ -d "$(LIB_DIR)" ] && [ "$$(ls -A $(LIB_DIR))" ]; then cp $(LIB_DIR)/* $(RELEASE_DIR)/; fi

# --- Clean up ---
# Cleans only the object files but leaves the executables
clean:
	rm -rf $(DEBUG_DIR)/obj $(RELEASE_DIR)/obj
	@echo "Object files cleaned."

# Completely wipes the build directory
clean-all:
	rm -rf $(BUILD_DIR)
	@echo "Entire build directory wiped."

# --- Info ---
# Prints out project information to verify makefile paths
info:
	@echo "--- Project Info ---"
	@echo "Compiler:      $(CXX)"
	@echo "Flags:         $(CXXFLAGS)"
	@echo "Includes:      $(INCLUDES)"
	@echo "Libraries:     $(LIBS)"
	@echo "Source files:  $(SRCS)"
	@echo "Build dir:     $(BUILD_DIR)"
	@echo "--------------------"