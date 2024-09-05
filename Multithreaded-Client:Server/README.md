# Multithreaded Client/Server Chatroom In C
•	Developed a multithreaded chat server in C to handle multiple simultaneous client connections.
•	Implemented socket programming and POSIX threads to manage the concurrent client communication.
•	Utilized mutexes and condition variables to prevent race conditions.
•	Gained experience in network programming, concurrency, and debugging in a UNIX environment.
•	Working to improve upon this project by adding client authentication as well as stored chat history in an SQL database.
# Run
Compile server/client via: gcc [server.c/client.c] -o [server\client]
Run: ./server or ./client

The server can handle many simultaneous messages sent from differnt clients.
