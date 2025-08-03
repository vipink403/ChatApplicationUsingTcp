#include <WS2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

/*
	// initialize winsock library
	// create the socket
	// connect to the server
	// send and recv data
	// close the socket
	// cleanup winsock library
*/

constexpr int BUFFER_SIZE = 512;
constexpr int PORT = 54000;

SOCKET clientSocket = INVALID_SOCKET;

bool InitializeWinsock()
{
	WSADATA wsData;
	return WSAStartup(MAKEWORD(2, 2), &wsData) == 0;
}

void Cleanup()
{
	if (clientSocket != INVALID_SOCKET)
		closesocket(clientSocket);
	
	WSACleanup();
}

BOOL WINAPI CtrlHandler(DWORD signal)
{
	if (signal == CTRL_C_EVENT) {
		std::cout << "\nShutting down client gracefully..." << std::endl;
		Cleanup();
		exit(0);
	}
	return TRUE;
}

void SendTextMessage(SOCKET clientSocket)
{
	string message, name;
	cout << "Enter your name: ";
	getline(cin, name);

	while (true) 
	{
		cout << "Enter message to send: ";
		getline(cin, message);
		if (message == "exit") {
			cout << "Exiting sender thread." << endl;
			break;
		}

		message = name + ": " + message; // Prepend name to the message
		
		// send function is a blocking call, it will wait until data is sent
		int byteSent = send(clientSocket, message.c_str(), message.length(), 0);
		if (byteSent == SOCKET_ERROR) {
			cerr << "Failed to send data. Error: " << WSAGetLastError() << endl;
			break;
		}
		cout << "Message sent: " << message << endl;
	}

	Cleanup();
}

void ReceiveTextMessage(SOCKET clientSocket)
{
	char buffer[BUFFER_SIZE];
	while (true) 
	{
		ZeroMemory(buffer, sizeof(buffer));
		
		// recv is a blocking call, it will wait until data is received
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived <= 0) 
		{
			if (bytesReceived == 0) {
				cout << "Server disconnected." << endl;
			} else {
				cerr << "recv failed. Error: " << WSAGetLastError() << endl;
			}
			break; // Exit the loop if no data is received or an error occurs
		}
		string message{ buffer, static_cast<const unsigned __int64>(bytesReceived) };
		cout << "Received message from Server: " << message << endl;
	}
}

int main()
{
	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	cout << "Client is starting..." << endl;

	if(!InitializeWinsock()) {
		cerr << "Failed to initialize Winsock. Error: " << WSAGetLastError() << endl;
		return 1;
	}

	// Create a socket
	clientSocket = socket(AF_INET, SOCK_STREAM, 0); // Tcp socket
	if(clientSocket == INVALID_SOCKET) 
	{
		cerr << "Failed to create socket. Error: " << WSAGetLastError() << endl;
		Cleanup();
		return 1;
	}

	// Set up the sockaddr_in structure
	string serverIP = "127.0.0.1"; // Change to your server's IP address
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

	// Connect to the server
	if(connect(clientSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cerr << "Failed to connect to server. Error: " << WSAGetLastError() << endl;
		Cleanup();
		return 1;
	}

	cout << "Successfully Connected to server at " << serverIP << ":" << PORT << endl;

	// send or receive data
	thread senderThread(SendTextMessage, clientSocket);
	thread receiverThread(ReceiveTextMessage, clientSocket);
	senderThread.join();
	receiverThread.join();

	return 0;
}