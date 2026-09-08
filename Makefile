## ============================================================================
##  Project configuration
## ============================================================================

NAME		= program

SRCS_DIR	= srcs
INC_DIR		= include
OBJ_DIR		= obj

CXX			= c++
CXXFLAGS	= -Wall -Wextra -std=c++17

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
MLX_LDFLAGS	= -L$(MLX_DIR) -lmlx $(SYS_LDFLAGS) -lm

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
	$(CXX) $(OBJS) $(MLX_LDFLAGS) -o $(NAME)

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

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)
	rm -rf $(MLX_DIR)

re: fclean all

.PHONY: all clean fclean re