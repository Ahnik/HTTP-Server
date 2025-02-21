#ifndef HELPER_H
#define HELPER_H

#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

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

#endif