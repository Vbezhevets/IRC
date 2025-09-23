#include "Irc.hpp"
#include "Server.hpp"
#include "Client.hpp"

void handlePASS(Server& S, Client& client, const IRC::command& cmd) {
    if (client.isRegistered()){ 
        IRC::sendNum(462, client); 
        return; 
    }
    if (cmd.params.empty() && cmd.trailing.empty()) {
        IRC::sendNum(461, client); 
        return;
    }

    if (cmd.params[0] != S.getPassword()) {
        IRC::sendNum(464, client);
        return;
    }

    client.passOk();
}

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