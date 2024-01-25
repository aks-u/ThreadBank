# ThreadBank
A rudimentary bank server application using threads and UNIX domain sockets. The bank has N service desks, each with its own queue and running on its own thread. When a client connects to the server, the client goes to the shortest queue and waits for service. (Fall 2023)
