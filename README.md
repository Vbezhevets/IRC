# IRC Server

A custom IRC server implemented in **C++** as part of the 42 Vienna curriculum.  
The goal of this project was to design a working IRC-like server supporting multiple clients, channels, and real-time message exchange.

## 🧩 Features
- Multi-client handling via non-blocking sockets (poll)
- Command parsing and message broadcasting
- Channel creation and user management
- Basic IRC protocol support (JOIN, PRIVMSG, PART, QUIT, etc.)
- Graceful error handling and server shutdown

## ⚙️ Technologies
- C++98
- POSIX sockets
- Makefile build system
- Linux environment

## 🚀 How to Run
```bash
make
./ircserv <port> <password>
