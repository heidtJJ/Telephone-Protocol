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
#define PROPER_GREETING "HELLO 1.7\r\n"
#define PROPER_GREETING_LEN 11
#define DATA "DATA\r\n"
#define DATA_LEN 6
#define CRLF "\r\n"
#define EOM "\r\n.\r\n"
#define MESSAGE_ID "MessageId"
#define MESSAGE_ID_LEN 9
#define SUCCESS "SUCCESS\r\n"
#define SUCCESS_LEN 9
#define WARNING "WARN\r\n"
#define WARNING_LEN 6
#define QUIT "QUIT\r\n"
#define QUIT_LEN 6
#define GOODBYE "GOODBYE\r\n"
#define GOODBYE_LEN 9
#define PROGRAM "Program"
#define PROGRAM_LEN 7
#define PLATFORM "System"
#define PLATFORM_LEN 6
#define TIMESTAMP "SendingTimestamp"
#define TIMESTAMP_LEN 16
#define HOP "Hop"
#define HOP_LEN 3
#define FROM_HOST "FromHost"
#define FROM_HOST_LEN 8
#define IP 0
#define PORT 1

// Server and Client.
void clientFunction(const string& sourceIP, const string& sourcePort, const string& desinationIP, const string& destinationPort, const bool originator, string& message);
string serverFunction(const string& sourceIP, const int& sourcePort, const string& desinationIP, const int& destinationPort, const bool originator, string& errorMessage);

// General Utility Functions.
vector<string> get_IP_PORT(const string& hostName);
uint16_t checksum(void* data, size_t size);
string intToHexStr(uint16_t i);
string getCurrentTimestamp();

// Validation Utility Functions.
string validateHeader(const string& message);

// Message Sending Utility Functions
bool receiveGreeting(const int& connectSocket, bool server);
bool sendGreeting(const int& connectSocket, bool server);
bool receiveDATAString(const int& connectSocket);
bool sendDATAString(const int& connectSocket);

// Header Manipulation Utility Functions
void appendMessageHeaders(string& message, const string& fromHost, const string& fromPort, const string& toHost, const string& toPort);
void initializeMessage(string& message, const string& fromHost, const string& fromPort, const string& toHost, const string& toPort);
string getHeaders(const string& fromHost, const string& fromPort, const string& toHost, const string& toPort, const int& nextId, const int& nextHop);
string getHeaderData(const string& message, const string& headerName);
string getMessageData(const string& message);
void printStatistics(const string& received);


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
    vector<string> ip_port_source = get_IP_PORT(source);
    if(ip_port_source.empty()){
        cout << "Invalid source address!" << endl;
        cout << "$ ./telephone <originator> <source> <dest>" << endl;
        exit(EXIT_FAILURE);
    }

    string dest = argv[3];
    vector<string> ip_port_dest = get_IP_PORT(dest);
    if(ip_port_dest.empty()){
        cout << "Invalid destination address!" << endl;
        cout << "$ ./telephone <originator> <source> <dest>" << endl;
        exit(EXIT_FAILURE);
    }
    int sourcePort = stoi(ip_port_source[PORT]);
    int destPort = stoi(ip_port_dest[PORT]);

    string errorMessage = "";
    if(originator == "1"){
        // User is originator.
        string transmitMessage = "";
        clientFunction(ip_port_source[IP], ip_port_source[PORT], ip_port_dest[IP], ip_port_dest[PORT], true, transmitMessage);

        string received = serverFunction(ip_port_source[IP], sourcePort, ip_port_dest[IP], destPort, true, errorMessage);
        cout << "FINAL RECEIVED MESSAGE..." << endl << endl << received << endl << endl;

        // Find statistics of final message.
        cout << "FINAL MESSAGE STATISTICS... \n";
        printStatistics(received);
    }
    else {
        // User is not originator.
        string received = serverFunction(ip_port_source[IP], sourcePort, ip_port_dest[IP], destPort, false, errorMessage);
        cout << "RECEIVED MESSAGE..." << endl << endl << received << endl;
        
        // Allow time for remote server to open up port (network delay).
        sleep(1);
        received = errorMessage + received;
        clientFunction(ip_port_source[IP], ip_port_source[PORT], ip_port_dest[IP], ip_port_dest[PORT], false, received);
    }

    return 0;
}

// **************************************************************************************************

string serverFunction(const string& sourceIP, const int& sourcePort, 
        const string& desinationIP, const int& destinationPort, const bool originator, string& errorMessage){
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
        exit(EXIT_FAILURE);
    }
    // Read data from client
    char buffer[1024] = {'\0'}; 
    read(connectSocket, buffer, 1024);
    string messageFromClient = buffer;

    // Validate headers.
    errorMessage = validateHeader(messageFromClient);
    if(errorMessage.empty())
        send(connectSocket, SUCCESS, SUCCESS_LEN, 0);
    else 
        send(connectSocket, WARNING, WARNING_LEN, 0); 

    // Read QUIT from client.
    read(connectSocket, buffer, QUIT_LEN);

    // Send GOODBYE to client.
    send(connectSocket, GOODBYE, GOODBYE_LEN, 0); 

    // Close connection socket.
    close(connectSocket);

    return messageFromClient;
}

// **************************************************************************************************

void clientFunction(const string& sourceIP, const string& sourcePort, 
        const string& destinationIP, const string& destinationPort, 
        const bool originator, string& message){
    if(originator){
        initializeMessage(message, sourceIP, sourcePort, destinationIP, destinationPort);
    }
    else {
        // Add own headers to message.
        appendMessageHeaders(message, sourceIP, sourcePort, destinationIP, destinationPort);
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
    serverAddress.sin_port = htons( atoi(destinationPort.c_str()) ); 
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

    // Read either SUCCESS or WARN from server.
    char buffer[SUCCESS_LEN+1] = {'\0'}; 
    read(connectSocket, buffer, SUCCESS_LEN);
    
    // Send QUIT to server.
    send(connectSocket, QUIT, QUIT_LEN, 0); 
    
    // Reset buffer
    memset(buffer, '\0', SUCCESS_LEN+1);

    // Read either SUCCESS or WARN from server.
    read(connectSocket, buffer, GOODBYE_LEN);

    close(connectSocket);
}


// **************************************************************************************************

string intToHexStr(uint16_t i){
  stringstream stream;
  stream << std::setfill ('0') << std::setw(sizeof(uint16_t)*2) 
         << std::hex << i;
  return stream.str();
}

// **************************************************************************************************

uint16_t checksum(void* data, size_t size) {
    if(data == NULL){
        cout << "Checksum input is NULL." << endl;
        exit(EXIT_FAILURE);
    }

    uint32_t sum = 0;
    /* Cast to uint16_t for pointer arithmetic */
    uint16_t* data16 = (uint16_t*) data;

    while(size > 1) {
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

vector<string> get_IP_PORT(const string& hostName){
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

// bool server is existent for debugging.
bool sendGreeting(const int& connectSocket, const bool server){
    int result = send(connectSocket, PROPER_GREETING, PROPER_GREETING_LEN, 0 ); 
    if(result != PROPER_GREETING_LEN){
        cout << "Greeting could not be sent." << endl; 
        return false;
    }
    return true;
}

// **************************************************************************************************

bool sendDATAString(const int& connectSocket){
    int result = send(connectSocket, DATA, DATA_LEN, 0); 
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
    if(numBytesRead != DATA_LEN){
        cout << "DATA string unsuccessfully received." << endl; 
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
    if(numBytesRead != PROPER_GREETING_LEN){
        // Version is unsupported.
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

    // Validate checksum.
    string receivedCheckSumStr = getHeaderData(message, "MessageChecksum");
    string messageData = getMessageData(message);

    // Recompute checksum.
    uint16_t actualCheckSum = checksum((void*)messageData.c_str(), messageData.length()); 
    string actualCheckSumStr = intToHexStr(actualCheckSum);

    // Compare checksums.
    string hop = to_string(atoi(getHeaderData(message, "Hop").c_str())+1);
    if(actualCheckSumStr != receivedCheckSumStr){
        errorMessage += "Warning: Checksum is not valid at hop " + hop + CRLF;
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
    
    /*
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
    */
    return errorMessage;
}

// **************************************************************************************************

// current UTC date/time based on current system
string getCurrentTimestamp(){
    timeval tvTime;
    gettimeofday(&tvTime, NULL);

    int iTotal_seconds = tvTime.tv_sec;
    struct tm *ptm = gmtime((const time_t *) & iTotal_seconds);

    if(ptm == NULL){
        cout << "Cannot retrieve UTC time." << endl;
        exit(EXIT_FAILURE);
    }

    int iHour = ptm->tm_hour;
    int iMinute = ptm->tm_min;
    int iSecond = ptm->tm_sec;
    int iMilliSec = tvTime.tv_usec / 1000;
    return to_string(iHour) + ":" + to_string(iMinute) + ":" + to_string(iSecond) + ":" + to_string(iMilliSec);
}

// **************************************************************************************************

string getHeaders(const string& fromHost, const string& fromPort, const string& toHost, 
        const string& toPort, const string& nextId, const int& nextHop){
    // Get system name.
    utsname name;
    if(uname(&name)) exit(EXIT_FAILURE);
    string sysname = name.sysname;
    string sysrelease = name.release;
    
    string timestamp = getCurrentTimestamp();
    string headers = "";
    headers += "Hop: " + to_string(nextHop) + "\r\n";
    headers += "MessageId: " + nextId + "\r\n";
    headers += "FromHost: " + fromHost + ":" + fromPort + CRLF;
    headers += "ToHost: " + toHost + ":" + toPort + CRLF;
    headers += "System: " + sysname + "/" + sysrelease + CRLF;
    headers += "Program: C++11/GCC\r\n";
    headers += "Author: Jared Heidt\r\n";
    headers += "SendingTimestamp: " + timestamp + CRLF;
    return headers;
}

// **************************************************************************************************

void initializeMessage(string& message, const string& fromHost, const string& fromPort, 
        const string& toHost, const string& toPort){
    string headers = getHeaders(fromHost, fromPort, toHost, toPort, "1", 0);
    message = headers;

    // Append checkSum.
    string messageData = "Hello! You're receiving a message from the telephone game!";
    uint16_t checkSum = checksum((void*)messageData.c_str(), messageData.length());
    message += "MessageChecksum: " + intToHexStr(checkSum) + CRLF CRLF;
    
    // Append data.
    message += messageData;
    message += EOM;
}

// **************************************************************************************************

void appendMessageHeaders(string& message, const string& fromHost, const string& fromPort, const string& toHost, const string& toPort){
    string nextId = getHeaderData(message, MESSAGE_ID);
    int nextHop = stoi( getHeaderData(message, "Hop") )+1;
    string headers = getHeaders(fromHost, fromPort, toHost, toPort, nextId, nextHop);

    // Append checkSum.
    string messageData = getMessageData(message);
    uint16_t checkSum = checksum((void*)messageData.c_str(), messageData.length());
    headers += "MessageChecksum: " + intToHexStr(checkSum) + CRLF;
    
    // Append data.
    message = headers + message;
}

// **************************************************************************************************

string getHeaderData(const string& message, const string& headerName){
    if(message.empty()) return message;
    size_t headerPosition = message.find(headerName);
    if(headerPosition == string::npos) return "";
    size_t endOfHeaderPosition = message.find(CRLF, headerPosition+1);
    // +2 for ':' and ' '
    return message.substr(headerPosition + headerName.length()+2, endOfHeaderPosition-(headerPosition + headerName.length()+2));
}

// **************************************************************************************************

string getMessageData(const string& message){
    if(message.empty()) return message;
    size_t startOfMessage = message.find("\r\n\r\n")+4;
    if(startOfMessage == string::npos) return "";
    size_t endOfMessage = message.length()-5;
    return message.substr(startOfMessage, endOfMessage - startOfMessage);
}

// **************************************************************************************************

struct Time {
    Time(int milli, int sec, int min, int hour) : 
        milliseconds(milli), seconds(sec), minutes(min), hours(hour) {};
    
    // hr:mi:se:mss format
    Time(const string& time) {
        int startIdx = 0;
        // Get hours
        int semiColonIdx = time.find(':', startIdx);
        hours = atoi(time.substr(startIdx, semiColonIdx).c_str());
        
        // Get minutes
        startIdx = semiColonIdx+1;
        semiColonIdx = time.find(':', startIdx);
        minutes = atoi(time.substr(startIdx, semiColonIdx - startIdx).c_str());

        // Get seconds
        startIdx = semiColonIdx+1;
        semiColonIdx = time.find(':', startIdx);
        seconds = atoi(time.substr(startIdx, semiColonIdx - startIdx).c_str());

        // Get milliseconds
        startIdx = semiColonIdx+1;
        semiColonIdx = time.find(':', startIdx);
        milliseconds = atoi(time.substr(startIdx).c_str());
    };    
    int hours;
    int milliseconds;
    int seconds;
    int minutes;
}; 

string differenceTimes(const Time& start, const Time& end){
    unsigned int milliSecStart = start.milliseconds + start.seconds*1000 +
            start.minutes*60*1000 + start.hours*3600*1000;
    unsigned int milliSecEnd = end.milliseconds + end.seconds*1000 +
            end.minutes*60*1000 + end.hours*3600*1000;   
    int result = milliSecEnd - milliSecStart;
    return to_string(result/1000/3600) + ":" + to_string(result/1000/60) + ":" + to_string(result/1000) + ":" + to_string(result%1000);
}

// **************************************************************************************************

void printStatistics(const string& message){
    // Print number of machines passed through.

    std::unordered_set<string> machinesUsed;
    size_t start = 0;
    while(start >= 0 && start < message.length()){
        // Find start index of the next message id.
        start = message.find(FROM_HOST, start);
        if(start == string::npos) break;
        // Find the end index of this header.
        size_t endOfHeaderPosition = message.find(CRLF, start+1);
        // +1 for ':' and +1 ' '
        string hostname = message.substr(start + FROM_HOST_LEN+2, endOfHeaderPosition-(start + FROM_HOST_LEN+2));
        vector<string> ip_port = get_IP_PORT(hostname);
        machinesUsed.insert(ip_port[0]);
        ++start;
    }
    cout << "Number of machines passed through: " << machinesUsed.size() << endl;

    // Print list of languages used.
    std::unordered_set<string> languagesUsed;
    start = 0;
    while(start >= 0 && start < message.length()){
        // Find start index of the next message id.
        start = message.find(PROGRAM, start);
        if(start == string::npos) break;
        // Find the end index of this header.
        size_t endOfHeaderPosition = message.find(CRLF, start+1);
        
        // +1 for ':' and +1 ' '
        string programName = message.substr(start + PROGRAM_LEN+2, endOfHeaderPosition-(start + PROGRAM_LEN+2));
        
        // Insert current id into the messageIds set.
        languagesUsed.insert(programName);
        ++start;
    }
    cout << "Languages used: "; 
    bool firstIt = true;
    for(const string& programName : languagesUsed){
        if(!firstIt){
            cout << ", ";
        }
        cout << programName;
        firstIt = false;
    }
    cout << endl;

    // Get list of platforms used.
    std::unordered_set<string> platformsUsed;
    start = 0;
    while(start >= 0 && start < message.length()){
        // Find start index of the next message id.
        start = message.find(PLATFORM, start);
        if(start == string::npos) break;
        // Find the end index of this header.
        size_t endOfHeaderPosition = message.find(CRLF, start+1);
        
        // +1 for ':' and +1 ' '
        string platformName = message.substr(start + PLATFORM_LEN+2, endOfHeaderPosition-(start + PLATFORM_LEN+2));
        
        // Insert current id into the messageIds set.
        platformsUsed.insert(platformName);
        ++start;
    }
    cout << "Platforms used: "; 
    firstIt = true;
    for(const string& platformName : platformsUsed){
        if(!firstIt){
            cout << ", ";
        }
        cout << platformName;
        firstIt = false;
    }
    cout << endl;


    // Get time spent at each hop.
    int curHop = atoi(getHeaderData(message, HOP).c_str());

    int end = 0;
    end = message.find(TIMESTAMP, end);
    if(end == string::npos)
        cout << "STATS: TIMESTAMP ERROR" << endl;

    start = 0; 
    while(start >= 0 && start < message.length() && end >= 0 && end < message.length()){
        start = end+1;
        start = message.find(TIMESTAMP, start);
        if(start == string::npos) break;
        
        // Find the end index of the end time header.
        size_t end_endOfHeaderPosition = message.find(CRLF, end+1);
        
        // Find the end index of the start time header.
        size_t start_endOfHeaderPosition = message.find(CRLF, start+1);
        
        // +1 for ':' and +1 ' '
        string endTimeString = message.substr(end + TIMESTAMP_LEN+2, end_endOfHeaderPosition-(end + TIMESTAMP_LEN+2));
        string startTimeString = message.substr(start + TIMESTAMP_LEN+2, start_endOfHeaderPosition-(start + TIMESTAMP_LEN+2));
        
        Time endTime(endTimeString);
        Time startTime(startTimeString);

        string diff = differenceTimes(startTime, endTime);
        cout << "Time spent between hop " << curHop-- << " and hop " << curHop << " is " << diff << endl;
        end = start;
    }
}