# Multithreaded Client/Server Chatroom in C

A multithreaded chatroom application built in **C**, featuring a server that supports multiple concurrent client connections.  
This project demonstrates skills in **network programming**, **concurrency control**, and **systems-level debugging** in a UNIX environment.  

## Features
- **Concurrent client handling** using POSIX threads  
- **Socket programming** for client/server communication  
- **Thread synchronization** with mutexes and condition variables to prevent race conditions  
- Server capable of broadcasting messages to all connected clients  
- Designed for scalability to handle many simultaneous client messages  

## Future Improvements
- Implement client authentication  
- Add persistent chat history with an SQL database  
- Enhance error handling and resilience  

## Getting Started

### Compilation
```bash
# Compile server
gcc server.c -o server -lpthread

# Compile client
gcc client.c -o client -lpthread
