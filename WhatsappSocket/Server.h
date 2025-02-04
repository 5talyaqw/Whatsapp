#pragma once
#include <vector>
#include <thread>
#include <WinSock2.h>
#include <Windows.h>
#include <map>
#include "MessageQueue.h"
#include "Helper.h"


class Server
{
public:
	Server();
	~Server();
	void serve(int port);

private:

	void acceptClient();
	void clientHandler(SOCKET clientSocket);
	void messageProcessor(); // New function for handling messages

	SOCKET _serverSocket;
	std::vector<std::thread> _clientThreads; //storing the client threads
	std::map<SOCKET, std::string> _clients;
	std::mutex _clientsMutex;
	MessageQueue _messageQueue;
	std::thread _messageThread;
};
