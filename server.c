#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "find_substring.h"

#define PORT 4221
#define MAX_STR_LENGTH 4096

void serve_user(int client_fd, int argc, char** argv){
	// Reading the HTTP request from the client
	char readBuffer[MAX_STR_LENGTH];
	readBuffer[MAX_STR_LENGTH-1] = '\0';
	
	ssize_t bytesRead = recv(client_fd, readBuffer, MAX_STR_LENGTH-1, 0);

	if(bytesRead < 0){
		printf("Receiving failed: %s\n", strerror(errno));
		close(client_fd);
		exit(1);
	}

 	readBuffer[bytesRead] = '\0';

	char* content = strdup(readBuffer);
	char* content_dup = strdup(readBuffer);
	printf("Content: %s\n", content);

	char* method = strtok(content_dup, " ");
	printf("Method: %s\n", method);

	char* reqPath = strtok(readBuffer, " ");
	reqPath = strtok(NULL, " ");

	ssize_t bytesSent;

	if(strncmp(reqPath, "/echo/", 6) == 0){
		char* str = strtok(reqPath, "/");
		str = strtok(NULL, "/");
		
		const char* format = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n\r\n%s";
		char res[MAX_STR_LENGTH];

		sprintf(res, format, strlen(str), str);
		bytesSent = send(client_fd, res, strlen(res), 0);	
	}
	else if(strncmp(reqPath, "/user-agent", 11) == 0){	/* Vulnerable to stack smashing */
		char* user_agent = strtok(NULL, " ");
		//printf("%s\n", user_agent);
		user_agent = strtok(NULL, " ");
		//printf("%s\n", user_agent);

		while(!find_substring("User-Agent:", user_agent)){
			user_agent = strtok(NULL, " ");
			//printf("%s\n", user_agent);

			if (find_substring("\r\n\r\n", user_agent)){
				printf("Error: User agent not found\n");
				close(client_fd);
				return;
			}
		}

		user_agent = strtok(NULL, " ");
		//printf("%s\n", user_agent);
		user_agent = strtok(user_agent, "\r\n");
		//printf("%s\n", user_agent);

		const char* format = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n\r\n%s";
		char res[MAX_STR_LENGTH];

		sprintf(res, format, strlen(user_agent), user_agent);
		bytesSent = send(client_fd, res, strlen(res), 0);
	}	
    else if(strncmp(reqPath, "/files/", 7) == 0 && argc > 2){
        char* directory;

        if(strcmp(argv[1], "--directory") == 0){
            directory = realpath(argv[2], NULL);
        }
        else{
            printf("Directory not mentioned: %s\n", strerror(errno));
            close(client_fd);
            exit(1);
        }

        char* filename = strtok(reqPath, "/");
        filename = strtok(NULL, "/");

        ssize_t pathsize = strlen(directory) + strlen(filename) + 2;

        directory = (char*)reallocarray(directory, pathsize, sizeof(*directory));

        strcat(directory, "/");
        strcat(directory, filename);

        printf("%s\n", directory);

        free(directory);
    }
	else if (strcmp(reqPath, "/") == 0){
		char* res = "HTTP/1.1 200 OK\r\n\r\n";
		bytesSent = send(client_fd, res, strlen(res), 0);
	}
	else{
		char* res = "HTTP/1.1 404 Not Found\r\n\r\n";
		bytesSent = send(client_fd, res, strlen(res), 0);
	}

	if (bytesSent <= 0){
		printf("Send failed: %s\n", strerror(errno));
		close(client_fd);
		return;
	}

    free(content);
    free(content_dup);
	close(client_fd);
}

int main(int argc, char** argv) {
    printf("%s\n", argv[1]);
	// Disable output buffering
	setbuf(stdout, NULL);		// Sets the stdout stream to be unbuffered
 	setbuf(stderr, NULL);		// Sets the stderr stream to be unbuffered

	int server_fd, client_addr_len;

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
		printf("Socket creation failed: %s...\n", strerror(errno));
	 	return 1;
	}
	
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
	 	printf("SO_REUSEADDR failed: %s \n", strerror(errno));
	 	return 1;
	 }
	
	// serv_addr is a struct specifying the socket address of the server.
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
	 								};
	
	// bind() assigns the address specified by serv_addr to our socket.
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
	 	printf("Bind failed: %s \n", strerror(errno));
	 	return 1;
	}
	
	// listen() marks the socket as a passive socket, that is, as a socket that will be used to accept incoming connection requests 
	// using accept(). The connection_backlog specifies the maximum length to which the queue of pending connections for the 
	// socket may grow.
	int connection_backlog = 10;
	if (listen(server_fd, connection_backlog) != 0) {
	 	printf("Listen failed: %s \n", strerror(errno));
	 	return 1;
	}
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
	
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

			serve_user(client_fd, argc, argv);
		}
		close(client_fd);
	}

	close(server_fd);

	return 0;
}	