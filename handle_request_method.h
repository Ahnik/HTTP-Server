#ifndef HANDLE_REQUEST_METHOD_H
#define HANDLE_REQUEST_METHOD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "helper.h"
#include "handle_cases.h"

#define HEADERS_END "\r\n\r\n"

// Function to handle the GET request method
int handle_get_method(int client_fd, char* path, int argc, const char** argv){
    int status;

	// Implementing support for /echo/{str} endpoint
	if(strncmp(path, "/echo/", 6) == 0)
    	status = handle_echo_route(client_fd, path);

	// Implementing support for /user-agent endpoint
	else if(strncmp(path, "/user-agent", 11) == 0)
		status = handle_user_agent_route(client_fd, path);
		
	// Implementing support for /files/{filename} endpoint
	else if(strncmp(path, "/files/", 7) == 0)
		status = handle_files_route(client_fd, path, argc, argv);

    // Implementing support for empty HTTP GET request
	else if (strcmp(path, "/") == 0){
		char* res = "HTTP/1.1 200 OK\r\n\r\n";
		ssize_t bytesSent = send(client_fd, res, strlen(res), 0);

		if(bytesSent == -1){
			fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
			return A_ERROR;
		}
	}

    // Implementing support for "Not Found" case
	else{
		char* res = "HTTP/1.1 404 Not Found\r\n\r\n";
		ssize_t bytesSent = send(client_fd, res, strlen(res), 0);

		if(bytesSent == -1){
			fprintf(stderr, "Error: Send failed - %s\n", strerror(errno));
			return A_ERROR;
		}
	}

    return status;
}

// Function to handle the POST request method with the /files/{file} endpoint
int handle_post_method(int client_fd, char* path, char* content, int argc, const char** argv){
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
			return A_ERROR;
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
	FILE* file = fopen(pathname, "wb");

	if(file == NULL){
		fprintf(stderr, "Error: Unable to create target file\n");
		free(pathname);
		return A_ERROR;
	}

	// Writing the contents into the file
	size_t bytesWritten = fwrite((void*)file_content, sizeof(char), strlen(file_content), file);
	if(ferror(file) != 0){
		fprintf(stderr, "Error: Unable to write onto the file\n");
		free(pathname);
		return A_ERROR;
	}

	printf("Bytes Written: %zu", bytesWritten);

	// Closing the file
	fclose(file);

	// Free up allocated memory
	free(pathname);

	return A_OK;
}


#endif