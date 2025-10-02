#pragma once

#include <string>
#include <set>
#include "../client/Client.hpp"

enum ChannelModes {
    MODE_INVITE_ONLY,
    MODE_TOPIC_OPERATOR_ONLY,
    MODE_USER_LIMIT,
    MODE_KEY_PROTECTED,
};

class Channel
{
public:
    Channel();
    Channel(std::string &name, Client *op);
    Channel(std::string &name, Client *op, std::string &key);
    ~Channel();

    Channel(const Channel &);
    Channel &operator=(const Channel &);

    void    addClient(const Client *client);
    void    removeClient(const Client *client);
    void    addOperator(const Client *client);
    void    removeOperator(const Client *client);

    const std::string   &getKey(void) const;
    void                setKey(const std::string &key);
    const std::string   &getName(void) const;
    void                setName(const std::string &name);

    std::string         getDisplayName(void) const;

    const std::set<const Client *>  &getClients(void) const;
    const std::set<const Client *>  &getOperators(void) const;

    bool    isOperator(const Client *op) const;

    void    setMode(int mode);
    void    unsetMode(int mode);
    int     getMode(void) const;
    bool    hasMode(int mode) const;

    void    setLimit(size_t limit);
    size_t  getLimit(void) const;

    bool    isFull(void) const;
    bool    isKey(const std::string &key) const;

    bool    hasClient(const Client *client) const;

private:
    std::string                 _name;
    std::string                 _key;
    std::set<const Client *>    _clients;
    std::set<const Client *>    _operators;

    size_t          _limit;
    unsigned char   _mode;
};
