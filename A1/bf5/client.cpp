#include <cstdlib>
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <sstream>

#define BUFFSIZE 4096

using namespace std;

int convert(string str) {  // convert the req_code from string to int
    int len = str.length();
    int ans = 0;
    int isNegative = 1; // 1 - Potivie, -1 - Negative
    if(len > 1 && str[0] == '-') {
        isNegative = -1;
        for(int i = 1; i < len; i++) {
            ans *= 10;
            if(str[i] < 48 && str[i] > 57) {
                throw "Invalid number input!";
            }
            ans += (str[i] - 48);
        }
    } else {
        for(int i = 0; i < len; i++) {
            ans *= 10;
            if(str[i] < 48 && str[i] > 57) {    // if the i-th position of the req_code is not number, throw it
                throw "Invalid number input!";
            }
            ans += (str[i] - 48);
        }
    }

    return ans * isNegative;
}

int main(int argc, char *argv[]) {
    try{
        if(argc < 4) {
            throw "Incorrect number of argument received! Expected format: client <server address> <n_port> <req_code> <list of msg>";
        }
        string server_address = argv[1];
        string n_port = argv[2];
        const int n_port_num = convert(n_port);
        string req_code = argv[3];

        int client_socket;
        char recvbuff[BUFFSIZE], sendbuff[BUFFSIZE];
        struct sockaddr_in address;

        // initialize the socket setting
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(client_socket < 0) {
            throw "FAIL to create socket";
        }

        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_port = htons(n_port_num);
        if(inet_pton(AF_INET, argv[1], &address.sin_addr.s_addr) <= 0) {
            throw "Fail to inet_pton";
        }

        // start connection
        if(connect(client_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
            throw "Fail to connect";
        }

        // send the request code to the server
        if(send(client_socket, argv[3], req_code.length(), 0) < 0) {
            throw "Fail to send request code";
        }

        // get the r_port from the server
        recv(client_socket, recvbuff, BUFFSIZE, 0);

        // convert the r_port from string to int
        string r_port = recvbuff;
        const int r_port_num = convert(r_port);

        close(client_socket);
        // stage 1 end


        // start the UDP connection

        unsigned int len = sizeof(struct sockaddr);
        string recvinfo;

        for(int i = 4; i < argc; i++) {
            memset(&address, 0, sizeof(address));
            memset(recvbuff, 0, sizeof(recvbuff));

            client_socket = socket(AF_INET, SOCK_DGRAM, 0);
            address.sin_family = AF_INET;
            address.sin_port = htons(r_port_num);

            if(inet_pton(AF_INET, argv[1], &address.sin_addr.s_addr) <= 0) {
                throw "Fail to inet_pton";
            }
            // send the message
            sendto(client_socket, argv[i], strlen(argv[i]), 0, (struct sockaddr*)&address, len);
            // receive the revised message
            recvfrom(client_socket, recvbuff, BUFFSIZE, 0, (struct sockaddr*)&address, &len);
            recvinfo = recvbuff;
            // print the revised message
            cout << recvinfo << endl;
            close(client_socket);
        }
        // send the "EXIT" message to the server after all messages are sent
        memset(&address, 0, sizeof(address));
        memset(recvbuff,0, sizeof(recvbuff));
        client_socket = socket(AF_INET, SOCK_DGRAM, 0);
        address.sin_family = AF_INET;
        address.sin_port = htons(r_port_num);

        if(inet_pton(AF_INET, argv[1], &address.sin_addr.s_addr) <= 0) {
            throw "Fail to inet_pton";
        }

        char temp[4] = {'E', 'X', 'I', 'T'};
        sendto(client_socket, temp, strlen(temp), 0, (struct sockaddr*)&address, len);
        close(client_socket);


    } catch(const char* err_msg) {
        cerr << err_msg << endl;
        return 1;
    }
}
