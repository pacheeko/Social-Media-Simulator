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


//Classes to keep track of Peers, Snippets, and UDP messages sent between peers
class Peer {

    private:
        string ip;
        string port;

    public:
        Peer(string addr) {
            int sep = addr.find(":");
            ip = addr.substr(0, sep);
            port = addr.substr(sep + 1, addr.size() - ip.size() - 1);
        }

        void setAddress(string addr) {
            int sep = addr.find(":");
            ip = addr.substr(0, sep);
            port = addr.substr(sep + 1, addr.size() - ip.size() - 1);
        }

        string getAddress() {
            return ip + ":" + port;        }

        string getIP() {
            return ip;
        }

        string getPort() {
            return port;
        }
};

class Snippet {
    private:
        int timepoint;
        string message;
        Peer source;

    public:
        Snippet(int t, string m, Peer p) : timepoint(t), message(m), source(p) {};

        string toString() {
            return timepoint + " " + message + " " + source.getAddress();
        }
};

class RecdPeer {

    private:
        Peer source;
        Peer recd;
        string date;

    public:
        RecdPeer(Peer s, Peer r, string d) : source(s), recd(r), date(d) {};

        string toString() {
            return "Source: " + source.getAddress() + " -- Recieved: " + recd.getAddress() + " -- Date: " + date;
        }

};

class SentPeer {
    private:
        Peer dest;
        Peer sent;
        string date;

    public:
    SentPeer(Peer p, Peer s, string d) : dest(p), sent(s), date(d) {};

    string toString() {
        return "Source: " + dest.getAddress() + " -- Recieved: " + sent.getAddress() + " -- Date: " + date;
    }
};

//Global data structures to keep track of peer information

vector<Snippet> snippets;
vector<RecdPeer> recievedPeers;
vector<RecdPeer> activePeers;
//Thread function definitions