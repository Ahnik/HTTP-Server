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
#include "handle_request_method.h"

// Function to handle each connection
void handle_connection(int client_fd, int argc, const char** argv){
	char* readBuffer = read_http_request(client_fd);

	if(readBuffer == NULL)
		return;

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

	// Variable to store the status code for a function 
	int status;

	// Implementing support for GET method
	if(strcmp(method, "GET") == 0)
		status = handle_get_method(client_fd, reqPath, argc, argv);
	
	// Implementing support for the POST method
	else if(strcmp(method, "POST") == 0){
		// If /files/ endpoint is encountered
		if(strncmp(reqPath, "/files/", 7) == 0)
			status = handle_post_method(client_fd, reqPath, content, argc, argv);
		// Otherwise send that it is a bad request
		else{
			char* res = "HTTP/1.1 400 Bad Request\r\n\r\n";
			ssize_t bytesSent = send(client_fd, res, strlen(res), 0);

			if(bytesSent == -1){
				fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
				return;
			}
		}
	}

	// Implementing support for the DELETE method
	else if(strcmp(method, "DELETE") == 0){
		// If /files/{files} endpoint is encountered
		if(strncmp(reqPath, "/files/", 7) == 0)
			status = handle_delete_request(client_fd, reqPath, argc, argv);
		// Otherwise send that it is a bad request
		else{
			char* res = "HTTP/1.1 400 Bad Request\r\n\r\n";
			ssize_t bytesSent = send(client_fd, res, strlen(res), 0);

			if(bytesSent == -1){
				fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
				return;
			}
		}
	}

	// If the client request couldn't be fulfilled due to some error
	if(status == A_ERROR){
		fprintf(stderr, "Unable to fulfil client request\n");

		char* res = "HTTP/1.1 500 Internal Server Error\r\n\r\n";

		if(send(client_fd, res, strlen(res), 0) == -1)
			fprintf(stderr, "Error: Unable to send HTTP status code 500 to client\n");
	}

	// Freeing up allocated memory
    free(content);
    free(content_dup);
	free(readBuffer);

	// Closing the client socket file descriptor
	close(client_fd);
}

#endif 