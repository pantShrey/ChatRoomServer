#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <winsock2.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional> 

#pragma comment(lib, "ws2_32.lib") // Link with the Windows Sockets Library (WS2_32)

const int BUFFER_SIZE = 4096;
const int MAX_CLIENTS = 10;
const std::string USER_DATABASE_FILE = "user_database.txt";
const std::string FILE_STORAGE_DIRECTORY = "file_storage/";

struct Client {
    SOCKET socket;
    std::string username;
    std::string profilePicture;
    std::string statusMessage;
    bool hasUnreadMessages;
};

struct ChatRoom {
    std::string name;
    std::unordered_set<SOCKET> members;
    std::vector<std::string> messageHistory;
};

std::vector<Client> clients;
std::unordered_map<std::string, std::string> userCredentials;
std::unordered_map<std::string, ChatRoom> chatRooms;
std::mutex clientsMutex;
std::condition_variable clientCV;

void broadcastMessage(const std::string& message, const std::unordered_set<SOCKET>& recipients, SOCKET senderSocket) {
    for (SOCKET recipientSocket : recipients) {
        if (recipientSocket != senderSocket) {
            send(recipientSocket, message.c_str(), static_cast<int>(message.length()), 0);
        }
    }
}

void joinChatRoom(const std::string& roomName, SOCKET clientSocket) {
    if (chatRooms.count(roomName) == 0) {
        chatRooms[roomName] = ChatRoom{ roomName };
    }

    chatRooms[roomName].members.insert(clientSocket);
}

void leaveChatRoom(const std::string& roomName, SOCKET clientSocket) {
    if (chatRooms.count(roomName) != 0) {
        chatRooms[roomName].members.erase(clientSocket);
    }
}

bool clientSocketMatches(const Client& client, SOCKET clientSocket) {
    return client.socket == clientSocket;
}

void sendNotificationToClient(const std::string& message, SOCKET clientSocket) {
    std::string notification = "[Notification] " + message + "\n";
    send(clientSocket, notification.c_str(), static_cast<int>(notification.length()), 0);
}

bool authenticateUser(const std::string& username, const std::string& password) {
    if (userCredentials.count(username) != 0) {
        return userCredentials[username] == password;
    }
    return false;
}

void sendAuthenticationResponse(bool authenticated, SOCKET clientSocket) {
    std::string response = authenticated ? "Authentication successful!\n" : "Authentication failed. Invalid credentials.\n";
    send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
}

std::string getChatRoomList() {
    std::string roomList;
    for (const auto& chatRoom : chatRooms) {
        roomList += chatRoom.second.name + "\n";
    }
    return roomList;
}

bool isClientInChatRoom(const std::string& roomName, SOCKET clientSocket) {
    if (chatRooms.count(roomName) != 0) {
        const auto& members = chatRooms[roomName].members;
        return members.count(clientSocket) != 0;
    }
    return false;
}

bool isUserModerator(const std::string& roomName, SOCKET clientSocket) {
    if (chatRooms.count(roomName) != 0) {
        const auto& members = chatRooms[roomName].members;
        for (const auto& client : clients) {
            if (client.socket == clientSocket && members.count(clientSocket) != 0) {
                return true;
            }
        }
    }
    return false;
}

void sendPrivateMessage(const std::string& recipientUsername, const std::string& message, SOCKET senderSocket) {
    std::string senderUsername;
    for (const auto& client : clients) {
        if (client.socket == senderSocket) {
            senderUsername = client.username;
            break;
        }
    }
    for (const auto& client : clients) {
        if (client.username == recipientUsername) {
            std::string privateMsg = "[Private] " + senderUsername + ": " + message + "\n";
            send(client.socket, privateMsg.c_str(), static_cast<int>(privateMsg.length()), 0);
            return;
        }
    }
}

void kickUserFromChatRoom(const std::string& roomName, SOCKET clientSocket) {
    if (chatRooms.count(roomName) != 0) {
        auto& members = chatRooms[roomName].members;
        members.erase(clientSocket);
        std::string kickMsg = "You have been kicked from the chat room: " + roomName + "\n";
        send(clientSocket, kickMsg.c_str(), static_cast<int>(kickMsg.length()), 0);
    }
}

void banUserFromChatRoom(const std::string& roomName, SOCKET clientSocket) {
    if (chatRooms.count(roomName) != 0) {
        auto& members = chatRooms[roomName].members;
        members.erase(clientSocket);
        std::string banMsg = "You have been banned from the chat room: " + roomName + "\n";
        send(clientSocket, banMsg.c_str(), static_cast<int>(banMsg.length()), 0);
    }
}

void grantModeratorRights(const std::string& roomName, SOCKET clientSocket) {
    if (chatRooms.count(roomName) != 0) {
        auto& members = chatRooms[roomName].members;
        for (auto& client : clients) {
            if (client.socket == clientSocket && members.count(clientSocket) != 0) {
                client.statusMessage = "[Moderator]";
                std::string grantModMsg = "You have been granted moderator rights in the chat room: " + roomName + "\n";
                send(client.socket, grantModMsg.c_str(), static_cast<int>(grantModMsg.length()), 0);
                return;
            }
        }
    }
}

void revokeModeratorRights(const std::string& roomName, SOCKET clientSocket) {
    if (chatRooms.count(roomName) != 0) {
        auto& members = chatRooms[roomName].members;
        for (auto& client : clients) {
            if (client.socket == clientSocket && members.count(clientSocket) != 0) {
                client.statusMessage = "";
                std::string revokeModMsg = "Your moderator rights have been revoked in the chat room: " + roomName + "\n";
                send(client.socket, revokeModMsg.c_str(), static_cast<int>(revokeModMsg.length()), 0);
                return;
            }
        }
    }
}

void sendUserProfile(const std::string& username, SOCKET clientSocket) {
    for (const auto& client : clients) {
        if (client.username == username) {
            std::string profile = "[Profile]\n";
            profile += "Username: " + client.username + "\n";
            profile += "Profile Picture: " + client.profilePicture + "\n";
            profile += "Status Message: " + client.statusMessage + "\n";
            send(clientSocket, profile.c_str(), static_cast<int>(profile.length()), 0);
            return;
        }
    }
}

void updateUserProfile(const std::string& username, const std::string& profilePicture, const std::string& statusMessage) {
    for (auto& client : clients) {
        if (client.username == username) {
            client.profilePicture = profilePicture;
            client.statusMessage = statusMessage;
            break;
        }
    }
}

void sendUnreadMessageNotification(SOCKET clientSocket) {
    std::string notification = "[Notification] You have unread messages in the chat rooms:\n";
    bool hasUnreadMessages = false;
    for (const auto& chatRoom : chatRooms) {
        if (chatRoom.second.members.count(clientSocket) != 0) {
            const auto& messageHistory = chatRoom.second.messageHistory;
            if (!messageHistory.empty()) {
                std::string lastMessage = messageHistory.back();
                if (lastMessage.substr(0, 3) != "[Notification]") {
                    hasUnreadMessages = true;
                    notification += "  - " + chatRoom.second.name + "\n";
                }
            }
        }
    }
    if (hasUnreadMessages) {
        sendNotificationToClient(notification, clientSocket);
    }
}

void sendUnreadMessages(SOCKET clientSocket) {
    for (const auto& chatRoom : chatRooms) {
        if (chatRoom.second.members.count(clientSocket) != 0) {
            const auto& messageHistory = chatRoom.second.messageHistory;
            if (!messageHistory.empty()) {
                std::string unreadMessages;
                bool hasUnreadMessages = false;
                for (const std::string& message : messageHistory) {
                    if (message.substr(0, 3) != "[Notification]") {
                        unreadMessages += message + "\n";
                        hasUnreadMessages = true;
                    }
                }
                if (hasUnreadMessages) {
                    send(clientSocket, unreadMessages.c_str(), static_cast<int>(unreadMessages.length()), 0);
                }
            }
        }
    }
}

bool createUser(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    if (userCredentials.count(username) == 0) {
        userCredentials[username] = password;
        std::ofstream userDB(USER_DATABASE_FILE, std::ios::app);
        if (userDB.is_open()) {
            userDB << username << ":" << password << "\n";
            userDB.close();
            return true;
        }
    }
    return false;
}

bool loadUserCredentials() {
    std::ifstream userDB(USER_DATABASE_FILE);
    if (userDB.is_open()) {
        std::string line;
        while (std::getline(userDB, line)) {
            size_t separatorPos = line.find(':');
            if (separatorPos != std::string::npos) {
                std::string username = line.substr(0, separatorPos);
                std::string password = line.substr(separatorPos + 1);
                userCredentials[username] = password;
            }
        }
        userDB.close();
        return true;
    }
    return false;
}

bool saveFile(const std::string& fileName, const std::string& data) {
    std::ofstream file(FILE_STORAGE_DIRECTORY + fileName, std::ios::binary);
    if (file.is_open()) {
        file << data;
        file.close();
        return true;
    }
    return false;
}

std::string readFile(const std::string& fileName) {
    std::ifstream file(FILE_STORAGE_DIRECTORY + fileName, std::ios::binary);
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return content;
    }
    return "";
}

void clientHandler(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];

    // Receive data from the client
    memset(buffer, 0, BUFFER_SIZE);
    int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead == SOCKET_ERROR) {
        std::cerr << "Error receiving data from client." << std::endl;
        closesocket(clientSocket);
        return;
    }

    std::string request(buffer);

    // Process the client's request
    if (request.substr(0, 13) == "AUTHENTICATE:") {
        std::string credentials = request.substr(13);
        size_t separatorPos = credentials.find(':');
        if (separatorPos != std::string::npos) {
            std::string username = credentials.substr(0, separatorPos);
            std::string password = credentials.substr(separatorPos + 1);
            bool authenticated = authenticateUser(username, password);
            sendAuthenticationResponse(authenticated, clientSocket);

            if (authenticated) {
                std::unique_lock<std::mutex> lock(clientsMutex);
                clients.push_back(Client{ clientSocket, username, "", "", false });
                lock.unlock();
                clientCV.notify_all();
                sendUnreadMessageNotification(clientSocket);
                sendUnreadMessages(clientSocket);
            }
        }
    }
    else if (request.substr(0, 9) == "REGISTER:") {
        std::string credentials = request.substr(9);
        size_t separatorPos = credentials.find(':');
        if (separatorPos != std::string::npos) {
            std::string username = credentials.substr(0, separatorPos);
            std::string password = credentials.substr(separatorPos + 1);
            bool registered = createUser(username, password);
            std::string response = registered ? "Registration successful!\n" : "Registration failed. Username already exists.\n";
            send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
        }
    }
    else {
        send(clientSocket, "Invalid request.\n", 17, 0);
    }

    // Main client handling loop
    while (true) {
        // Receive data from the client
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "Error receiving data from client." << std::endl;
            closesocket(clientSocket);
            break;
        }

        std::string message(buffer);

        // Process the client's message
        if (message.substr(0, 5) == "JOIN:") {
            std::string roomName = message.substr(5);
            joinChatRoom(roomName, clientSocket);
            std::string response = "Joined chat room: " + roomName + "\n";
            send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
        }
        else if (message.substr(0, 6) == "LEAVE:") {
            std::string roomName = message.substr(6);
            leaveChatRoom(roomName, clientSocket);
            std::string response = "Left chat room: " + roomName + "\n";
            send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
        }
        else if (message.substr(0, 10) == "SEND_ROOM:") {
            size_t separatorPos = message.find(':', 10);
            if (separatorPos != std::string::npos) {
                std::string roomName = message.substr(10, separatorPos - 10);
                std::string roomMessage = message.substr(separatorPos + 1);
                if (isClientInChatRoom(roomName, clientSocket)) {
                    ChatRoom& chatRoom = chatRooms[roomName];
                    std::string fullMessage = "[" + roomName + "] " + message.substr(separatorPos + 1);
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    auto& members = chatRoom.members;
                    if (members.count(clientSocket) != 0) {
                        for (SOCKET member : members) {
                            send(member, fullMessage.c_str(), static_cast<int>(fullMessage.length()), 0);
                        }
                        chatRoom.messageHistory.push_back(fullMessage);
                    }
                    for (auto& client : clients) {
                        if (client.socket == clientSocket) {
                            client.hasUnreadMessages = true;
                            break;
                        }
                    }
                }
                else {
                    std::string response = "You are not a member of the chat room: " + roomName + "\n";
                    send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
                }
            }
        }
        else if (message.substr(0, 14) == "SEND_PRIVATE:") {
            size_t separatorPos = message.find(':', 14);
            if (separatorPos != std::string::npos) {
                std::string recipientUsername = message.substr(14, separatorPos - 14);
                std::string privateMessage = message.substr(separatorPos + 1);
                sendPrivateMessage(recipientUsername, privateMessage, clientSocket);
            }
        }
        else if (message.substr(0, 5) == "LIST:") {
            std::string roomList = getChatRoomList();
            send(clientSocket, roomList.c_str(), static_cast<int>(roomList.length()), 0);
        }
        else if (message.substr(0, 10) == "KICK_USER:") {
            size_t separatorPos = message.find(':', 10);
            if (separatorPos != std::string::npos) {
                std::string roomName = message.substr(10, separatorPos - 10);
                SOCKET targetSocket = std::stoi(message.substr(separatorPos + 1));
                if (isUserModerator(roomName, clientSocket)) {
                    kickUserFromChatRoom(roomName, targetSocket);
                }
                else {
                    std::string response = "You do not have sufficient privileges to kick users from chat room: " + roomName + "\n";
                    send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
                }
            }
        }
        else if (message.substr(0, 9) == "BAN_USER:") {
            size_t separatorPos = message.find(':', 9);
            if (separatorPos != std::string::npos) {
                std::string roomName = message.substr(9, separatorPos - 9);
                SOCKET targetSocket = std::stoi(message.substr(separatorPos + 1));
                if (isUserModerator(roomName, clientSocket)) {
                    banUserFromChatRoom(roomName, targetSocket);
                }
                else {
                    std::string response = "You do not have sufficient privileges to ban users from chat room: " + roomName + "\n";
                    send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
                }
            }
        }
        else if (message.substr(0, 15) == "GRANT_MODERATOR:") {
            size_t separatorPos = message.find(':', 15);
            if (separatorPos != std::string::npos) {
                std::string roomName = message.substr(15, separatorPos - 15);
                SOCKET targetSocket = std::stoi(message.substr(separatorPos + 1));
                if (isUserModerator(roomName, clientSocket)) {
                    grantModeratorRights(roomName, targetSocket);
                }
                else {
                    std::string response = "You do not have sufficient privileges to grant moderator rights in chat room: " + roomName + "\n";
                    send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
                }
            }
        }
        else if (message.substr(0, 16) == "REVOKE_MODERATOR:") {
            size_t separatorPos = message.find(':', 16);
            if (separatorPos != std::string::npos) {
                std::string roomName = message.substr(16, separatorPos - 16);
                SOCKET targetSocket = std::stoi(message.substr(separatorPos + 1));
                if (isUserModerator(roomName, clientSocket)) {
                    revokeModeratorRights(roomName, targetSocket);
                }
                else {
                    std::string response = "You do not have sufficient privileges to revoke moderator rights in chat room: " + roomName + "\n";
                    send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
                }
            }
        }
        else if (message.substr(0, 13) == "SHOW_PROFILE:") {
            std::string username = message.substr(13);
            sendUserProfile(username, clientSocket);
        }
        else if (message.substr(0, 14) == "UPDATE_PROFILE:") {
            size_t separatorPos = message.find(':', 14);
            if (separatorPos != std::string::npos) {
                std::string profilePicture = message.substr(14, separatorPos - 14);
                std::string statusMessage = message.substr(separatorPos + 1);
                std::string username;
                for (auto& client : clients) {
                    if (client.socket == clientSocket) {
                        username = client.username;
                        break;
                    }
                }
                updateUserProfile(username, profilePicture, statusMessage);
                std::string response = "Profile updated successfully!\n";
                send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
            }
        }
        else if (message.substr(0, 4) == "SEND") {
            size_t separatorPos = message.find(':', 4);
            if (separatorPos != std::string::npos) {
                std::string fileName = message.substr(4, separatorPos - 4);
                std::string fileData = message.substr(separatorPos + 1);
                bool saved = saveFile(fileName, fileData);
                std::string response = saved ? "File saved successfully!\n" : "Failed to save file.\n";
                send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
            }
        }
        else if (message.substr(0, 3) == "GET") {
            std::string fileName = message.substr(3);
            std::string fileData = readFile(fileName);
            std::string response = fileData.empty() ? "File not found.\n" : fileData;
            send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);
        }
        else if (message.substr(0, 4) == "EXIT") {
            closesocket(clientSocket);
            break;
        }
        else {
            send(clientSocket, "Invalid command.\n", 17, 0);
        }
    }

    // Remove client from the list of connected clients
    std::unique_lock<std::mutex> lock(clientsMutex);
    clients.erase(std::remove_if(clients.begin(), clients.end(), std::bind(clientSocketMatches, std::placeholders::_1, clientSocket)), clients.end());
    lock.unlock();
    std::cout << "Client disconnected." << std::endl;
}

int main() {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return -1;
    }

    // Create a listening socket
    SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listeningSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create listening socket." << std::endl;
        return -1;
    }

    // Bind the listening socket to a local address and port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8888); // Port 8888
    if (bind(listeningSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind listening socket." << std::endl;
        closesocket(listeningSocket);
        return -1;
    }

    // Start listening for incoming connections
    if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Failed to start listening." << std::endl;
        closesocket(listeningSocket);
        return -1;
    }

    std::cout << "Server started. Waiting for incoming connections..." << std::endl;

    // Load user credentials from file
    if (!loadUserCredentials()) {
        std::cerr << "Failed to load user credentials." << std::endl;
        closesocket(listeningSocket);
        WSACleanup();
        return -1;
    }

    while (true) {
        // Accept a new client connection
        sockaddr_in clientAddress;
        int clientAddressSize = sizeof(clientAddress);
        SOCKET clientSocket = accept(listeningSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Failed to accept client connection." << std::endl;
            closesocket(listeningSocket);
            WSACleanup();
            return -1;
        }

        // Create a new thread to handle the client
        std::thread clientThread(clientHandler, clientSocket);
        clientThread.detach();
    }

    // Cleanup
    closesocket(listeningSocket);
    WSACleanup();

    return 0;
}
