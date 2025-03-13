#ifndef HANDLE_CASES_H
#define HANDLE_CASES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include "helper.h"

#define OCTET_STREAM_HEADERS_LENGTH strlen("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: \r\n\r\n")
#define TEXT_PLAIN_HEADERS_LENGTH strlen("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: \r\n\r\n")
#define HEADERS_END "\r\n\r\n"

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
		free(res);
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

		if (is_substring(HEADERS_END, user_agent)){
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
		free(res);
		return -1;
	}

	// Freeing up allocated memory
	free(res);

	return 0;
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
			return -1;
		}
	}

	else{
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
			return -1;
		}

		// The size of the requested file
		ssize_t filesize = fsize(file);

		if(filesize == -1){
			fprintf(stderr, "Error: Unable to determine the size of file requested \n");
			free(pathname);
			return -1;
		}

		// Dynamically allocating memory for a buffer for storing the contents of the requested file
		char* fileBuffer = (char*)calloc(filesize+1, sizeof(*fileBuffer));
		fileBuffer[filesize] = '\0';
		
		// Reading the contents of the file into the buffer
		size_t newLen = fread(fileBuffer, sizeof(char), (size_t)filesize, file);
		if(ferror(file) != 0){
			fprintf(stderr, "Error: Unable to read file\n");
			free(pathname);
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
			free(pathname);
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
		free(pathname);
		return -1;
	}

	// Freeing up allocated memory 
	free(pathname);

	return 0;
}

// Function to handle the POST method of the /files/{filesname} endpoint
int handle_post_route(int client_fd, char* path, char* content, int argc, const char** argv){
	// Extracting the requested filename
	char* filename = strtok(path, "/");
	filename = strtok(NULL, "/");

	if(argc <= 2){
		fprintf(stderr, "Usage: --directory <relative_directory_path\n");
		exit(1);
	}

	// Variable for storing the directory name where the requested file is to be located
	char* pathname;

	// Checking if the --directory flag is used while running this program or not
	if(strcmp(argv[1], "--directory") == 0){
		pathname = create_pathname(argv[2], filename);

		if(pathname == NULL){
			fprintf(stderr, "Error: %s\n", strerror(errno));
			free(pathname);
			return -1;
		}
	}
	
	else{
		fprintf(stderr, "Error: No --directory flag\n");
		exit(1);
	}

	printf("Content: \n%s\n", content);

	// The content of the file stored as a string
	char* file_content = strstr(content, HEADERS_END);
	file_content += 4;

	printf("File Content: %s\n", file_content);

	// Creating the file for writing 
	FILE* file = fopen(pathname, "w");

	if(file == NULL){
		fprintf(stderr, "Error: Unable to create target file\n");
		free(pathname);
		return -1;
	}

	// Writing the contents into the file
	size_t bytesWritten = fwrite((void*)file_content, sizeof(char), strlen(file_content), file);
	if(ferror(file) != 0){
		fprintf(stderr, "Error: Unable to write onto the file\n");
		free(pathname);
		return -1;
	}

	printf("Bytes Written: %zu", bytesWritten);

	// Closing the file
	fclose(file);

	// Free up allocated memory
	free(pathname);

	return 0;
}

#endif