#include <cstdlib>
#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <sstream>

#define N_PORT 27819  // fixed n_port
#define BUFFSIZE 4096

using namespace std;

bool isValid;   // store the status of whether the string version request code is valid

int convert(string str) {  // convert the req_code from string to int
    int len = str.length();
    int ans = 0;
    int isNegative = 1; // 1 - Potivie, -1 - Negative
    if(len > 1 && str[0] == '-') {
        isNegative = -1;
        for(int i = 1; i < len; i++) {
            ans *= 10;
            if(str[i] < 48 && str[i] > 57) {
                throw "Invalid <req_code>!";
            }
            ans += (str[i] - 48);
        }
    } else {
        for(int i = 0; i < len; i++) {
            ans *= 10;
            if(str[i] < 48 && str[i] > 57) {    // if the i-th position of the req_code is not number, throw it
                throw "Invalid <req_code>!";
            }
            ans += (str[i] - 48);
        }
    }

    return ans * isNegative;
}

int convert_without_throw(string str) {  // convert the req_code from string to int
    int len = str.length();
    int ans = 0;
    int isNegative = 1; // 1 - Potivie, -1 - Negative
    if(len > 1 && str[0] == '-') {
        isNegative = -1;
        for(int i = 1; i < len; i++) {
            ans *= 10;
            if(str[i] < 48 && str[i] > 57) {
                isValid = false;
                return 0;
            }
            ans += (str[i] - 48);
        }
    } else {
        for(int i = 0; i < len; i++) {
            ans *= 10;
            if(str[i] < 48 && str[i] > 57) {    // if the i-th position of the req_code is not number, throw it
                isValid = false;
                return 0;
            }
            ans += (str[i] - 48);
        }
    }

    return ans * isNegative;
}

string generate_r_port(int* r_port_receiver) {  // randomly generate the r_port number, and return it as a string. Also pass it by a pointer as int
    string port_number;
    int random_num = (rand() % 8000) + 1025;
    if(random_num == N_PORT)
        random_num++;
    stringstream ss;
    ss << random_num;
    ss >> port_number;
    *r_port_receiver = random_num;
    return port_number;
}

int main(int argc, char *argv[]) {
    try {
        if(argc != 2) {
            throw "Incorrect number of argument received! Expected format: server <req_code>";
        }
        int req_code = convert(argv[1]);   // req_code stores <req_code>

        int server_TCP_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(server_TCP_socket == -1) {
            throw "FAIL to create TCP socket!";
        }
        struct sockaddr_in addrss_tcp;
        memset(&addrss_tcp, 0, sizeof(addrss_tcp));
        addrss_tcp.sin_family = AF_INET;
        addrss_tcp.sin_port = htons(N_PORT);
        addrss_tcp.sin_addr.s_addr = INADDR_ANY;
        int addr_len = sizeof(addrss_tcp);
        unsigned int len = sizeof(struct sockaddr);

        if(bind(server_TCP_socket, (struct sockaddr*)&addrss_tcp, sizeof(addrss_tcp)) == -1) {    // bind the socket with port
            throw "FAIL to bind socket!";
        }

        while(1) {
//            int server_socket = socket(AF_INET, SOCK_STREAM, 0);
            int receiver;
            char buff[BUFFSIZE];
            char* sendbuff;
            string r_port_str;
            int r_port;
/*
            if(server_socket == -1) {
                throw "FAIL to create socket!";
            }
*/
            struct sockaddr_in address, remoteaddr;
            memset(&address, 0, sizeof(address));
            memset(&remoteaddr, 0, sizeof(remoteaddr));
            memset(&buff, 0, sizeof(buff));
/*
            address.sin_family = AF_INET;
            address.sin_port = htons(N_PORT);
            address.sin_addr.s_addr = INADDR_ANY;*/
            //int addr_len = sizeof(address);
            unsigned int remoteaddr_len = sizeof(remoteaddr);
/*
            if(bind(server_socket, (struct sockaddr*)&address, sizeof(address)) == -1) {    // bind the socket with port
                throw "FAIL to bind socket!";
            }*/

            if(listen(server_TCP_socket, 1) == -1) {    // initialize listening
                throw "Listen socket ERROR!";
            }

            cout << "1st Listen socket pass" <<endl;

            cout << "SERVER_PORT=" << N_PORT << endl;   // print the server port (N_PORT)

            while(1) {  // continue try to accept client request code
                receiver = accept(server_TCP_socket, (struct sockaddr*)&remoteaddr, &remoteaddr_len);
                if(receiver == -1) {
                    cerr << "Fail to accept socket!" << endl;
                    continue;
                }
                recv(receiver, buff, BUFFSIZE, 0);  // get the content
                string get_req = buff;
                isValid = true;
                int client_req = convert_without_throw(get_req);   // convert the string content into int

                if(isValid == false || client_req != req_code) {    // if the request code is not valid or not matched close the connection
                    close(receiver);
                    continue;
                } else if(client_req == req_code) {    // otherwise generate and send the r_port and close the connection. Then goto stage 2
                    srand(time(0));
                    r_port_str = generate_r_port(&r_port);  // generate r_port number
                    sendbuff = &r_port_str[0];
                    send(receiver, sendbuff, strlen(sendbuff), 0);
                    cout << endl;
                    close(receiver);
                    break;
                }

            }
            close(server_TCP_socket);
            // stage 1 end

            int server_socket = socket(AF_INET, SOCK_DGRAM, 0); // set the connection type be UDP
            memset(&address, 0, sizeof(address));
            memset(&remoteaddr, 0, sizeof(remoteaddr));
            memset(buff, 0 ,sizeof(buff));
            address.sin_family = AF_INET;
            address.sin_port = htons(r_port);
            address.sin_addr.s_addr = INADDR_ANY;

            if(bind(server_socket, (struct sockaddr*)&address, sizeof(address)) == -1) {    // bind the socket with port
                throw "FAIL to bind socket!";
            }

            while(1) {
                memset(buff, 0, sizeof(buff));
                // receive the strings from client
                recvfrom(server_socket, buff, BUFFSIZE, 0, (struct sockaddr*)&remoteaddr, &len);
                string info = buff;
                if(info.compare("EXIT") == 0) { // if "EXIT" receives, close the connection
                    break;
                }
                int strlenn = info.length();
                // reverse the string
                char temp;
                for(int i = 0; i < strlenn / 2; i++) {
                    temp = buff[i];
                    buff[i] = buff[strlenn - 1 - i];
                    buff[strlenn - 1 - i] = temp;
                }
                // send back to the client
                sendto(server_socket, buff, strlen(buff), 0, (struct sockaddr*)&remoteaddr, len);
            }

            close(server_socket);
        }


    } catch(const char* err_msg) { // exception catch, print the error message
        cerr << err_msg << endl;
        return 1;
    }
}
