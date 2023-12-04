#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <fstream>
#include <csignal>
#include <vector>
#include <queue>
#include <ctime>
#include <chrono>
#include <map>
#include <mutex>
#include <semaphore.h>
#include "ClientHandler.h"

using namespace std;
queue<ClientHandler> working;
static const int MAXPENDING = 5;
sem_t sem;
sem_t qsem;


void DieWithUserMessage(const char *msg, const char *detail){
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}
void DieWithSystemMessage(const char *msg){
    perror(msg);
    exit(1);
}


void ReceiveFile_And_Write(int clntSocket, string newfilename, int fileSize) {
    ofstream outputFile(newfilename, ios::binary);

    if (outputFile.is_open()) {
        char buffer[BUFSIZ];
        ssize_t numBytesRcvd;
        int t = 0, fz = fileSize;
        // Receive data in a loop
        while (t < fileSize) {
            // Write received data to the file
            numBytesRcvd = recv(clntSocket, buffer, min(fz, BUFSIZ), MSG_WAITALL);
            outputFile.write(buffer, numBytesRcvd);
            fz-=numBytesRcvd;
            t+=numBytesRcvd;
        }
        cout << "File received and saved as: " << newfilename << std::endl;
    } else {
        // send him error
      //  DieWithSystemMessage("Error opening file for writing.");
    }
    outputFile.close();
    cout << "File Closed" << endl;
}
void ReadFile_And_Send(int sock, string filePath){
    ifstream file(filePath, std::ios::binary);
    if (file.is_open()) {
        // Get the size of the file
        file.seekg(0, ios::end);
        streampos fileSize = file.tellg();
        file.seekg(0, ios::beg);
        string toBeSent = "HTTP/1.1 200 OK\r\n " + to_string(fileSize);
        // 17 + 18 = 35
        cout << "sizze is " << toBeSent.size() << endl;
        while(toBeSent.size() < 35)
            toBeSent += " ";
        cout << toBeSent << ";" << endl;
        send(sock, toBeSent.c_str(), (toBeSent.size()), 0);

        // Hal yenfa3 a5leh y wait hena?
        // Read the entire file into a vector
        vector<char> buffer(fileSize);
        file.read(buffer.data(), fileSize);

        // Send the file data
        send(sock, buffer.data(), fileSize, 0);
        char*B [BUFSIZ];
        recv(sock, B, 2, 0);

        file.close();
    } else {
        std::cout << "Failed to open file: " << filePath << std::endl;
    }

}

bool HandleTCPClient(int clntSocket){
    char buffer[BUFSIZ];
    ssize_t numBytesRcvd = recv(clntSocket, buffer, BUFSIZ, 0);
    cout<<"focus\n"<<buffer<<"\n";
    if(numBytesRcvd < 0){
        return false;
    }
    else if (numBytesRcvd == 0){
        return false;
    }
    // Send received string and receive again until end of system
    buffer[numBytesRcvd] = '\0';
    cout << "Received from client: " << buffer << endl;
    istringstream iss(buffer);
    string requestType, filePath, hostName;
    int fileSize;
    iss >> requestType >> filePath >> hostName;

    if (requestType == "client_get") {
        ReadFile_And_Send(clntSocket,filePath);
    }
    else if (requestType == "client_post") {
        // Extracting the filename from the original path
        iss >> fileSize;
        send(clntSocket, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
        size_t lastSlashPos = filePath.find_last_of('/');
        string filename = (lastSlashPos != string::npos) ? filePath.substr(lastSlashPos + 1) : filePath;
        size_t dotPos = filename.find_last_of('.');
        string newfilename = filename.substr(0, dotPos) + "_received" + filename.substr(dotPos);


        ReceiveFile_And_Write(clntSocket,newfilename,fileSize);

        // send ok to make user send the next request
        send(clntSocket, "ok\n", 2, 0);
    }
    return true;
}
void* handleClient(void* p){

    ClientHandler ch = *reinterpret_cast<ClientHandler*>(p);
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    // Add 5 seconds
    sem_wait(&qsem);
    std::chrono::seconds secondsToAdd(MAXPENDING-working.size());
    sem_post(&qsem);
    std::chrono::system_clock::time_point newTime = now + secondsToAdd;

    // Convert to time_t
    std::time_t newTime_t = std::chrono::system_clock::to_time_t(newTime);

    ch.finish_time = newTime_t;
    int clntSock = ch.clntSock;
    // Now clntSock is connected in client, It's ready for sending and receiving and sending
    cout << "Handling Client with sock no " << clntSock << endl;
    int i = 0;

    // it makes all requests from one user
    while (true) {
        cout << ch.thread << endl;
        cout << ch.finish_time << " " << time(NULL) << endl;
        if(ch.finish_time < time(NULL))
        {
            cout << "Time Out, ending now....." << endl;
            string s = "HTTP/1.1 404 Not Found\r\n           ";
            send(clntSock, s.c_str(), s.size(), 0);

            break;
        }
        cout << i++ << endl;
        bool x = HandleTCPClient(clntSock);
        if (!x) {
            break;
        }
    }
    sem_wait(&qsem);
    working.pop();
    sem_post(&qsem);
    close(clntSock);
    sem_post(&sem);
}
int main(int argc, char *argv[]) {
    if(argc != 2)
        DieWithUserMessage("Parameter(s)", "<Server Port>");
    in_port_t servPort = atoi(argv[1]); // local port
    // Create socket for incoming connection
    int servSock; // Socket descriptor for server
    if( (servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithSystemMessage("socket() failed");
    // Construct local address structure
    struct sockaddr_in servAddr; // Local address

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET; // Any incoming interface
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY Accept from any coming IP address
    servAddr.sin_port = htons(servPort);
  //  signal(SIGPIPE, SIG_IGN);
    // Bind to local address, send my local address as server to bind
    if(bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("bind() failed");

    if(listen(servSock, MAXPENDING) < 0)
        DieWithSystemMessage("accept() failed");

    struct sockaddr_in clntAddr;
    // Size of client address
    socklen_t clntAddrLen = sizeof(clntAddr);
    sem_init(&sem, 0, 1);
    sem_init(&qsem, 0, 1);
    while(true) {

        int clntSock;

        if ((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen)) < 0)
            continue;
        cout << "Saba7o" << endl;

        ClientHandler clientHandler = ClientHandler();
        sem_wait(&qsem);
        working.push(clientHandler);
        sem_post(&qsem);
        clientHandler.clntSock = clntSock;
        sem_wait(&sem);
        pthread_create(&clientHandler.thread, nullptr, handleClient, (void*) &clientHandler);

    }
}