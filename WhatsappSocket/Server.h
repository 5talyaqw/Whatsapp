#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <unordered_map>
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
	std::unordered_map<std::string, int> users;
	SOCKET _serverSocket;
};
