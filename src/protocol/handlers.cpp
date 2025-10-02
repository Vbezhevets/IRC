#include "Irc.hpp"
#include "../utils/utils.hpp"
#include <sstream>
#include <string>

bool checkNoCommand(Server& S, Client& client, IRC::command& cmd, int errNum){
    if (cmd.params.empty() && cmd.trailing.empty())  {
        S.sendToClient(client, IRC::makeNumString(errNum, client));
        return true;
    }
    return false;
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
    if (client.isRegistered()) {
        S.sendToClient(client, IRC::makeNumString(ERR_ALREADYREGISTRED, client));
        return;
    }
    if (cmd.params.size() < 3 || cmd.trailing.empty()) {
        S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client, "USER"));
        return;
    }
    client.setUser(cmd.params[0], cmd.trailing);
    S.tryRegister(client);
}

void IRC::  handlePING(Server& S, Client& client, IRC::command& cmd){
    (void)S;
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
            S.sendToClient(client, IRC::makeNumString(ERR_NOSUCHNICK, client)); return;
        }
        LOG_DEBUG << "Sending message to channel " << c->getName() << std::endl;
        std::set<const Client *>::iterator it = c->getClients().begin();
        std::set<const Client *>::iterator eit = c->getClients().end();

        LOG_DEBUG << "Channel has " << c->getClients().size() << " members" << std::endl;

        std::string msg = ":" + client.getMask() + " PRIVMSG " + "#" + c->getName() + " :" + cmd.trailing + "\r\n";
        for (; it != eit; it++) {
            if (*it == &client) continue ;
            LOG_DEBUG << "Sending message to client " << (*it)->getNick() << std::endl;
            S.sendToClient(*const_cast<Client *>(*it), msg);
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
    LOG_DEBUG << "reveived join command" << std::endl;
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
                        LOG_DEBUG << "Adding client to " << target << std::endl;
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
    std::string target = cmd.params[0].substr(1);

    Channel *chan = S.getChannelByName(target);
    if (chan) {
        if (chan->isOperator(&client)) {
            std::stringstream s(cmd.params[1]);
            char c;

            s >> c;
            if (c == '+') {
                while (!s.eof()) {
                    s >> c;
                    switch (c) {
                        case 'i':
                            chan->setMode(MODE_INVITE_ONLY);
                            break;
                        case 't':
                            chan->setMode(MODE_TOPIC_OPERATOR_ONLY);
                            break;
                        case 'o':
                            if (cmd.params.size() < 3) {
                                S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
                            } else {
                                Client *targetClient = S.getClientByNick(cmd.params[2]);
                                if (!targetClient) {
                                    S.sendToClient(client, IRC::makeNumStringName(ERR_USERNOTINCHANNEL, cmd.params[2] + chan->getDisplayName()));
                                    return ;
                                }
                                if (chan->hasClient(targetClient)) {
                                    if (!chan->isOperator(targetClient)) {
                                        chan->addOperator(targetClient);
                                    }
                                }
                            }
                            break;
                        case 'l':
                            if (cmd.params.size() < 3) {
                                S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
                            } else {
                                chan->setMode(MODE_USER_LIMIT);
                                std::stringstream s(cmd.params[2]);
                                size_t limit = 0;
                                s >> limit;
                                if (s.fail()) {
                                    S.sendToClient(client, IRC::makeNumStringName(ERR_UNKNOWNMODE, s.str()));
                                } else {
                                    chan->setLimit(limit);
                                }
                            }
                            break;
                        case 'k':
                            if (cmd.params.size() < 3) {
                                S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
                            } else {
                                if (chan->hasMode(MODE_KEY_PROTECTED)) {
                                    S.sendToClient(client, IRC::makeNumStringChannel(ERR_NEEDMOREPARAMS, *chan));
                                } else {
                                    chan->setMode(MODE_KEY_PROTECTED);
                                    chan->setKey(cmd.params[2]);
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }
            } else if (c == '-') {

                while (!s.eof()) {
                    s >> c;
                    switch (c) {
                        case 'i':
                            chan->unsetMode(MODE_INVITE_ONLY);
                            break;
                        case 't':
                            chan->unsetMode(MODE_TOPIC_OPERATOR_ONLY);
                            break;
                        case 'o':
                            if (cmd.params.size() < 3) {
                                S.sendToClient(client, IRC::makeNumString(ERR_NEEDMOREPARAMS, client));
                                return ;
                            } else {
                                Client *targetClient = S.getClientByNick(cmd.params[2]);
                                if (!targetClient) {
                                    S.sendToClient(client, IRC::makeNumString(ERR_NOSUCHNICK, client));
                                    return ;
                                }
                                if (chan->isOperator(targetClient)) {
                                    chan->removeOperator(targetClient);
                                } else {
                                    S.sendToClient(client, IRC::makeNumStringName(ERR_USERNOTINCHANNEL, cmd.params[2] + " " + chan->getDisplayName()));
                                    return ;
                                }
                            }
                            break;
                        case 'l':
                            if (cmd.params.size() > 2) {
                                S.sendToClient(client, IRC::makeNumStringName(ERR_UNKNOWNMODE, s.str()));
                                return ;
                            } else {
                                chan->unsetMode(MODE_USER_LIMIT);
                                chan->setLimit(0);
                            }
                            break;
                        case 'k':
                            if (cmd.params.size() > 2) {
                                S.sendToClient(client, IRC::makeNumStringName(ERR_UNKNOWNMODE, s.str()));
                                return ;
                            } else {
                                chan->unsetMode(MODE_KEY_PROTECTED);
                                chan->setKey("");
                            }
                            break;
                        default:
                            S.sendToClient(client, IRC::makeNumStringName(ERR_UNKNOWNMODE, s.str()));
                            return ;
                    }
                }
            } else {
                std::string s;
                s.push_back(c);
                S.sendToClient(client, IRC::makeNumStringName(ERR_UNKNOWNMODE, s));
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
    (void)S; (void)client; (void)cmd;
}
