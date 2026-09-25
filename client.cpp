#include "server.hpp"

client::client(int fd, const std::string& ip) : _fd(fd), _ip(ip), _state(UNREGISTERED) {}

client::client(const client& other) {
    *this = other;
}

client& client::operator=(const client& other) {
    if (this != &other) {
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

client::~client() {}

int client::getFd() const { return _fd; }
State client::getState() const { return _state; }
std::string client::getNick() const { return _nick; }

void client::setState(State s) { _state = s; }
void client::setNick(std::string nick) { _nick = nick; }
void client::setUser(std::string user) { _user = user; }

void client::appendRead(std::string data) {
    _readBuf += data;
}

void client::appendWrite(std::string data) {
    _writeBuf += data;
}

bool client::hasCommand() const {
    size_t position = _readBuf.find("\r\n");
    if (position == std::string::npos) {
        return false;
    } else {
        return true;
    }
}

std::string client::getCommand() {
    size_t position = _readBuf.find("\r\n");
    if (position == std::string::npos) {
        return "";
    }
    std::string cmd = _readBuf.substr(0, position);
    _readBuf.erase(0, position + 2);
    return cmd;
}

std::string client::getWriteBuf() const {
    return _writeBuf;
}

void client::clearWriteBuf() {
    _writeBuf.clear();
}