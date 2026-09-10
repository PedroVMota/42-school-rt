#ifndef KEYS_HPP
#define KEYS_HPP

#ifdef GPU_COMPUTING_COMPATIBILITY

/*
** SDL3 reports scancodes (physical key position, layout-independent) the
** same way on every platform, so unlike the MLX branch below there's no
** per-OS split needed here.
*/
# include "sdl_wrapper.hpp"

# define KEY_ESC	SDL_SCANCODE_ESCAPE
# define KEY_W		SDL_SCANCODE_W
# define KEY_S		SDL_SCANCODE_S
# define KEY_A		SDL_SCANCODE_A
# define KEY_D		SDL_SCANCODE_D
# define KEY_LEFT	SDL_SCANCODE_LEFT
# define KEY_RIGHT	SDL_SCANCODE_RIGHT
# define KEY_UP		SDL_SCANCODE_UP
# define KEY_DOWN	SDL_SCANCODE_DOWN

#else

/*
** MiniLibX reports raw platform keycodes, which differ between the X11
** (Linux) and Cocoa (macOS) backends the Makefile builds against. Centralize
** the mapping here instead of scattering #ifdef __APPLE__ through App.cpp.
*/
#if defined(__APPLE__)
	# define KEY_ESC	53
	# define KEY_W		13
	# define KEY_S		1
	# define KEY_A		0
	# define KEY_D		2
	# define KEY_LEFT	123
	# define KEY_RIGHT	124
	# define KEY_UP		126
	# define KEY_DOWN	125
#else
	# define KEY_ESC	0xff1b
	# define KEY_W		0x77
	# define KEY_S		0x73
	# define KEY_A		0x61
	# define KEY_D		0x64
	# define KEY_LEFT	0xff51
	# define KEY_RIGHT	0xff53
	# define KEY_UP		0xff52
	# define KEY_DOWN	0xff54
#endif

/*
** X11 protocol event codes / masks (Xproto.h). Both minilibx backends use
** these same numeric values internally, so they're safe to hardcode here
** instead of pulling in platform headers (Cocoa has no X11 headers at all).
** The macOS backend ignores the mask argument to mlx_hook, so only the
** event code matters there.
*/
# define MLX_KEYPRESS		2
# define MLX_KEYRELEASE		3
# define MLX_KEYPRESS_MASK		(1L << 0)
# define MLX_KEYRELEASE_MASK	(1L << 1)

#endif /* GPU_COMPUTING_COMPATIBILITY */

#endif
