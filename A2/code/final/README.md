Minqi Xu
20845758
m259xu

Environment: student linux environment, final test @ubuntu2004-002

Makefile can compile receiver.cpp and sender.cpp into to executable file namely receiver and sender.
Make version: GNU Make 4.2.1

After compile, by typing ./receiver <command line args> and ./sender <command line args> to run the program.

The compiler is the default one without adding any flags:
g++ sender.cpp -o sender
g++ receiver.cpp -o receiver.
version showed by typing "g++ -v": 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1)


receiver will create arrival.log which is the log file that required while running
sender will create ack.log, N.log, and seqnum.log as required while running

packet.py and network_emulator.py for testing are the provided one from the dropbox, no modifying occured
