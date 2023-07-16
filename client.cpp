#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

const int BUFFER_SIZE = 4096;
const std::string SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8888;

void printMenu() {
    std::cout << "=== Chat Client ===" << std::endl;
    std::cout << "1. Register" << std::endl;
    std::cout << "2. Login" << std::endl;
    std::cout << "3. Join Chat Room" << std::endl;
    std::cout << "4. Leave Chat Room" << std::endl;
    std::cout << "5. Send Message" << std::endl;
    std::cout << "6. List Chat Rooms" << std::endl;
    std::cout << "7. Kick User" << std::endl;
    std::cout << "8. Ban User" << std::endl;
    std::cout << "9. Grant Moderator Rights" << std::endl;
    std::cout << "10. Revoke Moderator Rights" << std::endl;
    std::cout << "11. Show Profile" << std::endl;
    std::cout << "12. Update Profile" << std::endl;
    std::cout << "13. Send File" << std::endl;
    std::cout << "14. Get File" << std::endl;
    std::cout << "15. Exit" << std::endl;
    std::cout << "==================" << std::endl;
    std::cout << "Select an option: ";
}

SOCKET connectToServer() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return INVALID_SOCKET;
    }

    // Create a socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Connect to the server
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP.c_str());
    if (serverAddress.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Failed to parse server IP address." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to the server." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return clientSocket;
}

void sendRequest(SOCKET clientSocket, const std::string& request) {
    send(clientSocket, request.c_str(), static_cast<int>(request.length()), 0);
}

std::string receiveResponse(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead <= 0) {
        std::cerr << "Error receiving response from the server." << std::endl;
        return "";
    }
    return std::string(buffer);
}

void registerUser(SOCKET clientSocket) {
    std::string username, password;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    std::cout << "Enter password: ";
    std::getline(std::cin, password);

    std::string request = "REGISTER:" + username + ":" + password + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void loginUser(SOCKET clientSocket) {
    std::string username, password;
    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    std::cout << "Enter password: ";
    std::getline(std::cin, password);

    std::string request = "AUTHENTICATE:" + username + ":" + password + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void joinChatRoom(SOCKET clientSocket) {
    std::string roomName;
    std::cout << "Enter chat room name: ";
    std::getline(std::cin, roomName);

    std::string request = "JOIN:" + roomName + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void leaveChatRoom(SOCKET clientSocket) {
    std::string roomName;
    std::cout << "Enter chat room name: ";
    std::getline(std::cin, roomName);

    std::string request = "LEAVE:" + roomName + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void sendMessage(SOCKET clientSocket) {
    std::string roomName, message;
    std::cout << "Enter chat room name: ";
    std::getline(std::cin, roomName);
    std::cout << "Enter message: ";
    std::getline(std::cin, message);

    std::string request = "SEND_ROOM:" + roomName + ":" + message + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void listChatRooms(SOCKET clientSocket) {
    std::string request = "LIST:\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void kickUser(SOCKET clientSocket) {
    std::string roomName;
    SOCKET targetSocket;
    std::cout << "Enter chat room name: ";
    std::getline(std::cin, roomName);
    std::cout << "Enter target user socket: ";
    std::cin >> targetSocket;
    std::cin.ignore(); // Ignore newline character

    std::string request = "KICK_USER:" + roomName + ":" + std::to_string(targetSocket) + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void banUser(SOCKET clientSocket) {
    std::string roomName;
    SOCKET targetSocket;
    std::cout << "Enter chat room name: ";
    std::getline(std::cin, roomName);
    std::cout << "Enter target user socket: ";
    std::cin >> targetSocket;
    std::cin.ignore(); // Ignore newline character

    std::string request = "BAN_USER:" + roomName + ":" + std::to_string(targetSocket) + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void grantModeratorRights(SOCKET clientSocket) {
    std::string roomName;
    SOCKET targetSocket;
    std::cout << "Enter chat room name: ";
    std::getline(std::cin, roomName);
    std::cout << "Enter target user socket: ";
    std::cin >> targetSocket;
    std::cin.ignore(); // Ignore newline character

    std::string request = "GRANT_MODERATOR:" + roomName + ":" + std::to_string(targetSocket) + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void revokeModeratorRights(SOCKET clientSocket) {
    std::string roomName;
    SOCKET targetSocket;
    std::cout << "Enter chat room name: ";
    std::getline(std::cin, roomName);
    std::cout << "Enter target user socket: ";
    std::cin >> targetSocket;
    std::cin.ignore(); // Ignore newline character

    std::string request = "REVOKE_MODERATOR:" + roomName + ":" + std::to_string(targetSocket) + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void showProfile(SOCKET clientSocket) {
    std::string request = "PROFILE:\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void updateProfile(SOCKET clientSocket) {
    std::string username, password;
    std::cout << "Enter new username: ";
    std::getline(std::cin, username);
    std::cout << "Enter new password: ";
    std::getline(std::cin, password);

    std::string request = "UPDATE_PROFILE:" + username + ":" + password + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void sendFile(SOCKET clientSocket) {
    std::string roomName, fileName;
    std::cout << "Enter chat room name: ";
    std::getline(std::cin, roomName);
    std::cout << "Enter file name: ";
    std::getline(std::cin, fileName);

    std::string request = "SEND_FILE:" + roomName + ":" + fileName + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

void getFile(SOCKET clientSocket) {
    std::string roomName, fileName;
    std::cout << "Enter chat room name: ";
    std::getline(std::cin, roomName);
    std::cout << "Enter file name: ";
    std::getline(std::cin, fileName);

    std::string request = "GET_FILE:" + roomName + ":" + fileName + "\n";
    sendRequest(clientSocket, request);

    std::string response = receiveResponse(clientSocket);
    std::cout << response << std::endl;
}

int main() {
    SOCKET clientSocket = connectToServer();
    if (clientSocket == INVALID_SOCKET) {
        return 1;
    }

    std::string option;
    while (true) {
        printMenu();
        std::getline(std::cin, option);

        if (option == "1") {
            registerUser(clientSocket);
        } else if (option == "2") {
            loginUser(clientSocket);
        } else if (option == "3") {
            joinChatRoom(clientSocket);
        } else if (option == "4") {
            leaveChatRoom(clientSocket);
        } else if (option == "5") {
            sendMessage(clientSocket);
        } else if (option == "6") {
            listChatRooms(clientSocket);
        } else if (option == "7") {
            kickUser(clientSocket);
        } else if (option == "8") {
            banUser(clientSocket);
        } else if (option == "9") {
            grantModeratorRights(clientSocket);
        } else if (option == "10") {
            revokeModeratorRights(clientSocket);
        } else if (option == "11") {
            showProfile(clientSocket);
        } else if (option == "12") {
            updateProfile(clientSocket);
        } else if (option == "13") {
            sendFile(clientSocket);
        } else if (option == "14") {
            getFile(clientSocket);
        } else if (option == "15") {
            break;
        } else {
            std::cout << "Invalid option. Please try again." << std::endl;
        }
    }

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}
