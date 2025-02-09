#pragma comment (lib, "ws2_32.lib")

#include "WSAInitializer.h"
#include "Server.h"
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    try
    {
        WSAInitializer wsaInit;
        Server myServer;

        myServer.serve(8826);

    }
    catch (std::exception& e)
    {
        std::cout << "Error occurred: " << e.what() << std::endl;
    }

    system("PAUSE");
    return 0;
}
