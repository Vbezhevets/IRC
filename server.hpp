#pragma once

#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <csignal>
#include <ctime>
#include "Irc.hpp"

#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "Client.hpp"

#define SERVERNAME "👾"
class Server {
private:
    int _listen_fd;
    int _port;
    std::string _pass;
    sockaddr_in _serv_addr;
    std::vector<pollfd> _pfds;
    std::map<int, Client> _clients; // key: fd

    void setEvents(int fd, short ev);
    Client& getClient(int fd);
    bool handleRead(int fd);
    bool handleWrite(int fd);
    void acceptNewClients(std::vector<pollfd>& toAdd);
    

public:

    typedef std::map<int, Client> ::iterator clIter; // :DDDDD
    Server(int port, const std::string& p);
        

    void sendToClient(Client& client, const std::string& line);
    void init();
    void run();
    void tick(std::vector<int>& toDrop);
    const std::string& getPassword();

    int setNick(Client& client, std::string nick  );
};