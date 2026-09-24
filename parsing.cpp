#include "server.hpp"

bool parsing(char *port, char *mdp)
{
	if (!*port || !*mdp)
	{
		std::cerr << "Error : empty argument(s)" << std::endl;
		return true;
	}
	char *end;
	long tmp = strtol(port, &end, 10);
	if (end || tmp < 1024 || tmp > 65535)
	{
		std::cerr << "Error : 'Incorrect argument(s) syntaxe'" << std::endl;
		return true;
	}
	return false;
}