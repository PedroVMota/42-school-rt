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
##  MiniLibX: pick the right archive/folder/lib per OS.
##  Linux  -> minilibx-linux.tgz        (built via its own ./configure + make)
##  macOS  -> minilibx_macos_opengl.tgz (built via its own make, Objective-C)
##  These tgz files are expected next to this Makefile.
## ============================================================================

ifeq ($(UNAME_S),Linux)
	MLX_ARCHIVE	= minilibx-linux.tgz
	MLX_DIR		= minilibx-linux
	MLX_LIB		= $(MLX_DIR)/libmlx.a
	SYS_LDFLAGS	= -lXext -lX11 -lbsd
	MLX_BUILD	= (cd $(MLX_DIR) && ./configure && $(MAKE))
else ifeq ($(UNAME_S),Darwin)
	MLX_ARCHIVE	= minilibx_macos_opengl.tgz
	MLX_DIR		= minilibx_opengl_20191021
	MLX_LIB		= $(MLX_DIR)/libmlx.a
	SYS_LDFLAGS	= -framework OpenGL -framework AppKit -lz
	MLX_BUILD	= (cd $(MLX_DIR) && $(MAKE))
	CXXFLAGS	+= -Wno-deprecated-declarations
else
	$(error Unsupported OS: $(UNAME_S) - only Linux and Darwin are handled)
endif

## NOTE: order matters to the linker - libmlx.a must come BEFORE the system
## libraries it depends on (X11/Xext on Linux), otherwise their symbols get
## dropped before libmlx.a asks for them and you get "undefined reference".
MLX_LDFLAGS	= -L$(MLX_DIR) -lmlx $(SYS_LDFLAGS) -lm -pthread

## ============================================================================
##  Project sources / includes (recursive)
## ============================================================================

SRCS		:= $(shell find $(SRCS_DIR) -name '*.cpp')
OBJS		:= $(patsubst $(SRCS_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS		:= $(OBJS:.o=.d)

INC_DIRS	:= $(shell find $(INC_DIR) -type d) $(MLX_DIR)
CXXFLAGS	+= $(addprefix -I,$(INC_DIRS))

## ============================================================================
##  Rules
## ============================================================================

all: $(NAME)

$(NAME): $(MLX_LIB) $(OBJS)
	$(CXX) $(MODE_LDFLAGS) $(OBJS) $(MLX_LDFLAGS) -o $(NAME)

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

re: fclean all

.PHONY: all dev debug release clean fclean re