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
        typedef void (*handler)(Server& , Client&, command& ) ;
        
        static std::map <std::string, handler> handlers ;
        static std::map <int , std::string> numAnswers;
        
        static void initHandlers();
        static void initNumAnswers();
        
        static bool extractOneMessage(std::string& buff, std::string& msg) ;
        static void handleMessage(Server& s, Client& cleint, const std::string& msg) ;
        static command  parseLine(std::string line);
        static void sendNum(int n, Client&, std::string cmd = "", const std::string& trailing = "");
        static void sendFromServ(Client& , const std::string&  );

        static void handlePASS(Server&, Client&, command&);
        static void handleNICK(Server&, Client&, command&);
        static void handleUSER(Server&, Client&, command&);
        static void handlePING(Server&, Client&, command&);
        static void handlePRIVMSG(Server&, Client&, command&);
        static void handleJOIN(Server&, Client&, command&);
        static void handlePART(Server&, Client&, command&);
        static void handlePONG(Server&, Client&, command&);
        
    };