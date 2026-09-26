#include "server.hpp"

int main(int ac, char **av)
{
	if (ac != 3) {
		std::cerr << "Error : too much or missing arguments" << std::endl;
		return 1;
	}
	if (parsing(av[1], av[2]))
		return 1;
	server test(atoi(av[1]), av[2]);

	try
	{
		test.init_server();
		test.run_server();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
}