#ifndef SDL_WRAPPER_HPP
#define SDL_WRAPPER_HPP

/*
** Only compiled when the build was configured with `make GPU=1`, which
** defines GPU_COMPUTING_COMPATIBILITY and links a self-built static SDL3
** instead of MiniLibX (see mlx_wrapper.hpp for the MLX side). Every class
** with an #ifdef GPU_COMPUTING_COMPATIBILITY branch includes this header
** instead of pulling <SDL3/SDL.h> directly, so the SDL3 dependency stays
** fully opt-in: a default `make` never sees this file's contents at all.
*/
#ifdef GPU_COMPUTING_COMPATIBILITY
	#include <SDL3/SDL.h>
#endif

#endif
