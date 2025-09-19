#include "Irc.hpp"
#include "Client.hpp"

std::string IRC:: handlePASS(Server& S, Client& client,  IRC::command& cmd){
   (void)S; (void)client; (void)cmd; return "";
};
  
std::string IRC:: handleNICK(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client; (void)cmd; return "";
};

std::string IRC:: handleUSER(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client; (void)cmd; return "";
};

std::string IRC:: handlePING(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client;
    if (!cmd.params.empty())
        return "PONG :" + cmd.params[0] + "\r\n";
    if (!cmd.trailing.empty())
        return "PONG :" + cmd.trailing + "\r\n";
    return "";
};
  

std::string IRC:: handlePONG(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)cmd;
    client.updateActive();
    client.setAwaitingPong(false);
    return "";
};

std::string IRC:: handlePRIVMSG(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client; (void)cmd; return "";
};

std::string IRC:: handleJOIN(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client; (void)cmd; return "";
};

std::string IRC:: handlePART(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client; (void)cmd; return "";
};  