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

long fsize(FILE* file){
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    return size;
}

#endif