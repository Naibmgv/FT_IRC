/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nmagamad <nmagamad@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/26 17:55:01 by ehattab           #+#    #+#             */
/*   Updated: 2026/09/30 16:05:50 by nmagamad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

channel::channel() : _name(""), _topic(""), _password(""), _inviteOnly(false), _topicRestricted(true), _userLimit(0)
{}

channel::channel(std::string name): _name(name), _topic(""), _password(""), _inviteOnly(false), _topicRestricted(true), _userLimit(0)
{}

channel::channel(const channel& other)
{
	*this = other;
}

channel& channel::operator=(const channel& other)
{
	if (this != &other)
	{
		_name = other._name;
		_topic = other._topic;
		_password = other._password;
		_inviteOnly = other._inviteOnly;
		_topicRestricted = other._topicRestricted;
		_userLimit = other._userLimit;
		_clients = other._clients;
		_operators = other._operators;
		_invited = other._invited;
	}
	return *this;
}

channel::~channel()
{}

std::string channel::getName() const
{
	return _name;
}

std::string channel::getTopic() const
{
	return _topic;
}

std::string channel::getPassword() const
{
	return _password;
}

bool	channel::isInviteOnly() const
{
	return _inviteOnly;
}

bool	channel::isTopicRestricted() const
{
	return _topicRestricted;
}

size_t	channel::getUserLimit() const
{
	return _userLimit;
}

void channel::setTopic(std::string topic)
{
	_topic = topic;
}

void channel::setPassword(std::string password)
{
	_password = password;
}

void channel::setInviteOnly(bool i)
{
	_inviteOnly = i;
}

void channel::setTopicRestricted(bool t)
{
	_topicRestricted = t;
}

void channel::setUserLimit(size_t limit)
{
	_userLimit = limit;
}

void channel::addClient(client* c)
{
	if (!hasClient(c))
		_clients.push_back(c);
}

void channel::removeClient(client* c)
{
	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (_clients[i]->getFd() == c->getFd())
		{
			_clients.erase(_clients.begin() + i);
			break;
		}
	}
	removeOperator(c);
}

bool channel::hasClient(client* c)
{
	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (_clients[i]->getFd() == c->getFd())
			return true;
	}
	return false;
}

void channel::addOperator(client* c)
{
	if (!isOperator(c))
		_operators.push_back(c);
}


bool channel::isOperator(client* c)
{
	for (size_t i = 0; i < _operators.size(); i++)
	{
		if (_operators[i]->getFd() == c->getFd())
			return true;
	}
	return false;
}

void channel::removeOperator(client* c)
{
	for (size_t i = 0; i < _operators.size(); i++)
	{
		if (_operators[i]->getFd() == c->getFd())
		{
			_operators.erase(_operators.begin() + i);
			break;
		}
	}
}

void channel::broadcast(std::string message, client* sender)
{
	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (sender == NULL || _clients[i]->getFd() != sender->getFd())
			_clients[i]->appendWrite(message + "\r\n");
	}
}

bool channel::isEmpty() const
{
	return _clients.empty();
}

void	channel::setName(const std::string& name)
{
	_name = name;
}


void channel::addInvited(client* c)
{
	if (!isInvited(c))
	{
		_invited.push_back(c);
	}
}

bool channel::isInvited(client* c)
{
	for (size_t i = 0; i < _invited.size(); i++)
	{
		if (_invited[i]->getFd() == c->getFd())
		{
			return true;
		}
	}
	return false;
}

void channel::removeInvited(client* c)
{
	for (size_t i = 0; i < _invited.size(); i++)
	{
		if (_invited[i]->getFd() == c->getFd())
		{
			_invited.erase(_invited.begin() + i);
			break;
		}
	}
}

size_t channel::getUserAmount() const
{
	return _clients.size();
}

std::string channel::getAllUsers() const
{
	if (_clients.empty()) return "";
	std::string nameslist;
	for(std::size_t i = 0; i < _clients.size(); i++)
	{
		bool is_op = false;
		for (size_t j = 0; j < _operators.size(); j++) 
		{
			if (_operators[j]->getFd() == _clients[i]->getFd()) 
			{
				is_op = true;
				break;
			}
		}
		if (is_op)
			nameslist += "@";
		nameslist += _clients[i]->getNick() + " ";
	}
	if (!nameslist.empty())
		nameslist.erase(nameslist.size() - 1);
	return nameslist;
}

void channel::setcurrentModes(std::string data)
{
	if (_currentModes.empty())
		_currentModes += "+";
	_currentModes += data;
}

void channel::removecurrentModes(std::string data)
{
	if (_currentModes.empty())
		return ;
	for (std::size_t i = 0; i < _currentModes.size(); i++)
	{
		if (data.find(_currentModes[i]))
			_currentModes.erase(_currentModes[i]);
	}
}

std::string channel::getcurrentModes()
{
	if (_currentModes.empty())
		_currentModes += "+";
	return _currentModes;
}
