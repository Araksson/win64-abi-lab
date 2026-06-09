// WARNING: didactic example (localhost test)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

constexpr int PORT = 54000;
constexpr int BUFFER_SIZE = 1024;
constexpr int CLIENTS = 8;
constexpr int ITERATIONS = 10000;

volatile int g_sink = 0;

extern bool InitializeServer();
extern void StartClient();

//------------------------------------------------------------
// Server (blocking)
//------------------------------------------------------------
static void server_thread()
{
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in hint{};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT);
    hint.sin_addr.s_addr = INADDR_ANY;

    bind(listenSock, (sockaddr*)&hint, sizeof(hint));
    listen(listenSock, SOMAXCONN);

    for (int i = 0; i < CLIENTS; i++)
    {
        SOCKET client = accept(listenSock, nullptr, nullptr);
        std::thread([client] ()
        {
            char buffer[BUFFER_SIZE];

            for (int i = 0; i < ITERATIONS; i++)
            {
                int bytes = recv(client, buffer, BUFFER_SIZE, 0);
                if (bytes <= 0) break;

                send(client, buffer, bytes, 0);
            }

            closesocket(client);

        }).detach();
    }
}

//------------------------------------------------------------
// Client
//------------------------------------------------------------
static void client_thread()
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in hint{};
    hint.sin_family = AF_INET;
    hint.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &hint.sin_addr);

    connect(sock, (sockaddr*)&hint, sizeof(hint));

    char buffer[BUFFER_SIZE] = {};

    for (int i = 0; i < ITERATIONS; i++)
    {
        send(sock, buffer, BUFFER_SIZE, 0);

        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes > 0)
        {
            g_sink += buffer[0];
        }
    }

    closesocket(sock);
}

//------------------------------------------------------------
// MAIN
//------------------------------------------------------------

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    std::thread(server_thread).detach();

    Sleep(100);

    std::vector<std::thread> clients;
    for (int i = 0; i < CLIENTS; i++)
    {
        clients.emplace_back(client_thread);
    }

    for (auto& t : clients)
    {
        t.join();
    }

    WSACleanup();

    std::cout << "Done\n";
    return 0;
}