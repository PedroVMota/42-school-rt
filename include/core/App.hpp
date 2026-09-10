#ifndef APP_HPP
#define APP_HPP

#ifdef GPU_COMPUTING_COMPATIBILITY
	#include "sdl_wrapper.hpp"
#else
	#include "mlx_wrapper.hpp"
#endif
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
#ifndef GPU_COMPUTING_COMPATIBILITY
		/* MLX event hooks only accept free-function-like pointers, so these
		** trampolines forward to the App instance passed as `param`. The
		** SDL backend polls events directly in run() instead, so it has no
		** need for these. */
		static int	onExpose(void *param);
		static int	onKeyDown(int keycode, void *param);
		static int	onKeyUp(int keycode, void *param);
		static int	onLoopTick(void *param);
#endif

		void	redraw();     /* blit the cached framebuffer, no ray tracing */
		void	rerender();   /* re-trace the scene, then blit */
		void	handleKeyDown(int keycode);
		void	handleKeyUp(int keycode);
		void	update();     /* per-tick: apply movement for currently held keys */

#ifdef GPU_COMPUTING_COMPATIBILITY
		void	*_win;        /* SDL_Window* */
		void	*_renderer;   /* SDL_Renderer* */
		void	*_gpuDevice;  /* SDL_GPUDevice*, see core/SDLUtils.hpp */
#else
		void	*_mlx;
		void	*_win;
#endif
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
