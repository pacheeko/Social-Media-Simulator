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

// Macros
#define SERVER_PORT 55921
#define SERVER_IP "192.168.1.75"
//#define SERVER_IP "136.159.5.22"
#define SIZE_OF_BUF 1000
#define TEAMNAME "Pacheco\n"

// Data structures
char input[SIZE_OF_BUF], output[SIZE_OF_BUF], largeBuffer[10000];
char* line;
int sock_fd;
typedef struct sockaddr_in sockaddr_in;
sockaddr_in server;

// Functions
void sendCode();
void recievePeers(char * peers);
void sendReport();

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

    //Connect to remote server
	if (connect(sock_fd , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
        close(sock_fd);
		puts("connect error");
		return 1;
	}
    puts("Connected to server");
    //Continue looping until the server says to close the connection
    while(1) {
        memset(output, 0, sizeof(output));
        cout << "Waiting for server.." << endl;
        if ( recv(sock_fd, output, sizeof(output), 0) < 0) {
                close(sock_fd);
                puts("Failed to recieve from server");
                return 1;
            }
        cout << "Incoming message: " << string(output) << endl;
        line = strtok(output,"\n");

        // If response is "get team name\n", simply send the TEAMNAME back to the server.
        // The team name is 'Pacheco'.
        if (strcmp(line,"get team name") == 0) {
            cout << "Sending team name" << endl;
           
            memset(input, 0, sizeof(input));
            strcpy(input, TEAMNAME);

            if (send(sock_fd, input, sizeof(input), 0) < 0) {
                close(sock_fd);
                puts("Failed to send team name to server");
                return 1;
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

            memset(output, 0, sizeof(output));
            //Send another recv request to the server to recieve the peers
            //The initial request only sends the line "receive peers"
            if ( recv(sock_fd, output, sizeof(output), 0) < 0) {
                close(sock_fd);
                puts("Failed to recieve from server");
                return 1;
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
    return 1;
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
        codeFile.open("Iteration1.cpp");      // open input file
        stringstream strStream;
        strStream << codeFile.rdbuf();         // read file into the stringstream
        string str = strStream.str();
        memset(largeBuffer, 0, sizeof(largeBuffer));
        strcpy(largeBuffer, "cpp\n");
        strcat(largeBuffer, str.c_str());
        strcat(largeBuffer, "\n...\n");
        if (send(sock_fd, largeBuffer, sizeof(largeBuffer), 0) < 0) {
            close(sock_fd);
            puts("Failed to send language to server");
            exit(1);
        }
        codeFile.close();                   // close file
}

/*
Adds the cource ip and port, date, number of peers, and peer data into the file peers.txt.
These items are added in comma separated form, to allow for easy readability.
ofstream file data structure is used to write to the file due to its thread safe functionality.
*/
void recievePeers(char * peers) {
    char * line = strtok(peers, "\n");
    cout << "Adding " << line << " new peer(s)." << endl;

    // ** some code taken from https://stackoverflow.com/questions/2393345/how-to-append-text-to-a-text-file-in-c **
    ofstream file;
    file.open("peers.txt", ios::out | ios::app);
    string output = "";
    string numOfPeers = line;
    numOfPeers.append(",");
    string address = SERVER_IP;
    address.append(":").append(to_string(SERVER_PORT)).append(",");
    output.append(address);

    // ** some code used from https://stackoverflow.com/questions/997946/how-to-get-current-time-and-date-in-c to get current time **
    time_t time = std::time(0);
    std::tm* now = std::localtime(&time);
    string date = to_string(now->tm_year + 1900) + "-" + to_string(now->tm_mon + 1) + "-" + to_string(now->tm_mday) + " " + to_string(now->tm_hour) + ":" + to_string(now->tm_min) + ":" + to_string(now->tm_sec) + ",";
    output.append(date);
    output.append(numOfPeers);
    line = strtok(NULL, "\n");
    string temp = string(line);
    //Server sometimes sends the next request at the end of the current message.
    //This if statement is used to ensure that request is processed 
    if (temp.compare("get report") == 0) {
        sendReport();
        line = NULL;
    }
    // Loop through all lines in the request, adding each one to the output string
    // separated by a comma. Then adds the output string to the file followed by
    // a new line character. The file is then closed.
    while (line != NULL) {
        output.append(line);
        output.append(",");
        line = strtok(NULL, "\n");
        if (line != NULL) {
            temp = string(line);
            if ((temp.compare("get report")) == 0) {
                file << output;
                file << "\n";
                file.close();
                cout << "Incoming message: get report" << endl;
                sendReport();
                return;
            }
        }
    }
    file << output;
    file << "\n";
    file.close();
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