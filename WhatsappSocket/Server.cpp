#include "Server.h"
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>



//global variables
std::queue<std::string> messageQueue;
std::mutex queueMutex;
std::condition_variable queueCV;



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

	while (true)
	{
		// the main thread is only accepting clients 
		// and add then to the list of handlers
		std::cout << "accepting client..." << std::endl;
		acceptClient();
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

		int messageCode = Helper::getMessageTypeCode(clientSocket);

		if (messageCode == MT_CLIENT_LOG_IN)
		{
			//getting the len of the user 
			int usernameLen = Helper::getIntPartFromSocket(clientSocket, 2);
			//getting the username based on the len
			std::string username = Helper::getStringPartFromSocket(clientSocket, usernameLen);

			//storing the username and user socket to the map
			users[username] = clientSocket;
			std::cout << "ADDED new Client " << clientSocket << ", " << username << " to clients list" << std::endl;
			
			//sending server update
			std::string allUsers;
			for (auto it = users.begin(); it != users.end(); it++)
			{
				if (!allUsers.empty())
				{
					allUsers += "&";
				}
				allUsers += it->first;
			}
			//getting the len for thr format
			std::string fileSize = Helper::getPaddedNumber(0, 5);
			std::string secondUserSize = Helper::getPaddedNumber(0, 2);
			std::string allUserSize = Helper::getPaddedNumber(allUsers.size(), 5);
			std::string response = std::to_string(MT_SERVER_UPDATE) + fileSize + secondUserSize + allUserSize + allUsers;

			Helper::sendData()
		}

		while (true)
		{
			messageCode = Helper::getMessageTypeCode(clientSocket);

			//if client exiting
			if (messageCode == 0 || messageCode == MT_CLIENT_EXIT)
			{
				break;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception was catch in function clientHandler. socket="
			<< clientSocket << ", what=" << e.what() << std::endl;
		std::cerr << "Recieved exit message from client" << std::endl;

		for (auto it = users.begin(); it != users.end(); it++)
		{
			if (it->second == clientSocket)
			{
				std::cout << "REMOVED " << clientSocket << ", " << it->first << " from clients list" << std::endl;
				users.erase(it);
				break;
			}
		}

		//closing client socket
		closesocket(clientSocket);
	}
	
}