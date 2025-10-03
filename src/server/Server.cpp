#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <cstddef>
#include <iostream>

#include "Server.hpp"
#include "../utils/utils.hpp"
#include "../channel/Channel.hpp"
#include "../client/Client.hpp"

extern volatile sig_atomic_t g_running;

Server::Server(int port, const std::string& p)
    : _listen_fd(-1), _port(port), _pass(p) {
}

pollfd newPfd(int fd) {
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    return pfd;
}

void Server::init() {

    LOG_INFO << "Starting Irc Server" << std::endl;

    _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_fd < 0)
        throw std::runtime_error("listening socket creation error");

    int yes = 1;
    if (setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        throw std::runtime_error("setsockopt SO_REUSEADDR error");

    std::memset(&_serv_addr, 0, sizeof(_serv_addr));
    _serv_addr.sin_family = AF_INET;
    _serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    _serv_addr.sin_port = htons(_port);

    if (bind(_listen_fd, reinterpret_cast<sockaddr*>(&_serv_addr), sizeof(_serv_addr)) < 0)
        throw std::runtime_error("bind error");

    if (listen(_listen_fd, CONNECTION_QUEUE_SIZE) < 0)
        throw std::runtime_error("listen error");

    LOG_DEBUG << "Listening on " << _serv_addr.sin_addr.s_addr << std::endl;

    int fl = fcntl(_listen_fd, F_GETFL, 0);
    if (fl == -1)
        throw std::runtime_error("fcntl F_GETFL listen");
    if (fcntl(_listen_fd, F_SETFL, fl | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl F_SETFL O_NONBLOCK listen");

    _pfds.push_back(newPfd(_listen_fd));
    IRC::initHandlers();
    IRC::initNumAnswers();
}

void Server::run() {
    std::vector<pollfd> toAdd;
    std::vector<int> toDrop;

    LOG_INFO << "Running..." << std::endl;
    while (g_running) {
        if (_pfds.empty())
            break;

        int r = poll(&_pfds[0], _pfds.size(), POLL_TIMEOUT);
        if (r < 0) {
            if (errno == EINTR) //прерван repeat
                continue;
            throw std::runtime_error("poll error");
        }

        for (std::size_t i = 0; i < _pfds.size(); ++i) {
            if (_pfds[i].revents == 0)
                continue;

            if (_pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) { // disconnected
                toDrop.push_back(_pfds[i].fd);
                continue;
            }

            if (_pfds[i].fd == _listen_fd && (_pfds[i].revents & POLLIN)) {
                acceptNewClients(toAdd);
                continue;
            }

            if (_pfds[i].revents & POLLIN) {
                if (!handleRead(_pfds[i].fd)) {
                    toDrop.push_back(_pfds[i].fd);
                    continue;
                }
            }

            if (_pfds[i].revents & POLLOUT) {
                if (!handleWrite(_pfds[i].fd)) {
                    toDrop.push_back(_pfds[i].fd);
                    continue;
                }
            }
        }
        tick(toDrop); //checking existing for hanging
        if (!toDrop.empty()) {

            for (std::vector<int>::iterator it = toDrop.begin(); it != toDrop.end(); ++it) {
                LOG_INFO << "Dropping client " << *it << std::endl;
                close(*it);
                _clients.erase(*it);

                for (std::vector<pollfd>::iterator pit = _pfds.begin(); pit != _pfds.end(); ) {
                    if (pit->fd == *it)
                        pit = _pfds.erase(pit);
                    else
                        ++pit; //
                }
            }
            toDrop.clear();
        }

        if (!toAdd.empty()) {
            for (std::vector<pollfd>::iterator it = toAdd.begin(); it != toAdd.end(); ++it) {
                LOG_INFO << "Adding Client " << it->fd << std::endl;
            }
            _pfds.insert(_pfds.end(), toAdd.begin(), toAdd.end());
            toAdd.clear();
        }
    }
}

void Server::acceptNewClients(std::vector<pollfd>& toAdd) {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    socklen_t len;

    while (true) {
        len = sizeof(addr);
        int client_fd = accept(_listen_fd, reinterpret_cast<sockaddr*>(&addr), &len);

        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) //  nothing (all from queue were accepted)
                break;
            if (errno == EINTR) //прерван repeat
                continue;
            throw std::runtime_error("accept error");
        }

        int cfl = fcntl(client_fd, F_GETFL, 0);
        if (cfl == -1 || fcntl(client_fd, F_SETFL, cfl | O_NONBLOCK) == -1) {
            close(client_fd);
            continue;
        }
        char ipbuf[INET_ADDRSTRLEN] = {0};
        // This didnt compile for me
        // if(inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf)) == nullptr)
        if(inet_ntop(AF_INET, &addr.sin_addr, ipbuf, sizeof(ipbuf)) == NULL)
            throw std::runtime_error("failed to get client's adress");
        std::string hostStr(ipbuf);
        _clients.insert(std::make_pair(client_fd, Client(client_fd, hostStr)));

        toAdd.push_back(newPfd(client_fd));
    }
}

void Server::setEvents(int fd, short ev) {
    for (std::size_t i = 0; i < _pfds.size(); ++i) {
        if (_pfds[i].fd == fd) {
            _pfds[i].events = ev;
            break;
        }
    }
}

Client& Server::getClient(int fd) {
    clIter it = _clients.find(fd);
    if (it == _clients.end())
        throw std::runtime_error("invalid fd client");
    return it->second;
}

bool Server::handleRead(int fd) {
    Client& client = getClient(fd);
    char buff[READ_BUF_SIZE];

    while (true) {
        ssize_t n = recv(fd, buff, sizeof(buff), 0);
        if (n > 0) {
            client.appendInBuff(buff, static_cast<std::size_t>(n));
            std::string msg;

            while (IRC::extractOneMessage(client.getInBuff(), msg))
                IRC::handleMessage(*this, client, msg);

            if (client.wantsWrite())
                setEvents(fd, POLLIN | POLLOUT);

            continue;
        }

        if (n == 0)
            return false; // peer closed

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return true;
            if (errno == EINTR)
                continue;
            return false;
        }
    }
}

bool Server::handleWrite(int fd) {
    Client& client = getClient(fd);
    std::string& outBuf = client.getOutBuff();

    while (!outBuf.empty()) {
        ssize_t n = send(fd, outBuf.c_str(), outBuf.size(), MSG_NOSIGNAL);
        if (n > 0) {
            outBuf.erase(0, static_cast<std::size_t>(n));
            continue;
        }
        if (n == 0)
            return true;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;
        if (errno == EINTR) //прерван repeat
            continue;
        return false;
    }

    setEvents(fd, POLLIN);
    return true;
}

void Server::tick(std::vector<int>& toDrop) {
    time_t now = time(NULL);
    for (clIter it = _clients.begin(); it != _clients.end(); ++it ){
        Client& client = it->second;

        if (!client.isAwaitingPong() && now - client.lastActive() > 120 ) {
            client.setAwaitingPong(true);
            client.addToOutBuff("PING :tick\r\n");
            setEvents(client.getFd(), POLLIN | POLLOUT);
        }
        else if (client.isAwaitingPong() && now - client.lastActive() > 150){
            toDrop.push_back(client.getFd());
        }
    }


}

const std::string& Server::getPassword() {
    return _pass;
}

void Server::sendToClient(Client& client, const std::string& line) {
    client.addToOutBuff(line);
    setEvents(client.getFd(), POLLIN | POLLOUT);
}

int Server::setNick(Client& client, std::string& nick){
    for (clIter it = _clients.begin(); it != _clients.end(); it++) {
        if (it->second.getFd() != client.getFd() && it->second.getNick() == nick)
            return ERR_NICKNAMEINUSE;
    }
    client.applyNick(nick);
    return 0;
}

Client* Server:: getClientByNick(const std::string& nick){
    for (clIter it = _clients.begin(); it != _clients.end(); it++)
        if (it->second.getNick() == nick)
            return &it->second;
    return NULL;
}

void Server:: tryRegister(Client& client) {
     client.tryMakeRegistered();
     if (client.isRegistered() && !client.isWelcomed()) {
         sendToClient(client, IRC::makeStringFromServ("001 " + client.getNick() + " :Welcome to the IRC server, " + client.getNick()));
         client.setWelcomed();
     }
};

Channel* Server::getChannelByName(std::string& name) {
    try {
        return &_channels.at(name);
    } catch (std::exception &e) {
        return NULL;
    }
}

void    Server::addChannel(Channel c) {
    _channels.insert(std::pair<std::string, Channel>(c.getName(), c));
}
