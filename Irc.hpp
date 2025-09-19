#pragma once
#include <sstream>
#include <string>
#include <sys/poll.h>
#include <vector>
#include <map>
class Server;
class Client;



class IRC {
    public:
        
        struct command{
            std::string prefix;
            std::string cmd;
            std::vector <std::string> params;
            std::string trailing;
        };

        //handlers for commands:
        typedef std::string (*handler)(Server& , Client&, command& ) ;
        
        static std::map <std::string, handler> handlers ;
        
        static void initHandlers();
        
        static bool extractOneMessage(std::string& buff, std::string& msg) ;
        static std::string handleMessage(Server& s, Client& cleint, const std::string& msg) ;
        static command  parseLine(std::string line);
        
        static std::string handlePASS(Server&, Client&, command&);
        static std::string handleNICK(Server&, Client&, command&);
        static std::string handleUSER(Server&, Client&, command&);
        static std::string handlePING(Server&, Client&, command&);
        static std::string handlePRIVMSG(Server&, Client&, command&);
        static std::string handleJOIN(Server&, Client&, command&);
        static std::string handlePART(Server&, Client&, command&);
        static std::string handlePONG(Server&, Client&, command&);
        
    };