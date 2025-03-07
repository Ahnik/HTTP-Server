#ifndef HELPER_H
#define HELPER_H

#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#define REQUEST_END "\r\n\r\n"

// Returns 1 if the substring occurs at the end of the string and 0 otherwise
int is_substring(char* substring, char* string){
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

// Function to read HTTP request into a dynamically allocated buffer
char* read_http_request(int client_fd){
    // Reading the HTTP request from the client in readBuffer whose memory is dynamically allocated
	char* readBuffer = NULL;
	char chara;
	size_t size = 0;

	// Reading the HTTP request and dynamically allocating memory for the buffer one byte at a time
	while(1){
		if(recv(client_fd, (void*)&chara, sizeof(chara), 0) == -1){
			fprintf(stderr, "Error: Receiving failed - %s\n", strerror(errno));
			return NULL;
		}

		size++;
		readBuffer = (char*)reallocarray((void*)readBuffer, size, sizeof(*readBuffer));
		if(readBuffer == NULL){
			fprintf(stderr, "Error: Memory allocation for HTTP request failed\n");
			return NULL;
		}
		readBuffer[size - 1] = chara;

		if(is_substring(REQUEST_END, readBuffer))
			break;
	}

	size++;
	readBuffer = (char*)reallocarray((void*)readBuffer, size, sizeof(*readBuffer));
	if(readBuffer == NULL){
		fprintf(stderr, "Error: Memory allocation for HTTP request failed\n");
		return NULL;
	}

 	readBuffer[size - 1] = '\0';

    return readBuffer;
}


#endif