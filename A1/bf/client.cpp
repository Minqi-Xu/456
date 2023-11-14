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

#define N_PORT 4123  // fixed n_port
#define BUFFSIZE 4096

using namespace std;

int main(int argc, char *argv[]) {
    try{
        if(argc < 4) {
            throw "Incorrect number of argument received! Expected format: client <server address> <n_port> <req_code> <list of msg>";
        }

    } catch(const string err_msg) {
        cerr << err_msg << endl;
        return 1;
    }
}
