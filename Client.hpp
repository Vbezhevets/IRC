#pragma once
#include <cstddef>
#include <ctime>
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
#include <netinet/in.h>

class Client{
    private:
        int         _fd;
        // sockaddr_in _addr;
        std::string _in_buff;
        std::string _out_buff;
        std::string _nick;
        std::string _user;
        time_t      _last_active;
        bool        awaitingPong;


    public:
        Client():_fd(-1) {};
        Client(int fd) : _fd(fd),  awaitingPong(false){
            updateActive();
            // std:: cout << std::to_string(_fd ) << std::endl;  // temporary
        };
        void appendInBuff(const char *data, size_t n) {
            _in_buff.append(data, n);
        }
        std::string& getInBuff() {
            return _in_buff;
        }
        std::string& getOutBuff() {
            return _out_buff;
        }
        void addToOutBuff(const std::string& s) {
            _out_buff += s;
        }
        
        bool wantsWrite() const {
            return !_out_buff.empty();
        }
        time_t lastActive(){
            return _last_active;
        }
        void updateActive(){
        _last_active = (time(NULL));
        }
        
        bool isAwaitingPong(){
            return awaitingPong;
        }
        void setAwaitingPong(bool b){
            awaitingPong = b;
        }
        int getFd()const {
            return _fd;
        }
};