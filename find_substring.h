#ifndef FIND_SUBSTRING_H
#define FIND_SUBSTRING_H

#include <string.h>

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

#endif