#include "Channel.hpp"
#include <string>

Channel::Channel() {}

Channel::Channel(std::string &name, Client *op) :
    _name(name),
    _key(""),
    _clients(std::set<const Client *>()),
    _operators(std::set<const Client *>())
{
    _clients.insert(op);
    _operators.insert(op);
}

Channel::Channel(std::string &name, Client *op, std::string &key) :
    _name(name),
    _key(key),
    _clients(std::set<const Client *>()),
    _operators(std::set<const Client *>())
{
    _clients.insert(op);
    _operators.insert(op);
}

Channel::~Channel() {}

Channel::Channel(const Channel& other) {
    if (this != &other) {
        *this = other;
    }
}

Channel &Channel::operator=(const Channel& other) {
    if (this != &other) {
        this->_clients = other._clients;
        this->_name = other._name;
        this->_operators = other._operators;
    }
    return (*this);
}

void Channel::addClient(const Client *client) {
    _clients.insert(client);
}

void Channel::removeClient(const Client *client) {
    _clients.erase(client);
}

void Channel::addOperator(const Client *op) {
    _operators.insert(op);
}

void Channel::removeOperator(const Client *op) {
    _operators.erase(op);
}

const std::set<const Client *> &Channel::getClients( void ) const {
    return _clients;
}

const std::set<const Client *> &Channel::getOperators( void ) const {
    return _operators;
}

const std::string &Channel::getName(void) const {
    return _name;
}

std::string Channel::getDisplayName(void) const {
    return std::string("#") + _name;
}

void Channel::setName(const std::string &name) {
    _name = name;
}

const std::string &Channel::getKey(void) const {
    return _key;
}

void Channel::setKey(const std::string &key) {
    _key = key;
}

bool Channel::isOperator(const Client *op) const {
    return _operators.find(op) != _operators.end();
}

int     Channel::getMode(void) const {
    return _mode;
}

void Channel::unsetMode(int mode) {
    _mode &= ~(1 << mode);
}

void Channel::setMode(int mode) {
    _mode |= (1 << mode);
}

bool Channel::hasMode(int mode) const {
    return _mode & (1 << mode);
}

void Channel::setLimit(size_t limit) {
    _limit = limit;
}

size_t Channel::getLimit(void) const {
    return _limit;
}

bool Channel::isFull(void) const {
    return hasMode(MODE_USER_LIMIT) && _clients.size() == _limit;
}

bool Channel::isKey(const std::string &key) const {
    return _key == key;
}

bool Channel::hasClient(const Client *client) const {
    return _clients.count(client) > 0;
}
