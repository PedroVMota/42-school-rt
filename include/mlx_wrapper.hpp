#ifndef MLX_WRAPPER_HPP
#define MLX_WRAPPER_HPP

/*
** mlx.h only has C declarations (no extern "C" guard inside it),
** but the compiled library (libmlx.a) has C-style symbol names
** (on macOS some of its own .m files are Objective-C, which also
** uses C linkage for these functions).
** If we #include "mlx.h" directly in a .cpp file without this,
** the C++ compiler mangles the symbol names it expects to link
** against, and you get "undefined reference to `mlx_init'" style
** errors at link time. Wrapping it in extern "C" fixes that.
*/
extern "C"
{
	#include "mlx.h"
}

#endif