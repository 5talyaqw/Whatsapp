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
	void writeChatToFile(std::string message, std::string s, std::string r);
	std::string readChatHistory(std::string s, std::string r); //sender and reciver
	std::map <std::string, int> users;
	SOCKET _serverSocket;
};
