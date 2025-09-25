
#include "Irc.hpp"
#include "Client.hpp"
#include "Server.hpp"


#include <string>
#include <vector>
#include <cctype>

std::map<std::string, IRC::handler> IRC::handlers;
std::map<int, std::string> IRC::numAnswers;
bool IRC::  extractOneMessage(std::string& buff, std::string& msg) {
    std::size_t pos = buff.find("\r\n");
    if (pos != std::string::npos) {
        msg = buff.substr(0, pos);
        buff.erase(0, pos + 2);
        return true;
    } else 
        return false;
    
    
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

}


static inline void upper(std::string &s) {
    for (std::size_t i = 0; i < s.size(); ++i)
        s[i] = toupper(s[i]);
}

void IRC::initNumAnswers() {
    numAnswers[421]    = "Unkonown command";
    numAnswers[409]    = "No origin specified";
    
             /* all will be here*/
    
    numAnswers[461] = "Not enough parameters";
    numAnswers[462] = "You may not reregister";
    numAnswers[464] = "Password incorrect";   
}


std::string  IRC:: makeStringFromServ(const std::string& message ){
 return (std::string(":") + SERVERNAME + " " + message + "\r\n");
}
std::string IRC:: makeNumString(int n, Client& client, std::string cmd , const std::string& trailing ) {
    char codeBuf[4];
    std::snprintf(codeBuf, sizeof(codeBuf), "%03d", n);

    std::string text = IRC::numAnswers.count(n) ? IRC::numAnswers[n] : trailing; // trailing give possibility to send here our own specific message

    std::string reply = std::string(":") + SERVERNAME + " " + std::string(codeBuf) + " " + client.getNick();

    if (!cmd.empty())
        reply += " " + cmd;

    if (!text.empty())
        reply += " :" + text;
    reply += "\r\n"; 

    return reply;
}

void IRC:: handleMessage(Server& s, Client& client, const std::string& msg) {
    client.updateActive();

    command tempCmd = parseLine(msg);
    if (tempCmd.cmd.empty()) {
       s.sendToClient(client, IRC::makeNumString(421,client, "")); return;
    }
    std::map <std::string, handler> ::iterator it = handlers.find(tempCmd.cmd) ;
    if (it != handlers.end())
        it->second(s, client, tempCmd);
    else 
        s.sendToClient(client, IRC::makeNumString(421,client, tempCmd.cmd));
};


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
    upper(out.cmd);

    for (size_t i = 0; i < out.cmd.size(); ++i)
        out.cmd[i] = (char)std::toupper((unsigned char)out.cmd[i]);

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