#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <fstream>
#include<cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFFSIZE 512

using namespace std;

uint32_t expected_seqnum;
uint32_t cumm_acked;
bool isBuff[32];        // store the whether the corresponding index sequence is buffered

struct Packet{
    uint32_t type;       // 0:ACK, 1: Data, 2: EOT
    uint32_t segnum;     // Modulo 32
    uint32_t length;     // Length of the String variable 'data', should in the range of 0 to 500
    char data[500];    // String with Max Length 500
};

Packet buffP[32];

struct undefPacket{
    uint32_t type;
    uint32_t segnum;
    uint32_t length;
};

void debug(int i) {
    cerr << "debug: " << i << endl;
}

int convert(string str) {   // convert the port number from string to int
    int len = str.length();
    int ans = 0;
    for(int i = 0; i < len; i++) {
        ans *= 10;
        if(str[i] < 48 && str[i] >57) {
            throw "Invalid port number";
        }
        ans += (str[i] - 48);
    }
    return ans;
}

void incre_expect() {
    expected_seqnum++;
    expected_seqnum = expected_seqnum % 32;
}

uint32_t decre_expect() {
    uint32_t ans = expected_seqnum;
    if(ans == 0) {
        ans = 32;
    }
    ans--;
    return ans;
}

void hostname_to_ip(char *hostname, char* ip) {  // get ip addr from hostname
    struct hostent *h;
    struct in_addr **addr_lst;
    if((h = gethostbyname(hostname)) == NULL) {
        throw "Error on gethostbyname";
    }
    addr_lst = (struct in_addr**)h->h_addr_list;

    for(int i = 0; addr_lst[i] != NULL; i++) {
        // return the first one
        strcpy(ip, inet_ntoa(*addr_lst[i]));
        return;
    }
    throw "Error hostname_to_ip";
}

int main(int argc, char *argv[]) {
    try{
        if(argc != 5) {
            throw "Incorrect number of argument received!";
        }
        // storing the command line argument
        string emulator_name = argv[1];
        int emulator_recv_port = convert(argv[2]);
        int receiver_recv_port = convert(argv[3]);
        string file = argv[4];
        char ip[100];
        hostname_to_ip(argv[1], ip);
//        string ip_addr = ip;

        for(int i = 0 ; i < 32; i++) {
            isBuff[i] = false;
        }

        // initialize the input and output file
        ofstream newfile;
        newfile.open(file);
        ofstream logfile;
        logfile.open("arrival.log");

        expected_seqnum = 0;
        cumm_acked = -1;
        //char buff[BUFFSIZE];
        Packet buff;

        // initialize the socket
        int sending_socket = socket(AF_INET, SOCK_DGRAM, 0);
        int receiver_socket = socket(AF_INET, SOCK_DGRAM, 0);

        if(receiver_socket == -1 || sending_socket == -1) {
            newfile.close();
            logfile.close();
            throw "FAIL to create socket!";
        }
        unsigned int len = sizeof(struct sockaddr);

        struct sockaddr_in address, remoteaddr, address2;
        address.sin_family = AF_INET;
        address.sin_port = htons(receiver_recv_port);
        address.sin_addr.s_addr = INADDR_ANY;
        if(bind(receiver_socket, (struct sockaddr*)&address, sizeof(address)) == -1) {
            newfile.close();
            logfile.close();
            throw "FAIL to bind socket!";
        }
        /*
        if(listen(receiver_socket, 1) == -1) {
            newfile.close();
            logfile.close();
            switch(errno) {
            case EADDRINUSE:
                cerr << 1 << endl;
                break;
            case EBADF:
                cerr << 3 << endl;
                break;
            case ENOTSOCK:
                cerr << 4 << endl;
                break;
            case EOPNOTSUPP:
                cerr << 5 << endl;
                break;
            default:
                cerr << 6 << endl;
                break;
            }
            throw "FAIL to listen socket!";
        }
        */
        address2.sin_family = AF_INET;
        address2.sin_port = htons(emulator_recv_port);
        if(inet_pton(AF_INET, ip, &address2.sin_addr.s_addr) <= 0) {
                throw "Fail to inet_pton";
        }



        while(1) {
            // reset the buff, and receive the new packet
            memset(&buff, 0, sizeof(buff));
            recvfrom(receiver_socket, &buff, BUFFSIZE, 0, (struct sockaddr*)&remoteaddr, &len);
            buff.type = ntohl(buff.type);
            buff.segnum = ntohl(buff.segnum);
            buff.length = ntohl(buff.length);
            if(buff.type == 2) {    // packet is EOT, send an EOT back, and quit
                debug(2);
                logfile << "EOT" << endl;
                undefPacket temp;
                temp.type = htonl(2);
                temp.length = htonl(0);
                sendto(sending_socket, &temp, sizeof(temp), 0, (struct sockaddr*)&address2, len);
                close(receiver_socket);
                close(sending_socket);
                break;
            }
            if(buff.segnum == expected_seqnum) {    // if segnum is what expected
                    debug(1);
                if(buff.type == 2) {    // packet is EOT, send an EOT back, and quit
                    debug(2);
                    logfile << "EOT" << endl;
//                    Packet temp;
                    undefPacket temp;
                    temp.type = htonl(2);
                    temp.length = htonl(0);

                    sendto(sending_socket, &temp, sizeof(temp), 0, (struct sockaddr*)&address2, len);
                    close(receiver_socket);
                    close(sending_socket);
                    break;
                } else if(buff.type == 1) { // packet is data
                    debug(3);
                    // write to logfile and newfile
                    logfile << buff.segnum <<endl;
                    for(int i = 0; i < buff.length; i++) {
                        cerr << "buff.data[" << i << "]: " << buff.data[i] << endl;
                        newfile << buff.data[i];
                    }
//                    newfile << buff.data;
                    incre_expect();
                    // test whether next seqnum packet is buffered
                    while(isBuff[expected_seqnum]) {
                        // if so, write to newfile, and repeate the process
                        for(int i = 0; i < buffP[expected_seqnum].length; i++) {
                            cerr << "buff.data[" << i << "]: " << buffP[expected_seqnum].data[i] << endl;
                            newfile << buffP[expected_seqnum].data[i];
                        }
//                        newfile << buffP[expected_seqnum].data;
                        isBuff[expected_seqnum] = false;
                        incre_expect();
                    }
                    // once the next seqnum is not buffered, send the cummulative ack seqnum in the ACK packet
                    cumm_acked = decre_expect();
                    //Packet temp;
                    undefPacket temp;
                    temp.type = htonl(0);
                    temp.length = htonl(0);
                    temp.segnum = htonl(cumm_acked);
                    sendto(sending_socket, &temp, sizeof(temp), 0, (struct sockaddr*)&address2, len);
                }
            } else {                // if the segment is not what expected
                debug(4);
                // write info to the log file
                logfile << buff.segnum << endl;
                bool isNext10 = false;
                uint32_t backup = expected_seqnum;
                for(int i = 0; i < 10; i++) {
                    incre_expect;
                    if(expected_seqnum == buff.segnum) {
                        isNext10 = true;
                        break;
                    }
                }
                expected_seqnum = backup;
                // test whether the seqnum is within next 10
                if(isNext10) {
                    // if so, buffer the packet, otherwise do nothing (discard packet)
                    buffP[buff.segnum] = buff;
                }
                // sending back a ACK packet
                //Packet temp;
                undefPacket temp;
                temp.type = htonl(0);
                temp.length = htonl(0);
                temp.segnum = htonl(cumm_acked);
                sendto(sending_socket, &temp, sizeof(temp), 0, (struct sockaddr*)&address2, len);
            }


        }

        newfile.close();
        logfile.close();
    } catch(const char* err_msg) {
        cerr << err_msg << endl;
        return 1;
    }

    return 0;
}
