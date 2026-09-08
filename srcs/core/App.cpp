#include "core/App.hpp"
#include "core/Renderer.hpp"
#include "core/Keys.hpp"
#include <cstdlib>
#include <cmath>

App::App(int width, int height, char *title)
	: _mlx(nullptr), _win(nullptr), _width(width), _height(height),
	  _camera(Vec3(0, 1.5f, 8.0f), Vec3(0, 0, -1), 60.0f, width, height)
{
	_mlx = mlx_init();
	_win = mlx_new_window(_mlx, _width, _height, title);

	_scene = Scene::buildDefault(_width, _height, _camera);
	_framebuffer = std::make_unique<FrameBuffer>(_mlx, _width, _height);

	rerender();

	mlx_expose_hook(_win, (int (*)())&App::onExpose, this);
	mlx_key_hook(_win, (int (*)())&App::onKeyPress, this);
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

int	App::onKeyPress(int keycode, void *param)
{
	static_cast<App *>(param)->handleKey(keycode);
	return 0;
}

void	App::redraw()
{
	mlx_put_image_to_window(_mlx, _win, _framebuffer->handle(), 0, 0);
}

void	App::rerender()
{
	Renderer::render(_scene, _camera, *_framebuffer);
	redraw();
}

void	App::handleKey(int keycode)
{
	const float	moveSpeed = 0.3f;
	const float	turnSpeed = 0.06f;
	Vec3		pos = _camera.position();
	Vec3		fwd = _camera.forward();
	Vec3		right = _camera.right();
	bool		moved = true;

	if (keycode == KEY_ESC)
	{
		if (_win)
			mlx_destroy_window(_mlx, _win);
		_win = nullptr;
		std::exit(0);
	}
	else if (keycode == KEY_W)
		_camera.setPosition(pos + fwd * moveSpeed);
	else if (keycode == KEY_S)
		_camera.setPosition(pos - fwd * moveSpeed);
	else if (keycode == KEY_A)
		_camera.setPosition(pos - right * moveSpeed);
	else if (keycode == KEY_D)
		_camera.setPosition(pos + right * moveSpeed);
	else if (keycode == KEY_LEFT)
		_camera.setDirection(fwd * std::cos(turnSpeed) - right * std::sin(turnSpeed));
	else if (keycode == KEY_RIGHT)
		_camera.setDirection(fwd * std::cos(turnSpeed) + right * std::sin(turnSpeed));
	else if (keycode == KEY_UP)
		_camera.setDirection(fwd + Vec3(0, 1, 0) * turnSpeed);
	else if (keycode == KEY_DOWN)
		_camera.setDirection(fwd - Vec3(0, 1, 0) * turnSpeed);
	else
		moved = false;

	/* Camera changed => the cached framebuffer is stale, so this is the one
	** path that pays for a full re-trace; a plain expose event never does. */
	if (moved)
		rerender();
}
