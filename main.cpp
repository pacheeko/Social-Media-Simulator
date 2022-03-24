/*
Braydon Pacheco
30100449
CPSC559 Iteration 2
March 6, 2022
*/
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
#include <atomic>
#include <mutex>
#include <unistd.h>
#include <algorithm>
#include "peer.cpp"

// Macros
#define SERVER_PORT 55921
#define PEER_PORT 55922
//#define SERVER_IP "169.254.126.124"
#define SERVER_IP "136.159.5.27"
//#define SERVER_IP "50.99.205.82"
#define SIZE_OF_BUF 1000
#define TEAMNAME "Pacheco\n"

// Data structures
char input[SIZE_OF_BUF], output[SIZE_OF_BUF], outputCopy[SIZE_OF_BUF], largeBuffer[8000];
char* line;
int sock_fd, udp_sockfd;
typedef struct sockaddr_in sockaddr_in;
sockaddr_in server;
atomic <bool> finished;
string snip_string = "", peer_string = "", ipAddress;
mutex mutex_snip, mutex_peer;
struct sockaddr_in servaddr, cliaddr;

// Functions
void sendCode();
void recievePeers(char * peers);
string createReport();
string getIPAddress();
void registryConnect();
void peerConnect();
void setSnipString(string str);
void setPeerString(string str);
string getDate();
int findPeer(vector<Peer> vec, Peer peer);
void sendPeerMessages(string message);
string getTimestamp();

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
    finished = false;
    
    if(argc != 2) {
        fprintf(stderr,"This program accepts exactly one argument: the public ip address used to connect to the registry.\n");
        exit(EXIT_FAILURE);
    }

    ipAddress = string(argv[1]);
    // Connect to the registry to gain the original list of peers, and let
    // the registry know that we are connected
    registryConnect();
    // Keep a UDP port open to send and recieve messages to/from other peers
    peerConnect();
    cout << "done with peers" << endl;
    finished = false;
    // Connect to the registry after given the stop command, signalling that 
    // the registry is shutting down
    registryConnect();

    return 1;
}

/*
Continues looping and receiving messages through a TCP connection
to the registry. When the registry sends the message 'close\n', the 
loop will break and the function will return
*/
void registryConnect() {
    // Set up socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        puts("Failed to create client socket");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    //Connect to remote server
    cout << "trying to connect" << endl;
    int error = connect(sock_fd , (struct sockaddr *)&server , sizeof(server));
	if (error < 0) 
	{
        cout << "Error: " << error << endl;
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
            strcat(input, getIPAddress().c_str());
            strcat(input, ":");
            strcat(input, to_string(PEER_PORT).c_str());
            strcat(input, "\n");
            cout << "Sending location: " << input << endl;
            string in = string(input);
            if (send(sock_fd, in.c_str(), in.size(), 0) < 0) {
                close(sock_fd);
                puts("Failed to send team name to server");
                exit(EXIT_FAILURE);
            }
        }
        // If the response is "get code\n", send the language (cpp), followed by the code in this file,
        // followed by three ellipses to indicate the end of the file
        else if (strcmp(line,"get code") == 0) {
            sendCode();
        }
        // If the response is "receive peers", create or append the file 'peers.txt' to include 
        // a new line in the form Source IP:Port,Date,# of Peers, Peer data (multiple peers possible)
        else if (strcmp(line,"receive peers") == 0) {

            //Send another recv request to the server to recieve the peers
            //The initial request only sends the line "receive peers"
            
            memset(output, 0, sizeof(output));
            memset(input, 0, sizeof(input));
            ssize_t result = 1;
            while (result > 0) {
                result = recv(sock_fd, input, sizeof(input), MSG_WAITALL);
                cout << "test: " << result << "\n----" << string(input) << endl;
                strcat(output, input);
                memset(input, 0, sizeof(input));
            }
            cout << output << endl;
            cout << "Adding new peers from source: " << SERVER_IP << ":" << to_string(SERVER_PORT) << "." <<endl;
            recievePeers(output);

        }
        // If the response is "get report\n", create a report of all unique peers and all sources (including duplicates)
        // that are found in the "peers.txt file"
        else if (strcmp(line, "get report") == 0) {
            cout << "Creating report" << endl;
            string report = createReport();
            string newReport = "123456\n";
            newReport.append(report);
            cout << "Sending Report." << endl;
            memset(input, 0, sizeof(input));
            strcpy(input, "report\n");
            int i = send(sock_fd, newReport.c_str(), newReport.size(), 0);
            if (i < 0) {
                close(sock_fd);
                puts("Failed to send report to server");
                exit(1);
            }
            
        }
        // If the response is "close\n", simply close the server connection and break from the loop
        else if (strcmp(line,"close") == 0) {
            cout << "Closing server connection" << endl;
            break;
        }
        else if (strcmp(line,"stop") == 0) {
            if (send(sock_fd, "test\n", 6, 0) < 0) {
                close(sock_fd);
                puts("Failed to send report to server");
                exit(1);
            }
        }
        else {
            cout << "Bad Server Response" << endl;
        }
    }
    
    close(sock_fd);
}

/*
Opens up a UDP server to listen for requests from other peers.
When a request comes in, it signals to different threads in order
to process the request
*/
void peerConnect() {
    cout << "Listening for peers"<< endl;
            
    // Set up UDP Server to listen for peer messages
    // *** Some code taken from https://www.geeksforgeeks.org/udp-server-client-implementation-c/ ***

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
    int n;

        thread snip(&processSnip);
        thread peer(&processPeer);
        thread menu(&endUserMenu);
        thread send(&sendPeersPeriodically);

    setSnipString("");
    setPeerString("");
    while(!finished) {
        cout << "UDP Server listening.." << endl;
        socklen_t len = sizeof(cliaddr);  

        n = recvfrom(udp_sockfd, (char *)input, SIZE_OF_BUF, 
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len);
        cout << "test" << endl;
        string message = string(input);
        cout << "New UDP Message:\n" << message << endl;
        string line = message.substr(0,4);
        char * ipString = inet_ntoa(cliaddr.sin_addr);
        string portString = to_string(cliaddr.sin_port);
        if ((line.compare("stop") == 0) || finished) {
            finished = true;
            cout << "Waiting for threads to finish execution.." << endl;
            snip.join();
            peer.join();
            menu.join();
            send.join();
            sendPeerMessages("stop");
            cout << "Initiating shutdown procedure." << endl;
            break;
        }

        else if (line.compare("snip") == 0) {
            cout << "testsnip" << endl;
            message.append(",");
            message.append(ipString);
            message.append(":");
            message.append(portString);
            while (!snip_string.empty()) {
                sleep(2);
            }
            setSnipString(message.substr(4, message.size() - 4));
        }

        else if (line.compare("peer") == 0) {
            cout << "testpeer" << endl;
            message.append(",");
            message.append(ipString);
            message.append(":");
            message.append(portString);
            while (!peer_string.empty()) {
                sleep(2);
            }
            setPeerString(message.substr(4, message.size() - 4));
        }

        else {
            cout << "Bad UDP request recieved." << endl;
        }
    }
}

/* 
Iterates through the main.cpp file as well as the peer.cpp file, 
sending 8000 bits at a time to the registry server through TCP.
Sends three elipsses in a row followed by a new line, to signal that
the code is finished sending.
*/
void sendCode() {
     // ** some code taken from https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring and modified**
        cout << "Sending code" << endl;
        ifstream codeFile;
        int length, delim = 0;
        codeFile.open("main.cpp");      // open input file
        memset(largeBuffer, 0, sizeof(largeBuffer));
        strcpy(largeBuffer, "cpp\nStart of main.cpp\n");
        if (send(sock_fd, largeBuffer, sizeof(largeBuffer), 0) < 0) {
            close(sock_fd);
            puts("Failed to send code to server");
            exit(EXIT_FAILURE);
        }
        stringstream strStream;
        strStream << codeFile.rdbuf();         // read file into the stringstream
        string str = strStream.str();
        while (delim < str.size()) {
            memset(largeBuffer, 0, sizeof(largeBuffer));
            strcpy(largeBuffer, str.substr(delim, 8000).c_str());
            if (send(sock_fd, largeBuffer, sizeof(largeBuffer), 0) < 0) {
                close(sock_fd);
                puts("Failed to send code to server");
                exit(EXIT_FAILURE);
            }
            delim += 8000;
        }
        length = delim - str.size();
        memset(largeBuffer, 0, sizeof(largeBuffer));
        strcpy(largeBuffer, str.substr(delim-8000, length).c_str());
        strcat(largeBuffer,"\nStart of peer.cpp\n");
        if (send(sock_fd, largeBuffer, sizeof(largeBuffer), 0) < 0) {
                close(sock_fd);
                puts("Failed to send code to server");
                exit(EXIT_FAILURE);
        }
        codeFile.close();

        codeFile.open("peer.cpp");
        strStream << codeFile.rdbuf();
        str = strStream.str();
        delim = 0;
        while (delim < str.size()) {
            memset(largeBuffer, 0, sizeof(largeBuffer));
            strcpy(largeBuffer, str.substr(delim, 8000).c_str());
            if (send(sock_fd, largeBuffer, sizeof(largeBuffer), 0) < 0) {
                close(sock_fd);
                puts("Failed to send code to server");
                exit(EXIT_FAILURE);
            }
            delim += 8000;
        }
        length = delim - str.size();
        memset(largeBuffer, 0, sizeof(largeBuffer));
        strcpy(largeBuffer, str.substr(delim-8000, length).c_str());
        strcat(largeBuffer, "\n..");
        strcat(largeBuffer, ".\n");
        if (send(sock_fd, largeBuffer, sizeof(largeBuffer), 0) < 0) {
                close(sock_fd);
                puts("Failed to send code to server");
                exit(EXIT_FAILURE);
        }
        codeFile.close();                   // close file
}

/*
Receives the list of original peers from the registry, and adds these
peers into into the allPeers vector and the recievedFromRegistry vector.
*/
void recievePeers(char * peers) {
    char * line = strtok(peers, "\n");
    cout << "Adding " << string(line) << " new peer(s)." << endl;
    string date = getDate();
    Peer source(string(SERVER_IP) + ":" + to_string(SERVER_PORT));
    line = strtok(NULL, "\n");
    //Add all peers recieved from the registry to the vector 'recievedPeers'
    while (line != NULL) {
        cout << line << endl;
        if (strcmp(line, "close") == 0) {
            finished = true;
            return;
        }
        Peer newPeer = Peer(string(line)); 
        allPeers.push_back(newPeer);
        RecdPeer recPeer(source, newPeer, date);
        recievedFromRegistry.push_back(recPeer);
        cout << "Peer added: " << recPeer.toString() << endl;
        line = strtok(NULL, "\n");
    }
}
/*
Encapsulates all information about peers and snippets into one string
and returns that string.
*/
string createReport() {
    string strOutput = "", temp = "", date = "invalid date";
    strOutput.append(to_string(allPeers.size())).append("\n");
    for (Peer peer : allPeers) {
        strOutput.append(peer.getAddress()).append("\n");
    }
    strOutput.append("1\n");
    string registryAddress = SERVER_IP;
    registryAddress.append(":").append(to_string(SERVER_PORT));
    strOutput.append(registryAddress).append("\n");
    for (RecdPeer peer : recievedFromRegistry) {
        temp.append(peer.getRecd().getAddress()).append("\n");
        date = peer.getDate();
    } 
    strOutput.append(date).append("\n");
    strOutput.append(to_string(recievedFromRegistry.size())).append("\n");
    strOutput.append(temp);
    strOutput.append(to_string(recievedPeers.size())).append("\n");
    for (RecdPeer peer : recievedPeers) {
        strOutput.append(peer.toString()).append("\n");
    }
    strOutput.append(to_string(sentPeers.size())).append("\n");
    for (SentPeer peer : sentPeers) {
        strOutput.append(peer.toString()).append("\n");
    }
    strOutput.append(to_string(snippets.size())).append("\n");
    for (Snippet snip : snippets) {
        strOutput.append(snip.toString()).append("\n");
    }
    return strOutput;
}

/*** Code taken from https://gist.github.com/quietcricket/2521037 ***
* Gets the IP address of the current machine and returns its as a string 
*/
string getIPAddress(){
    /*
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
    */
    return ipAddress;
}

//Thread functions
/*
Continues looping until the finished variable is set to true, then returns.
Keeps looking at the snip_string variable to see if there is a snip to process.
If snip_string is not empty, it will process this snip, adding it to the snippets vector.
*/
void processSnip() {
    string info, message;
    int timestamp, delim1, delim2;

    while (!finished) {
        if (snip_string.empty()) {
            sleep(5);
        }
        else {
            cout << "Processing snip request." << endl;
            info = snip_string;
            delim1 = (int) info.find(" ");
            timestamp = stoi(info.substr(0, delim1));
            delim2 = (int) info.find_last_of(",");
            Peer source(info.substr(delim2+1, info.size() - delim2));
            message = info.substr(delim1, info.size() - delim1 - delim2);
            Snippet snip(timestamp, message, source, getDate());
            snippets.push_back(snip);
            cout << "Sucessfully added snippet: " << snip.toString() << endl;
            setSnipString("");
        }
    }
}
/*
Continues looping until the finished variable is set to true, then returns.
Keeps looking at the peer_string variable to see if there is a peer to process.
If peer_string is not empty, it will process this peer, adding it to the recievedPeers vector.
*/
void processPeer() {
    string info;
    int delim;
    int it;
    while (!finished) {
        if (peer_string.empty()) {
            sleep(5);
        }
        else {
            cout << "Processing peer request." << endl;
            info = peer_string;
            info.erase(std::remove_if(info.begin(), info.end(), ::isspace), info.end());
            delim = info.find(",");
            if (delim < 10) {
                cerr << "Bad peer. Failed to add." << endl;
                setPeerString("");
            }
            else {
                Peer newPeer(info.substr(0, delim));
                Peer sourcePeer(info.substr(delim+1, info.size()));
                RecdPeer recdpeer(sourcePeer, newPeer, getDate());
                recievedPeers.push_back(recdpeer);
                it = findPeer(allPeers, newPeer);
                if (it != -1) {
                    cout << "Peer already in system: " << newPeer.getAddress() << endl;
                } else {
                    allPeers.push_back(newPeer);
                    cout << "Successfully added peer: " << newPeer.getAddress() << endl;
                }
                it = findPeer(allPeers, sourcePeer);
                if (it != -1) {
                    cout << "Peer already in system: " << sourcePeer.getAddress() << endl;
                    allPeers.at(it).setActive();
                } else {
                    allPeers.push_back(sourcePeer);
                    cout << "Successfully added peer: " << sourcePeer.getAddress() << endl;
                }
                setPeerString("");
            }
        }
    }
}
/*
Menu for the end user. The user can type 1 and then type a message they would like to send.
The user can type 2 to see a current report of users and snippets.
*/
void endUserMenu() {
    string menuMsg = "Welcome to the peer menu.\nType '1' to send your own message to other peers.\nType '2' to print out a the current report\nType anything else to bring back the menu screen.";
    string message;
    char cinput[256];
    string timestamp;
    string sourceAddr = getIPAddress().append(":").append(to_string(PEER_PORT));
    while (!finished) {
        cout << menuMsg << endl;
        cin.getline(cinput, 256);
        if (strcmp(cinput, "1") == 0) {
            cout << "Please type your message now:" << endl;
            cin.getline(cinput, 256);
            message = "snip";
            timestamp = getTimestamp();
            message.append(timestamp).append(" ").append(cinput);
            Peer source(sourceAddr);
            Snippet newSnip(stoi(timestamp), cinput, source ,getDate());
            snippets.push_back(newSnip);
            sendPeerMessages(message);
        }
        else if (strcmp(cinput, "2") == 0) {
            cout << "Creating report.."  << endl;
            message = createReport();
            cout << message << "\nEnd of report." << endl;
        }
        // For debugging
        else if (strcmp(cinput, "stop") == 0) {
            finished = true;
            cout << "stopping" << endl;
            sendPeerMessages(string("stop"));
            return;
        }
        else {
            cout << "Invalid input. Returning to menu." << endl;
        }
    }
}
/*
* Every 10 seconds, this thread will send a random peer address to all other active
* peers that it knows.
*/
void sendPeersPeriodically() {
    string peerToSend, message;
    int iter;
    while (!finished) {
        srand(time(NULL));
        iter = rand() % allPeers.size();
        if (allPeers.size() > iter) {
            peerToSend = allPeers[iter].getAddress();
            message = "peer";
            message.append(peerToSend);
            sendPeerMessages(message);
        }
        sleep(10);
    }
}

/*
Takes a string as input, and then sends this string to all active peers in the 
allPeers vector.
*/
void sendPeerMessages(string message) {
    struct sockaddr_in destAddr;
    string sourceAddr = getIPAddress().append(":").append(to_string(PEER_PORT));
    Peer source(sourceAddr);
    for (Peer peer : allPeers) {
        if (peer.active()) {
            destAddr.sin_family = AF_INET;
            int port = stoi(peer.getPort());
            destAddr.sin_port = htons(port);
            destAddr.sin_addr.s_addr = inet_addr(peer.getIP().c_str());
            SentPeer sent(source, peer, message);
            if (sendto(udp_sockfd, message.c_str(), message.size(), 0, (struct sockaddr *) &destAddr, sizeof(destAddr)) < 0) {
                cerr << "Error sending peer message" << endl;
            }
            else {
                cout << "Successfully sent message: " << message << "  to: " << peer.getAddress() << endl;
                sentPeers.push_back(sent);
            }
        }
        else {
            cout << "Peer: " << peer.getAddress() << " is not active." << endl;
        }
    }
}

/*
Setter functions for thread shared variables.
Mutexes are used to ensure no race conditions.  
*/
void setSnipString(string str) {
    mutex_snip.lock();
    snip_string = str;
    mutex_snip.unlock();
};

void setPeerString(string str) {
    mutex_peer.lock();
    peer_string = str;
    mutex_peer.unlock();
};

// Returns the current date and time as a string
string getDate() {
    // ** some code used from https://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c to get current time **
    time_t time = std::time(0);
    std::tm* now = std::localtime(&time);
    string date = to_string(now->tm_year + 1900) + "-" + 
                  to_string(now->tm_mon + 1) + "-" + 
                  to_string(now->tm_mday) + " " + 
                  to_string(now->tm_hour) + ":" + 
                  to_string(now->tm_min) + ":" + 
                  to_string(now->tm_sec);
    return date;
}

/* 
Takes a vector of Peers and a peer as input.
Returns -1 if that peer does not exist in vec. Otherwise,
returns the location of that peer in the vector given
*/
int findPeer(vector<Peer> vec, Peer peer) {
    for (int i = 0; i < vec.size(); i++) {
        if (vec[i].equals(peer)) {
            return i;
        }
    }
    return -1;
}

/*
Used when sending out a timestamp. Finds the highest timestamp in the 
current list of snippets, adds 1, and then returns that number.
*/
string getTimestamp() {
    int current = 0;
    for (Snippet snip : snippets) {
        if (snip.getTimestamp() >= current) {
            current = snip.getTimestamp() + 1;
        }
    }
    return to_string(current);
}