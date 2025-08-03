# Windows TCP Chat Application (C++ Winsock)

A simple multi-client chat application using TCP sockets in C++ on Windows.

## Features

- **Server**: Handles multiple client connections simultaneously
- **Client**: Connects to server and supports bidirectional messaging
- **Threaded Architecture**: Separate threads for sending/receiving messages
- **Graceful Shutdown**: Proper cleanup on CTRL+C
- **Basic Error Handling**: Winsock error reporting

## Prerequisites

- Windows OS
- C++17 compatible compiler (MSVC, MinGW)
- CMake (optional, for build automation)

## Build Instructions

### Using Visual Studio (MSVC)
1. Open the project folder in Visual Studio
2. Build the solution (F7)

### Using Command Line (MinGW)
```bash
g++ server/server.cpp -o server.exe -lws2_32
g++ client/client.cpp -o client.exe -lws2_32
