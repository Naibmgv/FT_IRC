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
    if (!it->second.hasClient(c)) {
        sendReply(c, "442", "ERR_NOTONCHANNEL");
        return ;
    }
    if (!it->second.isOperator(c)) {
        sendReply(c, "482", "ERR_CHANOPRIVSNEEDED")
        return;
    }
    std::map<int, client>::iterator cl_it = _clients.begin();
    while (cl_it != _clients.end())
    {
        if (cl_it->second.getNick() == args[2])
        {
            if (!it->second.hasClient(cl_it.second()))
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
    it->second.broadcast(fullmsg, c);
    it->second.removeClient(cl_it->second);
    if (it->second.isOperator(cl_it->second))
        it->second.removeOperator(cl_it->second);
    if (_channels[args[1]].isEmpty())
        _channels.erase(args[1]);
}

void server::executeMode(client& c, std::vector<std::string> args)
{
    std::string command = args.size() >= 1 ? args[0] : "";
    std::string channel = args.size() >= 2 ? args[1] : ""; 
    std::string modes = args.size() >= 3 ? args[2] : "";
    if (args.size() > 3)
    {
        for(std::size_t i = 3; i < args.size(); i++)
    }
    if (args.size() == 1) {
        sendReply(c, "461", "ERR_NEEDMOREPARAMS");
        return ;
    }
    if (args.size() == 2 && !args[1].empty())
    {
        channels_iterator it = _channels.find(args[1]);
        if (it == _channels.end())
        {
            sendReply(c, "403", "ERR_NOSUCHCHANNEL");
            return;
        }
        sendReply(c, "324", args[1] + " " + it->second.getcurrentModes());
        return ;
    }
    channels_iterator it = _channels.find(args[1]);
    if (it == _channels.end()) {
        sendReply(c, "403", "ERR_NOSUCHCHANNEL");
        return;
    }
    if (args.size() == 3 && !args[2].empty())
    {
        for (int i = 0; i < args[2].size(); i++)
        {

        }
    }



    if (!it->second.hasClient(c)) {
        sendReply(c, "442", "ERR_NOTONCHANNEL");
        return ;
    }
    if (!it->second.isOperator(c)) {
        sendReply(c, "482", "ERR_CHANOPRIVSNEEDED")
        return;
    }
    
}
