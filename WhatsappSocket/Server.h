#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <map>
#include "Helper.h"



class Server
{
public:
	Server();
	~Server();
	void serve(int port);

private:
	void messageProcessor();
	void acceptClient();
	void clientHandler(SOCKET clientSocket);
	std::string readChatHistory(std::string filePath);
	void writeMessagesToUsersFile(std::string filePath, std::string users);
	std::map <std::string, int> users;
	SOCKET _serverSocket;
};
