# Telephone-Protocol

Author - Jared Heidt

SELF_SELF.txt - Contains the output (on the originator) of running 2 copies of my program in a loop.
	In one terminal: ./telephone 0 127.0.0.1:8081 127.0.0.1:8082

	In another terminal: ./telephone 1 127.0.0.1:8082 127.0.0.1:8081

SELF_SELF.cap - Containing a wireshark trace taken on the originator of the case above

src directory - Contains the source file of the telephone protocol program.  All code is contained 
		in this file for simplicity of compliling and running the code. This program could 
		be broken up into objected oriented components such as Client, Server, and Util.
