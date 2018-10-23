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
#include <sys/utsname.h>
#include <sys/time.h>

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;
using std::to_string;

// Constants 
#define PROPER_GREETING "HELLO 1.7"
#define PROPER_GREETING_LEN 9
#define DATA "DATA"
#define DATA_LEN 4
#define CRLF \r\n

// Utility functions
uint16_t checksum(void *data, size_t size);
vector<string> getIpAndPort(const string& hostName);
bool receiveGreeting(const int& connectSocket, bool server);

// Server/Client
void serverFunction(const string& sourceIP, const int& sourcePort, const string& desinationIP, const int& destinationPort, const bool originator);
void clientFunction(const string& sourceIP, const int& sourcePort, const string& desinationIP, const int& destinationPort, const bool originator);
void initializeMessage(string& transmitMessage, const string& fromHost, const string& toHost);

// System name

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
    string sourceIP = sourceIpPortPair[0];
    int sourcePort = stoi(sourceIpPortPair[1]);

    string destinationIP = destIpPortPair[0];
    int destPort = stoi(destIpPortPair[1]);

    if(originator == "1"){
        // user is originator
        string transmitMessage = "";
        initializeMessage(transmitMessage, sourceIP, destinationIP);
        clientFunction(sourceIP, sourcePort, destinationIP, destPort, true);
        serverFunction(sourceIP, sourcePort, destinationIP, destPort, true);
    }
    else {
        // user is not originator
        serverFunction(sourceIP, sourcePort, destinationIP, destPort, false);
        clientFunction(sourceIP, sourcePort, destinationIP, destPort, false);
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

bool sendGreeting(const int& connectSocket, bool server){
    int result = send(connectSocket, PROPER_GREETING, PROPER_GREETING_LEN, 0 ); 
    if(server) cout << "Server sent: " << PROPER_GREETING << " -> " << PROPER_GREETING_LEN << endl;
    else cout << "Client sent: " << PROPER_GREETING << " -> " << PROPER_GREETING_LEN << endl;
    if(result != PROPER_GREETING_LEN){
        cout << "Greeting could not be sent." << endl; 
        return false;
    }
    return true;
}

// **************************************************************************************************

bool sendDATAString(const int& connectSocket){
    int result = send(connectSocket, DATA, DATA_LEN, 0); 
    cout << "Client sent: " << DATA << " -> " << DATA_LEN << endl;
    if(result != DATA_LEN){
        cout << "DATA could not be sent." << endl; 
        return false;
    }
    return true;
}

// **************************************************************************************************

/*
    Server will be using this function to acknowledge that
    data will be sent next from client. 
 */
bool receiveDATAString(const int& connectSocket){
    char buffer[1024] = {0}; 
    int numBytesRead = read(connectSocket, buffer, 1024);
    cout << "Server read: " << buffer << " -> " << numBytesRead << endl;
    if(numBytesRead != DATA_LEN){
        perror("DATA string unsuccessfully received."); 
        return false;
    }

    string greeting = buffer;
    memset(buffer, '\0', 1024); 

    if(greeting != DATA){
        send(connectSocket, "QUIT", 4, 0); 
        perror("DATA string unsuccessfully received."); 
        return false; 
    }
    return true;
}

// **************************************************************************************************

bool receiveGreeting(const int& connectSocket, bool server){
    char buffer[1024] = {0}; 
    int numBytesRead = read(connectSocket, buffer, 1024);
    if(server)
        cout << "server read: " << buffer << " -> " << numBytesRead << endl;
    else 
        cout << "client read: " << buffer << " -> " << numBytesRead << endl;

    if(numBytesRead != PROPER_GREETING_LEN){
        cout << "Telephone version is unsupported. numBytesRead != PROPER_GREETING_LEN" << endl; 
        close(connectSocket); 
        return false;
    }

    string greeting = buffer;
    if(greeting != PROPER_GREETING){
        // Version is unsupported.
        send(connectSocket, "QUIT", 4, 0); 
        cout << "Telephone version is unsupported. greeting != PROPER_GREETING" << endl; 
        close(connectSocket); 
        return false; 
    }
    memset(buffer, '\0', 1024);
    return true;
}

// **************************************************************************************************

void serverFunction(const string& sourceIP, const int& sourcePort, 
                        const string& desinationIP, const int& destinationPort, const bool originator){
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
    bool success = sendGreeting(connectSocket, 1);
    if(!success) exit(EXIT_FAILURE);// Error is already printed.

    // Receive greeting back from client (or acknowledge error).
    success = receiveGreeting(connectSocket, true);
    if(!success) exit(EXIT_FAILURE);// Error is already printed.

    // Receive "DATA" from client.
    success = receiveDATAString(connectSocket);
    if(!success){
        // Handle "DATA" not properly received.
        cout << "DATA not properly recieved from server." << endl;
    }
    

    cout << "Server done" << endl << endl;
    close(connectSocket);
}

// **************************************************************************************************

void clientFunction(const string& sourceIP, const int& sourcePort, 
                        const string& desinationIP, const int& destinationPort, const bool originator){
    int connectSocket;
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
        close(connectSocket);
        exit(EXIT_FAILURE);
    }

    if (connect(connectSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        cout << "Connection Failed" << endl; 
        close(connectSocket);
        exit(EXIT_FAILURE);
    }

    // Connected to server socket. Wait for greeting.
    bool success = receiveGreeting(connectSocket, false);
    if(!success) exit(EXIT_FAILURE);// Error is already printed.

    // Acknowledge server by sending back greeting.
    success = sendGreeting(connectSocket, 0);
    if(!success) exit(EXIT_FAILURE);// Error is already printed.
    
    // Send "DATA" to server.
    success = sendDATAString(connectSocket);
    if(!success) {
        // Handle "DATA" not able to be sent.
        close(connectSocket);
        cout << "DATA could not be sent from client to server." << endl;
        exit(EXIT_FAILURE);
    }
/*
    // Send actual data to server. Start with headers.
    string curHeader = "Hop: ";
    if(originator) curHeader += 
    send(connectSocket, PROPER_GREETING, PROPER_GREETING_LEN, 0 ); 
*/
    close(connectSocket);

    cout << "Client done" << endl<< endl;
}

string getCurrentTimestamp(){
    // current date/time based on current system
    timeval tvTime;
    gettimeofday(&tvTime, NULL);

    int iTotal_seconds = tvTime.tv_sec;
    struct tm *ptm = localtime((const time_t *) & iTotal_seconds);

    int iHour = ptm->tm_hour;;
    int iMinute = ptm->tm_min;
    int iSecond = ptm->tm_sec;
    int iMilliSec = tvTime.tv_usec / 1000;
    return to_string(iHour) + ":" + to_string(iMinute) + ":" + to_string(iSecond) + ":" + to_string(iMilliSec);
}

void initializeMessage(string& transmitMessage, const string& fromHost, const string& toHost){
	
    // Get system name.
    utsname name;
    if(uname(&name)) exit(-1);
    string sysname = name.sysname;
    string sysrelease = name.sysname;
    
    string timestamp = getCurrentTimestamp();

    transmitMessage += "Hop: 0\r\n";
    transmitMessage += "MessageId: 1234\r\n";
    transmitMessage += "FromHost: " + fromHost + "\r\n";
    transmitMessage += "ToHost: " + toHost + "\r\n";
    transmitMessage += "System: " + sysname + "/" + sysrelease + "\r\n";
    transmitMessage += "Program: C++11/GCC\r\n";
    transmitMessage += "Author: Jared Heidt\r\n";
    transmitMessage += "SendingTimestamp: " + timestamp + "\r\n";


	printf("Your computer's OS is %s@%s\n", name.sysname, name.release);


}
