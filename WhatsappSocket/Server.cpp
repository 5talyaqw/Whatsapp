#include "Server.h"
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <fstream>
#include <algorithm>
#include <array>


//global variables
std::queue<std::string> messageQueue; //global queue for all messages
std::mutex queueMutex; //mutex for the queue
std::condition_variable queueCV; //condition variable for messages queue
std::mutex usersMutex; //mutex for users



Server::Server()
{

	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_serverSocket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__ " - socket");
}

Server::~Server()
{
	try
	{
		// the only use of the destructor should be for freeing 
		// resources that was allocated in the constructor
		closesocket(_serverSocket);

	}
	catch (...) {}
}

void Server::serve(int port)
{
	std::cout << "Starting..." << std::endl;
	struct sockaddr_in sa = { 0 };

	sa.sin_port = htons(port); // port that server will listen for
	sa.sin_family = AF_INET;   // must be AF_INET
	sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

	// Connects between the socket and the configuration (port and etc..)
	if (bind(_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - bind");
	std::cout << "binded" << std::endl;

	// Start listening for incoming requests of clients
	if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - listen");
	std::cout << "Listening... " << std::endl;

	//main messages thread
	std::thread(&Server::messageProcessor, this).detach();

	while (true)
	{
		// the main thread is only accepting clients 
		// and add then to the list of handlers
		std::cout << "accepting client..." << std::endl;
		acceptClient();
	}
}

//function for deleting the messages off the qqueue
void Server::messageProcessor()
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(queueMutex);
		queueCV.wait(lock, [] { return !messageQueue.empty(); });
		messageQueue.pop();

	}
}

void Server::acceptClient()
{

	// this accepts the client and create a specific socket from server to this client
	// the process will not continue until a client connects to the server
	SOCKET client_socket = accept(_serverSocket, NULL, NULL);
	if (client_socket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__);

	std::cout << "Client accepted !" << std::endl;

	//thread for every client
	std::thread(&Server::clientHandler, this, client_socket).detach();
	
}


void Server::clientHandler(SOCKET clientSocket)
{
	try
	{
		std::string allUsers;
		std::string username;
		int messageCode;
		while(true)
		{
			//getting the message code for different conditons(200, 204, 208)
			messageCode = Helper::getMessageTypeCode(clientSocket);
			
			//if client asking for log in request(200)
			if (messageCode == MT_CLIENT_LOG_IN)
			{
				int userLen;

				userLen = Helper::getIntPartFromSocket(clientSocket, 2);
				username = Helper::getStringPartFromSocket(clientSocket, userLen);

				// Adding user to the users map
				std::lock_guard<std::mutex> lock(usersMutex); //shared object so locking
				users[username] = clientSocket;

				std::cout << "ADDED new client " << clientSocket << ", " << username << " to the clients list" << std::endl;

				//sending all of the users as serverUpdatePacket
				allUsers.clear();
				for (auto it = users.begin(); it != users.end(); it++)
				{
					if (!allUsers.empty())
					{
						allUsers += '&';
					}
					allUsers += it->first;
				}
				
				//sending to all of the connected users
				for (auto it = users.begin(); it != users.end(); it++)
				{
					Helper::send_update_message_to_client(it->second, "", "", allUsers);
				}
			}
			else if(messageCode == MT_CLIENT_UPDATE)
			{
				int secondUserLen = Helper::getIntPartFromSocket(clientSocket, 2);
				std::string secondUser = Helper::getStringPartFromSocket(clientSocket, secondUserLen);
				
				//if its update request
				if (secondUser.empty())
				{
					std::lock_guard<std::mutex> lock(usersMutex);
					allUsers.clear(); //clearing the list
					for (auto it = users.begin(); it != users.end(); it++)
					{
						if (!allUsers.empty())
						{
							allUsers += '&';
						}
						allUsers += it->first;
					}
					Helper::send_update_message_to_client(clientSocket, "", "", allUsers); //sending an update for the client
				}

				//else the client asks to send message
				else
				{
					int messageLen = Helper::getIntPartFromSocket(clientSocket, 5);
					std::string message = Helper::getStringPartFromSocket(clientSocket, messageLen); //getting the message
					if (!message.empty())
					{
						std::string formattedMessage = "&MAGSH_MESSAGE&&Author&" + username + "&DATA&" + message;
						writeChatToFile(formattedMessage, username, secondUser);
						
						std::lock_guard<std::mutex> lock(queueMutex);
						messageQueue.push(formattedMessage); //pushing the message
						queueCV.notify_one(); //notifying the queue there is message
						
						std::string content = readChatHistory(username, secondUser); //getting the chat content
						
						Helper::send_update_message_to_client(clientSocket, content, secondUser, allUsers); //sending update to the client
						Helper::send_update_message_to_client(users[secondUser], content, username, allUsers); //sending update to the second user too
					}
				}
			}
			//if client want to exit
			else if (messageCode == MT_CLIENT_EXIT)
			{
				break; //breaking the loop
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception was catch in function clientHandler. socket=" << clientSocket << ", what=" << e.what() << std::endl;
		
		//removing the user off the map
		std::lock_guard<std::mutex> lock(usersMutex);
		for (auto it = users.begin(); it != users.end(); it++)
		{
			if (it->second == clientSocket)
			{
				std::cout << "REMOVED " << clientSocket << ", " << it->first << " from clients list" << std::endl;
				users.erase(it);
				break;
			}
		}
		closesocket(clientSocket); //closing socket
	}
}

/*
*function writing to file and creating one, adding the messages
* output: nothing
* input: s - sender(client that sends the message), r - reciver(client that gets it), message - the formatted message
*/
void Server::writeChatToFile(std::string message, std::string s, std::string r)
{
	//getting the filePath/name by sorting the names
	std::array<std::string, 2>sortedUsers = { s, r }; 
	std::sort(sortedUsers.begin(), sortedUsers.end());
	std::string filePath = "C:\\Users\\Cyber_User\\Desktop\\EkronotExercises\\Whatsapp\\" + sortedUsers[0] + "&" + sortedUsers[1] + ".txt";

	//creating and opening a file in append mode
	std::ofstream file(filePath, std::ios::app);
	if (file.is_open())
	{
		file << message << std::endl; //ssetting the formatted message in
	}
	else
	{
		std::cout << "file cannot be opened" << std::endl;
	}
}

/*
*function reading from file and getting the content of it
* output: chat history/file content
* input: s - sender(client that sends the message), r - reciver(client that gets it)
*/
std::string Server::readChatHistory(std::string s, std::string r)
{
	//getting the filePath/name by sorting the names
	std::array<std::string, 2>sortedUsers = { s, r };
	std::sort(sortedUsers.begin(), sortedUsers.end());
	std::string filePath = "C:\\Users\\Cyber_User\\Desktop\\EkronotExercises\\Whatsapp\\" + sortedUsers[0] + "&" + sortedUsers[1] + ".txt";

	//opening the file for reading
	std::ifstream file(filePath);
	if (!file)
	{
		std::cerr << "Error: Cannot open file " << filePath << std::endl;
		return "";
	}

	//read file contents
	std::string history((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return history; //returning the content
}