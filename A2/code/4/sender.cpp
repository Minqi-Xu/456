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
#include <chrono>
#include <fcntl.h>

#define BUFFSIZE 512
#define MAX_DATA_SIZE 500

using namespace std;

void debug(int i) {
    cerr << "debug: " << i << endl;
}

struct Packet{
    uint32_t type;       // 0:ACK, 1: Data, 2: EOT
    uint32_t segnum;     // Modulo 32
    uint32_t length;     // Length of the String variable 'data', should in the range of 0 to 500
//    string data;    // String with Max Length 500
//    char* data;
    char data[500];
};

struct mystruct{
    int32_t type;
    int32_t segnum;
    int32_t length;
    char data[500];
};

struct undefPacket{
    uint32_t type;
    uint32_t segnum;
    uint32_t length;
};

struct timer{
    bool isRun;
    uint32_t seqnum;
    uint64_t record;
};

uint64_t getTime() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}



int convert(string str) {   // convert the number from string to int
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

uint32_t lastACK, lastACK2, lastACK3;   // store the seqnum of the last 3 ACKS
int N;      // window size
int t;      // timestamp
Packet packqueue[32];           // store the next 32 packets, the index corresponds to their seqnum
int absi[32];           // store the absolute index of the corresponding packet in packqueue;
int sendbase;       // the send base is the left most of the window, this stores the absolute index (1..numPacket)
int current_load;   // store the highest absolute index that is loaded into packqueue
int next_send;    // store the next packet that are planned to send
int lim;            // store the highest absolute index that is sent, can only increase

void incre_N() {
    if(N >= 10) {
        N = 10;
        return;
    }
    N++;
}

void d() {
    cerr << "sendbase= " << sendbase << endl;
}

void incre_32(uint32_t* i) {
    *i = *i + 1;
    if(*i == 32) {
        *i = 0;
    }
}

int main(int argc, char *argv[]) {
    try{
        if(argc != 6) {
            throw "Incorrect number of argument received!";
        }
        // store the command line argument
        string emulator_addr = argv[1];
        int emulator_send_port = convert(argv[2]);
        int sender_recv_port = convert(argv[3]);
        int timeout = convert(argv[4]);
        string file = argv[5];


        // file operation initialization
        ifstream myfile(file);
        ofstream seqnum_log;
        seqnum_log.open("seqnum.log");
        ofstream ack_log;
        ack_log.open("ack.log");
        ofstream n_log;
        n_log.open("N.log");

        // get the size of the file
        myfile.seekg(0, ifstream::end);
        int size = myfile.tellg();
        myfile.seekg(0);

        // calculate the number of packets need to send the file
        int numPacket = size / MAX_DATA_SIZE;
        if(size % MAX_DATA_SIZE != 0) {
            numPacket++;
        }

        // split the file and stores the data of each packet into string array datas[]
        string datas[numPacket];
        char *tempbuff = new char[MAX_DATA_SIZE];

        for(int i = 0; i < numPacket - 1; i++) {
            memset(tempbuff, 0, MAX_DATA_SIZE);
            myfile.read(tempbuff, MAX_DATA_SIZE);
            datas[i] = tempbuff;
        }
        memset(tempbuff, 0, MAX_DATA_SIZE);

        if(numPacket != 0) {
            if(size % MAX_DATA_SIZE == 0) {
                myfile.read(tempbuff, MAX_DATA_SIZE);
                datas[numPacket - 1] = tempbuff;
            } else {
                myfile.read(tempbuff, size - (numPacket - 1) * MAX_DATA_SIZE);
                datas[numPacket - 1] = tempbuff;
            }
        }


        delete[] tempbuff;

        bool isSentEOT = false;
        N = 1;
        t = 0;
        current_load = 0;
        next_send = 0;
        lim = -1;
        lastACK = -1;
        lastACK2 = -1;
        lastACK3 = -1;
        sendbase = 0;
        for(int i = 0; i < 32; i++) {
            absi[i] = -1;
        }
        for(current_load = 0; current_load < numPacket && current_load < 32; current_load++) {
            Packet temp;
            temp.type = 1;
            temp.segnum = current_load;
            temp.length = datas[current_load].length();
//            temp.data = new char[temp.length];
            for(int i = 0; i < temp.length; i++) {
                temp.data[i] = datas[current_load][i];
            }
            //temp.data = datas[current_load];
            absi[current_load] = current_load;
            packqueue[current_load] = temp;
            for(int i = 0; i < temp.length; i++) {
                packqueue[current_load].data[i] = datas[current_load][i];
            }
        }

        // initialize the socket
        int receive_socket = socket(AF_INET, SOCK_DGRAM, 0);
        int sending_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if(receive_socket == -1 || sending_socket == -1) {
            myfile.close();
            seqnum_log.close();
            ack_log.close();
            n_log.close();
            throw "FAIL to create socket!";
        }

        // set the receive socket to be non-block
        int flags = fcntl(receive_socket, F_GETFL, 0);
        if(flags == -1) {
            myfile.close();
            seqnum_log.close();
            ack_log.close();
            n_log.close();
            throw "FAIL to get flags of socket!";
        }
        flags |= O_NONBLOCK;
        flags = fcntl(receive_socket, F_SETFL, flags);

        // continue initialize sockets
        unsigned int len = sizeof(struct sockaddr);
        struct sockaddr_in address, remoteaddr, address2;
        unsigned int remoteaddr_len = sizeof(remoteaddr);
        address.sin_family = AF_INET;
        address.sin_port = htons(sender_recv_port);
        address.sin_addr.s_addr = INADDR_ANY;
        if(bind(receive_socket, (struct sockaddr*)&address, sizeof(address)) == -1) {
            myfile.close();
            seqnum_log.close();
            ack_log.close();
            n_log.close();
            throw "FAIL to bind socket!";
        }

        address2.sin_family = AF_INET;
        address2.sin_port = htons(emulator_send_port);
        if(inet_pton(AF_INET, argv[1], &address2.sin_addr.s_addr) <= 0) {
                throw "Fail to inet_pton";
        }
        /*
        if(listen(receive_socket, 1) == -1) {
            myfile.close();
            seqnum_log.close();
            ack_log.close();
            n_log.close();
            throw "FAIL to listen socket!";
        }
        */
        n_log << "t=" << t << " " << N << endl;
        t++;

        timer TT;
        TT.isRun = false;

        int receiver;
        Packet buff;
        while(1) {

            memset(&buff, 0, sizeof(buff));
            if(next_send < numPacket) {
                if(next_send - sendbase < N) {
                    cerr << "next_send " << next_send << " sendbase " << sendbase << endl;
                    // window is not full
                    int templen = (packqueue[next_send%32].length);
                    mystruct sendPack;
                    sendPack.type = packqueue[next_send%32].type;
                    sendPack.type = htonl(sendPack.type);
                    sendPack.segnum = packqueue[next_send%32].segnum;
                    sendPack.segnum = htonl(sendPack.segnum);
                    sendPack.length = packqueue[next_send%32].length;
                    sendPack.length = htonl(sendPack.length);
                    for(int i = 0; i < packqueue[next_send%32].length; i++) {
                        sendPack.data[i] = packqueue[next_send%32].data[i];
                    }
                    if(sendto(sending_socket, &sendPack, 3*4+packqueue[next_send%32].length , 0, (struct sockaddr*)&address2, len) == -1) {
                        throw "sendto error";
                    }
//                    sendto(sending_socket, &sendPack, 3*4+sendPack.length , 0, (struct sockaddr*)&address2, len);

/*                    sendto(sending_socket, &packqueue[next_send%32],
                           sizeof(packqueue[next_send%32]), 0, (struct sockaddr*)&address2, len);
*/
                    lim = max(next_send, lim);
                    seqnum_log << "t=" << t << " " << next_send % 32 << endl;

                    if(TT.isRun == false) {
                        TT.record = getTime();
                        TT.seqnum = next_send % 32;
                        TT.isRun = true;
                    }
                    next_send++;
                    t++;
                }
            }

            if(!isSentEOT && next_send >= numPacket && sendbase >= next_send) {
                /*
                Packet eot;
                eot.length = 0;
                eot.type = 2;
                */
                undefPacket eot;
                eot.length = htonl(0);
                eot.type = htonl(2);
                isSentEOT = true;
                sendto(sending_socket, &eot, sizeof(eot), 0, (struct sockaddr*)&address2, len);
                seqnum_log << "t=" << t << " EOT" << endl;
                t++;
            }

            if(TT.isRun == true && getTime()-TT.record > timeout && next_send - sendbase > 0) {
                // timeout!
                N = 1;
                next_send = absi[TT.seqnum];
                mystruct sendPack;
                sendPack.type = packqueue[next_send%32].type;
                sendPack.type = htonl(sendPack.type);
                sendPack.segnum = packqueue[next_send%32].segnum;
                sendPack.segnum = htonl(sendPack.segnum);
                sendPack.length = packqueue[next_send%32].length;
                sendPack.length = htonl(sendPack.length);
                for(int i = 0; i < packqueue[next_send%32].length; i++) {
                    sendPack.data[i] = packqueue[next_send%32].data[i];
                }
                sendto(sending_socket, &sendPack, 3*4+packqueue[next_send%32].length , 0, (struct sockaddr*)&address2, len);
                /*
                sendto(sending_socket, &packqueue[next_send%32],
                       sizeof(packqueue[next_send%32]), 0, (struct sockaddr*)&address2, len);
                */
                lim = max(next_send, lim);
                seqnum_log << "t=" << t << " " << next_send % 32 << endl;
                n_log << "t=" << t << " " << N <<endl;
                TT.record = getTime();
                TT.seqnum = next_send % 32;
                TT.isRun = true;
                next_send++;
                t++;
                lastACK = -1;
                lastACK2 = -1;
                lastACK3 = -1;

            }
            /*
            receiver = accept(receive_socket, (struct sockaddr*)&remoteaddr, &remoteaddr_len);
            if(receiver == -1) {
                continue;
            }
            */
            /*
            if(recv(receive_socket, &buff, BUFFSIZE, 0) == -1) {
                continue;
            }
            */
            int retval = 0;
            try{
                retval = recv(receive_socket, &buff, BUFFSIZE, 0);
            } catch(const std::exception& e) {}

            if(retval == -1) {
                /*cerr << "numPack " << numPacket << endl;
                cerr << "sendbase " << sendbase << endl;
                cerr << "next_send " << next_send << endl;*/
                continue;
            }
            buff.type = ntohl(buff.type);
            buff.segnum = ntohl(buff.segnum);
            buff.length = ntohl(buff.length);
            if(buff.type == 0) {
                // ACK received
                if(lastACK3 == buff.segnum && lastACK2 == buff.segnum && lastACK == buff.segnum && buff.segnum == TT.seqnum % 32) {
                    // 3 duplicated ACK received
                    N = 1;
                    next_send = absi[TT.seqnum];
                    mystruct sendPack;
                    sendPack.type = packqueue[next_send%32].type;
                    sendPack.type = htonl(sendPack.type);
                    sendPack.segnum = packqueue[next_send%32].segnum;
                    sendPack.segnum = htonl(sendPack.segnum);
                    sendPack.length = packqueue[next_send%32].length;
                    sendPack.length = htonl(sendPack.length);
                    for(int i = 0; i < packqueue[next_send%32].length; i++) {
                        sendPack.data[i] = packqueue[next_send%32].data[i];
                    }
                    sendto(sending_socket, &sendPack, 3*4+packqueue[next_send%32].length , 0, (struct sockaddr*)&address2, len);
                    /*
                    sendto(sending_socket, &packqueue[next_send%32],
                       sizeof(packqueue[next_send%32]), 0, (struct sockaddr*)&address2, len);
                    */
                    lim = max(next_send, lim);
                    ack_log << "t=" << t << " " << next_send % 32 << endl;
                    n_log << "t=" << t << " " << N << endl;
                    TT.record = getTime();
                    TT.seqnum = next_send % 32;
                    TT.isRun = true;
                    next_send++;
                    t++;
                    lastACK = -1;
                    lastACK2 = -1;
                    lastACK3 = -1;
                } else {
                    // test whether the current ACK is transmitted before
                    if(absi[buff.segnum] <= lim) {
                        // if it is a valid(sent) packet
                        if(absi[buff.segnum] != sendbase - 1) {
                            // if it is a new ACK (not duplicate)
                            if(next_send - sendbase <= 0) {
                                TT.isRun = false;
                            } else {
                                TT.record = getTime();
                                uint32_t ptr = sendbase % 32;
                                uint32_t endat = buff.segnum;
                                incre_32(&endat);
                                for(; ptr != endat; incre_32(&ptr)) {
//                                    delete[] packqueue[ptr].data;
                                    if(current_load < numPacket) {
                                        // if there is another packet to load to packqueue
                                        Packet temp;
                                        temp.type = 1;
                                        temp.segnum = current_load % 32;
                                        temp.length = datas[current_load].length();
//                                        temp.data = new char[temp.length];
                                        for(int i = 0; i < temp.length; i++) {
                                            temp.data[i] = datas[current_load][i];
                                        }
                                        //temp.data = datas[current_load];
                                        absi[current_load % 32] = current_load;
//                                        memset(packqueue[current_load%32].data, 0, 500);
                                        packqueue[current_load % 32] = temp;
                                        for(int i = 0; i < temp.length; i++) {
                                            packqueue[current_load % 32].data[i] = datas[current_load][i];
                                        }
                                        current_load++;
                                    }
                                    sendbase++;
                                }
                                TT.isRun = true;
                                if(next_send - sendbase <= 0) {
                                    TT.isRun = false;
                                }
                                TT.seqnum = next_send % 32;

                            }
                            if(N < 10) {
                                incre_N();
                                n_log << "t=" << t << " " << N << endl;
                            }
                            ack_log << "t=" << t << " " << buff.segnum << endl;

                            t++;
                            lastACK3 = lastACK2;
                            lastACK2 = lastACK;
                            lastACK = buff.segnum;

                        } else {
                            // this is a duplicate ACK
                            lastACK3 = lastACK2;
                            lastACK2 = lastACK;
                            lastACK = buff.segnum;
                            ack_log << "t=" << t << " " << buff.segnum << endl;
                            t++;
                        }
                    } else {
                        // not a valid packet, discard after write into log
                        ack_log << "t=" << t << " " << buff.segnum << endl;
                        t++;
                    }
                    // test whether the current ACK is in the window (i.e. is waiting to be acked)

                }
            } else if(buff.type == 2) {

                ack_log << "t=" << t << " EOT" << endl;
                break;
            }

        }


        myfile.close();
        seqnum_log.close();
        ack_log.close();
        n_log.close();
    } catch(const char* err_msg) {
        cerr << err_msg << endl;
        return 1;
    }

    return 0;
}
