#include "core/App.hpp"
#include "core/Renderer.hpp"
#include "core/SceneParser.hpp"
#include "core/Keys.hpp"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <chrono>
#include <stdexcept>

#ifdef GPU_COMPUTING_COMPATIBILITY

#include "core/SDLUtils.hpp"

App::App(int width, int height, char *title, bool verbose, const char *sceneFile)
	: _win(nullptr), _renderer(nullptr), _gpuDevice(nullptr), _width(width), _height(height), _verbose(verbose),
	  _camera(Vec3(0, 1.5f, 8.0f), Vec3(0, 0, -1), 60.0f, width, height)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *win = SDL_CreateWindow(title, _width, _height, 0);
	_win = win;
	_renderer = SDL_CreateRenderer(win, nullptr);

	/* Device creation/teardown round-trips (see Phase 3). The compute
	** pipeline and its buffers (see Phase 6/7) are built lazily by
	** Renderer::render on first use, and reused across frames from there. */
	_gpuDevice = SDL_UTILS::createDevice(win);

	try
	{
		if (sceneFile)
			_scene = SceneParser::parseFile(sceneFile, _width, _height, _camera);
		else
			_scene = Scene::buildDefault(_width, _height, _camera);
	}
	catch (const std::exception &e)
	{
		std::fprintf(stderr, "rt: %s\n", e.what());
		SDL_UTILS::destroyDevice(static_cast<SDL_GPUDevice *>(_gpuDevice), win);
		if (_renderer)
			SDL_DestroyRenderer(static_cast<SDL_Renderer *>(_renderer));
		if (_win)
			SDL_DestroyWindow(win);
		SDL_Quit();
		std::exit(1);
	}
	_framebuffer = std::make_unique<FrameBuffer>(_renderer, _width, _height);

	rerender();
}

App::~App()
{
	SDL_UTILS::destroyDevice(static_cast<SDL_GPUDevice *>(_gpuDevice), static_cast<SDL_Window *>(_win));
	if (_renderer)
		SDL_DestroyRenderer(static_cast<SDL_Renderer *>(_renderer));
	if (_win)
		SDL_DestroyWindow(static_cast<SDL_Window *>(_win));
	SDL_Quit();
}

void	App::run()
{
	bool		running = true;
	SDL_Event	event;

	SDL_UTILS::resetDeltaTime();
	while (running)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
				running = false;
			else if (event.type == SDL_EVENT_KEY_DOWN)
				handleKeyDown((int)event.key.scancode);
			else if (event.type == SDL_EVENT_KEY_UP)
				handleKeyUp((int)event.key.scancode);
		}
		if (!running)
			break;
		update();
		redraw();
	}
}

void	App::redraw()
{
	SDL_Renderer	*renderer = static_cast<SDL_Renderer *>(_renderer);

	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, static_cast<SDL_Texture *>(_framebuffer->handle()), nullptr, nullptr);
	SDL_RenderPresent(renderer);
}

#else

App::App(int width, int height, char *title, bool verbose, const char *sceneFile)
	: _mlx(nullptr), _win(nullptr), _width(width), _height(height), _verbose(verbose),
	  _camera(Vec3(0, 1.5f, 8.0f), Vec3(0, 0, -1), 60.0f, width, height)
{
	_mlx = mlx_init();
	_win = mlx_new_window(_mlx, _width, _height, title);

	try
	{
		if (sceneFile)
			_scene = SceneParser::parseFile(sceneFile, _width, _height, _camera);
		else
			_scene = Scene::buildDefault(_width, _height, _camera);
	}
	catch (const std::exception &e)
	{
		std::fprintf(stderr, "rt: %s\n", e.what());
		if (_win)
			mlx_destroy_window(_mlx, _win);
		std::exit(1);
	}
	_framebuffer = std::make_unique<FrameBuffer>(_mlx, _width, _height);

	rerender();

	mlx_expose_hook(_win, (int (*)())&App::onExpose, this);

	/* mlx_key_hook fires on key *release* on both minilibx backends, which
	** only ever gives a single step per press. Track press/release state
	** ourselves and poll it once per loop tick so movement is continuous
	** for as long as a key is held down. */
	mlx_hook(_win, MLX_KEYPRESS, MLX_KEYPRESS_MASK, (int (*)())&App::onKeyDown, this);
	mlx_hook(_win, MLX_KEYRELEASE, MLX_KEYRELEASE_MASK, (int (*)())&App::onKeyUp, this);
	mlx_loop_hook(_mlx, (int (*)())&App::onLoopTick, this);
}

App::~App()
{
	if (_win)
		mlx_destroy_window(_mlx, _win);
}

void	App::run()
{
	mlx_loop(_mlx);
}

int	App::onExpose(void *param)
{
	static_cast<App *>(param)->redraw();
	return 0;
}

int	App::onKeyDown(int keycode, void *param)
{
	static_cast<App *>(param)->handleKeyDown(keycode);
	return 0;
}

int	App::onKeyUp(int keycode, void *param)
{
	static_cast<App *>(param)->handleKeyUp(keycode);
	return 0;
}

int	App::onLoopTick(void *param)
{
	static_cast<App *>(param)->update();
	return 0;
}

void	App::redraw()
{
	mlx_put_image_to_window(_mlx, _win, _framebuffer->handle(), 0, 0);
}

#endif /* GPU_COMPUTING_COMPATIBILITY */

void	App::rerender()
{
#ifdef GPU_COMPUTING_COMPATIBILITY
	auto	doRender = [this]() {
		Renderer::render(_scene, _camera, *_framebuffer, static_cast<SDL_GPUDevice *>(_gpuDevice));
	};
#else
	auto	doRender = [this]() {
		Renderer::render(_scene, _camera, *_framebuffer);
	};
#endif

	if (_verbose)
	{
		auto start = std::chrono::steady_clock::now();
		doRender();
		auto end = std::chrono::steady_clock::now();
		double ms = std::chrono::duration<double, std::milli>(end - start).count();
		std::fprintf(stderr, "[verbose] render took %.2f ms\n", ms);
	}
	else
		doRender();
	_framebuffer->present();
	redraw();
}

void	App::handleKeyDown(int keycode)
{
	if (keycode == KEY_ESC)
	{
#ifdef GPU_COMPUTING_COMPATIBILITY
		if (_renderer)
			SDL_DestroyRenderer(static_cast<SDL_Renderer *>(_renderer));
		if (_win)
			SDL_DestroyWindow(static_cast<SDL_Window *>(_win));
		SDL_Quit();
#else
		if (_win)
			mlx_destroy_window(_mlx, _win);
#endif
		_win = nullptr;
		std::exit(0);
	}
	_pressedKeys.insert(keycode);
}

void	App::handleKeyUp(int keycode)
{
	_pressedKeys.erase(keycode);
}

void	App::update()
{
#ifdef GPU_COMPUTING_COMPATIBILITY
	const float	dt = SDL_UTILS::getDeltaTime();
#else
	const float	dt = 1.0f / 60.0f;
#endif
	const float	moveSpeed = 3.0f * dt;
	const float	turnSpeed = 1.5f * dt;
	Vec3		pos = _camera.position();
	Vec3		fwd = _camera.forward();
	Vec3		right = _camera.right();
	bool		moved = false;

	auto held = [this](int keycode) { return _pressedKeys.count(keycode) != 0; };

	if (held(KEY_W))
	{
		pos = pos + fwd * moveSpeed;
		moved = true;
	}
	if (held(KEY_S))
	{
		pos = pos - fwd * moveSpeed;
		moved = true;
	}
	if (held(KEY_A))
	{
		pos = pos - right * moveSpeed;
		moved = true;
	}
	if (held(KEY_D))
	{
		pos = pos + right * moveSpeed;
		moved = true;
	}
	if (moved)
		_camera.setPosition(pos);

	bool turned = false;
	Vec3 newFwd = fwd;

	if (held(KEY_LEFT))
	{
		newFwd = newFwd * std::cos(turnSpeed) - right * std::sin(turnSpeed);
		turned = true;
	}
	if (held(KEY_RIGHT))
	{
		newFwd = newFwd * std::cos(turnSpeed) + right * std::sin(turnSpeed);
		turned = true;
	}
	if (held(KEY_UP))
	{
		newFwd = newFwd + Vec3(0, 1, 0) * turnSpeed;
		turned = true;
	}
	if (held(KEY_DOWN))
	{
		newFwd = newFwd - Vec3(0, 1, 0) * turnSpeed;
		turned = true;
	}
	if (turned)
		_camera.setDirection(newFwd);

	/* Camera changed => the cached framebuffer is stale, so this is the one
	** path that pays for a full re-trace; a plain expose event never does. */
	if (moved || turned)
		rerender();
}
