#include "server.hpp"
#include <unistd.h>

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
	std::cout << " Server listen() port:" << _port << std::endl;
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
		for (std::size_t i = 1; i < pollfds.size(); i++) 
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
				while (true)
				{
					std::string::size_type pos = _clients[pollfds[i].fd].getreadBuf().find("\r\n");
					if (pos != std::string::npos)
					{
						std::string command = _clients[pollfds[i].fd].getreadBuf().substr(0, pos);
						std::cout << "commande recu : " << command << std::endl;
						parseAndExecute(_clients[pollfds[i].fd], command);
						_clients[pollfds[i].fd].resetreadbuf(pos + 2);
					}
					else
						break ;
				}
			}
			if (pollfds[i].revents & POLLOUT)
			{
				if (_clients[pollfds[i].fd].getWritebuf().empty()) continue;
				std::cout << "commande envoye : " <<  _clients[pollfds[i].fd].getWritebuf().c_str() << std::endl;
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
	if (args.size() < 2 || !args[1][0])
	{
		sendReply(c, "461", "ERR_NEEDMOREPARAMS");
		return;
	}
	if (_channels.find(args[1]) == _channels.end())
	{
		channel tmp;
		tmp.setName(args[1]);
		tmp.addOperator(&c);
		_channels[args[1]] = tmp;
	}
	if (_channels[args[1]].isInviteOnly())
	{
		if (!_channels[args[1]].isInvited(&c))
		{
			sendReply(c, "473", args[1] + " :ERR_INVITEONLYCHAN");
			return;
		}
		_channels[args[1]].removeInvited(&c);
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
	std::string joinMsg = ":" + c.getNick() + "!" + c.getUser() + "@" + c.getIp() + " JOIN :" + args[1];
	_channels[args[1]].broadcast(joinMsg, NULL);
	if (_channels[args[1]].getTopic().empty())
		c.appendWrite(":localhost 331 " + c.getNick() + " " + args[1] + " :No topic is set\r\n");
	else
		c.appendWrite(":localhost 332 " + c.getNick() + " " + args[1] + " :" + _channels[args[1]].getTopic() + "\r\n");
	
	c.appendWrite(":localhost 353 " + c.getNick() + " " + args[1] + " :" + _channels[args[1]].getAllUsers() + "\r\n");
	c.appendWrite(":localhost 366 " + c.getNick() + " " + args[1] + " :End of /NAMES list\r\n");
}

void server::executePrivmsg(client& c, std::vector<std::string> args)
{
	if (args.size() < 2 || (args.size() == 2 && args[1].empty()))
	{
		sendReply(c, "411", "ERR_NORECIPIENT");
		return;
	}
	if (args.size() < 3 || (args.size() == 3 && args[2].empty()))
	{
		sendReply(c, "412", "ERR_NOTEXTTOSEND");
		return ;
	}
	std::string msg;
	for(std::size_t i = 2; i < args.size(); i++)
		msg += args[i] + ' ';
	msg.erase(msg.size() - 1);
	if (msg[0] == ':')
		msg.erase(0, 1);
	std::string fullmsg = ":" + c.getNick() + "!" + c.getUser() + "@" + c.getIp() + " PRIVMSG " + args[1] + " :" + msg;
	if (args[1][0] == '#')
	{
		channels_iterator it = _channels.find(args[1]);
		if (it == _channels.end())
		{
			sendReply(c, "403", args[1] + " ERR_NOSUCHCHANNEL");
			return;
		}
		it->second.broadcast(fullmsg, &c);
	}
	else
	{
		std::map<int, client>::iterator it = _clients.begin();
		while (it != _clients.end())
		{
			if (it->second.getNick() == args[1])
			{
				it->second.appendWrite(fullmsg + "\r\n");
				break ;
			}
			++it;
		}
		if (it == _clients.end())
		{
			sendReply(c, "401", args[1] + " ERR_NOSUCHNICK");
			return ;
		}
	}
}

void server::executePass(client& c, std::vector<std::string> args)
{
	if (args.size() < 2)
	{
		sendReply(c, "461", "PASS :ERR_NEEDMOREPARAMS");
		return;
	}
	if (c.getState() != UNREGISTERED)
	{
		sendReply(c, "462", "ERR_ALREADYREGISTRED");
		return;
	}
	if (args[1] == _password)
		c.setState(PASS_OK);
	else
		sendReply(c, "464", "ERR_PASSWDMISMATCH");
}

void server::executeNick(client& c, std::vector<std::string> args)
{
	if (args.size() < 2 || args[1].empty())
	{
		sendReply(c, "431", "ERR_NONICKNAMEGIVEN");
		return;
	}
	std::string new_nick = args[1];
	std::map<int, client>::iterator it = _clients.begin();
	
	while (it != _clients.end())
	{
		if (it->second.getNick() == new_nick && it->first != c.getFd())
		{
			sendReply(c, "433", new_nick + " :ERR_NICKNAMEINUSE");
			return;
		}
		++it;
	}
	c.setNick(new_nick);
	checkRegistration(c);
}


void server::executeUser(client& c, std::vector<std::string> args)
{
	if (args.size() < 5)
	{
		sendReply(c, "461", "ERR_NEEDMOREPARAMS");
		return;
	}
	if (c.getState() == REGISTERED || !c.getUser().empty())
	{
		sendReply(c, "462", "ERR_ALREADYREGISTRED");
		return;
	}
	c.setUser(args[1]);
	checkRegistration(c);
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

void server::executeTopic(client& c, std::vector<std::string> args)
{
	if (args.size() < 2)
	{
		sendReply(c, "461", "ERR_NEEDMOREPARAMS");
		return;
	}
	std::map<std::string, channel>::iterator it = _channels.find(args[1]);
	if (it == _channels.end())
	{
		sendReply(c, "403", "ERR_NOSUCHCHANNEL");
		return;
	}
	if (!it->second.hasClient(&c))
	{
		sendReply(c, "442", "ERR_NOTONCHANNEL");
		return;
	}
	if (args.size() > 2)
	{
		if (it->second.isTopicRestricted()&& !it->second.isOperator(&c))
		{
			sendReply(c, "482", "ERR_CHANOPRIVSNEEDED");
			return;
		}
		it->second.setTopic(args[2]);
		it->second.broadcast(":" + c.getNick() + "!" + c.getUser() + "@" + c.getIp() + " TOPIC "
		 + it->second.getName() + " :" + it->second.getTopic(), NULL);
	}
	else
	{
		if (it->second.getTopic().empty())
			sendReply(c, "331", args[1] + " : NOTOPIC");
		else
			sendReply(c, "332", args[1] + " :" + it->second.getTopic());
	}
}

void server::executeInvite(client& c, std::vector<std::string> args)
{
	if (args.size() < 3)
	{
		sendReply(c, "461", "ERR_NEEDMOREPARAMS");
		return;
	}

	std::string target_nick = args[1];
	std::string chan_name = args[2];
	channels_iterator chan_it = _channels.find(chan_name);
	if (chan_it == _channels.end())
	{
		sendReply(c, "403", chan_name + "ERR_NOSUCHCHANNEL");
		return;
	}
	if (!chan_it->second.hasClient(&c))
	{
		sendReply(c, "442", chan_name + "ERR_NOTONCHANNEL");
		return;
	}
	if (chan_it->second.isInviteOnly() && !chan_it->second.isOperator(&c))
	{
		sendReply(c, "482", chan_name + "ERR_CHANOPRIVSNEEDED");
		return;
	}
	client* target_client = NULL;
	std::map<int, client>::iterator cli_it = _clients.begin();
	while (cli_it != _clients.end())
	{
		if (cli_it->second.getNick() == target_nick)
		{
			target_client = &(cli_it->second);
			break;
		}
		++cli_it;
	}
	if (target_client == NULL)
	{
		sendReply(c, "401", target_nick + "ERR_NOSUCHNICK");
		return;
	}
	if (chan_it->second.hasClient(target_client))
	{
		sendReply(c, "443", target_nick + " " + chan_name + "ERR_USERONCHANNEL");
		return;
	}
	chan_it->second.addInvited(target_client);
	sendReply(c, "341", target_nick + " " + chan_name);
	target_client->appendWrite(":" + c.getNick() + "!" + c.getUser() + "@" + c.getIp()
	+ " INVITE " + target_nick + " :"+ chan_name + "\r\n");
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

void server::parseAndExecute(client &c, std::string full_command)
{
	std::vector<std::string> args = splitCommand(full_command);
	if (args.empty())
		return;
	std::string command_name = args[0];
	if (c.getState() != REGISTERED)
	{
		if (command_name == "CAP") return;
		if (command_name == "PASS")
			executePass(c, args);
		else if (command_name == "NICK")
			executeNick(c, args);
		else if (command_name == "USER")
			executeUser(c, args);
		else
			sendReply(c, "451", "You have not registered");
		return;
	}
	if (command_name == "WHO")
	{
		// if (args.size() >= 2)
		// 	c.appendWrite(":localhost 315 " + c.getNick() + " " + args[1] + " :End of /WHO list\r\n");
		return;
	}
	else if (command_name == "CAP" || command_name == "VERSION" || command_name == "MOTD" || command_name == "LUSERS")
		return ;
	else if (command_name == "PING")
		executePONG(c, args);
	else if (command_name == "JOIN")
		executeJoin(c, args);
	else if (command_name == "PRIVMSG")
		executePrivmsg(c, args);
	else if (command_name == "MODE")
		executeMode(c, args);
	else if (command_name == "NICK")
		executeNick(c, args);
	else if (command_name == "TOPIC")
		executeTopic(c, args);
	else if (command_name == "INVITE")
		executeInvite(c, args);
	else if (command_name == "KICK")
		executeKick(c, args);
	else
		sendReply(c, "421", command_name + " :Unknown command");
}
