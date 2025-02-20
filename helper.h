#ifndef HELPER_H
#define HELPER_H

#include <string.h>
#include <stdio.h>

int find_substring(char* substring, char* string){
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

ssize_t fsize(FILE* file){
    if(fseek(file, 0L, SEEK_END) == -1)
        return -1;
    ssize_t size = ftell(file);

    if(fseek(file, 0L, SEEK_SET) == -1)
        return -1;

    return size;
}

size_t count_digits(ssize_t num){
    size_t cnt = 1;

    while(num%10 != num){
        num /= 10;
        cnt++;
    }
    
    return cnt; 
}

#endif