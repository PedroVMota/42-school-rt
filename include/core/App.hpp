#ifndef APP_HPP
#define APP_HPP

#include "mlx_wrapper.hpp"
#include "core/FrameBuffer.hpp"
#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include <memory>
#include <unordered_set>

class App
{
	public:
		App(int width, int height, char *title, bool verbose = false, const char *sceneFile = nullptr);
		~App();

		void	run();

	private:
		/* MLX event hooks only accept free-function-like pointers, so these
		** trampolines forward to the App instance passed as `param`. */
		static int	onExpose(void *param);
		static int	onKeyDown(int keycode, void *param);
		static int	onKeyUp(int keycode, void *param);
		static int	onLoopTick(void *param);

		void	redraw();     /* blit the cached framebuffer, no ray tracing */
		void	rerender();   /* re-trace the scene, then blit */
		void	handleKeyDown(int keycode);
		void	handleKeyUp(int keycode);
		void	update();     /* per-tick: apply movement for currently held keys */

		void	*_mlx;
		void	*_win;
		int		_width;
		int		_height;
		bool	_verbose;

		/* Keys currently held down, driven by KeyPress/KeyRelease hooks and
		** polled every loop tick so movement is continuous while held,
		** instead of a single step per keypress. */
		std::unordered_set<int>		_pressedKeys;

		Scene						_scene;
		Camera						_camera;
		std::unique_ptr<FrameBuffer>	_framebuffer;
};

#endif
