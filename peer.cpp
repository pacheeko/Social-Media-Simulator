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
#include<chrono>


//Classes to keep track of Peers, Snippets, and UDP messages sent between peers
class Peer {

    private:
        string ip;
        string port;
        int seconds_since_active;

    public:
        Peer(string addr) {
            int sep = addr.find(":");
            ip = addr.substr(0, sep);
            port = addr.substr(sep + 1, addr.size() - ip.size() - 1);
            setActive();
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

        void setActive() {
            auto temp = std::chrono::system_clock::now().time_since_epoch().count();
            seconds_since_active = (int) temp;
        }

        bool active() {
            int current_time;
            auto temp = std::chrono::system_clock::now().time_since_epoch().count();
            current_time = (int) temp;
            if (current_time - seconds_since_active > 10) {
                return false;
            } else {
                return true;
            }
        }

        bool equals(Peer p) {
            if ((p.getIP().compare(this->ip) == 0) && (p.getPort().compare(this->port) == 0)) {
                return true;
            } else {
                return false;
            }
        }

};

class Snippet {
    private:
        int timestamp;
        string message;
        Peer source;
        string date;

    public:
        Snippet(int t, string m, Peer p, string d) : timestamp(t), message(m), source(p), date(d) {};

        string toString() {
            return to_string(timestamp) + " " + message + " " + source.getAddress();
        }

        int getTimestamp() {
            return timestamp;
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
            return source.getAddress() + " " + recd.getAddress() + " " + date;
        }

        Peer getSource() {
            return source;
        }

        Peer getRecd() {
            return recd;
        }

        string getDate() {
            return date;
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
        return dest.getAddress() + " " + sent.getAddress() + " " + date;
        }
};

//Global data structures to keep track of peer information

vector<Snippet> snippets;
vector<RecdPeer> recievedPeers;
vector<SentPeer> sentPeers;
vector<Peer> allPeers;
vector<RecdPeer> recievedFromRegistry;
