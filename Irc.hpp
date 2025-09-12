#include <sstream>
#include <string>
#include <sys/poll.h>

class IRC {

    public:
        static bool extractOneMessage(std::string& buff, std::string& msg) {
            std::size_t pos = buff.find("\r\n");
            if (pos != std::string::npos) {
                msg = buff.substr(0, pos);
                buff.erase(0, pos + 2);
                return true;
            } else 
                return false;
            
            
        }
        static std::string handleMessage(int clientfd, std::string msg) {
            std::ostringstream oss;
            oss << "received from " + std::to_string(clientfd) + "message " + msg << "\r\n" ;  //
            return oss.str();
        };
    };