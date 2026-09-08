#ifndef APP_HPP
#define APP_HPP

#include "mlx_wrapper.hpp"
#include "core/FrameBuffer.hpp"
#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include <memory>

class App
{
	public:
		App(int width, int height, char *title);
		~App();

		void	run();

	private:
		/* MLX event hooks only accept free-function-like pointers, so these
		** trampolines forward to the App instance passed as `param`. */
		static int	onExpose(void *param);
		static int	onKeyPress(int keycode, void *param);

		void	redraw();     /* blit the cached framebuffer, no ray tracing */
		void	rerender();   /* re-trace the scene, then blit */
		void	handleKey(int keycode);

		void	*_mlx;
		void	*_win;
		int		_width;
		int		_height;

		Scene						_scene;
		Camera						_camera;
		std::unique_ptr<FrameBuffer>	_framebuffer;
};

#endif
