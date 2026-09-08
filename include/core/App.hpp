#ifndef APP_HPP
#define APP_HPP

#include "mlx_wrapper.hpp"

class App
{
	public:
		App(int width, int height, char *title);
		~App();

		void	run();

	private:
		void	*_mlx;
		void	*_win;
		int		_width;
		int		_height;
};

#endif