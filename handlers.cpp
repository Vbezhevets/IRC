#include "Irc.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include <random>
#include <streambuf>

int checkNoCommand(Server& S, Client& client,  IRC::command& cmd, int errNum){
    if (cmd.params.empty() && cmd.trailing.empty())  {
        S.sendToClient(client, IRC::makeNumString(errNum, client));
        return true;
    }
    return false;
}
void IRC::  handlePASS(Server& S, Client& client,  IRC::command& cmd) {
    if (client.isRegistered())
        S.sendToClient(client, IRC::makeNumString(462, client));

    else if (checkNoCommand(S, client, cmd, 461))
        return;
    else if (!cmd.params.empty() && cmd.params[0] != S.getPassword()) 
        S.sendToClient(client, IRC::makeNumString(464, client));

    else {
        client.passOk(); 
         S.tryRegister(client);;
    }

}

void IRC::  handleNICK(Server& S, Client& client,  IRC::command& cmd){
    if (checkNoCommand(S, client, cmd, 431)) return;

    std::string& nick = cmd.params[0];
   if (S.setNick(client, nick) == 433) {
        S.sendToClient(client, IRC::makeNumString(433, client, nick));
        return;
    }
    if (!client.isRegistered())
        S.tryRegister(client);
    else {
        std::string oldMask = client.getMask();
        std::string msg = ":" + oldMask + " NICK :" + nick + "\r\n";
        S.sendToClient(client, msg);
        //S.broadcastToCommonChannels(client, msg); send to all participants in canal about change ;
    }
};

void IRC::  handleUSER(Server& S, Client& client,  IRC::command& cmd){
    if (checkNoCommand(S, client, cmd, 431)) return;
    if (client.isRegistered()) {
         S.sendToClient(client, IRC::makeNumString(462, client));
        return;
    }
    if (cmd.params.size() < 3 || cmd.trailing.empty()) {
        S.sendToClient(client, IRC::makeNumString(461, client, "USER")); 
        return;
    }
    client.setUser(cmd.params[0], cmd.trailing);
     S.tryRegister(client);

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