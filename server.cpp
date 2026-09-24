#include "server.hpp"

void server::init_server()
{
	_sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sfd == -1) 
		throw server_error();
	int opt = 1;
	if (setsockopt(_sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw server_error();
	if (fcntl(_sfd, F_SETFL, O_NONBLOCK) == -1)
		throw server_error();
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(_sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		throw server_error();
	if (listen(_sfd, SOMAXCONN) == -1)
		throw server_error();
}

void server::run_server()
{
	std::vector<struct pollfd> fds;

	fds.push_back(pollfd());
	fds.back().fd = _sfd;
	fds.back().events = POLLIN;
	fds.back().revents = 0;
	while (true)
	{
		poll(&fds.front(), fds.size(), -1);
		if (fds.front().revents == POLLIN)
		{
			fds.push_back(pollfd());
			struct sockaddr_in tmp;
			socklen_t tmp_len = sizeof(tmp);
			int fd = accept(_sfd, (struct sockaddr *)&tmp, &tmp_len);
			fds.back().fd = fd;
			client
			_clients[fd] = 
		}
	}
}