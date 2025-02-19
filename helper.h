#ifndef HELPER_H
#define HELPER_H

#include <string.h>
#include <stdio.h>

int find_substring(char* substring, char* string){
    int substringLen = strlen(substring);
    int stringLen = strlen(string);

    char* p1 = substring + substringLen - 1;
    char* p2 = string + stringLen - 1;

    if (substringLen <= stringLen){
        for (int i=0; i<substringLen; i++){
            if (*p1 == *p2){
                p1--;
                p2--;
            }
            else
                return 0;
        }

        return 1;
    }
    
    return 0;
}

ssize_t fsize(FILE* file){
    if(fseek(file, 0L, SEEK_END) == -1)
        return -1;
    ssize_t size = ftell(file);

    if(fseek(file, 0L, SEEK_SET) == -1)
        return -1;

    return size;
}

#endif