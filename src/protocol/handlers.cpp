#include "Irc.hpp"
#include "../utils/utils.hpp"
#include <string>
#include "../server/Server.hpp"
#include "../client/Client.hpp"
#include "../channel/Channel.hpp"

bool checkNoCommand(Server& S, Client& client, IRC::command& cmd, int errNum){
    if (cmd.params.empty() && cmd.trailing.empty())  {
        S.sendToClient(client, IRC::makeNumString(errNum, client));
        return true;
    }
    return false;
}


bool isValidNick(const std::string& nick) {
    if (nick.empty() || nick.length() > 9) return false;
    if (isdigit(nick[0]) || nick[0] == '-') return false;

    for (size_t i = 0; i < nick.length(); i++) {
        char c = nick[i];
        if (!isalnum(c) && 
            c != '[' && c != ']' && c != '\\' && c != '`' && 
            c != '_' && c != '^' && c != '{' && c != '|' && c != '}') {
            return false;
        }
    }
    return true;
}

// bool checkNoCommand(Server& S, Client& client,  IRC::command& cmd, int errNum){
//     if (cmd.params.empty()) {S.sendToClient(client, IRC::makeNumString(411, client)); return true;}
//     if (cmd.trailing.empty()) {S.sendToClient(client, IRC::makeNumString(412, client)); return true;}
//     return false;
// }

void IRC::  handlePASS(Server& S, Client& client, IRC::command& cmd) {
    if (client.isRegistered())
        S.sendToClient(client, IRC::makeNumString(ERR_ALREADYREGISTRED, client));
    else if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS))
        return;
    else if (!cmd.params.empty() && cmd.params[0] != S.getPassword())
        S.sendToClient(client, IRC::makeNumString(ERR_PASSWDMISMATCH, client));
    else {
        client.passOk();
        S.tryRegister(client);;
    }

}

void IRC::  handleNICK(Server& S, Client& client, IRC::command& cmd){
    if (checkNoCommand(S, client, cmd, ERR_NONICKNAMEGIVEN)) return;

    std::string& nick = cmd.params[0];
    if (!isValidNick(nick)) {
        S.sendToClient(client, IRC::makeNumStringName(ERR_ERRONEUSNICKNAME, nick));
        return;
    }
    if (S.setNick(client, nick) == ERR_NICKNAMEINUSE) {
        S.sendToClient(client, IRC::makeNumString(ERR_NICKNAMEINUSE, client, nick));
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
}

void IRC::  handleUSER(Server& S, Client& client, IRC::command& cmd){
    if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS)) return;
    if (cmd.params.size() < 3 || cmd.trailing.empty()) {
        S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client, "USER"));
        return;
    }
    client.setUser(cmd.params[0], cmd.trailing);
    S.tryRegister(client);
}

void IRC::  handlePING(Server& S, Client& client, IRC::command& cmd){
    client.updateActive();
    if (cmd.params.empty() && cmd.trailing.empty())
        S.sendToClient(client, IRC::makeNumString(ERR_NOORIGIN, client));
    else {
        std::string tok = cmd.params.empty() ? cmd.trailing : cmd.params[0];
        LOG_DEBUG << "Replying to ping" << std::endl;
        S.sendToClient(client, IRC::makeStringFromServ(std::string("PONG " ) + SERVERNAME + " :" + tok ));
    }
}

void IRC::  handlePONG(Server& S, Client& client, IRC::command& cmd){
    (void)S; (void)cmd;
    client.updateActive();
    client.setAwaitingPong(false);
}

void IRC::  handlePRIVMSG(Server& S, Client& client, IRC::command& cmd){
    if (cmd.params.empty())  {
        S.sendToClient(client, IRC::makeNumString(ERR_NORECIPIENT, client)); return;
    }
    if (cmd.trailing.empty())  {
        S.sendToClient(client, IRC::makeNumString(ERR_NOTEXTTOSEND, client)); return;
    }

    std::string &target = cmd.params[0];

    if (target[0] == '#' || target[0] == '&') {
        std::string target_name = target.substr(1);
        Channel *c = S.getChannelByName(target_name);
        if (!c) {
            S.sendToClient(client, IRC::makeNumStringName(ERR_NOTONCHANNEL, target)); return;
        } else if (!c->hasClient(&client)) {
            S.sendToClient(client, IRC::makeNumStringChannel(ERR_NOTONCHANNEL, *c)); return;
        }
        LOG_DEBUG << "Sending message to channel " << c->getName() << std::endl;
        LOG_DEBUG << "Channel has " << c->getClients().size() << " members" << std::endl;
        std::string msg = ":" + client.getMask() + " PRIVMSG " + "#" + c->getName() + " :" + cmd.trailing + "\r\n";
        std::set<const Client *>::iterator it = c->getClients().begin();

        while (it != c->getClients().end()) {
            if (*it != &client) {
                S.sendToClient(*const_cast<Client *>(*it), msg);
            }
            it++;
        }
    } else {
        Client *reciept = S.getClientByNick(cmd.params[0]);
        if (!reciept) {
             S.sendToClient(client, IRC::makeNumString(ERR_NOSUCHNICK, client, cmd.params[0])); return;
        }
        S.sendToClient(*reciept,
                  ":" + client.getMask() + " PRIVMSG " + reciept->getNick() + " :" + cmd.trailing + "\r\n");
    }
}

void IRC::  handleJOIN(Server& S, Client& client, IRC::command& cmd){
    if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS)) return;
    std::string target = cmd.params[0];
    LOG_DEBUG << "reveived join command " << std::endl;
    if (target[0] == '#' || target[0] == '&') {
        std::string target = cmd.params[0].substr(1);
        Channel *c = S.getChannelByName(target);

        switch (cmd.params.size()) {
            case 1:
                if (c) {
                    if (c->hasMode(MODE_INVITE_ONLY)) {
                        S.sendToClient(client, IRC::makeNumStringChannel(ERR_INVITEONLYCHAN, *c));
                    } else if (c->isFull()) {
                        S.sendToClient(client, IRC::makeNumStringChannel(ERR_CHANNELISFULL, *c));
                    } else if  (c->hasMode(MODE_KEY_PROTECTED)) {
                        S.sendToClient(client, IRC::makeNumStringChannel(ERR_BADCHANNELKEY, *c));
                    } else {
                        S.sendToClient(client, ":" + client.getMask() + " JOIN :" + c->getDisplayName() + "\n");
                        if (c->hasTopic()) {
                            S.sendToClient(client, IRC::makeNumStringName(RPL_TOPIC, SERVERNAME, "", c->getTopic()));
                        }
                        std::string names = c->getUsersOnChannel();
                        size_t messageSize
                        while (1) {
                            std::string namesSplit = names.substr()
                        }
                        c->addClient(const_cast<const Client *>(&client));

                    }
                } else {
                    LOG_DEBUG << "Creating channel " << target << std::endl;
                    Channel new_channel(target, &client);
                    S.addChannel(new_channel);
                }
                break;
            case 2:
                if (c) {
                    if (c->hasMode(MODE_INVITE_ONLY)) {
                        S.sendToClient(client, IRC::makeNumStringChannel(ERR_INVITEONLYCHAN, *c));
                    } else if (c->isFull()) {
                        S.sendToClient(client, IRC::makeNumStringChannel(ERR_CHANNELISFULL, *c));
                    } else if (c->hasMode(MODE_KEY_PROTECTED) && !c->isKey(cmd.params[1])) {
                        S.sendToClient(client, IRC::makeNumStringChannel(ERR_BADCHANNELKEY, *c));
                    } else {
                        LOG_DEBUG << "Adding client with key to " << target << std::endl;
                        c->addClient(const_cast<const Client *>(&client));
                    }
                } else {
                    Channel new_channel(target, &client, cmd.params[1]);
                    new_channel.setMode(MODE_KEY_PROTECTED);
                    S.addChannel(new_channel);
                }
                break;
        }
    } else {
        S.sendToClient(client, IRC::makeNumString(ERR_NOSUCHCHANNEL, client));
    }
}

void handleChannelMODE(Server& S, Client& client, IRC::command& cmd) {
    if (!client.isRegistered()) {
        S.sendToClient(client, IRC::makeNumString(ERR_NOTREGISTERED, client)); return;
    }
    std::string target = cmd.params[0].substr(1);
    std::string modes = cmd.params[1];
    Channel *chan = S.getChannelByName(target);

    if (chan) {
        if (chan->isOperator(&client)) {
            if (modes[0] == '+') {
                chan->handleModeSet(S, client, modes.substr(1), cmd);
            } else if (modes[0] == '-') {
                chan->handleModeUnset(S, client, modes.substr(1), cmd);
            } else {
                S.sendToClient(client, IRC::makeNumStringName(ERR_UNKNOWNMODE, std::string() + target[0]));
            }
        } else {
            S.sendToClient(client, IRC::makeNumString(ERR_CHANOPRIVNEEDED, client));
        }
    } else {
        S.sendToClient(client, IRC::makeNumString(ERR_NOSUCHNICK, client));
    }
}

void handleUserMODE(Server& S, Client& client, IRC::command& cmd) {
    (void)S; (void)client; (void)cmd;
}

void IRC::  handleMODE(Server& S, Client& client, IRC::command& cmd){
    if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS)) return;
    std::string &target = cmd.params[0];
    if (cmd.params.size() > 1) {
        if (target[0] == '#' || target[1] == '&') {
            handleChannelMODE(S, client, cmd);
        } else {
            handleUserMODE(S, client, cmd);
        }
    } else {
        S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
    }
}

void IRC::  handlePART(Server& S, Client& client, IRC::command& cmd){
    if (checkNoCommand(S, client, cmd, ERR_NEEDMOREPARAMS)) return;
    if (cmd.params.size() < 1) {
        S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
        return;
    }

    for (size_t i = 0; i < cmd.params.size(); i++) {
        std::string target = cmd.params[i];
        if (target[0] != '#' && target[0] != '&') {
            S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHCHANNEL, target));
        } else {
            std::string channel_name = cmd.params[i].substr(1);
            Channel *c = S.getChannelByName(channel_name);
            if (!c) {
                S.sendToClient(client, IRC::makeNumStringName(ERR_NOSUCHCHANNEL, target));
            } else {
                if (c->hasClient(&client)) {
                    c->removeClient(&client);
                } else {
                    S.sendToClient(client, IRC::makeNumStringName(ERR_NOTONCHANNEL, target));
                }
            }
        }
    }
}
