#include "server.hpp"
#include <exception>
#include <sstream>
#include <stdexcept>

volatile sig_atomic_t g_running = 1;

unsigned short parsePort(const std::string s){
    if (s.empty())
        throw std::invalid_argument("empty port");
    std::stringstream ss(s);
    long p;
    if (!(ss >> p)) 
        throw std::invalid_argument("invalid port");
    if (p > 65535 || p <= 0)
        throw std::invalid_argument("invalid port");
    return (unsigned short)p;
}
void signal_handler(int sig){
    if (sig == SIGINT)  
        g_running = 0;
}
int main(int argc , char **argv) {
    
    if (argc!=3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
        return 1;
    }
    try {
        unsigned short port = parsePort(argv[1]);
        Server s(port, argv[2]);
        std::signal(SIGINT, signal_handler);
        s.init();
        // s.run();
    }
    catch (const std::exception &e){
        std::cerr << "Error: " << e.what() <<std::endl; 
        return 1;
    }
    return 0;
}
