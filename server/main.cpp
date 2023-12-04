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


void ReceiveFile_And_Write(int clntSocket, string newfilename) {
    ofstream outputFile(newfilename, ios::binary);

    if (outputFile.is_open()) {
        char buffer[BUFSIZ];
        ssize_t numBytesRcvd;

        // Receive data in a loop
        while ((numBytesRcvd = recv(clntSocket, buffer, BUFSIZ, 0)) > 0) {
            // Write received data to the file
            outputFile.write(buffer, numBytesRcvd);

            // Check if the entire file has been received
            if (numBytesRcvd < BUFSIZ) {
                break;  // Break the loop if less than BUFSIZ bytes are received
            }

        }

        cout << "File received and saved as: " << newfilename << std::endl;
    } else {
        DieWithSystemMessage("Error opening file for writing.");
    }

    outputFile.close();
    cout << "File Closed" << endl;
}
void ReadFile_And_Send(int sock,string filePath){
    ifstream file(filePath, std::ios::binary);
    if (file.is_open()) {
        // Get the size of the file
        file.seekg(0, ios::end);
        streampos fileSize = file.tellg();
        file.seekg(0, ios::beg);

        // Read the entire file into a vector
        vector<char> buffer(fileSize);
        file.read(buffer.data(), fileSize);

        // Send the file data
        send(sock, buffer.data(), fileSize, 0);
        file.close();
    } else {
        std::cout << "Failed to open file: " << filePath << std::endl;
        DieWithSystemMessage("Failed to open file: ");
    }

}

bool HandleTCPClient(int clntSocket){
    char buffer[BUFSIZ];
    // Receive message from client

    /** This is the problem */
    ssize_t numBytesRcvd = recv(clntSocket, buffer, BUFSIZ, 0);
    cout<<"focus\n"<<buffer<<"\n";
    if(numBytesRcvd < 0)
        DieWithSystemMessage("recv() failed");
    else if (numBytesRcvd ==0){
        return false;
    }
    // Send received string and receive again until end of system
    buffer[numBytesRcvd] = '\0';
    cout << "Received from client: " << buffer << endl;
    istringstream iss(buffer);
    string requestType, filePath, hostName;
    iss >> requestType >> filePath >> hostName;

    send(clntSocket, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"), 0);

    if (requestType == "client_get") {

        ReadFile_And_Send(clntSocket,filePath);

    }
    else if (requestType == "client_post") {
        // Extracting the filename from the original path
        size_t lastSlashPos = filePath.find_last_of('/');
        string filename = (lastSlashPos != string::npos) ? filePath.substr(lastSlashPos + 1) : filePath;

        // Appending "_received" to the filename
        size_t dotPos = filename.find_last_of('.');
        string newfilename = filename.substr(0, dotPos) + "_received" + filename.substr(dotPos);

        ReceiveFile_And_Write(clntSocket,newfilename);

        // send ok to make user send the next request
        send(clntSocket, "ok\n", 2, 0);
    }
    cout << "Returning" << endl;
    return true;
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

    cout<<clntName<<"\n";
    int i = 0;


    // it makes all requests from one user
    while (true){
        cout << i++ << endl;
        bool x=HandleTCPClient(clntSock);
        if(!x){
            break;
        }
        cout << "Returned1" << endl;
    }
    cout<<"end\n";
}