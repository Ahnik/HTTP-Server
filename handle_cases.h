#ifndef HANDLE_CASES_H
#define HANDLE_CASES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include "helper.h"
#include "encoder.h"

#define OCTET_STREAM_HEADERS_LENGTH strlen("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: \r\n\r\n")
#define TEXT_PLAIN_HEADERS_LENGTH strlen("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: \r\n\r\n")
#define HEADERS_END "\r\n\r\n"

#define A_OK 0
#define A_ERROR -1

// Function for handling the /echo/{str} endpoint
int handle_echo_route(int client_fd, char* path){
	// The string to be returned back to the client
	char* str = strtok(path, "/");
	str = strtok(NULL, "/");

	size_t str_len = strlen(str);

	// Dynamically allocating memory for the HTTP response
	size_t res_size = TEXT_PLAIN_HEADERS_LENGTH + str_len + count_digits(str_len) + 1;
	unsigned char* res = (unsigned char*)calloc(res_size, sizeof(*res));

	if(res == NULL){
		fprintf(stderr, "Error: Memory allocation for HTTP response failed\n");
		return A_ERROR;
	}
	
	// The format of the HTTP response
	const char* format = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n\r\n%s";

	// Sending the HTTP response
	sprintf((char*)res, format, strlen(str), str);

	size_t response_size = strlen((char*)res);
	ssize_t bytesSent = send(client_fd, res, response_size, 0);	
	

	if(bytesSent == -1){
		fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
		free(res);
		return A_ERROR;
	}

	// Freeing up allocated memory
	free(res);

	return A_OK;
}

// Function for handling the /user-agent endpoint
int handle_user_agent_route(int client_fd){
	// Variable for storing user agent used by the client
	char* user_agent = strtok(NULL, " ");
	user_agent = strtok(NULL, " ");

	while(!is_substring("User-Agent:", user_agent)){
		user_agent = strtok(NULL, " ");

		if (is_substring(HEADERS_END, user_agent)){
			fprintf(stderr, "Error: User agent not found\n");
			return A_ERROR;
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
		return A_ERROR;
	}

	// Sending the HTTP response to the client
	sprintf(res, format, strlen(user_agent), user_agent);
	ssize_t bytesSent = send(client_fd, res, strlen(res), 0);

	if(bytesSent == -1){
		fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
		free(res);
		return A_ERROR;
	}

	// Freeing up allocated memory
	free(res);

	return A_OK;
}

// Function for handling the /files/{file} endpoint
int handle_files_route(int client_fd, char* path, int argc, const char** argv){
	// Extracting the requested filename
	char* filename = strtok(path, "/");
	filename = strtok(NULL, "/");

	if(argc <= 2){
		fprintf(stderr, "Usage: --directory <relative_directory_path\n");
		exit(1);
	}

	// Variable for storing the directory name where the requested file is to be located
	char* pathname;

	// Variable to return the number of bytes sent
	ssize_t bytesSent;

	// Checking if the --directory flag is used while running this program or not
	if(strcmp(argv[1], "--directory") == 0){
		pathname = create_pathname(argv[2], filename);

		if(pathname == NULL){
			fprintf(stderr, "Error: %s\n", strerror(errno));
			free(pathname);
			return A_ERROR;
		}
	}else{
		fprintf(stderr, "Error: No --directory flag\n");
		exit(1);
	}

	// If the server can access the file, open it
	if(!access(pathname, F_OK)){
		// File pointer to the requested file
		FILE* file = fopen(pathname, "rb");

		if(file == NULL){
			fprintf(stderr, "Error: Unable to open file requested\n");
			free(pathname);
			return A_ERROR;
		}

		// The size of the requested file
		ssize_t filesize = fsize(file);

		if(filesize == -1){
			fprintf(stderr, "Error: Unable to determine the size of file requested\n");
			free(pathname);
			return A_ERROR;
		}

		char* fileBuffer = read_file(file, filesize);

		if(fileBuffer == NULL){
			free(pathname);
			return A_ERROR;
		}

		fclose(file);

		// The format of the HTTP response
		const char* format = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length:%zu \r\n\r\n%s";	   
		
		// Dynamically allocate memory for the HTTP response
		char* res = (char*)calloc(OCTET_STREAM_HEADERS_LENGTH + count_digits(filesize) + filesize + 1, sizeof(*res));

		if(res == NULL){
			fprintf(stderr, "Error: Memory allocation for HTTP response failed\n");
			free(pathname);
			free(fileBuffer);
			return A_ERROR;
		}

		// Sending the HTTP response
		sprintf(res, format, filesize, fileBuffer);
		bytesSent = send(client_fd, res, strlen(res), 0);

		// Freeing up allocated memory
		free(res);
		free(fileBuffer);
	}else{		// Otherwise send Not Found
		char* res = "HTTP/1.1 404 Not Found\r\n\r\n";
		bytesSent = send(client_fd, res, strlen(res), 0);
	}

	if(bytesSent == -1){
		fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
		free(pathname);
		return A_ERROR;
	}

	// Freeing up allocated memory 
	free(pathname);

	return A_OK;
}

#endif