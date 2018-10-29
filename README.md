# Telephone-Protocol

Author - Jared Heidt


SELF_SELF.txt - Contains the output (on the originator) of running 2 copies of my program in a loop. This program was ran entirely on my local machine from 2 different ports. Below are my commands from two seperate Linux Ubuntu terminals.

	In one terminal: ./telephone 0 127.0.0.1:8081 127.0.0.1:8082

	In another terminal: ./telephone 1 127.0.0.1:8082 127.0.0.1:8081


SELF_SELF.cap - Contains a wireshark trace taken on the originator of the case above.


src directory - Contains the source file of the telephone protocol program. All code is contained in one file for simplicity of compliling and running the code. This program could be broken up into objected oriented components such as Client, Server, and Util.


Compiling the program - The main.cpp file in the source directory is a C++11 file. This is compiled with the command:

	g++ -std=c++11 main.cpp -o telephone


To run the program:

	./telephone <originator> <source> <dest>


Refer to the RFC.txt file for details on the protocol.

Known bugs - none.

Known Issues: When running on Clarkson Odin (odin.cslabs.clarkson.edu), the Odin system does not provide the program a working UTC timestamp. This error is handled by exiting the program with failure. This occurs on lines 544-549. This may be a problem on other Clarkson virtual machines because of the (older) Linux versions running. My code was developed and ran in Ubuntu 16.04.
