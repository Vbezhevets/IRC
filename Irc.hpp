#include <sstream>
#include <string>
#include <sys/poll.h>
#include <vector>
#include <map>
#include "Server.hpp"
#include "Client.hpp"


class IRC {
    public:
        
        struct command{
            std::string prefix;
            std::string cmd;
            std::vector <std::string> params;
            std::string trailing;
        };

        //handlers for commands:
        static std::string handlePASS(Server&, Client&, command&);
        static std::string handleNICK(Server&, Client&, command&);
        static std::string handleUSER(Server&, Client&, command&);
        static std::string handlePING(Server&, Client&, command&);
        static std::string handlePRIVMSG(Server&, Client&, command&);
        static std::string handleJOIN(Server&, Client&, command&);
        static std::string handlePART(Server&, Client&, command&);

        typedef std::string (*handler)(Server& , Client&, command& ) ;
        static std::map <std::string, handler> handlers ;
        
        static bool extractOneMessage(std::string& buff, std::string& msg) ;
        static std::string handleMessage(Server&, Client&, std::string& msg) ;
        static command  parseLine(std::string line);
        static void initHandlers();
    };