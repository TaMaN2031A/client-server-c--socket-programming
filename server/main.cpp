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

using namespace std;

static const int MAXPENDING = 5;
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
void HandleTCPClient(int clntSocket){
    char buffer[BUFSIZ];
    // Receive message from client

    /** This is the problem */
    ssize_t numBytesRcvd = recv(clntSocket, buffer, 600, 0);
    cout << "What's up" << endl;
    if(numBytesRcvd < 0)
        DieWithSystemMessage("recv() failed");
    // Send received string and receive again until end of system
    buffer[numBytesRcvd] = '\0';
    cout << "Received from client: " << buffer << endl;
    istringstream iss(buffer);
    string requestType, filePath, hostName;
    iss >> requestType >> filePath >> hostName;
    if (requestType == "client_get") {
        ifstream file(filePath);
        if (file.is_open()) {
            // Send HTTP response header
            send(clntSocket, "HTTP/1.1 200 OK\r\n", BUFSIZ - 1, 0);
            // Send the file content
            string line;
            // Same3 ??
            while (getline(file, line)) {
                send(clntSocket, line.c_str(), BUFSIZ-1, 0);
            }

            // Send a blank line to indicate the end of the file
            send(clntSocket, "\r\n", BUFSIZ -1, 0);

            file.close();
        } else {
            // File not found
            send(clntSocket, "HTTP/1.1 404 Not Found\r\n\r\n", BUFSIZ-1, 0);
        }
    } else if (requestType == "client_post") {
        // Receive and save the file
        send(clntSocket, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);
        // Extracting the filename from the original path
        size_t lastSlashPos = filePath.find_last_of('/');
        string filename = (lastSlashPos != string::npos) ? filePath.substr(lastSlashPos + 1) : filePath;

        // Appending "_received" to the filename
        size_t dotPos = filename.find_last_of('.');
        string newFilename = filename.substr(0, dotPos) + "_received" + filename.substr(dotPos);

        ofstream outfile(newFilename, ios::out | ios::binary);
        while (true) {
            if (numBytesRcvd == 0) {
                break; // Connection closed
            }
            if (numBytesRcvd < 0) {
                DieWithSystemMessage("recv() failed");
            }
            numBytesRcvd = recv(clntSocket, buffer, BUFSIZ, 0);
            // Print the content (for debugging purposes)
        //    cout << "Received content: " << buffer << endl;
            outfile.write(buffer, numBytesRcvd);
            if(strstr(buffer, "\r\n")){
                cout << "FINE" << endl;
                break;
            }
        }

        outfile.close();
        cout << "File Closed" << endl;
        // Send HTTP response header
    }
    cout << "Returning" << endl;
  //  close(clntSocket);
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

    // Bind to local address, send my local address as server to bind
    if(bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("bind() failed");

    if(listen(servSock, MAXPENDING) < 0)
        DieWithSystemMessage("accept() failed");

    struct sockaddr_in clntAddr;
    // Size of client address
    socklen_t clntAddrLen = sizeof(clntAddr);
    /*** Wait for a client to connect, the queue part is here, but how to know how many waiting */
    // Threads should start
    int clntSock = accept(servSock, (struct sockaddr *)&clntAddr, &clntAddrLen);

    if(clntSock < 0)
        DieWithSystemMessage("accept() failed");

    // Now clntSock is connected in client, It's ready for sending and receiving and sending
    char clntName[INET_ADDRSTRLEN]; // COntani the client IP Adress
    if(inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName))
       != NULL)
        printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
    else
        puts("Unable to get client address");

    for(;;){
        int i = 0;
        cout << i++ << endl;
        HandleTCPClient(clntSock);
        cout << "Returned" << endl;
    }
}
