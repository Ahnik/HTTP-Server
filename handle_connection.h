#ifndef HANDLE_CONNECTION_H
#define HANDLE_CONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include "helper.h"

#define OCTET_STREAM_HEADERS_LENGTH strlen("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: \r\n\r\n")
#define TEXT_PLAIN_HEADERS_LENGTH strlen("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: \r\n\r\n")
#define REQUEST_END "\r\n\r\n"

int handle_echo_route(int client_fd, char* path);
int handle_user_agent_route(int client_fd, char* path);
int handle_files_route(int client_fd, char* path, int argc, char** argv);

// Function to handle each connection
void handle_connection(int client_fd, int argc, char** argv){
	// Reading the HTTP request from the client in readBuffer whose memory is dynamically allocated
	char* readBuffer = NULL;
	char chara;
	size_t size = 0;

	// Reading the HTTP request and dynamically allocating memory for the buffer one byte at a time
	while(1){
		if(recv(client_fd, (void*)&chara, sizeof(chara), 0) == -1){
			fprintf(stderr, "Error: Receiving failed - %s\n", strerror(errno));
			close(client_fd);
			return;
		}

		size++;
		readBuffer = (char*)reallocarray((void*)readBuffer, size, sizeof(*readBuffer));
		if(readBuffer == NULL){
			fprintf(stderr, "Error: Memory allocation for HTTP request failed\n");
			close(client_fd); 	
			return;
		}
		readBuffer[size - 1] = chara;

		if(is_substring(REQUEST_END, readBuffer))
			break;
	}

	size++;
	readBuffer = (char*)reallocarray((void*)readBuffer, size, sizeof(*readBuffer));
	if(readBuffer == NULL){
		fprintf(stderr, "Error: Memory allocation for HTTP request failed\n");
		close(client_fd); 	
		return;
	}

 	readBuffer[size - 1] = '\0';

	// Extracting the content of the HTTP request
	char* content = strdup(readBuffer);
	char* content_dup = strdup(readBuffer);
	printf("Content: %s\n", content);

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
		if(strncmp(reqPath, "/echo/", 6) == 0){
			status = handle_echo_route(client_fd, reqPath);
		}

		// Implementing support for /user-agent endpoint
		else if(strncmp(reqPath, "/user-agent", 11) == 0){	
			status = handle_user_agent_route(client_fd, reqPath);
		}	

		// Implementing support for /files/{filename} endpoint
		else if(strncmp(reqPath, "/files/", 7) == 0){
			status = handle_files_route(client_fd, reqPath, argc, argv);
		}

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
			// Using this endpoint, the server will be able to accept text from the client and create a new file with that text.
		}
	}

	// Freeing up allocated memory
    free(content);
    free(content_dup);
	free(readBuffer);

	// Closing the client socket file descriptor
	close(client_fd);
}

// Function for handling the /echo/{str} endpoint
int handle_echo_route(int client_fd, char* path){
	// The string to be returned back to the client
	char* str = strtok(path, "/");
	str = strtok(NULL, "/");
	
	// The format of the HTTP response
	const char* format = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n\r\n%s";
	size_t str_len = strlen(str);

	// Dynamically allocating memory for the HTTP response
	char* res = (char*)calloc(TEXT_PLAIN_HEADERS_LENGTH + str_len + count_digits(str_len) + 1, sizeof(*res));

	if(res == NULL){
		fprintf(stderr, "Error: Memory allocation for HTTP response failed\n");
		return -1;
	}

	// Sending the HTTP response
	sprintf(res, format, strlen(str), str);
	ssize_t bytesSent = send(client_fd, res, strlen(res), 0);	

	if(bytesSent == -1){
		fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
		return -1;
	}

	// Freeing up allocated memory
	free(res);

	return 0;
}

// Function for handling the /user-agent endpoint
int handle_user_agent_route(int client_fd, char* path){
	// Variable for storing user agent used by the client
	char* user_agent = strtok(NULL, " ");
	user_agent = strtok(NULL, " ");

	while(!is_substring("User-Agent:", user_agent)){
		user_agent = strtok(NULL, " ");

		if (is_substring(REQUEST_END, user_agent)){
			fprintf(stderr, "Error: User agent not found\n");
			return -1;
		}
	}

	user_agent = strtok(NULL, " ");
	user_agent = strtok(user_agent, "\r\n");

	// The format of the HTTP response
	const char* format = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n\r\n%s";
	size_t user_agent_len = strlen(user_agent);

	// Dynamically allocating memory for the HTTP response
	char* res = (char*)calloc(TEXT_PLAIN_HEADERS_LENGTH + user_agent_len + count_digits(user_agent_len) + 1, sizeof(*res));

	if(res == NULL){
		fprintf(stderr, "Error: Memory allocation for HTTP response failed\n");
		return -1;
	}

	// Sending the HTTP response to the client
	sprintf(res, format, strlen(user_agent), user_agent);
	ssize_t bytesSent = send(client_fd, res, strlen(res), 0);

	if(bytesSent == -1){
		fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
		return -1;
	}

	// Freeing up allocated memory
	free(res);

	return 0;
}

// Function for handling the /files/{file} endpoint
int handle_files_route(int client_fd, char* path, int argc, char** argv){
	// Using this endpoint, the server will return the requested file.
	if(argc <= 2){
		fprintf(stderr, "Usage: --directory <relative_directory_path\n");
		exit(1);
	}

	/* TODO: Implement error handling for if the directory name is invalid */
	// Variable for storing the directory name where the requested file is to be located
	char* directory;

	// Variable to return the number of bytes sent
	ssize_t bytesSent;

	// Extracting the requested filename
	char* filename = strtok(path, "/");
	filename = strtok(NULL, "/");

	// Checking if the --directory flag is used while running this program or not
	if(strcmp(argv[1], "--directory") == 0){
		if(argv[2][0] == '.'){
			// Extracting the real path for the directory
			directory = realpath(argv[2], NULL);

			if(directory == NULL){
				fprintf(stderr, "Error: Unable to resolve absolute path - %s\n", strerror(errno));
				exit(1);
			}
		}
		else{
			// Allocating memory for the real path of the directory
			directory = (char*)calloc((strlen(argv[2]) + 1), sizeof(*directory));

			if(directory == NULL){
				fprintf(stderr, "Error: Unable to allocate memory for the absolute path - %s\n", strerror(errno));
				return -1;
			}

			strcpy(directory, argv[2]);

			// Check if the directory exists
			if(is_directory_exists(directory) == -1){
				fprintf(stderr, "Error: %s\n", strerror(errno));
				exit(1);
			}
		}
	}
	else{
		fprintf(stderr, "Error: No --directory flag\n");
		exit(1);
	}

	// The size of the real path of the file
	ssize_t pathsize = strlen(directory) + strlen(filename) + 2;

	// Dynamically allocating memory store the full real path name of the requested file
	directory = (char*)reallocarray(directory, pathsize, sizeof(*directory));

	if(directory == NULL){
		fprintf(stderr, "Error: Memory reallocation for storing the directory name failed\n");
		return -1;
	}

	strcat(directory, "/");
	strcat(directory, filename);

	// If the server can access the file, open it
	if(!access(directory, F_OK)){
		// File pointer to the requested file
		FILE* file = fopen(directory, "rb");

		if(file == NULL){
			fprintf(stderr, "Error: Unable to open file requested\n");
			return -1;
		}

		// The size of the requested file
		ssize_t filesize = fsize(file);

		if(filesize == -1){
			fprintf(stderr, "Error: Unable to determine the size of file requested \n");
			return -1;
		}

		// Dynamically allocating memory for a buffer for storing the contents of the requested file
		char* fileBuffer = (char*)calloc(filesize+1, sizeof(*fileBuffer));
		fileBuffer[filesize] = '\0';
		
		// Reading the contents of the file into the buffer
		size_t newLen = fread(fileBuffer, sizeof(char), (size_t)filesize, file);
		if(ferror(file) != 0){
			fprintf(stderr, "Error: Unable to read file\n");
			return -1;
		}
		
		// NULL terminating the buffer and closing the requested file
		fileBuffer[newLen++] = '\0';
		fclose(file);

		// The format of the HTTP response
		const char* format = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length:%zu \r\n\r\n%s";	   
		
		// Dynamically allocate memory for the HTTP response
		char* res = (char*)calloc(OCTET_STREAM_HEADERS_LENGTH + count_digits(filesize) + filesize + 1, sizeof(*res));

		if(res == NULL){
			fprintf(stderr, "Error: Memory allocation for HTTP response failed\n");
			return -1;
		}

		// Sending the HTTP response
		sprintf(res, format, filesize, fileBuffer);
		bytesSent = send(client_fd, res, strlen(res), 0);

		// Freeing up allocated memory
		free(res);
		free(fileBuffer);
	}
	// Otherwise send Not Found
	else{
		char* res = "HTTP/1.1 404 Not Found\r\n\r\n";
		bytesSent = send(client_fd, res, strlen(res), 0);
	}

	if(bytesSent == -1){
		fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
		return -1;
	}

	// Freeing up allocated memory 
	free(directory);

	return 0;
}

#endif 