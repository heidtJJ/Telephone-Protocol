#include <iostream>
#include <string>
#include <stddef.h>
#include <stdint.h> // uint16_t
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <string.h> 
#include <netinet/in.h> 
#include <unistd.h> 
#include <vector> 
#include <sstream> 

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;

// Constants 
#define PROPER_GREETING "HELLO 1.7"
#define PROPER_GREETING_LEN 9
#define DATA "DATA"
#define DATA_LEN 4

// Utility functions
uint16_t checksum(void *data, size_t size);
vector<string> getIpAndPort(const string& hostName);
bool receiveGreeting(const int& connectSocket);

// Server/Client
void serverFunction(const string& sourceIP, const int& sourcePort, const string& desinationIP, const int& destinationPort);
void clientFunction(const string& sourceIP, const int& sourcePort, const string& desinationIP, const int& destinationPort);

int main(int argc, char* argv[]){
    if(argc != 4){
        cout << "$ ./telephone <originator> <source> <dest>" << endl;
        exit(EXIT_FAILURE);
    }
    string originator = argv[1];
    if(originator != "0" && originator != "1"){
        cout << "Originator is invalid! Must be 0 or 1." << endl;
        exit(EXIT_FAILURE);
    }
    string source = argv[2];
    // Get port and IP from source
    vector<string> sourceIpPortPair = getIpAndPort(source);
    if(sourceIpPortPair.empty()){
        cout << "Invalid source address!" << endl;
        cout << "$ ./telephone <originator> <source> <dest>" << endl;
        exit(EXIT_FAILURE);
    }

    string dest = argv[3];
    vector<string> destIpPortPair = getIpAndPort(dest);
    if(destIpPortPair.empty()){
        cout << "Invalid destination address!" << endl;
        cout << "$ ./telephone <originator> <source> <dest>" << endl;
        exit(EXIT_FAILURE);
    }
    int sourcePort = stoi(sourceIpPortPair[1]);
    int destPort = stoi(destIpPortPair[1]);

    if(originator == "1"){
        // user is originator
        clientFunction(sourceIpPortPair[0], sourcePort, destIpPortPair[0], destPort);
        serverFunction(sourceIpPortPair[0], sourcePort, destIpPortPair[0], destPort);
    }
    else {
        // user is not originator        
        serverFunction(sourceIpPortPair[0], sourcePort, destIpPortPair[0], destPort);
        clientFunction(sourceIpPortPair[0], sourcePort, destIpPortPair[0], destPort);
    }

    return 0;
}

// **************************************************************************************************

uint16_t checksum(void *data, size_t size) {
    uint32_t sum = 0;
    /* Cast to uint16_t for pointer arithmetic */
    uint16_t* data16 = (uint16_t*) data;

    while(size > 0) {
        sum += *data16++;
        size -= 2;
    }

    /* For the extraneous byte, if any: */
    if(size > 0) 
        sum += *((uint8_t *) data16);

    /* Fold the sum as needed */
    while(sum >> 16) 
        sum = (sum & 0xFFFF) + (sum >> 16);

    /* One's complement is binary inversion: */
    return ~sum;
}

// **************************************************************************************************

vector<string> getIpAndPort(const string& hostName){
    vector<string> ipPortPair;

    size_t colonPos = hostName.find(':');
    if(colonPos != std::string::npos)
    {
        std::string hostPart = hostName.substr(0, colonPos);
        std::string portPart = hostName.substr(colonPos+1);

        std::stringstream parser(portPart);
        int port = 0;
        if( parser >> port ) {
            // hostname in hostPart, port in port
            ipPortPair.push_back(hostPart);
            ipPortPair.push_back(portPart);
            // could check port >= 0 and port < 65536 here
        }
        else {
            // port not convertible to an integer
            return ipPortPair;
        }
    }
    else {
        // Missing port?
        return ipPortPair;
    }
}

// **************************************************************************************************

bool sendGreeting(const int& connectSocket){
    int result = send(connectSocket, PROPER_GREETING, PROPER_GREETING_LEN, 0 ); 
    if(result != PROPER_GREETING_LEN){
        perror("Greeting could not be sent."); 
        return false;
    }
    return true;
}

// **************************************************************************************************

bool sendDataString(const int& connectSocket){
    int result = send(connectSocket, DATA, DATA_LEN, 0); 
    if(result != DATA_LEN){
        perror("DATA could not be sent."); 
        return false;
    }
    return true;
}

// **************************************************************************************************

/*
    Server will be using this function to acknowledge that
    data will be sent next from client. 
 */
bool receiveDataString(const int& connectSocket){
    char buffer[1024] = {0}; 
    int numBytesRead = read(connectSocket, buffer, 1024);
    if(numBytesRead != DATA_LEN){
        perror("DATA string unsuccessfully received."); 
        return false;
    }

    string greeting = buffer;
    memset(buffer, '\0', 1024); 

    if(greeting != DATA){
        // send(connectSocket, "QUIT", 4, 0); 
        perror("DATA string unsuccessfully received."); 
        return false; 
    }
    return true;
}

// **************************************************************************************************

bool receiveGreeting(const int& connectSocket){
    char buffer[1024] = {0}; 
    int numBytesRead = read(connectSocket, buffer, 1024);
    if(numBytesRead != PROPER_GREETING_LEN){
        perror("Telephone version is unsupported."); 
        close(connectSocket); 
        return false;
    }

    string greeting = buffer;
    if(greeting != PROPER_GREETING){
        // Version is unsupported.
        send(connectSocket, "QUIT", 4, 0); 
        perror("Telephone version is unsupported."); 
        close(connectSocket); 
        return false; 
    }
    return true;
}

// **************************************************************************************************

void serverFunction(const string& sourceIP, const int& sourcePort, 
                        const string& desinationIP, const int& destinationPort){
    int doorbellSocket;
    int connectSocket;

    // Initialize sockaddr_in.
    sockaddr_in address = { 0 }; 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( sourcePort ); 

    int opt = 1; 
    int addrlen = sizeof(address); 
    
    // Create doorbellSocket file descriptor.
    if ((doorbellSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
        perror("Server doorbell socket failed to be created."); 
        exit(EXIT_FAILURE); 
    } 
       
    /* This helps in manipulating options for the socket referred by the file descriptor 
        sockfd. This is completely optional, but it helps in reuse of address and port. 
        Prevents error such as: “address already in use”.
    */
    if (setsockopt(doorbellSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
        perror("Server can not setsockopt."); 
        exit(EXIT_FAILURE); 
    } 
    
    // Forcefully attaching socket to the port 8080 
    if (bind(doorbellSocket, (struct sockaddr *)&address, sizeof(address))<0) { 
        perror("Server can not bind to socket."); 
        exit(EXIT_FAILURE); 
    } 
    if (listen(doorbellSocket, 3) < 0) { 
        perror("Server can not listen for client."); 
        exit(EXIT_FAILURE); 
    } 
    if ( (connectSocket = accept(doorbellSocket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Server can not accept client connection.");
        exit(EXIT_FAILURE);
    }

    // Server is first to send greeting. Send greeting to client.
    bool success = sendGreeting(connectSocket);
    if(!success) exit(EXIT_FAILURE);// Error is already printed.

    // Receive greeting back from client (or acknowledge error).
    success = receiveGreeting(connectSocket);
    if(!success) exit(EXIT_FAILURE);// Error is already printed.

    // Receive "DATA" from client.
    success = receiveDataString(connectSocket);
    if(!success){
        // Handle "DATA" not properly received.
    }

    cout << "doneserv";

}

// **************************************************************************************************

void clientFunction(const string& sourceIP, const int& sourcePort, 
                        const string& desinationIP, const int& destinationPort){
    int connectSocket = 0;
    sockaddr_in serverAddress; 

    if ((connectSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        cout << "Socket creation error" << endl; 
        exit(EXIT_FAILURE);
    } 

    memset(&serverAddress, '0', sizeof(serverAddress)); 
   
    /* the htons() function converts values between host and network byte orders. 
    There is a difference between big-endian and little-endian and network byte order
    depending on your machine and network protocol in use. */
    serverAddress.sin_port = htons(destinationPort); 
    serverAddress.sin_family = AF_INET;

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, desinationIP.c_str(), &serverAddress.sin_addr)<=0)  
    { 
        cout << "Invalid address/ Address not supported" << endl;
        exit(EXIT_FAILURE);
    }

    if (connect(connectSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        cout << "Connection Failed" << endl; 
        exit(EXIT_FAILURE);
    }

    // Connected to server socket. Wait for greeting.
    bool success = receiveGreeting(connectSocket);
    if(!success) exit(EXIT_FAILURE);// Error is already printed.

    // Acknowledge server by sending back greeting.
    success = sendGreeting(connectSocket);
    if(!success) exit(EXIT_FAILURE);// Error is already printed.

    // Send "DATA" to server
    success = sendDataString(connectSocket);
    if(!success) {
        // Handle "DATA" not able to be sent.
    }

    cout << "doneclinet";
}
