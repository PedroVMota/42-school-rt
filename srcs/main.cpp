#include "core/App.hpp"
#include <cstring>
#include <cstdio>

int	main(int argc, char **argv)
{
	char		title[] = "RT - Ray Tracer";
	bool		verbose = false;
	const char	*sceneFile = nullptr;

	for (int i = 1; i < argc; ++i)
	{
		if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0)
			verbose = true;
		else if (argv[i][0] == '-')
		{
			std::fprintf(stderr, "rt: unknown option '%s'\n", argv[i]);
			return (1);
		}
		else if (sceneFile)
		{
			std::fprintf(stderr, "rt: only one scene file may be given\n");
			return (1);
		}
		else
			sceneFile = argv[i];
	}

	App		app(1024, 768, title, verbose, sceneFile);

	app.run();
	return (0);
}
