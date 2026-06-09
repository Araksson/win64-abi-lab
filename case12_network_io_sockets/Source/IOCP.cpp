//The IOCP code looks like this:

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#define PORT 5000
#define BUFFER_SIZE 4096
#define MAX_WORKER_THREADS 4

static DWORD WINAPI WorkerThread(LPVOID lpParam);

//Context per Client
struct ClientContext 
{
    SOCKET socket;
    CHAR buffer[BUFFER_SIZE];
    WSABUF wsaBuf;
    OVERLAPPED overlapped;
    DWORD bytesReceived;

    ClientContext()
    {
        ZeroMemory(&overlapped, sizeof(OVERLAPPED));
        wsaBuf.len = BUFFER_SIZE;
        wsaBuf.buf = buffer;
        bytesReceived = 0;
    }
};

//Type of Thread Operation
enum OperationType { OP_READ, OP_WRITE };

struct IOContext 
{
    OVERLAPPED overlapped;
    WSABUF wsaBuf;
    OperationType opType;
    ClientContext* clientCtx;
    CHAR buffer[BUFFER_SIZE];

    IOContext() 
    {
        ZeroMemory(&overlapped, sizeof(OVERLAPPED));
        wsaBuf.len = 0;
        wsaBuf.buf = buffer;
        clientCtx = nullptr;
    }
};

HANDLE g_hIOCP = NULL;

bool InitializeServer()
{
    WSADATA wsaData = {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return false;
    }

    //Create Listen Socket
    SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listenSocket == INVALID_SOCKET)
    {
        return false;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(listenSocket);
        return false;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) 
    {
        closesocket(listenSocket);
        return false;
    }

    //Create IOCP: 0 in nNumOfConcurrentThreads -> CPU Cores * 2
    g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (g_hIOCP == NULL) 
    {
        closesocket(listenSocket);
        return false;
    }

    //Init the Worker Threads
    std::vector<HANDLE> threads;
    for (int i = 0; i < MAX_WORKER_THREADS; ++i) 
    {
        threads.push_back(CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(WorkerThread), NULL, 0, NULL));
    }

    //Client acceptance loop (may be in a dedicated thread)
    while (true) 
    {
        sockaddr_in clientAddr = {};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);

        if (clientSocket != INVALID_SOCKET) 
        {
            //Associate the new socket with the IOCP
            //The third parameter(CompletionKey) is usually a pointer to the client context
            ClientContext* clientCtx = new ClientContext();
            clientCtx->socket = clientSocket;

            HANDLE hCP = CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), g_hIOCP, reinterpret_cast<ULONG_PTR>(clientCtx), 0);

            if (hCP == NULL) 
            {
                delete clientCtx;
                closesocket(clientSocket);
                continue;
            }

            //Start the first asynchronous read operation
            DWORD flags = 0;
            DWORD bytesRead = 0;
            int result = WSARecv(clientSocket, &clientCtx->wsaBuf, 1, &bytesRead, &flags, &clientCtx->overlapped, NULL);

            if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
            {
                //Error
                delete clientCtx;
                closesocket(clientSocket);
            }
        }
    }

    closesocket(listenSocket);
    return true;
}

static DWORD WINAPI WorkerThread(LPVOID lpParam)
{
    DWORD bytesTransferred = 0;
    ClientContext* clientCtx = NULL;
    LPOVERLAPPED pOverlapped = NULL;

    while (true)
    {
        //Blocking until an operation is completed or a timeout occurs
        BOOL result = GetQueuedCompletionStatus(g_hIOCP, &bytesTransferred, reinterpret_cast<PULONG_PTR>(&clientCtx), &pOverlapped, INFINITE);
        if (!result || (result && bytesTransferred == 0))
        {
            //Error or Client Disconnect
            if (clientCtx)
            {
                closesocket(clientCtx->socket);
                delete clientCtx;
            }

            continue;
        }

        //Process the completed operation
        //In a real - world scenario, you would verify what type of operation it was(read / write)
        //Here we assume it's a completed read on clientCtx->buffer

        std::cout << "Data received: " << std::string(clientCtx->buffer, bytesTransferred) << std::endl;

        //Restart the reading to continue receiving data
        ZeroMemory(&clientCtx->overlapped, sizeof(OVERLAPPED));

        DWORD flags = 0;
        DWORD bytesRead = 0;
        int recvResult = WSARecv(clientCtx->socket, &clientCtx->wsaBuf, 1, &bytesRead, &flags, &clientCtx->overlapped, NULL);
        if (recvResult == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        {
            closesocket(clientCtx->socket);
            delete clientCtx;
        }
    }

    return 0;
}

//Basic Example of Client TCP
void StartClient()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cout << "Connection Fail" << std::endl;
        closesocket(clientSocket);
        return;
    }

    const char* msg = "Hi IOCP Server";
    send(clientSocket, msg, static_cast<int>(strlen(msg)), 0);

    char buffer[4096];
    int recvLen = recv(clientSocket, buffer, 4096, 0);
    if (recvLen > 0)
    {
        std::cout << "Response: " << std::string(buffer, recvLen) << std::endl;
    }

    closesocket(clientSocket);
    WSACleanup();
}