#include "Server.h"
#include <exception>
#include <iostream>
#include <string>
#include <thread>


Server::Server()
{

	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_serverSocket == INVALID_SOCKET)
		throw std::exception(__FUNCTION__ " - socket");

	// Start the message processing thread
	_messageThread = std::thread(&Server::messageProcessor, this);
}

Server::~Server()
{
	try
	{
		// the only use of the destructor should be for freeing 
		// resources that was allocated in the constructor
		closesocket(_serverSocket);

		for (auto it = _clientThreads.begin(); it != _clientThreads.end(); it++)
		{
			it->join();
			
		}
		_messageThread.join();
	}
	catch (...) {}
}

void Server::serve(int port)
{

	struct sockaddr_in sa = { 0 };

	sa.sin_port = htons(port); // port that server will listen for
	sa.sin_family = AF_INET;   // must be AF_INET
	sa.sin_addr.s_addr = INADDR_ANY;    // when there are few ip's for the machine. We will use always "INADDR_ANY"

	// Connects between the socket and the configuration (port and etc..)
	if (bind(_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - bind");

	// Start listening for incoming requests of clients
	if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw std::exception(__FUNCTION__ " - listen");

	std::cout << "Starting...\n";
	std::cout << "Binded\n";
	std::cout << "Listening...\n";

	while (true)
	{
		// the main thread is only accepting clients 
		// and add then to the list of handlers
		std::cout << "accepting client...\n" << std::endl;
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

	std::cout << "Client accepted !" << std::endl; // Print after accept()

	//new thread for new client
	std::thread clientThread(&Server::clientHandler, this, client_socket);
	_clientThreads.push_back(std::move(clientThread)); // Store the thread
}


void Server::clientHandler(SOCKET clientSocket)
{
	try
	{
		std::string s = "Welcome! What is your name (4 bytes)? ";
		send(clientSocket, s.c_str(), s.size(), 0);  // last parameter: flag. for us will be 0.

		char m[5];
		recv(clientSocket, m, 4, 0);
		m[4] = 0;

		int clientID = static_cast<int>(clientSocket); // Use socket as ID
		_clients[clientID] = std::string(m);

		std::cout << "ADDED new client " << clientID << ", " << m << " to clients list" << std::endl;
		{
			std::lock_guard<std::mutex> lock(_clientsMutex);
			_clients[clientSocket] = m;
		}
		while (true)
		{
			int messageType = Helper::getMessageTypeCode(clientSocket);
			//client disconnected
			if (messageType == 0 || messageType == MT_CLIENT_EXIT)
			{
				std::cout << "Received exit message from client\n";
				break;
			}
			std::string message = Helper::getStringPartFromSocket(clientSocket, 1024);
			_messageQueue.push(m + ': ' + message);
		}

		//closing socket for client
		closesocket(clientSocket);
		{
			std::lock_guard<std::mutex> lock(_clientsMutex);
			std::cout << "REMOVED " << clientSocket << ", " << _clients[clientSocket] << " from clients list\n";
			_clients.erase(clientSocket);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception was caught in function clientHandler. socket=" << clientSocket
			<< ", what=" << e.what() << std::endl;
		closesocket(clientSocket);
	}


}

void Server::messageProcessor()
{
	while (true)
	{
		std::string message = _messageQueue.pop();
		std::cout << "Processing message: " << message << std::endl;

		std::lock_guard<std::mutex> lock(_clientsMutex);
		for (auto it = _clients.begin(); it != _clients.end(); it++)
		{
			try
			{
				Helper::sendData(it->first, message);
			}
			catch (...)
			{
				std::cerr << "Failed to send message to client." << std::endl;
			}
		}
	}
}
