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
			// client
			// _clients[fd] = 
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
	std::cout << "[DEBUG] JOIN target: " << args[0] << std::endl;
	// TODO IRC: Channel creation/join logic, RPL_JOIN, RPL_TOPIC, RPL_NAMREPLY
}

void server::executeNick(client& c, std::vector<std::string> args)
{
	if (args.empty()) {
		// TODO IRC: Send ERR_NONICKNAMEGIVEN (431)
		return;
	}
	c.setNick(args[0]);
	std::cout << "[DEBUG] NICK set to: " << args[0] << std::endl;
	// TODO IRC: Check duplication (ERR_NICKNAMEINUSE 433)
}

void server::executePrivmsg(client& c, std::vector<std::string> args)
{
	if (args.size() < 2) {
		// TODO IRC: Send ERR_NEEDMOREPARAMS (461)
		return;
	}
	std::cout << "[DEBUG] PRIVMSG to " << args[0] << " msg: " << args[1] << std::endl;
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