#ifndef HELPER_H
#define HELPER_H

#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#define HEADERS_END "\r\n\r\n"
#define CONTENT_LENGTH_HEADER_LEN strlen("Content-Length: ")

// Returns 1 if the substring occurs at the end of the string and 0 otherwise
int is_substring(const char* substring, const char* string){
    size_t substringLen = strlen(substring);
    size_t stringLen = strlen(string);

    size_t i = substringLen - 1;
    size_t j = stringLen - 1;

    if (substringLen <= stringLen){
        for (int k=0; k<substringLen; k++){
            if (*(substring + i) == *(string + j)){
                i--;
                j--;
            }
            else
                return 0;
        }

        return 1;
    }
    
    return 0;
}

// Returns the size of the file normally and returns -1 in case of any error
ssize_t fsize(FILE* file){
    if(fseek(file, 0L, SEEK_END) == -1)
        return -1;
    ssize_t size = ftell(file);

    if(fseek(file, 0L, SEEK_SET) == -1)
        return -1;

    return size;
}

// Returns the number of digits in an integral number
size_t count_digits(ssize_t num){
    size_t cnt = 1;

    while(num%10 != num){
        num /= 10;
        cnt++;
    }
    
    return cnt; 
}

// Returns 1 if a particular directory whose path is given exists and returns 0 otherwise
int is_directory_exists(const char* path){
    struct stat stats;

    if(stat(path, &stats) == -1)
        return -1;

    // Check for directory existence
    if(S_ISDIR(stats.st_mode))
        return 1;
    
    return 0;
}

// Function that takes the directory path(absolute or relative) and filename as arguments and returns the complete pathname
char* create_pathname(const char* directory, const char* filename){
    char* pathname;
    if(directory[0] == '.'){
        // Extracting the real path for the directory
        pathname = realpath(directory, NULL);

        if(pathname == NULL)
            exit(1);
    }
    else{
        // Allocating memory for the real path of the directory
        pathname = (char*)calloc((strlen(directory) + 1), sizeof(*pathname));

        if(pathname == NULL)
            return NULL;

        strcpy(pathname, directory);

        // Check if the directory exists
        if(is_directory_exists(pathname) == -1)
            exit(1);
    }

    // The size of the real path of the file
	ssize_t pathsize = strlen(pathname) + strlen(filename) + 2;

	// Dynamically allocating memory store the full real path name of the requested file
	pathname = (char*)reallocarray(pathname, pathsize, sizeof(*pathname));

	if(pathname == NULL)
		return NULL;

	strcat(pathname, "/");
	strcat(pathname, filename);

    return pathname;
}

// Function to read the request body
char* read_request_body(int client_fd, char* readBuffer, size_t size){
    char chara;

    // Extract the content length from the content-length header.
    size_t content_length;
    char* content_length_header = strstr(readBuffer, "Content-Length: ");

    if(content_length_header == NULL){
        fprintf(stderr, "Error: Content-Length header not found\n");
        free(readBuffer);
        return NULL;
    }

    sscanf(content_length_header + CONTENT_LENGTH_HEADER_LEN, "%zu", &content_length);

    // Increase the buffer size to accomodate the request body
    readBuffer = (char*)reallocarray((void*)readBuffer, size + content_length, sizeof(*readBuffer));
    if(readBuffer == NULL){
        fprintf(stderr, "Error: Memory allocation for HTTP request failed\n");
        free(readBuffer);
        return NULL;
    }

    // NULL terminate the buffer
    readBuffer[size + content_length - 1] = '\0';

    // Fill in the request body into the buffer
    for(int i=0; i<content_length; i++){
        if(recv(client_fd, (void*)&chara, sizeof(chara), 0) <= 0){
            fprintf(stderr, "Error: Receiving failed - %s\n", strerror(errno));
            free(readBuffer);
            return NULL;
        }

        readBuffer[size + i - 1] = chara;
    }

    return readBuffer;
}


// Function to read HTTP request into a dynamically allocated buffer
char* read_http_request(int client_fd){
    // Reading the HTTP request from the client in readBuffer whose memory is dynamically allocated
	size_t size = 1;
	char* readBuffer = calloc(size, sizeof(*readBuffer));

    if(readBuffer == NULL){
        fprintf(stderr, "Error: Memory allocation for HTTP request failed\n");
        free(readBuffer);
        return NULL;
    }

	char chara;

	// Reading the HTTP request and dynamically allocating memory for the buffer one byte at a time
	while(1){
		if(recv(client_fd, (void*)&chara, sizeof(chara), 0) <= 0){
			fprintf(stderr, "Error: Receiving failed - %s\n", strerror(errno));
            free(readBuffer);
			return NULL;
		}

        // Incrementing the size of the buffer every time the loop runs until we have read the entire HTTP request
		size++;
		readBuffer = (char*)reallocarray((void*)readBuffer, size, sizeof(*readBuffer));
		if(readBuffer == NULL){
			fprintf(stderr, "Error: Memory allocation for HTTP request failed\n");
            free(readBuffer);
			return NULL;
		}

        // Add the NULL character on the last spot and the new character in the second last spot.
		readBuffer[size - 1] = '\0';
        readBuffer[size - 2] = chara;

		if(is_substring(HEADERS_END, readBuffer)){
            // If the HTTP request is a GET request
            if(readBuffer[0] == 'G')
                break;

            // If the HTTP request is a POST request
            else if(readBuffer[0] == 'P'){
                readBuffer = read_request_body(client_fd, readBuffer, size);
                break;
            }
        }
	}

    return readBuffer;
}

// Function to read a file upto a certain size and return its contents in a dynamically allocated character array
char* read_file(FILE* file, ssize_t filesize){
    // Dynamically allocating memory for a buffer for storing the contents of the requested file
	char* fileBuffer = (char*)calloc(filesize+1, sizeof(*fileBuffer));

	if(fileBuffer == NULL){
		fprintf(stderr, "Error: Memory allocation for HTTP response failed\n");
		return NULL;
	}

	fileBuffer[filesize] = '\0';
		
	// Reading the contents of the file into the buffer
	size_t newLen = fread(fileBuffer, sizeof(char), (size_t)filesize, file);
	if(ferror(file) != 0){
		fprintf(stderr, "Error: Unable to read file\n");
		free(fileBuffer);
		return NULL;
	}
		
	// NULL terminating the buffer and closing the requested file
	fileBuffer[newLen++] = '\0';

    return fileBuffer;
}

// Function to send an HTTP status code 500 to a client file descriptor
int send_status_500(int client_fd){
    char* res = "HTTP/1.1 500 Internal Server Error\r\n\r\n";

	if(send(client_fd, res, strlen(res), 0))
        return 0;
    else
        return -1;
}


#endif