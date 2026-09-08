#include "core/App.hpp"

int	main(void)
{
	char	title[] = "RT - Ray Tracer";
	App		app(1024, 768, title);

	app.run();
	return (0);
}