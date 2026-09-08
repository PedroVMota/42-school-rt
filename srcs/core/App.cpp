#include "core/App.hpp"
#include "core/Renderer.hpp"
#include "core/SceneParser.hpp"
#include "core/Keys.hpp"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <chrono>
#include <stdexcept>

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

void	App::rerender()
{
	if (_verbose)
	{
		auto start = std::chrono::steady_clock::now();
		Renderer::render(_scene, _camera, *_framebuffer);
		auto end = std::chrono::steady_clock::now();
		double ms = std::chrono::duration<double, std::milli>(end - start).count();
		std::fprintf(stderr, "[verbose] render took %.2f ms\n", ms);
	}
	else
		Renderer::render(_scene, _camera, *_framebuffer);
	redraw();
}

void	App::handleKeyDown(int keycode)
{
	if (keycode == KEY_ESC)
	{
		if (_win)
			mlx_destroy_window(_mlx, _win);
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
	const float	moveSpeed = 0.3f;
	const float	turnSpeed = 0.06f;
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
