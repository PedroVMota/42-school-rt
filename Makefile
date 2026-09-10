## ============================================================================
##  Project configuration
## ============================================================================

NAME		= rt

SRCS_DIR	= srcs
INC_DIR		= include

CXX			= c++
CXXFLAGS	= -Wall -Wextra -std=c++17 -pthread

## ============================================================================
##  Build mode: release (default) or debug/dev.
##  Usage: make            -> release
##         make dev        -> debug (alias: make debug)
##         make MODE=debug -> debug, explicitly
##  Object files live under separate obj/<mode> dirs so switching modes never
##  mixes stale objects built with different flags.
## ============================================================================

MODE ?= debug

ifeq ($(MODE),debug)
	OBJ_DIR		= obj/debug
	CXXFLAGS	+= -g3 -O0 -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer
	MODE_LDFLAGS	= -fsanitize=address,undefined
else ifeq ($(MODE),release)
	OBJ_DIR		= obj/release
	CXXFLAGS	+= -O3 -DNDEBUG -ffast-math -funroll-loops
else
	$(error Unknown MODE '$(MODE)' - expected 'debug' or 'release')
endif

## ============================================================================
##  OS detection
## ============================================================================

UNAME_S := $(shell uname -s)

## ============================================================================
##  Backend toggle: MiniLibX (default) or a self-fetched, self-built static
##  SDL3 (`make GPU=1`). GPU=1 also defines 3D_GPU_COMPUTING_COMPATIBILITY,
##  which every windowing/rendering class branches on (#ifdef) to pick its
##  SDL3 implementation instead of its MiniLibX one. Neither backend is a
##  system/package-manager dependency: both are fetched/built by this
##  Makefile the same way, so a clean checkout only needs a toolchain (plus
##  cmake and curl for the GPU=1 path) to build either one.
## ============================================================================

GPU ?= 0

## ---- SDL3 archive/dir/lib naming, defined unconditionally so `fclean`
## ---- can remove them regardless of which mode `make` last ran in. ----
SDL_VERSION	= 3.4.16
SDL_ARCHIVE	= SDL3-$(SDL_VERSION).tar.gz
SDL_URL		= https://github.com/libsdl-org/SDL/releases/download/release-$(SDL_VERSION)/$(SDL_ARCHIVE)
SDL_SRC_DIR	= SDL3-$(SDL_VERSION)
SDL_BUILD_DIR	= $(SDL_SRC_DIR)/build
SDL_LIB		= $(SDL_BUILD_DIR)/libSDL3.a

## ============================================================================
##  MiniLibX: pick the right archive/folder/lib per OS.
##  Linux  -> minilibx-linux.tgz        (built via its own ./configure + make)
##  macOS  -> minilibx_macos_opengl.tgz (built via its own make, Objective-C)
##  These tgz files are expected next to this Makefile.
## ============================================================================

ifeq ($(UNAME_S),Linux)
	MLX_ARCHIVE	= minilibx-linux.tgz
	MLX_DIR		= minilibx-linux
	MLX_LIB		= $(MLX_DIR)/libmlx.a
	MLX_SYS_LDFLAGS	= -lXext -lX11 -lbsd
	MLX_BUILD	= (cd $(MLX_DIR) && ./configure && $(MAKE))
	SDL_SYS_LDFLAGS	= -ldl -lpthread -lm
else ifeq ($(UNAME_S),Darwin)
	MLX_ARCHIVE	= minilibx_macos_opengl.tgz
	MLX_DIR		= minilibx_opengl_20191021
	MLX_LIB		= $(MLX_DIR)/libmlx.a
	MLX_SYS_LDFLAGS	= -framework OpenGL -framework AppKit -lz
	MLX_BUILD	= (cd $(MLX_DIR) && $(MAKE))
	CXXFLAGS	+= -Wno-deprecated-declarations
	## SDL3's own CMake build reports the exact frameworks it needs for a
	## static link; this list matches what SDL3's static build pulls in on
	## macOS (Cocoa windowing, Metal for the GPU API, Core* for audio/HID).
	SDL_SYS_LDFLAGS	= -framework Cocoa -framework Metal -framework QuartzCore \
			  -framework CoreVideo -framework CoreAudio -framework AudioToolbox \
			  -framework ForceFeedback -framework GameController -framework CoreHaptics \
			  -framework IOKit -framework Carbon -framework UniformTypeIdentifiers \
			  -framework CoreMedia -framework AVFoundation
else
	$(error Unsupported OS: $(UNAME_S) - only Linux and Darwin are handled)
endif

## NOTE: order matters to the linker - libmlx.a/libSDL3.a must come BEFORE
## the system libraries they depend on, otherwise their symbols get dropped
## before the archive asks for them and you get "undefined reference".
MLX_LDFLAGS	= -L$(MLX_DIR) -lmlx $(MLX_SYS_LDFLAGS) -lm -pthread
SDL_LDFLAGS	= $(SDL_LIB) $(SDL_SYS_LDFLAGS) -pthread

ifeq ($(GPU),1)
	BACKEND_LIB	= $(SDL_LIB)
	BACKEND_LDFLAGS	= $(SDL_LDFLAGS)
	BACKEND_INC	= $(SDL_SRC_DIR)/include
	CXXFLAGS	+= -DGPU_COMPUTING_COMPATIBILITY
else
	BACKEND_LIB	= $(MLX_LIB)
	BACKEND_LDFLAGS	= $(MLX_LDFLAGS)
	BACKEND_INC	= $(MLX_DIR)
endif

## ============================================================================
##  Project sources / includes (recursive)
## ============================================================================

SRCS		:= $(shell find $(SRCS_DIR) -name '*.cpp')
OBJS		:= $(patsubst $(SRCS_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS		:= $(OBJS:.o=.d)

INC_DIRS	:= $(shell find $(INC_DIR) -type d) $(BACKEND_INC)
CXXFLAGS	+= $(addprefix -I,$(INC_DIRS))

## ============================================================================
##  Rules
## ============================================================================

all: $(NAME)

$(NAME): $(BACKEND_LIB) $(OBJS)
	$(CXX) $(MODE_LDFLAGS) $(OBJS) $(BACKEND_LDFLAGS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRCS_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

## Extract the tgz only if the source folder isn't there yet, then build it
## with its own build system.
$(MLX_LIB): | $(MLX_DIR)
	$(MLX_BUILD)

$(MLX_DIR):
	tar -xzf $(MLX_ARCHIVE)

## Fetch the SDL3 release source tarball only if it isn't there yet, extract
## it, then build a static libSDL3.a via SDL3's own CMake build. Only reached
## when GPU=1 pulls in $(SDL_LIB) as a prerequisite of $(NAME).
$(SDL_LIB): | $(SDL_SRC_DIR)
	cmake -S $(SDL_SRC_DIR) -B $(SDL_BUILD_DIR) -DCMAKE_BUILD_TYPE=Release \
		-DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_TEST_LIBRARY=OFF
	cmake --build $(SDL_BUILD_DIR) --parallel

$(SDL_SRC_DIR):
	curl -fL -o $(SDL_ARCHIVE) $(SDL_URL)
	tar -xzf $(SDL_ARCHIVE)

## Convenience aliases to build a specific mode regardless of the current
## MODE value (they just re-invoke make with MODE set).
dev debug:
	$(MAKE) MODE=debug

release:
	$(MAKE) MODE=release

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME)
	rm -rf $(MLX_DIR)
	rm -rf $(SDL_SRC_DIR) $(SDL_ARCHIVE)

re: fclean all

.PHONY: all dev debug release clean fclean re
