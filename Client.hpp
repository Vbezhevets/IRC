#pragma once

#include <string>
#include <ctime>

class Client {
private:
    int         _fd;
    std::string _in_buff; //recieved from client to Serv
    std::string _out_buff_for_client; // server writes to client here

    std::string _nick;
    std::string _user;
    std::string _realname;
    std::string _host;

    bool        _passOk; 
    bool        _hasNick; 
    bool        _hasUser; 
    bool        _isRegistered; 
 

    std::time_t _last_active;
    bool        _awaitingPong;

    
    public:
    Client();
    Client(int fd, std::string host);
    ~Client();
    
    
    void appendInBuff(const char* data, std::size_t n);
    std::string& getInBuff();
    std::string& getOutBuff();
    void addToOutBuff(const std::string& s);
    bool wantsWrite() const;
    
    std::time_t lastActive() const;
    void updateActive();
    
    void tryMakeRegistered();    

    bool isAwaitingPong() const;
    void setAwaitingPong(bool b);

    int getFd() const;



    const std::string& getNick() const;

    void setUser(const std::string& user, const std::string& realname);
    const std::string& getUserName() const;
    const std::string& getRealName() const;
    const std::string& getHost() const;

    void passOk();
    void applyNick(const std::string& nick);

    bool isRegistered() const;
    std::string getMask() const ;
};