#include "Server.hpp"
#include "Irc.hpp"

extern volatile sig_atomic_t g_running;

Server::Server(int port, const std::string& p)
    : _listen_fd(-1), _port(port), _pass(p) {
}

void Server::init() {
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

    if (listen(_listen_fd, 100) < 0)
        throw std::runtime_error("listen error");

    int fl = fcntl(_listen_fd, F_GETFL, 0);
    if (fl == -1)
        throw std::runtime_error("fcntl F_GETFL listen");
    if (fcntl(_listen_fd, F_SETFL, fl | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl F_SETFL O_NONBLOCK listen");

    pollfd pfd; 
    pfd.fd = _listen_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pfds.push_back(pfd);
    IRC::initHandlers();
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

        _clients.insert(std::make_pair(client_fd, Client(client_fd)));

        pollfd pfd;
        pfd.fd = client_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        toAdd.push_back(pfd);
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
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        throw std::runtime_error("invalid fd client");
    return it->second;
}

bool Server::handleRead(int fd) {
    Client& client = getClient(fd);
    char buff[1024];

    while (true) {
        ssize_t n = recv(fd, buff, sizeof(buff), 0);
        if (n > 0) {
            _clients[fd].appendInBuff(buff, static_cast<std::size_t>(n));
            std::string msg;

            while (IRC::extractOneMessage(client.getInBuff(), msg)) { // we clean bufer inside if needed (full mess recieved)
                std::string resp = IRC::handleMessage(*this, client, msg);
                if (!resp.empty())
                    client.addToOutBuff(resp);
            }

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

void Server::run() {
    while (g_running) {
        if (_pfds.empty())
            break;

        std::vector<pollfd> toAdd;
        std::vector<int> toDrop;
        int r = poll(&_pfds[0], _pfds.size(), 200);
        if (r < 0) {
            if (errno == EINTR) //прерван repeat
                continue;
            throw std::runtime_error("poll error");
        }
        // if (r == 0) {
        //     tick(toDrop); //checking existing for hanging
        //     continue;
        // }



        for (std::size_t i = 0; i < _pfds.size(); ++i) {
            if (_pfds[i].revents == 0)
                continue;

            if (_pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) { // disconnected
                toDrop.push_back(_pfds[i].fd);
                continue;
            }

            if (_pfds[i].fd == _listen_fd && (_pfds[i].revents & POLLIN)) { // new
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

        if (!toDrop.empty()) {
            for (std::vector<int>::iterator it = toDrop.begin(); it != toDrop.end(); ++it) {
                close(*it);
                _clients.erase(*it);

                for (std::vector<pollfd>::iterator pit = _pfds.begin(); pit != _pfds.end(); ) {
                    if (pit->fd == *it)
                        pit = _pfds.erase(pit);
                    else
                        ++pit;
                }
            }
        }

        if (!toAdd.empty()) {
            _pfds.insert(_pfds.end(), toAdd.begin(), toAdd.end());
        }
    }
}
// void Server::tick(std::vector<int>& toDrop) {
//     time_t now = time(NULL);
//     // if no activity toDrop.push_back(fd);
 
// }

/*
	•	POLLERR → ошибка на дескрипторе.
	•	POLLHUP → разрыв соединения (закрыт другой стороной).
	•	POLLNVAL → неверный дескриптор (обычно баг в программе).
        EINTR
⸻
*/