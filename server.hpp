#pragma once
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <sys/poll.h>
#include <vector>
#include <map>
#include <cerrno>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include "Client.hpp"
#include "Irc.hpp"

extern volatile sig_atomic_t g_running; 
class Server {
        private:
            int _listen_fd; // fd socket for server listening - used only to wait for new connections
            int _port;
            std::string _pass; // when starting program 
            struct sockaddr_in _serv_addr; 
            std::vector<pollfd> _pfds; // struct pollfd - describes fdescirptor and events to track on it
                                                    //  int   fd;      dscriptor (like with files but fir socket)
                                                    //  short events;  - here we ser wich evetnts to track (POLLIN, POLLOUT, etc.)
                                                    //  short revents; // what events actually happened (filled by the core)
                                                    //};
            std::map <int, Client> _clients; // <fd, Client> // descriptor here is socket's descriptor
        public:
            Server(int port, std::string p) : _port(port), _pass(p)  { }

        void init(){

            if ((_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // AF_INET - IPv4
                throw std::runtime_error("listening socket creation error");
            int yes = 1;
            if ((setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) < 0) // SO_REUSEADDR  - adress can be reused just after dicsonect // yes - swith on options 
                throw std::runtime_error("listening socket creation error");
            
            memset(&_serv_addr, 0, sizeof(_serv_addr));
            _serv_addr.sin_family = AF_INET; // IPv4
            _serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //Bind socket to all available IP addresses of this computer
            _serv_addr.sin_port = htons(_port); // convert 
            if (bind(_listen_fd, (struct sockaddr *)&_serv_addr, sizeof(_serv_addr)) < 0)
                throw std::runtime_error("bind error");

            if (listen(_listen_fd, 100 ) < 0 ) // 100 clients in queue for accept
                throw std::runtime_error("listen error");
                
            int fl = fcntl(_listen_fd, F_GETFL, 0); // returns bit mask of socket 
            if (fl == -1) throw std::runtime_error("fcntl F_GETFL listen");
            if (fcntl(_listen_fd, F_SETFL, fl | O_NONBLOCK) == -1) // set new flags  - Non Block - for multilistening 
                throw std::runtime_error("fcntl F_SETFL O_NONBLOCK listen");

            pollfd pfd;
            pfd.fd = _listen_fd;
            pfd.events = POLLIN; //in event - whant to connect in this case           
            pfd.revents = 0; //no events yet
            _pfds.push_back(pfd);
        }
/*

In blocking mode O_NONBLOCK :
• recv() waits for data and hangs if there is none.
• send() may wait if the kernel buffer is full.
• In non-blocking mode:
• recv() will immediately return -1 and EAGAIN if there is no data.
• send() will return -1 and EAGAIN if it cannot send right now.

*/
        void acceptNewClients(std::vector<pollfd>& toAdd) {
            sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            socklen_t len;
            while (true){
                len =  sizeof(addr);
                int client_fd = accept(_listen_fd, (sockaddr *)&addr, &len);
              
                if (client_fd < 0){
                    if (errno == EAGAIN || errno == EWOULDBLOCK) // means empty queue 
                        break;
                    if (errno == EINTR)
                        continue;
                    throw std::runtime_error("accept error");
                }
                int cfl = fcntl(client_fd, F_GETFL, 0);
                if (cfl == -1 || fcntl(client_fd, F_SETFL, cfl | O_NONBLOCK) == -1) { //// socket in non-blocking mode so that it does not hang on accept/recv/send, but returns EAGAIN/EWOULDBLOCK
                   close(client_fd); continue; } 


                _clients.insert(std::make_pair(client_fd, Client(client_fd)));
                pollfd pfd;
                pfd.fd = client_fd;
                pfd.events = POLLIN;           
                pfd.revents = 0;
                
                toAdd.push_back(pfd); // adding for monitorring
            }
        }

         void setEvents(int fd, short ev) {
            for (unsigned long i = 0; i < _pfds.size(); i++){
                if (_pfds[i].fd == fd) {
                    _pfds[i].events = ev; 
                    break;
                }
            }
        }
        Client& getClient(int fd){
            std::map<int, Client>::iterator it = _clients.find(fd);
            if (it == _clients.end())
                throw std::runtime_error("invalid  fd client");
            return it->second;
        }
        bool handleRead (int fd){
            Client &client = getClient(fd);
            char buff[1024];
            while (true){ 
                ssize_t n = recv(fd, buff, sizeof(buff), 0);
                if (n > 0 ) {
                    _clients[fd].appendInBuff(buff, n);
                    std::string msg;
                    
                    while (IRC::extractOneMessage(client.getInBuff(), msg)) {  
                        std::string resp = IRC::handleMessage(fd , msg); // do smth 
                        if (!resp.empty())
                            client.addToOutBuff(resp); // response
                    }

                    if (client.wantsWrite())  // if ooutBuf not empty
                        setEvents(fd, POLLIN | POLLOUT); // adding POLLOUT

                    continue;
                } 
                if (n == 0)  // socket closed, drop
                    return false;
                
                if (n < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) // n < 0 and no more data
                        return true;
                    if (errno == EINTR) 
                        continue; // interrupted, retry
                    return false; // hard error
                }
            }
        }

        bool handleWrite(int fd) {
            Client &client = getClient(fd);
            
            std::string& outBuf = client.getOutBuff();
            while (!outBuf.empty()){
                ssize_t n = send(fd, outBuf.c_str(), outBuf.size(), MSG_NOSIGNAL);
                if(n > 0 )
                    outBuf.erase(0, n);
                if (n == 0) 
                    return true;
            
                else if (n < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) //  core buffer is full. Waiting for free // The kernel buffer is temporarily full: leave POLLOUT enabled so that poll wakes us up when it's possible to write again.
                        return true;
                    if (errno == EINTR) 
                        continue; // interrupted, retry
                    return false; // hard error
                }
            }

            setEvents(fd, POLLIN); //disable POLLOUT 
            return true;
        }

        void run () {
            while (g_running) { // the server runs the main loop until interrupted by a signal
                if (_pfds.empty()) // even _listen is absent
                    break;

                int r = (poll(&_pfds[0], _pfds.size(), 200)); //checking listening fd 200 ms
                if (r < 0 ) {
                    if (errno == EINTR) //SIGINT  
                        continue; 
                    else
                        throw std::runtime_error("poll error");
                }
                if (r == 0) {
                    // tick(); // included  for maintenance. I haven't gotten to that yet.
                    continue;
                }
//we use temporary vectors so as not to do everything on the fly and not to slow down the server's work with operations (poll can slow down. no need for clients to wait while new clients accepted)
//Draining backlog to EAGAIN → fewer syscalls, lower client latency, lower risk of queue overflow. EAGAIN = no data avaible right now, call later

                std:: vector <pollfd> toAdd; //temporary structure of descriptors to add up those who knocked during the iteration 
                std:: vector <int> toDrop;     // to correctly sequentially disconnect everyone who is no longer with us

                for (unsigned int i = 0 ;  i < _pfds.size(); i++) {
                    
                    if (_pfds[i].revents == 0) // no events on this socket
                        continue; // go to next

                    if (_pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL ) ){ //errors, 
                        toDrop.push_back(_pfds[i].fd); continue; }

                    if (_pfds[i].fd == _listen_fd && (_pfds[i].revents & POLLIN)) { 
                        acceptNewClients(toAdd); continue; } // accpeting and adding everyone who joined _listen to connect
                    
                    if (_pfds[i].revents & POLLIN) { 
                        if (!handleRead(_pfds[i].fd)) 
                            toDrop.push_back(_pfds[i].fd); continue;   //if it couldn't be read, we add it to the trash and move on to the next client
                         }
                    
                    if (_pfds[i].revents & POLLOUT) {
                        if (!handleWrite(_pfds[i].fd)) {
                            toDrop.push_back(_pfds[i].fd); continue;  
                        } 
                    }

                }

                if (!toDrop.empty()) {
                    for (std::vector <int> ::iterator it = toDrop.begin(); it != toDrop.end(); it++){
                        close(*it);
                        _clients.erase(*it);
                        
                        for(std::vector <pollfd> :: iterator pollit = _pfds.begin(); pollit != _pfds.end();) {
                            if (pollit->fd == *it )
                                pollit = _pfds.erase(pollit);
                            else 
                                ++pollit;
                        }
                    }
                    toDrop.clear();
                }
                if (!toAdd.empty()) {
                        _pfds.insert(_pfds.end(), toAdd.begin(), toAdd.end());
                        toAdd.clear();
                }

            }
        }

    };
 


    /* 
    •	POLLIN → читаем
    •	POLLOUT → пишем
    •	POLLHUP → клиент ушёл
    •	POLLERR / POLLNVAL → ошибка
    */