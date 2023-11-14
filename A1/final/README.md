Program was built and tested on the Linux student server with compile command: g++ <sourcecode> -o <executable>
After running the server, it will shows the n_port followed by a blank line. And when the UDP connection receives "EXIT" as the signal, it will back into the original status(i.e. waiting for an TCP connection receiving the req_code). And It will shows the n_port again followed by a blank line.
For client, after running it, if nothing goes wrong, it will print an blank line followed by the reversed strings input as the command line argument. And an "EXIT" signal will send to the server automatically after all arguments have been passed to the server.
Do not forget to add the permission for executing client.sh and server.sh
