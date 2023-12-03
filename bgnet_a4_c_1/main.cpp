#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
using namespace std;
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
void ReceiveFile(int sock, const char *filePath) {
    ofstream file(filePath, ios::out | ios::binary);
    if (!file.is_open()) {
        DieWithUserMessage("File Open Error", "Unable to open file for writing");
    }

    char buffer[BUFSIZ];
    ssize_t numBytesRcvd;

    while ((numBytesRcvd = recv(sock, buffer, BUFSIZ, 0)) > 0) {
        file.write(buffer, numBytesRcvd);
        cout << "4a8al" << endl;
    }

    if (numBytesRcvd < 0) {
        DieWithSystemMessage("recv() failed");
    }

    file.close();
}
// g++ main.cpp -o my_client
// ./my_client 169.1.1.1 "Echo this!"
int main(int argc, char *argv[])
{
    if(argc < 2 || argc > 3)
        DieWithUserMessage("Parameter(s)", "<Server Address> [<Server Port>]");
    char *servIP = argv[1];
    in_port_t servPort = (argc == 3) ? atoi(argv[2]) : 8080;
    const char *inputFile = "/home/eyad/in";
    ifstream file(inputFile);
    if (!file.is_open()) {
        DieWithUserMessage("File Open Error", "Unable to open input file");
    }
    string line;
    // Creating a reliable, stream socket using TCP
    // Returns an integer value descriptor or "handle" for the socket if successful
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock < 0) // -1
        DieWithSystemMessage("socket() failed");
    // Socket cannot be connected without specifying the address and port to connect to, TCP way of course tells s theat
    // For this we use the struct sockaddr_in which is a container of this information
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    // Filling the sockaddre_in
    // We use address family AF_INET as required in the lab
    servAddr.sin_family = AF_INET; // Internet Domain
    // Specifying the address and port of the server
    // inet_pton converts the string representation of the server's internet address
    // - which is in dotted-quad notation into 32-bit binary representation

    int rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
    if(rtnVal == 0)
        DieWithUserMessage("inet_pton() failed", "invalid address string");
    if(rtnVal < 0)
        DieWithSystemMessage("inet_pton() failed");
    // htons : host representation to network representation
    // it makes sure that the number is represented in the way that network works with
    // and the API accepts
    servAddr.sin_port = htons(servPort);
    /** To be modified later, here we are establishing an echo connection to
     * and echo server */
    if(connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        DieWithSystemMessage("connect() failed");
    // Local internet address and port no. in client are determined at this call to connect, we didn't use bind
    // connect will do it for us

    // Now we'll read the requests
    while (getline(file, line)) {
        istringstream iss(line);
        string requestType, filePath, hostName;
        iss >> requestType >> filePath >> hostName;

        // Send the request to the server
        string request = requestType + " " + filePath + " " + hostName;
        cout << request << endl;
        // Here is the problem, we need to send the body
        ssize_t numBytes = send(sock, request.c_str(), request.length(), 0);
        if (numBytes < 0) {
            DieWithSystemMessage("send() failed");
        }

        // Receive and print the server's response
        char buffer[BUFSIZ];
        /** This is the problem*/
        ssize_t numBytesRcvd = recv(sock, buffer, BUFSIZ, 0);
        cout << "Suspected" << endl;

        if (numBytesRcvd < 0) {
            DieWithSystemMessage("recv() failed");
        }
        cout << "LOL" << endl;

        buffer[numBytesRcvd] = '\0';

        if (strncmp(buffer, "HTTP/1.1 200 OK", 15) == 0) {
            // Server responded with 200 OK, receive and save the file
            if(requestType == "client_get"){
                cout << "Received 200 OK. Receiving file..." << endl;

                // Extracting the filename from the original path
                size_t lastSlashPos = filePath.find_last_of('/');
                string filename = (lastSlashPos != string::npos) ? filePath.substr(lastSlashPos + 1) : filePath;

                // Appending "_received" to the filename
                size_t dotPos = filename.find_last_of('.');
                string newFilename = filename.substr(0, dotPos) + "_received" + filename.substr(dotPos);
                ReceiveFile(sock, newFilename.c_str());
                cout << "File received and saved as: " << newFilename << endl;

            }else{
                ifstream file(filePath);
                if (file.is_open()){
                    while (getline(file, line)) {
                        send(sock, line.c_str(), strlen(line.c_str()), 0);
                      //  cout << "Read from file: " << line << endl;
                    }
                    send(sock, "\r\n", strlen("\r\n"), 0);
                }else {
                    cout << "Wrong file" << endl;
                    // Lazem yob2a mwgod
                }
            }
            } else {
            // Server responded with an error
            cout << "Server response: " << buffer << endl;
        }
    }

    fputc('\n', stdout);
    close(sock);
    exit(0);


}
