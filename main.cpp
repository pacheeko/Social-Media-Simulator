using namespace std;

#include<stdio.h>
#include<string.h>	//strlen
#include<unistd.h> //close, fork
#include<stdlib.h> //exit, rand
#include<sys/socket.h> //connect, send, recv
#include<arpa/inet.h>	//inet_addr
#include<stdbool.h> //bool
#include<ctype.h>
#include<string>
#include<iostream>
#include<fstream>
#include<vector>
#include<ctime>
#include<set>
#include<sstream>
#include<thread>
#include <sys/types.h>
#include <ifaddrs.h>
#include "peer.cpp"

// Macros
#define SERVER_PORT 55921
#define PEER_PORT 55922
#define SERVER_IP "136.159.5.22"
//#define SERVER_IP "192.168.1.82"
#define SIZE_OF_BUF 1000
#define TEAMNAME "Pacheco\n"

// Data structures
char input[SIZE_OF_BUF], output[SIZE_OF_BUF], outputCopy[SIZE_OF_BUF], largeBuffer[8000];
char* line;
int sock_fd;
typedef struct sockaddr_in sockaddr_in;
sockaddr_in server;
bool finished = false;

// Functions
void sendCode();
void recievePeers(char * peers);
void sendReport();
string getIPAddress();
void registryConnect();
void peerConnect();
// Thread Functions
void processSnip();
void processPeer();
void endUserMenu();
void sendPeersPeriodically();

int main(int argc, char** argv) {
    /*
    Connects to the server specified in the macro SERVER_IP at port SERVER_PORT.
    Gives a response to the server based on the request sent by the server.
    */

    if(argc < 1) {
        fprintf(stderr,"This program does not accept args\r\n");
        return 1;
    }

    // Set up socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        puts("Failed to create client socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Connect to the registry to gain the original list of peers, and let
    // the registry know that we are connected
    registryConnect();

    // Keep a UDP port open to send and recieve messages to/from other peers
    peerConnect();

    // Connect to the registry after given the stop command, signalling that 
    // the registry is shutting down
    registryConnect();

    return 1;
}

void registryConnect() {
    //Connect to remote server
	if (connect(sock_fd , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
        close(sock_fd);
		puts("connect error");
		exit(EXIT_FAILURE);
	}
    puts("Connected to server");
    //Continue looping until the server says to close the connection
    while(1) {
        memset(output, 0, sizeof(output));
        if (finished == true) {
            finished = false;
            strcpy(output, "close\n");
        }
        else {
            cout << "Waiting for server.." << endl;
            if ( recv(sock_fd, output, sizeof(output), 0) < 0) {
                    close(sock_fd);
                    puts("Failed to recieve from server");
                    exit(EXIT_FAILURE);
                }
            cout << "Incoming message: " << string(output) << endl;
        }
        strcpy(outputCopy, output);
        line = strtok(outputCopy,"\n");
        // If response is "get team name\n", simply send the TEAMNAME back to the server.
        // The team name is 'Pacheco'.
        if (strcmp(line,"get team name") == 0) {
           
            memset(input, 0, sizeof(input));
            strcpy(input, TEAMNAME);
            cout << "Sending team name: " << input << endl;
            if (send(sock_fd, input, sizeof(input), 0) < 0) {
                close(sock_fd);
                puts("Failed to send team name to server");
                exit(EXIT_FAILURE);
            }

        }
        // Send location
        else if (strcmp(line, "get location") == 0) {

            memset(input, 0, sizeof(input));
            strcpy(input, getIPAddress().c_str());
            strcat(input, ":");
            strcat(input, to_string(PEER_PORT).c_str());
            cout << "Sending location: " << input << endl;
            strcat(input, "\n");
            string in = string(input);
            if (send(sock_fd, input, in.size(), 0) < 0) {
                close(sock_fd);
                puts("Failed to send team name to server");
                exit(EXIT_FAILURE);
            }


        }
        // If the response is "get code\n", send the language (cpp), followed by the code in this file,
        // followed by three ellipses to indicate the end of the file
        else if (strcmp(line,"get code") == 0) {
           //sendCode();
            memset(input, 0, sizeof(input));
            string str = "cpp\nThis is my code!\n...\n";
            strcpy(input, str.c_str());
            cout << input << endl;
            if (send(sock_fd, input, sizeof(input), 0) < 0) {
                close(sock_fd);
                puts("Failed to send language to server");
                exit(EXIT_FAILURE);
            }
        }
        // If the response is "receive peers", create or append the file 'peers.txt' to include 
        // a new line in the form Source IP:Port,Date,# of Peers, Peer data (multiple peers possible)
        else if (strcmp(line,"receive peers") == 0) {

            //Send another recv request to the server to recieve the peers
            //The initial request only sends the line "receive peers"
            
            memset(output, 0, sizeof(output));
            if ( recv(sock_fd, output, sizeof(output), 0) < 0) {
                close(sock_fd);
                puts("Failed to recieve from server");
                exit(EXIT_FAILURE);
            }
            
            cout << "Adding new peers from source: " << SERVER_IP << ":" << to_string(SERVER_PORT) << "." <<endl;
            recievePeers(output);

        }
        // If the response is "get report\n", create a report of all unique peers and all sources (including duplicates)
        // that are found in the "peers.txt file"
        else if (strcmp(line, "get report") == 0) {
           cout << "Creating report" << endl;
           sendReport();
        }
        // If the response is "close\n", simply close the server connection and break from the loop
        else if (strcmp(line,"close") == 0) {
            cout << "Closing server connection" << endl;
            break;
        }

        else {
            cout << "Bad Server Response" << endl;
        }
    }
    
    close(sock_fd);
}

void peerConnect() {
    cout << "Connected to Peers"<< endl;
    while(1) {
        
    }
/*
        thread thread_snip, thread_peer;
        // Creates and runs a thread for the end user to use
        thread thread_user (&endUserMenu);
        // Creates and runs a thread that sends information to other peers periodically
        thread thread_send (&sendPeersPeriodically);

        // Set up UDP Server to listen for peer messages
        // *** Some code taken from https://www.geeksforgeeks.org/udp-server-client-implementation-c/ ***
        int udp_sockfd;
        struct sockaddr_in servaddr, cliaddr;
            
        // Creating socket file descriptor
        if ( (udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }
            
        memset(&servaddr, 0, sizeof(servaddr));
        memset(&cliaddr, 0, sizeof(cliaddr));
            
        // Filling server information
        servaddr.sin_family    = AF_INET; // IPv4
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(PEER_PORT);
            
        // Bind the socket with the server address
        if ( bind(udp_sockfd, (const struct sockaddr *)&servaddr, 
                sizeof(servaddr)) < 0 )
        {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        cout << "UDP server running" << endl;
        int n;

        while(1) {

        socklen_t len = sizeof(cliaddr);  //len is value/resuslt

        n = recvfrom(udp_sockfd, (char *)input, SIZE_OF_BUF, 
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);

        string message = string(input);
        string line = message.substr(0,4);

        if (line.compare("stop") == 0) {
            finished = true;
            thread_snip.join();
            thread_peer.join();
            thread_user.join();
            thread_send.join();
            break;
        }

        else if (line.compare("snip") == 0) {
            thread newthread(&processSnip);
        }

        else if (line.compare("peer") == 0) {
            thread newthread(&processPeer);
        }

        else {
            cout << "Bad UDP Request" << endl;
        }
    }
    */
}

/* 
Opens the 'Iteration1.cpp' file using ifstream (thread safe),
then puts 'cpp\ncodefile\n...\n' into the largeBuffer and sends
that back to the server.  
*/
void sendCode() {
     // ** some code taken from https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring and modified**
        cout << "Sending code" << endl;
        ifstream codeFile;
        int length;
        codeFile.open("main.cpp");      // open input file
        stringstream strStream;
        strStream << codeFile.rdbuf();         // read file into the stringstream
        string str = strStream.str();
        memset(largeBuffer, 0, sizeof(largeBuffer));
        strcpy(largeBuffer, "cpp\n");
        //strcat(largeBuffer, str.c_str());
        strcat(largeBuffer, "This is my code!");
        strcat(largeBuffer, "\n...\n");
        if (send(sock_fd, largeBuffer, sizeof(largeBuffer), 0) < 0) {
            close(sock_fd);
            puts("Failed to send language to server");
            exit(EXIT_FAILURE);
        }
        codeFile.close();                   // close file
}

/*

*/
void recievePeers(char * peers) {
    char * line = strtok(peers, "\n");
    cout << "Adding " << string(line) << " new peer(s)." << endl;
    // ** some code used from https://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c to get current time **
    time_t time = std::time(0);
    std::tm* now = std::localtime(&time);
    string date = to_string(now->tm_year + 1900) + "-" + 
                  to_string(now->tm_mon + 1) + "-" + 
                  to_string(now->tm_mday) + " " + 
                  to_string(now->tm_hour) + ":" + 
                  to_string(now->tm_min) + ":" + 
                  to_string(now->tm_sec);
    Peer source(string(SERVER_IP) + ":" + to_string(SERVER_PORT));
    line = strtok(NULL, "\n");
    //Add all peers recieved from the registry to the vector 'recievedPeers'
    while (line != NULL) {
        if (strcmp(line, "close") == 0) {
            finished = true;
            return;
        }
        Peer newPeer = Peer(string(line)); 
        RecdPeer recPeer(source, newPeer, date);
        recievedPeers.push_back(recPeer);
        cout << "Peer added: " << recPeer.toString() << endl;
        line = strtok(NULL, "\n");
    }
}

/* 
Forms a report in the form:
    # Peers in total
    Peers (multiple lines)
    # Sources in total
    Sources (multiple lines)
*/
void sendReport() {
    ifstream file("peers.txt");
    memset(output, 0, sizeof(output));
    // If we have already recieved peers, create a report of the peers received
    // and send that report back to the server
    if (file.good()) {
        string line, token;
        int sources = 0;
        set<string> peers;
        string sourceList;
        //Iterate through each line of the file, adding each value to the sourceList
        //When reading a peer, add the peer to the set peers
        while (getline(file, line)) {
            sources++;
            stringstream stream(line);
            int it = 0;
            while (getline(stream, token, ',')) {
                sourceList.append(token).append("\n");
                cout << it << ": " << token << endl;
                if (it > 2) {
                    peers.insert(token);
                }
                it ++;
            }
        }
        string peerList;
        string numPeers = to_string(peers.size());
        peerList.append(numPeers + "\n");
        for (string p : peers) {
            peerList.append(p + "\n");
        }
        strcpy(output, peerList.c_str());
        strcat(output, to_string(sources).c_str());
        strcat(output, "\n");
        strcat(output, sourceList.c_str());
    }
    // If we have not received any peers, let the server know that
    else {
        strcpy(output, "0 Peers found.\n0 Sources found.\n");
        
    }
    cout << "Sending Report." << endl;
    if (send(sock_fd, output, sizeof(output), 0) < 0) {
        close(sock_fd);
        puts("Failed to send report to server");
        exit(1);
    }
}

//** Code taken from https://gist.github.com/quietcricket/2521037 ***
string getIPAddress(){
    string ipAddress="Unable to get IP Address";
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_addr = NULL;
    int success = 0;
    // retrieve the current interfaces - returns 0 on success
    success = getifaddrs(&interfaces);
    if (success == 0) {
        // Loop through linked list of interfaces
        temp_addr = interfaces;
        while(temp_addr != NULL) {
            if(temp_addr->ifa_addr->sa_family == AF_INET) {
                // Check if interface is en0 which is the wifi connection on the iPhone
                if(strcmp(temp_addr->ifa_name, "en0")){
                    ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
                }
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    // Free memory
    freeifaddrs(interfaces);
    return ipAddress;
}

//Thread functions
void processSnip() {
    cout << "Processing snip request." << endl;
}

void processPeer() {
    cout << "Processing peer request." << endl;
}

void endUserMenu() {
    cout << "Make a menu" << endl;
    while (!finished) {
        //TODO: menu
    }
}

void sendPeersPeriodically() {
    cout << "finish send peers function" << endl;
    while (!finished) {
        //TODO
    }
}