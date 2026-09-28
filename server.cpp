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
			if (_clients[pollfds[i].fd].getWritebuf().empty())
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
					removeClientGlobally(&_clients[i]);
					close(pollfds[i].fd);
					_clients.erase(pollfds[i].fd);
					pollfds.erase(pollfds.begin() + i);
					i--;
					continue;
				}
				_clients[pollfds[i].fd].appendreadBuf(tmp_buff, rval);
				if (_clients[pollfds[i].fd].getreadBuf().size() > 5000) // Systeme antiDDos
				{
					removeClientGlobally(&_clients[i]);
					close(pollfds[i].fd);
					_clients.erase(pollfds[i].fd);
					pollfds.erase(pollfds.begin() + i);
					i--;
					continue;
				}
				std::string::size_type pos = _clients[pollfds[i].fd].getreadBuf().find("\r\n");
				if (pos != std::string::npos)
				{
					std::string command = _clients[pollfds[i].fd].getreadBuf().substr(0, pos);
					parseAndExecute(_clients[pollfds[i].fd], command);
					_clients[pollfds[i].fd].resetreadbuf(pos + 2);
				}
			}
			if (pollfds[i].revents & POLLOUT)
			{
				if (_clients[pollfds[i].fd].getWritebuf().empty()) continue;
				int rval = send(pollfds[i].fd, _clients[pollfds[i].fd].getWritebuf().c_str(), _clients[pollfds[i].fd].getWritebuf().size(), 0);
				if (rval == -1)
				{
					removeClientGlobally(&_clients[i]);
					close(pollfds[i].fd);
					_clients.erase(pollfds[i].fd);
					pollfds.erase(pollfds.begin() + i);
					i--;
					continue;
				}
				_clients[pollfds[i].fd].resetwritebuf(rval);
			}
		}
	}
}

void server::sendReply(client& c, std::string code, std::string message)
{
	std::string nick;

	if (c.getNick().empty())
		nick = "*";
	else
		nick = c.getNick();
	std::string reply;
	if (code[0])
		reply = ":localhost " + code + " " + nick + " :" + message + "\r\n";
	else
		reply = ":localhost " + nick + " :" + message + "\r\n";
	c.appendWrite(reply);
}

void server::removeClientGlobally(client* c)
{
	if (_channels.empty()) return;
	std::map<std::string, channel>::iterator it = _channels.begin();
	while(it != _channels.end())
	{
		if (it->second.hasClient(c))
		{
			std::string quitMsg = ":" + c->getNick() + "!" + c->getUser() + "@" + c->getIp() + "QUIT :Client disconnected\r\n";
			it->second.broadcast(quitMsg, c);
			it->second.removeClient(c);
		}
		if (it->second.isEmpty())
			_channels.erase(it++);
		else
			++it;
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

void server::executeJoin(client& c, std::vector<std::string> args)
{
	if (args.empty() || !args[1][0])
	{
		sendReply(c, "461", "ERR_NEEDMOREPARAMS");
		return;
	}
	if (_channels.find(args[1]) == _channels.end())
	{
		channel tmp;
		tmp.setName(args[1]);
	}
	if (_channels[args[1]].getUserLimit() != 0)
	{
		if (_channels[args[1]].getUserAmount() >= _channels[args[1]].getUserLimit())
		{
			sendReply(c, "471", "ERR_CHANNELISFULL");
			return ;
		}
	}
	_channels[args[1]].addClient(&c);
	_channels[args[1]].broadcast(c.getUser() + "is joining the channnel", &c);
	if (_channels[args[1]].getTopic().empty())
		sendReply(c, "331", args[1] + " :No topic is set");
	else
		sendReply(c, "332", args[1] + _channels[args[1]].getTopic());
	sendReply (c, "", _channels[args[1]].getAllUsers());
}

void server::executePrivmsg(client& c, std::vector<std::string> args)
{
	if (args.size() == 1 || (args.size() == 2 && args[2].empty()))
	{
		sendReply(c, "411", "ERR_NORECIPIENT");
		return;
	}
	if (args.size() == 2 || (args.size() == 3 && args[2].empty()))
	{
		sendReply(c, "412", "ERR_NOTEXTTOSEND");
		return ;
	}
	std::string msg;
	for(int i = 2; i < args.size(); i++)
		msg += args[i] + ' ';
	msg.erase(msg.size() - 1);
	if (msg[0] == ':')
		msg.erase(msg.front());
	std::string fullmsg = ":" + c.getNick() + "!" + c.getUser() + "@" + c.getIp() + " PRIVMSG " + args[1] + " :" + msg + "\r\n";
	if (args[2][0] == '#')
	{
		channels_iterator it = _channels.find(args[2]);
		if (it == _channels.end())
		{
			sendReply(c, "403", "ERR_NOSUCHCHANNEL");
			return;
		}
		it->second.broadcast(msg, &c);
	}
	else
	{
		std::map<int, client>::iterator it = _clients.begin();
		while (it != _clients.end())
		{
			if (it->second.getNick() == args[1])
			{
				it->second.appendWrite(fullmsg);
				break ;
			}
			++it;
		}
		if (it == _clients.end())
		{
			sendReply(c, "401", "ERR_NOSUCHNICK");
			return ;
		}
	}
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

void server::executePONG(client& c, std::vector<std::string> args)
{
	if (args.size() == 1)
	{
		sendReply(c, "409", "ERR_NOORIGIN");
		return ;
	}
	c.appendWrite(":localhost PONG :" + args[1] + "\r\n");
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

	if (command_name == "WHO" || command_name == "VERSION" || command_name == "CAP LS" || command_name == "MOTD" || command_name == "LUSERS")
		return ;
	else if (command_name == "PING")
		executePONG(c, args);
	else if (command_name == "JOIN")
		executeJoin(c, args);
	else if (command_name == "PRIVMSG")
		executePrivmsg(c, args);
	else if (command_name == "NICK")
		executeNick(c, args);
	else if (commande_name == "MODE")
		executeMode(c, args);
	else
		sendReply(c, "421", command_name + " :Unknown command");
}
