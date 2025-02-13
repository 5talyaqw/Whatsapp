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
	void acceptClient();
	void clientHandler(SOCKET clientSocket);

	//added methods 
	void writeChatToFile(std::string message, std::string s, std::string r);
	std::string readChatHistory(std::string s, std::string r); //sender and reciver
	void messageProcessor();

	//variables for users
	std::map <std::string, int> users;
	SOCKET _serverSocket;
};
