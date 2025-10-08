#pragma once

#include <string>
#include <sys/poll.h>
#include <vector>
#include <map>
#include "../client/Client.hpp"

#define MAX_MESSAGE_LEN 510

enum Replies {
    RPL_CHANNELMODEIS       = 324,
    RPL_NOTOPIC             = 331,
    RPL_TOPIC               = 332,
    RPL_NAMREPLY            = 353,
    RPL_ENDOFNAME           = 366,
    RPL_INVITING            = 341,
};

enum ErrReplies {
    ERR_NOSUCHNICK          = 401,
    ERR_NOSUCHCHANNEL       = 403,


    ERR_NOORIGIN            = 409,
    ERR_NORECIPIENT         = 411,
    ERR_NOTEXTTOSEND        = 412,
    ERR_UNKNOWNCOMMAND      = 421,
    ERR_NONICKNAMEGIVEN     = 431,
    ERR_ERRONEUSNICKNAME    = 432,
    ERR_NICKNAMEINUSE       = 433,

    ERR_USERNOTINCHANNEL    = 441,
    ERR_NOTONCHANNEL        = 442,
    ERR_USERONCHANNEL       = 443,
    ERR_NOTREGISTERED       = 451,

    ERR_NEEDMOREPARAMS      = 461,
    ERR_ALREADYREGISTRED    = 462,
    ERR_PASSWDMISMATCH      = 464,
    ERR_KEYSET              = 467,

    ERR_CHANNELISFULL       = 471,
    ERR_UNKNOWNMODE         = 472,
    ERR_INVITEONLYCHAN      = 473,
    ERR_BADCHANNELKEY       = 475,
    ERR_BADCHANMASK         = 476,
    ERR_CHANOPRIVNEEDED     = 482,

    ERR_UMODEUNKNOWNFLAG     = 501,
};

class Server;
class Channel;

#ifndef SERVERNAME
# define SERVERNAME "👾"
#endif

class IRC {
    public:

        struct command {
            std::string prefix;
            std::string cmd;
            std::vector <std::string> params;
            std::string trailing;
            bool        had_trailing;
        };

        //handlers for commands:
        typedef void (*handler)(Server& , Client&, command& ) ;

        static std::map <std::string, handler>  handlers;
        static std::map <int, std::string>      numAnswers;

        static void initHandlers();
        static void initNumAnswers();

        static bool     extractOneMessage(std::string& buff, std::string& msg) ;
        static void     handleMessage(Server& s, Client& cleint, const std::string& msg) ;
        static command  parseLine(std::string line);

        static std::string makeNumString(int n, Client& client, const std::string &prefix = SERVERNAME, std::string cmd = "", const std::string& trailing = "" );
        static std::string makeNumStringName(int n, const std::string &name, const std::string &prefix = SERVERNAME, std::string cmd = "", const std::string& trailing = "" );
        static std::string makeNumStringChannel(int n, Channel &channel, const std::string &prefix = SERVERNAME, std::string cmd = "", const std::string& trailing = "" );
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
        static void handleINVITE(Server&, Client&, command&);
        static void handleKICK(Server&, Client&, command&);
        static void handleTOPIC(Server&, Client&, command&);
        static void handleQUIT(Server&, Client&, command&);

};
