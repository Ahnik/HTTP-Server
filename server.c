/* This is the main file of this project that is to be compiled and executed. 
The executable is to executed as "./server --directory <directory_relative_path>"*/

// This program is to be executed in a Linux environment(native or virtual).
// The source file is to be compiled and executed with atleast two arguments.
// The first argument should be a flag, that is "--directory" and the second argument should be the directory path of the directory 
// in which the files which can be requested by the client are located, relative to the current working directory.
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <errno.h>
#include "handle_connection.h"

#define PORT 4221

int main(int argc, const char** argv) {
	// Disable output buffering
	setbuf(stdout, NULL);		// Sets the stdout stream to be unbuffered
 	setbuf(stderr, NULL);		// Sets the stderr stream to be unbuffered

	int server_fd;
	socklen_t client_addr_len;

	// client_addr is a struct sockaddr_in specifying the client address.	
	struct sockaddr_in client_addr;
	
	// The function socket() creates an endpoint for communication and returns a file descriptor that refers to that endpoint.
	// AF_INET means we will be using the IPv4 Internet protocol family for our communication domain.
	// SOCK_STREAM provides sequenced, reliable, two-way, connection-based byte streams. An out-of-band data transmission mechanism
	// might be supported.
	// The third argument specifies a particular protocol to be used with the socket. In this case, only a single protocol exists to 
	// support a particular socket type within the IPv4 family, thus it is set to 0.
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		fprintf(stderr, "Error: Socket creation failed - %s\n", strerror(errno));
	 	return 1;
	}
	
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
	 	fprintf(stderr, "Error: SO_REUSEADDR failed - %s\n", strerror(errno));
	 	return 1;
	 }
	
	// serv_addr is a struct specifying the socket address of the server.
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(PORT),
									 .sin_addr = { htonl(INADDR_ANY) },
	 								};
	
	// bind() assigns the address specified by serv_addr to our socket.
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
	 	fprintf(stderr, "Error: Bind failed - %s\n", strerror(errno));
	 	return 1;
	}
	
	// listen() marks the socket as a passive socket, that is, as a socket that will be used to accept incoming connection requests 
	// using accept(). The connection_backlog specifies the maximum length to which the queue of pending connections for the 
	// socket may grow.
	int connection_backlog = 10;
	if (listen(server_fd, connection_backlog) != 0) {
	 	fprintf(stderr, "Error: Listen failed - %s\n", strerror(errno));
	 	return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = (socklen_t)sizeof(client_addr);
	
	// The accept() system call extracts the first connection request on the queue of pending connections for the listening socket.
	// It then creates a new connected socket, and returns a new file descriptor referring to that socket.
	// The newly created socket is not in listening state and the original socket is unaffected by this call.
	while(1){
		int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);

		if(client_fd == -1)
			continue;

		printf("Client connected\n");

		if(!fork()){
			close(server_fd);

			handle_connection(client_fd, argc, argv);
		}
		close(client_fd);
	}

	close(server_fd);

	return 0;
}	