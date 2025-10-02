#pragma once
#include <string>
#include <sys/poll.h>
#include <vector>
#include <map>
#include "../server/Server.hpp"
#include <iostream>

enum ErrReplies {
    ERR_NOSUCHNICK          = 401,
    ERR_NOSUCHCHANNEL       = 403,

    ERR_NOORIGIN            = 409,
    ERR_NORECIPIENT         = 411,
    ERR_NOTEXTTOSEND        = 412,
    ERR_UNKNOWNCOMMAND      = 421,
    ERR_NONICKNAMEGIVEN     = 431,
    ERR_NICKNAMEINUSE       = 433,

    ERR_USERNOTINCHANNEL    = 442,

    ERR_NEEDMOREPARAMS      = 461,
    ERR_ALREADYREGISTRED    = 462,
    ERR_PASSWDMISMATCH      = 464,
    ERR_KEYSET              = 467,

    ERR_CHANNELISFULL       = 471,
    ERR_UNKNOWNMODE         = 472,
    ERR_INVITEONLYCHAN      = 473,
    ERR_BADCHANNELKEY       = 475,
    ERR_CHANOPRIVNEEDED     = 482,
};

class Server;
class Client;
class Channel;

class IRC {
    public:

        struct command {
            std::string prefix;
            std::string cmd;
            std::vector <std::string> params;
            std::string trailing;

            void display() {
                std::cerr
                    << "PREF: " << prefix << std::endl
                    << "CMD: " << cmd << std::endl
                    << "PARAMS: " << std::endl;
                for (size_t i = 0; i < params.size(); i++) {
                    std::cerr << "\t" << i << ": " << params[i] << std::endl;
                }
                std::cerr << "TRAIL: " << trailing << std::endl;
            }
        };

        //handlers for commands:
        typedef void (*handler)(Server& , Client&, command& ) ;

        static std::map <std::string, handler>  handlers ;
        static std::map <int, std::string>     numAnswers;

        static void initHandlers();
        static void initNumAnswers();

        static bool     extractOneMessage(std::string& buff, std::string& msg) ;
        static void     handleMessage(Server& s, Client& cleint, const std::string& msg) ;
        static command  parseLine(std::string line);

        static std::string makeNumString(int n, Client& client, std::string cmd = "", const std::string& trailing = "" );
        static std::string makeNumStringName(int n, const std::string &name, std::string cmd = "", const std::string& trailing = "" );
        static std::string makeNumStringChannel(int n, Channel &channel, std::string cmd = "", const std::string& trailing = "" );
        static std::string makeStringFromServ( const std::string& message);

        static void handlePASS(Server&, Client&, command&);
        static void handleNICK(Server&, Client&, command&);
        static void handleUSER(Server&, Client&, command&);
        static void handlePING(Server&, Client&, command&);
        static void handlePRIVMSG(Server&, Client&, command&);
        static void handleJOIN(Server&, Client&, command&);
        static void handlePART(Server&, Client&, command&);
        static void handlePONG(Server&, Client&, command&);
        static void handleMODE(Server&, Client&, command&);
};
