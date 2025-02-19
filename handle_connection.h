#ifndef HANDLE_CONNECTION_H
#define HANDLE_CONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "helper.h"

#define MAX_STR_LENGTH 4096
#define OCTET_STREAM_HEADERS_LENGTH strlen("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: \r\n\r\n")
#define TEXT_PLAIN_HEADERS_LENGTH strlen("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: \r\n\r\n")

void handle_connection(int client_fd, int argc, char** argv){
	// Reading the HTTP request from the client
	char readBuffer[MAX_STR_LENGTH];
	readBuffer[MAX_STR_LENGTH-1] = '\0';
	
	ssize_t bytesRead = recv(client_fd, readBuffer, MAX_STR_LENGTH-1, 0);

	if(bytesRead < 0){
		fprintf(stderr, "Error: Receiving failed\n");
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
		user_agent = strtok(NULL, " ");

		while(!find_substring("User-Agent:", user_agent)){
			user_agent = strtok(NULL, " ");

			if (find_substring("\r\n\r\n", user_agent)){
				printf("Error: User agent not found\n");
				close(client_fd);
				return;
			}
		}

		user_agent = strtok(NULL, " ");
		user_agent = strtok(user_agent, "\r\n");

		const char* format = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n\r\n%s";
        size_t user_agent_len = strlen(user_agent);

		char* res = (char*)calloc(TEXT_PLAIN_HEADERS_LENGTH + user_agent_len + count_digits(user_agent_len) + 1, sizeof(*res));

        if(res == NULL){
            fprintf(stderr, "Error: Memory allocation for HTTP response failed\n");
            close(client_fd);
            return;
        }

		sprintf(res, format, strlen(user_agent), user_agent);
		bytesSent = send(client_fd, res, strlen(res), 0);
        free(res);
	}	
    else if(strncmp(reqPath, "/files/", 7) == 0 && argc > 2){
        char* directory;

        if(strcmp(argv[1], "--directory") == 0){
            directory = realpath(argv[2], NULL);
        }
        else{
            fprintf(stderr, "Error: Directory not mentioned\n");
            close(client_fd);
            exit(1);
        }

        char* filename = strtok(reqPath, "/");
        filename = strtok(NULL, "/");

        ssize_t pathsize = strlen(directory) + strlen(filename) + 2;

        directory = (char*)reallocarray(directory, pathsize, sizeof(*directory));

        if(directory == NULL){
            fprintf(stderr, "Error: Memory reallocation for storing the directory name failed\n");
            close(client_fd);
            return;
        }

        strcat(directory, "/");
        strcat(directory, filename);

        printf("%s\n", directory);

        if(!access(directory, F_OK)){
            FILE* file = fopen(directory, "rb");

            if(file == NULL){
                fprintf(stderr, "Error: Unable to open file requested\n");
                close(client_fd);
                return;
            }

			ssize_t filesize = fsize(file);

            if(filesize == -1){
                fprintf(stderr, "Error: Unable to determine the size of file requested\n");
                close(client_fd);
                return;
            }

			char* fileBuffer = (char*)calloc(filesize+1, sizeof(*fileBuffer));
			fileBuffer[filesize] = '\0';
			
			size_t newLen = fread(fileBuffer, sizeof(char), (size_t)filesize, file);
			if(ferror(file) != 0){
				fprintf(stderr, "Error: Unable to read file\n");
                close(client_fd);
				return;
			}
			
			fileBuffer[newLen++] = '\0';
			fclose(file);

			const char* format = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length:%zu \r\n\r\n%s";	            
            char* res = (char*)calloc(OCTET_STREAM_HEADERS_LENGTH + count_digits(filesize) + filesize + 1, sizeof(*res));

            if(res == NULL){
                fprintf(stderr, "Error: Memory allocation for HTTP response failed\n");
                close(client_fd);
                return;
            }

			sprintf(res, format, filesize, fileBuffer);
			bytesSent = send(client_fd, res, strlen(res), 0);

			printf("%s\n", fileBuffer);
            free(res);
			free(fileBuffer);
        }
        else{
            char* res = "HTTP/1.1 404 Not Found\r\n\r\n";
		    bytesSent = send(client_fd, res, strlen(res), 0);
        }

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
		fprintf(stderr, "Error: Send failed\n");
		close(client_fd);
		return;
	}

    free(content);
    free(content_dup);
	close(client_fd);
}

#endif 