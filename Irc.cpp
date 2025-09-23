
#include "Irc.hpp"
#include "Client.hpp"
#include "Server.hpp"


#include <string>
#include <vector>
#include <cctype>

std::map<std::string, IRC::handler> IRC::handlers;

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

static inline char toupper_char(unsigned char c) {
    switch (c) {
        case '{': return '[';
        case '}': return ']';
        case '|': return '\\';
        case '~': return '^';
        default:  return (char)std::toupper(c);
    }
}
static inline void upper(std::string &s) {
    for (std::size_t i = 0; i < s.size(); ++i)
        s[i] = toupper_char((unsigned char)s[i]);
}

void IRC::initNumAnsers() {
    numAnswers[421]    = "Unkonown command";
    
             /* all will be here*/
    
    numAnswers[461] = "Not enough parameters";
    numAnswers[462] = "You may not reregister";
    numAnswers[464] = "Password incorrect";   
}


void sendNum(int n, Client& client, std::string cmd = "", const std::string& trailing = "") {
    char codeBuf[4];
    std::snprintf(codeBuf, sizeof(codeBuf), "%03d", n);

    std::string text = IRC::numAnswers.count(n) ? IRC::numAnswers[n] : trailing;

    std::string reply = std::string(":") + SERVERNAME + " " + std::string(codeBuf) + " " + client.getNick();

    if (!cmd.empty())
        reply += " " + cmd;

    if (!text.empty())
        reply += " :" + text;

    reply += "\r\n";

    client.addToOutBuff(reply);
}

void IRC:: handleMessage(Server& s, Client& client, const std::string& msg) {
    command tempCmd = parseLine(msg);
    if (tempCmd.cmd.empty()) {
        sendNum(421,client, ""); return;
    }
    std::map <std::string, handler> ::iterator it = handlers.find(tempCmd.cmd) ;
    if (it != handlers.end())
        it->second(s, client, tempCmd);
    else 
        sendNum(421,client, tempCmd.cmd, "");
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