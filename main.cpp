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
#include <sstream>
#include <unordered_set>
#include <iomanip>

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;
using std::to_string;
using std::stringstream;

// Constants 
#define PROPER_GREETING "HELLO 1.7"
#define PROPER_GREETING_LEN 9
#define DATA "DATA"
#define DATA_LEN 4
#define CRLF "\r\n"
#define EOM "\r\n.\r\n"
#define MESSAGE_ID "MessageId"
#define MESSAGE_ID_LEN 9
#define SUCCESS "SUCCESS"
#define SUCCESS_LEN 7
#define WARNING "WARN"
#define WARNING_LEN 4
#define QUIT "QUIT"
#define QUIT_LEN 4
#define GOODBYE "GOODBYE"
#define GOODBYE_LEN 7

// Utility functions
uint16_t checksum(void *data, size_t size);
vector<string> getIpAndPort(const string& hostName);
bool receiveGreeting(const int& connectSocket, bool server);

// Server/Client
string serverFunction(const string& sourceIP, const int& sourcePort, const string& desinationIP, const int& destinationPort, const bool originator);
void clientFunction(const string& sourceIP, const int& sourcePort, const string& desinationIP, const int& destinationPort, const bool originator, string& message);
void initializeMessage(string& message, const string& fromHost, const string& fromPort, const string& toHost, const string& toPort);

// Retreives the data of a header in a message.
string getHeaderData(const string& message, const string& headerName){
    if(message.empty()) return message;
    size_t headerPosition = message.find(headerName);
    if(headerPosition == string::npos) return "";
    size_t endOfHeaderPosition = message.find(CRLF, headerPosition+1);
    // +2 for ':' and ' '
    return message.substr(headerPosition + headerName.length()+2, endOfHeaderPosition-(headerPosition + headerName.length()+2));
}

string getMessageData(const string& message){
    if(message.empty()) return message;
    size_t startOfMessage = message.find("\r\n\r\n")+4;
    if(startOfMessage == string::npos) return "";
    size_t endOfMessage = message.length()-5;
    return message.substr(startOfMessage, endOfMessage - startOfMessage);
}

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

    string transmitMessage = "";
    if(originator == "1"){
        // user is originator
        clientFunction(sourceIP, sourcePort, destinationIP, destPort, true, transmitMessage);
        string received = serverFunction(sourceIP, sourcePort, destinationIP, destPort, true);
    }
    else {
        // user is not originator
        string received = serverFunction(sourceIP, sourcePort, destinationIP, destPort, false);
        clientFunction(sourceIP, sourcePort, destinationIP, destPort, false, transmitMessage);
    }

    return 0;
}

// **************************************************************************************************

template< typename T >
std::string int_to_hexStr( T i ) {
  std::stringstream stream;
  stream << std::setfill ('0') << std::setw(sizeof(T)*2) 
         << std::hex << i;
    cout << "int_to_hexStr\n\n\n";
  return stream.str();
}

// **************************************************************************************************

uint16_t checksum(void* data, size_t size) {
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
    char buffer[DATA_LEN+1] = {'\0'}; 
    int numBytesRead = read(connectSocket, buffer, DATA_LEN);
    cout << "Server read: " << buffer << " -> " << numBytesRead << endl;
    if(numBytesRead != DATA_LEN){
        perror("DATA string unsuccessfully received."); 
        return false;
    }

    string greeting = buffer;
    if(greeting != DATA){
        send(connectSocket, "QUIT", 4, 0); 
        perror("DATA string unsuccessfully received."); 
        return false; 
    }
    return true;
}

// **************************************************************************************************

bool receiveGreeting(const int& connectSocket, bool server){
    char buffer[PROPER_GREETING_LEN+1] = {'\0'};
    int numBytesRead = read(connectSocket, buffer, PROPER_GREETING_LEN);
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
    return true;
}

// **************************************************************************************************

string validateHeader(const string& message){
    string errorMessage = "";
    cout << "Server read: " << message << endl;

    // Validate checksum.
    string receivedCheckSumStr = getHeaderData(message, "MessageChecksum");
    string messageData = getMessageData(message);
    cout << "messageData: " << messageData << endl;
    // Recompute checksum.
    uint16_t actualCheckSum = checksum((void*)messageData.c_str(), messageData.length()); 
    string actualCheckSumStr = int_to_hexStr(actualCheckSum);
    // Compare checksums.

    if(actualCheckSumStr == receivedCheckSumStr) cout << "YAS\n";
    else{
        cout << "Recieved: " << receivedCheckSumStr << endl;
        cout << "Actual: " << messageData << endl;
    }
    if(actualCheckSumStr != receivedCheckSumStr
            || actualCheckSumStr.length() != 4
            || receivedCheckSumStr.length() != 4){

        cout << actualCheckSumStr << " != " << receivedCheckSumStr << endl;
        errorMessage += "Checksum is not valid at hop " + getHeaderData(message, "Hop") + CRLF;
    }

    // Check for all valid headers.
    // Hop
    if(getHeaderData(message, "Hop").empty())
        errorMessage += "Warning: Missing required header - Hop" CRLF;

    // MessageId
    if(getHeaderData(message, "MessageId").empty())
        errorMessage += "Warning: Missing required header - MessageId" CRLF;
    
    // FromHost
    if(getHeaderData(message, "FromHost").empty())
        errorMessage += "Warning: Missing required header - FromHost" CRLF;
    
    // ToHost
    if(getHeaderData(message, "ToHost").empty())
        errorMessage += "Warning: Missing required header - ToHost" CRLF;
    
    // System
    if(getHeaderData(message, "System").empty())
        errorMessage += "Warning: Missing required header - System" CRLF;
    
    // Program
    if(getHeaderData(message, "Program").empty())
        errorMessage += "Warning: Missing required header - Program" CRLF;
    
    // Author
    if(getHeaderData(message, "Author").empty())
        errorMessage += "Warning: Missing required header - Author" CRLF;
    
    // SendingTimestamp
    if(getHeaderData(message, "SendingTimestamp").empty())
        errorMessage += "Warning: Missing required header - SendingTimestamp" CRLF;
    
    // MessageChecksum
    if(getHeaderData(message, "MessageChecksum").empty())
        errorMessage += "Warning: Missing required header - MessageChecksum" CRLF;
    
    // Search for duplicate messageId
    std::unordered_set<string> messageIds;
    size_t start = 0;
    while(start >= 0 && start < message.length()){
        // Find start index of the next message id.
        start = message.find(MESSAGE_ID, start);
        if(start == string::npos) break;
        // Find the end index of this header.
        size_t endOfHeaderPosition = message.find(CRLF, start+1);
        // +1 for ':' and +1 ' '
        string id = message.substr(start + MESSAGE_ID_LEN+2, endOfHeaderPosition-(start + MESSAGE_ID_LEN+2));
        
        // Check for duplicated ID.
        if(messageIds.find(id) != messageIds.end()){
            errorMessage += "Warning: Duplicate message ID - " + id + CRLF;
            break;
        }
        // Insert current id into the messageIds set.
        messageIds.insert(id);
        ++start;
    }

    cout << "Error Message: " << errorMessage << endl;
    cout << "End of validation" << endl;
    return errorMessage;
}

// **************************************************************************************************

string serverFunction(const string& sourceIP, const int& sourcePort, 
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
    // Read data from client
    char buffer[1024] = {'\0'}; 
    read(connectSocket, buffer, 1024);
    string buff = buffer;
    
    // Validate headers.
    string errorMessage = validateHeader(buff);
    if(errorMessage.empty()){
        send(connectSocket, SUCCESS, SUCCESS_LEN, 0);
        cout << "Server sent SUCCESS" << endl;
    }
    else {
        buff = "Warning: " + errorMessage + buff; 
        send(connectSocket, WARNING, WARNING_LEN, 0); 
        cout << "Server sent WARNING" << endl;
    }

    // Read QUIT from client.
    read(connectSocket, buffer, QUIT_LEN);
    cout << "Server read QUIT" << endl;

    // Send GOODBYE to client.
    send(connectSocket, GOODBYE, GOODBYE_LEN, 0); 
    cout << "Server sent GOODBYE" << endl;

    // Close connection socket.
    close(connectSocket);
    cout << "Server closed connection." << endl << endl;

    cout << "Server function returning:\n" << buff << endl;
    return buff;
}

// **************************************************************************************************

void clientFunction(const string& sourceIP, const int& sourcePort, 
                        const string& destinationIP, const int& destinationPort, 
                        const bool originator, string& message){

    if(originator){
        initializeMessage(message, sourceIP, to_string(sourcePort), destinationIP, to_string(destinationPort));
    }
    else {
        
    }
    
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
    if(inet_pton(AF_INET, destinationIP.c_str(), &serverAddress.sin_addr)<=0)  
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
    // START OF HANDSHAKE
    // Connected to server socket. Wait for greeting.
    bool success = receiveGreeting(connectSocket, false);
    if(!success) exit(EXIT_FAILURE);// Error is already printed.

    // Acknowledge server by sending back greeting.
    success = sendGreeting(connectSocket, 0);
    if(!success) exit(EXIT_FAILURE);// Error is already printed.
    // END OF HANDSHAKE

    // START OF DATA TRANSFER
    // Send "DATA" string to server.
    success = sendDATAString(connectSocket);
    if(!success) {
        // Handle "DATA" not able to be sent.
        close(connectSocket);
        cout << "DATA could not be sent from client to server." << endl;
        exit(EXIT_FAILURE);
    }

    // Send actual data to server. Start with headers.
    send(connectSocket, message.c_str(), message.length(), 0 ); 
    cout << endl << "Client sent: " << message << endl;

    // Read either SUCCESS or WARN from server.
    char buffer[SUCCESS_LEN+1] = {'\0'}; 
    read(connectSocket, buffer, SUCCESS_LEN);
    cout << "Client read " << buffer << endl;
    
    // Send QUIT to server.
    send(connectSocket, QUIT, QUIT_LEN, 0); 
    cout << "Client sent " << QUIT << endl;
    
    // Reset buffer
    memset(buffer, '\0', SUCCESS_LEN+1);

    // Read either SUCCESS or WARN from server.
    read(connectSocket, buffer, GOODBYE_LEN);
    cout << "Client read " << GOODBYE << endl;

    int val = close(connectSocket);
    cout << "Client closed connection: " << val << endl;
}

// **************************************************************************************************

// current UTC date/time based on current system
string getCurrentTimestamp(){
    timeval tvTime;
    gettimeofday(&tvTime, NULL);

    int iTotal_seconds = tvTime.tv_sec;
    struct tm *ptm = gmtime((const time_t *) & iTotal_seconds);

    int iHour = ptm->tm_hour;;
    int iMinute = ptm->tm_min;
    int iSecond = ptm->tm_sec;
    int iMilliSec = tvTime.tv_usec / 1000;
    return to_string(iHour) + ":" + to_string(iMinute) + ":" + to_string(iSecond) + ":" + to_string(iMilliSec);
}

// **************************************************************************************************

string getHeaders(const string& fromHost, const string& fromPort, const string& toHost, const string& toPort){
    // Get system name.
    utsname name;
    if(uname(&name)) exit(-1);
    string sysname = name.sysname;
    string sysrelease = name.release;
    
    string timestamp = getCurrentTimestamp();
    string headers = "";
    headers += "Hop: 0\r\n";
    headers += "MessageId: 1234\r\n";
    headers += "FromHost: " + fromHost + ":" + fromPort + CRLF;
    headers += "ToHost: " + toHost + ":" + toPort + CRLF;
    headers += "System: " + sysname + "/" + sysrelease + CRLF;
    headers += "Program: C++11/GCC\r\n";
    headers += "Author: Jared Heidt\r\n";
    headers += "SendingTimestamp: " + timestamp + CRLF;
    return headers;
}

// **************************************************************************************************

void initializeMessage(string& message, const string& fromHost, const string& fromPort, const string& toHost, const string& toPort){
    string headers = getHeaders(fromHost, fromPort, toHost, toPort);
    message = headers;

    // Append checkSum.
    string messageData = "Hello! You're receiving a message from the telephone game!";
    uint16_t checkSum = checksum((void*)messageData.c_str(), messageData.length());
    message += "MessageChecksum: " + int_to_hexStr(checkSum) + CRLF CRLF;
    
    // Append data.
    message += messageData;
    message += EOM;
}
