#include "core/App.hpp"

int	main(void)
{
	char	title[] = "minilibx-cpp";
	App		app(800, 600, title);

	app.run();
	return (0);
}