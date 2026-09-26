#include "server.hpp"

void server::init_server()
{
	_sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (_sfd == -1) 
		throw servinit_error();
	int opt = 1;
	if (setsockopt(_sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw servinit_error();
	if (fcntl(_sfd, F_SETFL, O_NONBLOCK) == -1)
		throw servinit_error();
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(_sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		throw servinit_error();
	if (listen(_sfd, SOMAXCONN) == -1)
		throw servinit_error();
}

void server::run_server()
{
	std::vector<struct pollfd> pollfds;

	pollfds.push_back(pollfd());
	pollfds.back().fd = _sfd;
	pollfds.back().events = POLLIN;
	pollfds.back().revents = 0;
	while (true)
	{
		for(std::size_t i = 1; i < pollfds.size(); i++) // ckeck si on a un message a envoyer
		{
			if (_clients[pollfds[i].fd]._msgToSend.empty())
				pollfds[i].events = POLLIN;
			else
				pollfds[i].events = POLLIN | POLLOUT;
		}
		if (poll(&pollfds.front(), pollfds.size(), -1) == -1)
			throw std::runtime_error("Erreur critique de poll()");
		if (pollfds.front().revents == POLLIN) // Ajout de nouveau client
		{
			struct sockaddr_in tmp;
			socklen_t tmp_len = sizeof(tmp);
			int fd_tmp = accept(_sfd, (struct sockaddr *)&tmp, &tmp_len);
			if (fd_tmp == -1)
				continue;
			pollfds.push_back(pollfd());
			pollfds.back().fd = fd_tmp;
			_clients[fd_tmp] = client(fd_tmp, inet_ntoa(tmp.sin_addr));
		}
		for (std::size_t i = 0; i < pollfds.size(); i++) 
		{
			if (pollfds[i].revents & POLLIN) // Reception message client
			{
				char tmp_buff[1024];
				int rval = recv(pollfds[i].fd, tmp_buff, 1024, 0);
				if (rval == -1 || rval == 0)
				{
					// *** Prevenir DEV B de faire les procedure de depart d'un client (quitter les channels etc..) avant de supprimer les variables*** //
					close(pollfds[i].fd);
					_clients.erase(pollfds[i].fd);
					pollfds.erase(pollfds.begin() + i);
					i--;
					continue;
				}
				_clients[pollfds[i].fd]._buffer.append(tmp_buff, rval);
				if (_clients[pollfds[i].fd]._buffer.size() > 5000) // Systeme antiDDos
				{
					// *** Prevenir DEV B de faire les procedure de depart d'un client (quitter les channels etc..) avant de supprimer les variables*** //
					close(pollfds[i].fd);
					_clients.erase(pollfds[i].fd);
					pollfds.erase(pollfds.begin() + i);
					i--;
					continue;
				}
				std::string::size_type pos = _clients[pollfds[i].fd]._buffer.find("\r\n");
				if (pos != std::string::npos)
				{
					std::string command = _clients[pollfds[i].fd]._buffer.substr();
					// *** Appeler la fonction de DEV B pour gerer les commandes *** //
				}
			}
			if (pollfds[i].revents & POLLOUT)
			{
				if (_clients[pollfds[i].fd]._msgToSend.empty()) continue;
				if (send(pollfds[i].fd, _clients[pollfds[i].fd]._msgToSend.c_str(), sizeof(_clients[pollfds[i].fd]._msgToSend.c_str()), 0) == -1) // Systeme antiDDos
				{
					// *** Prevenir DEV B de faire les procedure de depart d'un client (quitter les channels etc..) avant de supprimer les variables*** //
					close(pollfds[i].fd);
					_clients.erase(pollfds[i].fd);
					pollfds.erase(pollfds.begin() + i);
					i--;
					continue;
				}
				_clients[pollfds[i].fd]._msgToSend.clear();
			}
		}
	}
}

std::vector<std::string> server::splitCommand(std::string str)
{
	std::vector<std::string> args;
	size_t i = 0;

	while (i < str.length())
	{
		while (i < str.length() && str[i] == ' ')
			i++;
		
		if (i >= str.length())
			break;

		if (str[i] == ':')
		{
			args.push_back(str.substr(i + 1));
			break;
		}

		size_t start = i;
		while (i < str.length() && str[i] != ' ')
			i++;

		args.push_back(str.substr(start, i - start));
	}
	
	return args;
}

void server::sendReply(client& c, std::string code, std::string message)
{
	std::string nick;

	if (c.getNick().empty())
		nick = "*";
	else
		nick = c.getNick();
	std::string reply = ":localhost " + code + " " + nick + " :" + message + "\r\n";
	c.appendWrite(reply);
}

void server::executeJoin(client& c, std::vector<std::string> args)
{
	if (args.empty())
	{
		// TODO IRC: Send ERR_NEEDMOREPARAMS (461)
		return;
	}

	// TODO IRC: Channel creation/join logic, RPL_JOIN, RPL_TOPIC, RPL_NAMREPLY
}

void server::executeNick(client& c, std::vector<std::string> args)
{
	if (args.empty()) {
		// TODO IRC: Send ERR_NONICKNAMEGIVEN (431)
		return;
	}

	// TODO IRC: Check duplication (ERR_NICKNAMEINUSE 433)
}

void server::executeUser(client& c, std::vector<std::string> args)
{
	//TODO
}

void server::executePrivmsg(client& c, std::vector<std::string> args)
{
	if (args.size() < 2) {
		// TODO IRC: Send ERR_NEEDMOREPARAMS (461)
		return;
	}
	// TODO IRC: Find target, append message to target's writeBuffer
}

void server::executePass(client& c, std::vector<std::string> args)
{
	if (args.empty())
	{
		sendReply(c, "461", "PASS :Not enough parameters");
		return;
	}
	if (c.getState() != UNREGISTERED)
	{
		sendReply(c, "462", "You may not reregister");
		return;
	}
	if (args[0] == _password)
		c.setState(PASS_OK);
	else
		sendReply(c, "464", "Password incorrect");
}

void server::checkRegistration(client &c)
{
	if (c.getState() == REGISTERED)
		return;
	if (c.getState() == PASS_OK && !c.getNick().empty() && !c.getUser().empty())
	{
		c.setState(REGISTERED);
		sendReply(c, "001", "Welcome to the 42 IRC Network " + c.getNick());
	}
}

void server::parseAndExecute(client &c, std::string full_command)
{
	std::vector<std::string> args = splitCommand(full_command);
	if (args.empty())
	return;
	std::string command_name = args[0];
	args.erase(args.begin());
	if (c.getState() != REGISTERED)
	{
		if (command_name == "PASS") executePass(c, args);
		else if (command_name == "NICK") executeNick(c, args);
		else if (command_name == "USER") executeUser(c, args);
		else sendReply(c, "451", "You have not registered");
		return;
	}

	if (command_name == "JOIN")
		executeJoin(c, args);
	else if (command_name == "PRIVMSG")
		executePrivmsg(c, args);
	else if (command_name == "NICK")
		executeNick(c, args);
	else
		sendReply(c, "421", command_name + " :Unknown command");
}
