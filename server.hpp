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
};

class client
{
	private:
		int _fd;
		std::string _ip;
	public:
		client(int fd, const std::string& ip) : _fd(fd),_ip(ip) {};
		client(const client& other) : _fd(other._fd), _ip(other._ip), _buffer(other._buffer) {};
		client& operator=(const client& other) 
		{
			if (this != &other)
			{
				_fd = other._fd;
				_ip = other._ip;
				_buffer = other._buffer;
			}
			return *this;
		};

		std::string _buffer;
		std::string _msgToSend;
};

bool parsing(char *port, char *mdp);

#endif