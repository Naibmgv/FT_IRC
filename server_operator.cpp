#include "server.hpp"

void server::executeKick(client& c, std::vector<std::string> args)
{
	if (args.size() == 1) {
		sendReply(c, "461", "ERR_NEEDMOREPARAMS");
		return ;
	}
	channels_iterator it = _channels.find(args[1]);
	if (it == _channels.end()) {
		sendReply(c, "403", "ERR_NOSUCHCHANNEL");
		return;
	}
	if (!it->second.hasClient(&c)) {
		sendReply(c, "442", "ERR_NOTONCHANNEL");
		return ;
	}
	if (!it->second.isOperator(&c)) {
		sendReply(c, "482", "ERR_CHANOPRIVSNEEDED");
		return;
	}
	std::map<int, client>::iterator cl_it = _clients.begin();
	while (cl_it != _clients.end())
	{
		if (cl_it->second.getNick() == args[2])
		{
			if (!it->second.hasClient(&cl_it->second))
			{
				sendReply(c, "441", "ERR_USERNOTINCHANNEL");
				return;
			}
			break;
		}
	}
	std::string fullmsg = ":" + c.getNick() + "!" + c.getUser() + "@localhost KICK " + args[1] + args[2];
	if (args.size() > 3 && !args[3].empty())
	{
		fullmsg += " :";
		for (std::size_t i = 3; i < args.size(); i++)
			fullmsg += args[i] + " ";
	}
	fullmsg.erase(fullmsg.size() - 1);
	it->second.broadcast(fullmsg, &c);
	it->second.removeClient(&cl_it->second);
	if (it->second.isOperator(&cl_it->second))
		it->second.removeOperator(&cl_it->second);
	if (_channels[args[1]].isEmpty())
		_channels.erase(args[1]);
}

void server::executeMode(client& c, std::vector<std::string> args)
{
	std::string command = args.size() >= 1 ? args[0] : "";
	std::string channel = args.size() >= 2 ? args[1] : ""; 
	std::string modes = args.size() >= 3 ? args[2] : "";
	std::vector<std::string> params;
	if (args.size() > 3)
	{
		for(std::size_t i = 3; i < args.size(); i++)
			params.push_back(args[i]);
	}
	if (args.size() == 1) 
	{
		sendReply(c, "461", "ERR_NEEDMOREPARAMS");
		return ;
	}
	if (args.size() == 2)
	{
		channels_iterator it = _channels.find(channel);
		if (it == _channels.end())
		{
			sendReply(c, "403", "ERR_NOSUCHCHANNEL");
			return;
		}
		sendReply(c, "324", channel + " " + it->second.getcurrentModes());
		return ;
	}
	channels_iterator it = _channels.find(channel);
	if (it == _channels.end()) {
		sendReply(c, "403", "ERR_NOSUCHCHANNEL");
		return;
	}
	if (!it->second.hasClient(&c)) {
		sendReply(c, "442", "ERR_NOTONCHANNEL");
		return ;
	}
	if (!it->second.isOperator(&c)) {
		sendReply(c, "482", "ERR_CHANOPRIVSNEEDED");
		return;
	}
	bool toAdd = false;
	std::vector<std::string>::iterator it_params = params.begin();
	for (std::size_t i = 0; i < modes.size(); i++)
	{
		if (modes[i] == '+') toAdd = true;
		else if (modes[i] == '-') toAdd = false;
		else if (toAdd == true)
		{
			if (modes[i] == 'i') it->second.setInviteOnly(true);
			else if (modes[i] == 't') it->second.setTopicRestricted(true);
			else if (modes[i] == 'k')
			{
				if (it_params == params.end())
					continue;
				it->second.setPassword(*it_params++);
			}
			else if (modes[i] == 'o')
			{
				if (it_params == params.end())
					continue;
				std::map<int, client>::iterator it_client = _clients.begin();
				while (it_client != _clients.end())
				{
					if (it_client->second.getNick() == *it_params)
						break ;
					++it;
				}
				++it_params;
				if (it_client == _clients.end())
				{
					sendReply(c, "442", "ERR_USERNOTONCHANNEL");
					continue;
				}
				it->second.addOperator(&it_client->second);
			}
			else if (modes[i] == 'l')
			{
				if (it_params == params.end())
					continue;
				char *end;
				std::string tmp = *it_params++; 
				size_t val = std::strtol(tmp.c_str(), &end, 10);
				if (end || val <= 0) continue;
				it->second.setUserLimit(val);
			}
		}
		else if (toAdd == false)
		{
			if (modes[i] == 'i') _channels[channel].setInviteOnly(false);
			else if (modes[i] == 't') it->second.setTopicRestricted(false);
			else if (modes[i] == 'k') it->second.setPassword("");
			else if (modes[i] == 'o')
			{
				if (it_params == params.end())
					continue;
				std::map<int, client>::iterator it_client = _clients.begin();
				while (it_client != _clients.end())
				{
					if (it_client->second.getNick() == *it_params)
						break ;
					++it;
				}
				++it_params;
				if (it_client == _clients.end())
				{
					sendReply(c, "442", "ERR_USERNOTONCHANNEL");
					continue;
				}
				it->second.removeOperator(&it_client->second);
			}
			else if (modes[i] == 'l') it->second.setUserLimit(0);
		}
	}
}
