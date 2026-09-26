
#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <map>
#include <exception>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sys/socket.h> // -> socket() etc
#include <sys/types.h> 	// 
#include <fcntl.h> 		// -> fcntl()
#include <unistd.h> 	// -> close()
#include <arpa/inet.h> 	// -> inet_ntoa()
#include <poll.h> 		// -> poll()
#include <csignal>		// -> signal
#include <netinet/in.h> // -> sockaddr_in (struct speciale ipv4).

#define RED "\e[1;31m" // -> red
#define WHI "\e[0;37m" // -> white
#define GRE "\e[1;32m" // -> green
#define YEL "\e[1;33m" // -> yellow

class client;

class server
{
	private:
		int _sfd;
		int _port;
		std::string _password;
		std::map<int, client> _clients;
		std::map<std::string, channel> _channels;
		std::vector<std::string> splitCommand(std::string str);
		void executePass(client& c, std::vector<std::string> args);
		void executeUser(client& c, std::vector<std::string> args);
		void executeJoin(client& c, std::vector<std::string> args);
		void executeNick(client& c, std::vector<std::string> args);
		void executePrivmsg(client& c, std::vector<std::string> args);
		void sendReply(client& c, std::string code, std::string message);
		void checkRegistration(client& c);
	public:
		server(int port, const std::string& password) : _port(port), _password(password) {};
		server(const server& other) : _sfd(other._sfd) {};
		server& operator=(const server& other) 
		{
			if (this != &other) {
				_sfd = other._sfd;
			}
			return *this;
		};
		~server() {};

		class servinit_error : public std::exception {
			public :
				const char* what() const throw() {
					return "Server error: initiation failed";
				}
		};
	
		void init_server();
		void run_server();

		void parseAndExecute(client& c, std::string full_command);
};

enum State {
	UNREGISTERED,
	PASS_OK,
	REGISTERED
};

class client
{
	private:
		int			_fd;
		std::string	_ip;
		State		_state;
		std::string _nick;
		std::string _user;
		std::string _readBuf;
		std::string _writeBuf;
	public:
		client() : _fd(-1), _ip("") {};
		client(int fd, const std::string& ip);
		client(const client& other);
		client& operator=(const client& other);
		~client();
		int		getFd() const;
		State	getState() const;
		std::string	getNick() const;
		std::string	getUser() const;
		void	setState(State s);
		void	setNick(std::string nick);
		void	setUser(std::string user);
		void	appendRead(std::string data);
		void	appendWrite(std::string data);
		bool	hasCommand() const;
		std::string	getCommand();
		std::string	getWriteBuf() const;
		void	clearWriteBuf();
};


class channel
{
	private:
		std::string	_name;
		std::string	_topic;
		std::string	_password;
		bool	_inviteOnly;
		bool	_topicRestricted;
		size_t	_userLimit;
		std::vector<client*>	_clients;
		std::vector<client*>	_operators;
		std::vector<client*>	_invited;
	public:
		channel();
		channel(std::string name);
		channel(const channel& other);
		channel& operator=(const channel& other);
		~channel();
		std::string	getName() const;
		std::string	getTopic() const;
		std::string	getPassword() const;
		bool	isInviteOnly() const;
		bool	isTopicRestricted() const;
		size_t	getUserLimit() const;
		void	setTopic(std::string topic);
		void	setPassword(std::string password);
		void	setInviteOnly(bool i);
		void	setTopicRestricted(bool t);
		void	setUserLimit(size_t limit);
		void	addClient(client* c);
		void	removeClient(client* c);
		bool	hasClient(client* c);
		void	addOperator(client* c);
		void	removeOperator(client* c);
		bool	isOperator(client* c);
		void	broadcast(std::string message, client* sender);
};

#endif