//
// Created by eyad on 04/12/23.
//

#ifndef SERVER_CLIENTHANDLER_H
#define SERVER_CLIENTHANDLER_H

#include <csignal>

class ClientHandler {
public:
    pthread_t thread;
    time_t finish_time;
    int clntSock;
};
#endif //SERVER_CLIENTHANDLER_H
