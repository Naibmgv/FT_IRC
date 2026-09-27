/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagamad <nmagamad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/26 17:55:05 by ehattab           #+#    #+#             */
/*   Updated: 2026/09/27 15:37:05 by nmagamad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

client::client(int fd, const std::string& ip) : _fd(fd), _ip(ip), _state(UNREGISTERED) {}

client::client(const client& other)
{
	*this = other;
}

client& client::operator=(const client& other)
{
	if (this != &other)
	{
		_fd = other._fd;
		_ip = other._ip;
		_state = other._state;
		_nick = other._nick;
		_user = other._user;
		_readBuf = other._readBuf;
		_writeBuf = other._writeBuf;
	}
	return *this;
}

client::~client()
{}

int client::getFd() const
{
	return _fd;
}
State client::getState() const
{
	return _state;
}

std::string client::getUser() const
{
	return _user;
}

std::string client::getNick() const
{
	return _nick;
}

void client::setState(State s)
{
	_state = s;
}
void client::setNick(std::string nick)
{
	_nick = nick;
}
void client::setUser(std::string user)
{
	_user = user;
}

void client::appendWrite(std::string data)
{
	_writeBuf += data;
}

std::string client::getreadBuf() const
{
	return _readBuf;
}

void client::appendreadBuf(std::string data, int rval)
{
	_readBuf.append(data, rval);
}

void client::resetreadbuf(std::string::size_type pos)
{
		_readBuf.erase(0, pos);
}

std::string client::getWritebuf() const
{
	return _writeBuf;
}

void	client::resetwritebuf(std::string::size_type pos)
{
	_writeBuf.erase(0, pos);
}

std::string client::getIp() const
{
	return _ip;
}
