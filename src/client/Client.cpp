#include "Client.hpp"
#include "../protocol/Irc.hpp"

Client::Client() {}

Client::Client(int fd, std::string host) : _fd(fd), _nick("*"), _user("*"), _host(host), _passOk(false),  _hasNick(false), _hasUser(false),
  _isRegistered(false), _isWelcomed(false),  _last_active(std::time(NULL)), _awaitingPong(false) {}

Client::~Client() {}

void Client::tryMakeRegistered() {
    if (_passOk && _hasNick && _hasUser && !_isRegistered) {
        _isRegistered = true;
    }
}

void Client::appendInBuff(const char* data, std::size_t n) {
    _in_buff.append(data, n);
}

std::string& Client::getInBuff() {
    return _in_buff;
}

std::string& Client::getOutBuff() {
    return _out_buff_for_client;
}

void Client::addToOutBuff(const std::string& s) {
    std::string toAdd = s;
    if (s.size() > MAX_MESSAGE_LEN + 2) {
        toAdd = s.substr(0, MAX_MESSAGE_LEN) + "\r\n";
    }
    _out_buff_for_client += toAdd;
}

bool Client::wantsWrite() const {
    return !_out_buff_for_client.empty();
}


std::time_t Client::lastActive() const {
    return _last_active;
}

void Client::updateActive() {
    _last_active = std::time(NULL);
}

bool Client::isAwaitingPong() const {
    return _awaitingPong;
}

void Client::setAwaitingPong(bool b) {
    _awaitingPong = b;
}

int Client::getFd() const {
    return _fd;
}

void Client::applyNick(const std::string& nick) {
    _nick = nick;
    _hasNick = true;
    tryMakeRegistered();
}

const std::string& Client::getNick() const {
    return _nick;
}

void Client::setUser(const std::string& user, const std::string& realname) {
    _user = user;
    _realname = realname;
    _hasUser = true;
    tryMakeRegistered();
}

const std::string& Client::getUserName() const {
    return _user;
}

const std::string& Client::getRealName() const {
    return _realname;
}

const std::string& Client::getHost() const {
    return _host;
}

void Client::passOk() {
    _passOk = true;
    tryMakeRegistered();
}

bool Client::isRegistered() const {
    return _isRegistered;
}

std::string Client::getMask() const{
    return  _nick + "!" + _user + "@" + _host;
}

bool Client::isWelcomed() const {
    return _isWelcomed;
}

void Client::setWelcomed()  {
     _isWelcomed = true;
}
