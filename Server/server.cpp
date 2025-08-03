#include <WS2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <tchar.h> // Add this include to define _T macro
using namespace std;

#pragma comment(lib, "ws2_32.lib")

/*
	// initialize winsock library
	// create the socket
	// get ip and port from the user
	// bind the socket to the ip and port
	// listen for incoming connections
	// accept incoming connections
	// recv and send data
	// close the socket
	// cleanup winsock library
*/

constexpr int BUFFER_SIZE = 512;
constexpr int PORT = 54000;

SOCKET serverSocket = INVALID_SOCKET;
SOCKET clientSocket = INVALID_SOCKET;
mutex clientMutex; // Mutex to protect shared resources
vector<thread> clientWorkerThreads; // Vector to hold client worker threads

bool InitializeWinsock()
{
	WSADATA wsData;
	return WSAStartup(MAKEWORD(2, 2), &wsData) == 0;
}

void Cleanup(bool bflag = true)
{
	if (bflag && clientSocket != INVALID_SOCKET)
		closesocket(clientSocket);

	if (serverSocket != INVALID_SOCKET)
		closesocket(serverSocket);

	WSACleanup();
}

BOOL WINAPI CtrlHandler(DWORD signal)
{
	if (signal == CTRL_C_EVENT) {
		std::cout << "\nShutting down server gracefully..." << std::endl;
		Cleanup();
		exit(0);
	}
	return TRUE;
}

void InteractWithClient(SOCKET clientSocket, vector<SOCKET>& clientSockets)
{
	cout << "Client connected successfully." << endl;

	char buffer[BUFFER_SIZE];
	while (true)
	{
		ZeroMemory(buffer, sizeof(buffer));

		//recv is a blocking call, it will wait until data is received
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if(bytesReceived <= 0) 
		{
			if (bytesReceived == 0) {
				cout << "Client disconnected." << endl;
			} else {
				cerr << "recv failed. Error: " << WSAGetLastError() << endl;
			}
			break; // Exit the loop if no data is received or an error occurs
		}

		string message{ buffer, static_cast<const unsigned __int64>(bytesReceived) };
		cout << "Received message from Client : " << message << endl;

		for (auto client : clientSockets)
		{
			if (client == clientSocket) 
				continue; // Skip sending to the same client
			send(client, buffer, bytesReceived, 0); // Send the received message to all connected clients
		}
	}

	auto it = find(clientSockets.begin(), clientSockets.end(), clientSocket);
	if (it != clientSockets.end()) 
	{
		lock_guard<mutex> lock(clientMutex); // Lock the mutex to protect shared resources
		clientSockets.erase(it); // Remove the client socket from the list
		cout << "Client disconnected and removed from the list." << endl;
	}

	closesocket(clientSocket);
}

int main()
{
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	cout << "Server is starting..." << endl;

	// Initialize Winsock
	if(!InitializeWinsock()) {
		cerr << "Failed to initialize Winsock. Error: " << WSAGetLastError() << endl;
		return 1;
	}

	// Create a socket
	serverSocket = socket(AF_INET, SOCK_STREAM, 0); // Tcp socket
	if(serverSocket == INVALID_SOCKET) 
	{
		cerr << "Failed to create socket. Error: " << WSAGetLastError() << endl;
		Cleanup();
		return 1;
	}

	// Set up the sockaddr_in structure
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT); // convert port number from little-endian to big-endian

	//Convert IPv4 address from string to binary form
	if(InetPton(AF_INET, _T("0.0.0.0"), &serverAddr.sin_addr) != 1) 
	{
		cerr << "Setting Server IP address structure failed." << endl;
		Cleanup();
		return 1;
	}

	// bind the socket to the IP and port
	if(bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cerr << "Failed to bind socket. Error: " << WSAGetLastError() << endl;
		Cleanup();
		return 1;
	}

	// listen for incoming connections
	if(listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "Failed to listen on socket. Error: " << WSAGetLastError() << endl;
		Cleanup();
		return 1;
	}

	cout << "Server is listening on port " << PORT << "..." << endl;
	vector<SOCKET> clientSockets;
	while (true)
	{	
		// Accept incoming connections
		clientSocket = accept(serverSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET)
		{
			cerr << "Failed to accept connection. Error: " << WSAGetLastError() << endl;
			Cleanup();
			return 1;
		}

		lock_guard<mutex> lock(clientMutex); // Lock the mutex to protect shared resources
		clientSockets.push_back(clientSocket);

		// keep track of all client worker threads:
		clientWorkerThreads.emplace_back(InteractWithClient, clientSocket, std::ref(clientSockets));
	}

	// Close sockets and shutdown the server
	Cleanup(false);

	// Wait for all threads to finish
	for (auto& thread : clientWorkerThreads) 
	{
		if (thread.joinable()) 
		{
			thread.join();
		}
	}

	return 0;
}
