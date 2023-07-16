# Chat Room Server

This is a Chat Room Server implemented in C++ using the Winsock library. It allows multiple clients to connect, join chat rooms, and communicate with each other in real-time. The server supports the following features:

## Features:

1. Authentication: Clients can authenticate by providing their username and password. The server verifies the credentials against a user database file.

2. Registration: New users can register by providing a unique username and password. The server checks if the username is available and adds the user to the user database file.

3. Join/Leave Chat Rooms: Authenticated users can join and leave different chat rooms. Each chat room has a unique name and maintains a list of connected members.

4. Room Messaging: Users in the same chat room can send messages that are broadcasted to all other members in the room.

5. Private Messaging: Users can send private messages to other users by specifying the recipient's username.

6. User Management: Moderators can kick users from a chat room, ban users from a chat room, grant moderator rights to users, and revoke moderator rights from users.

7. User Profiles: Users can view their profiles and update their profile information, including profile picture and status message.

8. File Upload/Download: Users can upload files to the server, and other users can download the files.

## How These Features Are Attained:

- The server uses TCP/IP sockets for communication with clients.
- Client connections are handled in separate threads for concurrent processing.
- Authentication and user management are achieved through user credentials stored in a user database file.
- Chat rooms are implemented as unordered sets of client sockets, allowing efficient member management and message broadcasting.
- Private messaging is accomplished by searching for the recipient's username in the client list and sending the message directly to them.
- User profiles are stored and updated in the client data structure.
- File upload/download is implemented using file I/O operations, allowing files to be saved and retrieved from the server's file storage directory.

## Usage:
*Please note that this code assumes you are running it on a Windows machine and have the necessary dependencies installed. Also, make sure to update the USER_DATABASE_FILE and FILE_STORAGE_DIRECTORY variables according to your needs.


1. Compile the server code using the command:
   g++ -std=c++11 -pthread -o server server.cpp -lws2_32
2. Compile the client code using the command:
   g++ -std=c++11 -o client client.cpp -lws2_32
3. run the server :
   ./server.exe
4. run the client:
   ./client.exe

5.Follow the client-server interaction guidelines mentioned in the code to test different features.

Please note that this is a basic example implementation and may require further modifications or enhancements depending on your specific needs.

1. Clone the repository:

```shell
git clone https://github.com/your-username/chat-room-server.git
