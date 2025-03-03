#ifndef HANDLE_CONNECTION_H
#define HANDLE_CONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include "helper.h"
#include "handle_cases.h"

// Function to handle each connection
void handle_connection(int client_fd, int argc, const char** argv){
	char* readBuffer = read_http_request(client_fd);

	// Extracting the content of the HTTP request
	char* content = strdup(readBuffer);
	char* content_dup = strdup(readBuffer);
	printf("Content: \n%s\n", content);

	// Extracting the HTTP method in the HTTP request
	char* method = strtok(content_dup, " ");
	printf("Method: %s\n", method);

	// Extracting requested target pathname in the HTTP request
	char* reqPath = strtok(readBuffer, " ");
	reqPath = strtok(NULL, " ");

	// Variable to store the number of bytes sent 
	int status;

	// Implementing support for GET method
	if(strcmp(method, "GET") == 0){
		// Implementing support for /echo/{str} endpoint
		if(strncmp(reqPath, "/echo/", 6) == 0)
			status = handle_echo_route(client_fd, reqPath);

		// Implementing support for /user-agent endpoint
		else if(strncmp(reqPath, "/user-agent", 11) == 0)
			status = handle_user_agent_route(client_fd, reqPath);
		
		// Implementing support for /files/{filename} endpoint
		else if(strncmp(reqPath, "/files/", 7) == 0)
			status = handle_files_route(client_fd, reqPath, argc, argv);

		// Implementing support for empty HTTP GET request
		else if (strcmp(reqPath, "/") == 0){
			char* res = "HTTP/1.1 200 OK\r\n\r\n";
			ssize_t bytesSent = send(client_fd, res, strlen(res), 0);

			if(bytesSent == -1){
				fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
				close(client_fd);
				free(content);
				free(content_dup);
				free(readBuffer);
				return;
			}
		}

		// Implementing support for "Not Found" case
		else{
			char* res = "HTTP/1.1 404 Not Found\r\n\r\n";
			ssize_t bytesSent = send(client_fd, res, strlen(res), 0);

			if(bytesSent == -1){
				fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
				close(client_fd);
				free(content);
				free(content_dup);
				free(readBuffer);
				return;
			}
		}
	}

	// Implementing support for the POST method
	else if(strcmp(method, "POST") == 0){
		if(strncmp(reqPath, "/files/", 7) == 0){
			if(handle_post_route(client_fd, reqPath, content, argc, argv) == 0){
				char* res = "HTTP/1.1 201 Created\r\n\r\n";
				ssize_t bytesSent = send(client_fd, res, strlen(res), 0);

				if(bytesSent == -1){
					fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
					close(client_fd);
					free(content);
					free(content_dup);
					free(readBuffer);
					return;
				}
			}
		}
	}

	// If the client request couldn't be fulfilled due to some error
	if(status == -1){
		fprintf(stderr, "Unable to fulfil client request\n");
	}

	// Freeing up allocated memory
    free(content);
    free(content_dup);
	free(readBuffer);

	// Closing the client socket file descriptor
	close(client_fd);
}

#endif 