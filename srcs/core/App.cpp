#include "App.hpp"

App::App(int width, int height, char *title)
	: _mlx(nullptr), _win(nullptr), _width(width), _height(height)
{
	_mlx = mlx_init();
	_win = mlx_new_window(_mlx, _width, _height, title);
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