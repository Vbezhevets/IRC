#include "Irc.hpp"
#include "Server.hpp"
#include "Client.hpp"

void IRC::  handlePASS(Server& S, Client& client,  IRC::command& cmd) {
    if (client.isRegistered())
        S.sendToClient(client, IRC::makeNumString(462, client));
 
    else if (cmd.params.empty() && cmd.trailing.empty()) 
        S.sendToClient(client, IRC::makeNumString(461, client)); 

    else if (!cmd.params.empty() && cmd.params[0] != S.getPassword()) 
        S.sendToClient(client, IRC::makeNumString(464, client));

    else
        client.passOk();
}

void IRC::  handleNICK(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client; (void)cmd;
};

void IRC::  handleUSER(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client; (void)cmd; 
};

void IRC::  handlePING(Server& S, Client& client,  IRC::command& cmd){
    (void)S; 
    client.updateActive();
    if (cmd.params.empty() && cmd.trailing.empty())
        S.sendToClient(client, IRC::makeNumString(409, client));
    else {
        std::string tok =  cmd.params.empty() ? cmd.trailing : cmd.params[0];
        S.sendToClient(client, IRC::  makeStringFromServ(std::string("PONG" )  +  SERVERNAME + " :" + tok ));
    }
};
  

void IRC::  handlePONG(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)cmd;
    client.updateActive();
    client.setAwaitingPong(false);
    
};

void IRC::  handlePRIVMSG(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client; (void)cmd; 
};

void IRC::  handleJOIN(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client; (void)cmd; 
};

void IRC::  handlePART(Server& S, Client& client,  IRC::command& cmd){
    (void)S; (void)client; (void)cmd; 
};  