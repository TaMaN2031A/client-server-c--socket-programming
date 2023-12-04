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
     //   while(recv(clntSocket, buffer, BUFSIZ, 0));
    } else {
        // send him error
        //  DieWithSystemMessage("Error opening file for writing.");
    }
    outputFile.close();
    cout << "File Closed" << endl;
}

int main(int argc, char *argv[])
{
    if(argc < 2 || argc > 3)
        DieWithUserMessage("Parameter(s)", "<Server Address> [<Server Port>]");
    char *servIP = argv[1];
    in_port_t servPort = (argc == 3) ? atoi(argv[2]) : 8080;
    const char *inputFile = "in.txt";
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
    char buffer[BUFSIZ];
    // Now we'll read the requests




    while (getline(file, line)) {
        istringstream iss(line);
        string requestType, filePath, hostName;
        iss >> requestType >> filePath >> hostName;
        /** b5tabero bas ya kareem fr 7etet el timeout*/
      //  this_thread::sleep_for(chrono::seconds(5));
        // Format the common header of request
        string request = requestType + " " + filePath + " " + hostName;
        cout << request << endl;
        // Here is the problem, we need to send the body
        if(requestType == "client_post"){
            ifstream file(filePath, std::ios::binary);
            if (file.is_open()) {
                // Get the size of the file
                file.seekg(0, ios::end);
                streampos fileSize = file.tellg();
                file.seekg(0, ios::beg);
                request += " " + to_string(fileSize);
                ssize_t numBytes = send(sock, request.c_str(), request.length(), 0);
                if (numBytes < 0) {
                    DieWithSystemMessage("send() failed");
                }
                ssize_t numBytesRcvd = recv(sock, buffer, 17, MSG_WAITALL);
                // 15 3a4an e7na 3arfeen 7agm el accept w reject
                // 7agmohom 15
                if(numBytesRcvd < 0){
                    DieWithSystemMessage("send() failed");
                }
                cout<<buffer;
                cout << "Suspected" << endl;
                if (strncmp(buffer, "HTTP/1.1 200 OK", 15) == 0){
                    cout << "Received 200 OK. Sending File..." << endl;
                    vector<char> buff(fileSize);
                    file.read(buff.data(), fileSize);
                    file.close();

                    // Send the file data
                    send(sock, buff.data(), fileSize, 0);
                    //Stop the client from sending more files till server tells him go
                    recv(sock, buffer, BUFSIZ, 0);
                }else{
                    cout << "HTTP/1.1    404" << endl;
                }

                // Read the entire file into a vector

            } else {
                std::cout << "Failed to open file: " << filePath << std::endl;
                DieWithSystemMessage("Failed to open file: ");
            }
        }else {
                ssize_t numBytesSnt = send(sock, request.c_str(), request.length(), 0);
                cout << "l" << endl;
                if(numBytesSnt < 0){
                    DieWithSystemMessage("send() failed");
                }

                ssize_t numBytesRcvd;
                cout << "Protect " << endl;
                numBytesRcvd = recv(sock, buffer, 35, MSG_WAITALL);
                puts(buffer);
                cout << "this is the buffer" << endl;
                if (strncmp(buffer, "HTTP/1.1 200 OK", 15) == 0){
                    cout << "Received 200 OK. Receiving file..." << endl;
                    //  chrono::seconds  duration(6);
                    //  this_thread::sleep_for(duration);
                    // Extracting the filename from the original path
                    istringstream iss(buffer);
                    string a; int fileSize;
                    iss >> a >> a >> a >> fileSize;

                    size_t lastSlashPos = filePath.find_last_of('/');
                    string filename = (lastSlashPos != string::npos) ? filePath.substr(lastSlashPos + 1) : filePath;

                    // Appending "_received" to the filename
                    size_t dotPos = filename.find_last_of('.');
                    string newFilename = filename.substr(0, dotPos) + "_received" + filename.substr(dotPos);
                    ReceiveFile_And_Write(sock,newFilename, fileSize);
                    send(sock, "ok", 2, 0);
                    // send ok to make user send the next request
                    cout << "File received and saved as: " << newFilename << endl;

                }else{
                    cout << "HTTP/1.1 404 Not Found\r\n";
                }
        }
        cout<<"end1\n";
    }
    cout<<"while done";
    fputc('\n', stdout);
    close(sock);
    exit(0);


}