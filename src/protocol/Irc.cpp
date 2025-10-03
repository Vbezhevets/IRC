#include "Irc.hpp"
#include <string>
#include <vector>
#include <cctype>
#include <cstdio>
#include "../utils/utils.hpp"
#include "../channel/Channel.hpp"
#include "../client/Client.hpp"
#include "../server/Server.hpp"

std::map<std::string, IRC::handler> IRC::handlers;
std::map<int, std::string> IRC::numAnswers;

void IRC::initNumAnswers() {
    numAnswers[ERR_NOSUCHNICK]          = "No such nick/channel";
    numAnswers[ERR_NOORIGIN]            = "No origin specified";
    numAnswers[ERR_NORECIPIENT]         = "No recipient given";
    numAnswers[ERR_NOTEXTTOSEND]        = "No text to send";

    numAnswers[ERR_UNKNOWNCOMMAND]      = "Unknown command";

    numAnswers[ERR_NICKNAMEINUSE]       = "Nickname is already in use";
    numAnswers[ERR_NONICKNAMEGIVEN]     = "No nickname given";
    numAnswers[ERR_ERRONEUSNICKNAME]    = "Erroneus nickname";

    numAnswers[ERR_USERNOTINCHANNEL]    = "They aren't on that channel";
    numAnswers[ERR_NOTONCHANNEL]        = "You're not on that channel";
    numAnswers[ERR_USERONCHANNEL]       = "is already in channel";
    numAnswers[ERR_NOTREGISTERED]       = "You have not registered";

    /* all errors will be here*/

    numAnswers[ERR_NEEDMOREPARAMS]      = "Not enough parameters";
    numAnswers[ERR_ALREADYREGISTRED]    = "You may not reregister";
    numAnswers[ERR_PASSWDMISMATCH]      = "Password incorrect";
    numAnswers[ERR_KEYSET]              = "Channel key already set";

    numAnswers[ERR_UNKNOWNMODE]         = "is unknown mode char to me";
    numAnswers[ERR_BADCHANNELKEY]       = "Cannot join channel (+k)";
    numAnswers[ERR_CHANNELISFULL]       = "Cannot join channel (+l)";
    numAnswers[ERR_INVITEONLYCHAN]      = "Cannot join channel (+i)";
    numAnswers[ERR_CHANOPRIVNEEDED]     = "You're not channel operator";

}

void IRC::initHandlers() {
    handlers["PASS"]    = &handlePASS;
    handlers["NICK"]    = &handleNICK;
    handlers["USER"]    = &handleUSER;
    handlers["PING"]    = &handlePING;
    handlers["PRIVMSG"] = &handlePRIVMSG;
    handlers["JOIN"]    = &handleJOIN;
    handlers["PART"]    = &handlePART;
    handlers["PONG"]    = &handlePONG;
    handlers["MODE"]    = &handleMODE;
}

bool IRC::  extractOneMessage(std::string& buff, std::string& msg) {
    std::size_t pos = buff.find("\r\n");
    if (pos != std::string::npos) {
        size_t msgEnd = (pos > MAX_MESSAGE_LEN) ? MAX_MESSAGE_LEN : pos;
        msg = buff.substr(0, msgEnd);
        buff.erase(0, pos + 2);
        return true;
    } else
        return false;
}

static inline void strToUpper(std::string &s) {
    for (std::size_t i = 0; i < s.size(); ++i)
        s[i] = toupper(s[i]);
}

std::string  IRC:: makeStringFromServ(const std::string& message ) {
    return (std::string(":") + SERVERNAME + " " + message + "\r\n");
}

std::string IRC:: makeNumStringName(int n, const std::string &name, std::string cmd, const std::string &trailing) {
    char codeBuf[4];
    std::snprintf(codeBuf, sizeof(codeBuf), "%03d", n);

    std::string text = IRC::numAnswers.count(n) ? IRC::numAnswers[n] : trailing; // trailing give possibility to send here our own specific message

    std::string reply = std::string(":") + SERVERNAME + " " + std::string(codeBuf) + " " + name;

    if (!cmd.empty())
        reply += " " + cmd;

    if (!text.empty())
        reply += " :" + text;
    reply += "\r\n";

    return reply;
}

std::string IRC::makeNumStringChannel(int n, Channel &channel, std::string cmd, const std::string& trailing) {
    return makeNumStringName(n, channel.getName(), cmd, trailing);
}

std::string IRC:: makeNumString(int n, Client& client, std::string cmd, const std::string& trailing) {
    return makeNumStringName(n, client.getNick(), cmd, trailing);
}

void IRC:: handleMessage(Server& s, Client& client, const std::string& msg) {
    client.updateActive();

    LOG_DEBUG << "Received Message " << msg << " from client " << client.getFd() << std::endl;

    command tempCmd = parseLine(msg);
    if (tempCmd.cmd.empty()) {
       s.sendToClient(client, IRC::makeNumString(ERR_UNKNOWNCOMMAND, client, "")); return;
    }

    tempCmd.display();

    if (tempCmd.cmd == "PRIVMSG"
        || tempCmd.cmd == "JOIN"
        || tempCmd.cmd == "MODE"
        || tempCmd.cmd == "PART"
        || tempCmd.cmd == "INVITE") {
        if (!client.isRegistered()) {
            s.sendToClient(client, IRC::makeNumString(ERR_NOTREGISTERED, client));
            return ;
        }
    }

    std::map <std::string, handler> ::iterator it = handlers.find(tempCmd.cmd) ;
    if (it != handlers.end())
        it->second(s, client, tempCmd);
    else
        s.sendToClient(client, IRC::makeNumString(ERR_UNKNOWNCOMMAND, client, tempCmd.cmd));
}

static void trimRight(std::string &s) {
    while (!s.empty() && (s[s.size()-1] == ' ' || s[s.size()-1] == '\t'))
        s.erase(s.size()-1);
}

IRC::command IRC:: parseLine( std::string s) {
    command out;
    trimRight(s);
    if (s.empty()) return out;

    const char *p = s.c_str();
    const char *end = p + s.size();

    if (*p == ':') {
        ++p;
        const char *start = p;
        while (p < end && *p != ' ') ++p;
        out.prefix = std::string(start, p - start);
        while (p < end && *p == ' ') ++p;
    }
    if (p >= end)
        return out;
    const char *cmdStart = p;
    while (p < end && *p != ' ') ++p;
    out.cmd = std::string(cmdStart, p - cmdStart);

    strToUpper(out.cmd);

    while (p < end) {
        while (p < end && *p == ' ') ++p;
        if (p >= end) break;

        if (*p == ':') {
            ++p;
            out.trailing = std::string(p, end - p);
            break;
        }

        const char *argStart = p;
        while (p < end && *p != ' ') ++p;
        out.params.push_back(std::string(argStart, p - argStart));
    }

    return out;
}
