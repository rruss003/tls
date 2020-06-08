#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "hash.h"

//polynomial rolling hash function, for the rendezvous_hashing
long long polyhash(const char* str, const int len){
    //input is the filename, which is lowercase, uppercase letters and some punctuation; the total kinds of characters used will be less than 100;
    int p = 101;
    int m = 1E9+9;
    long long hash_value = 0;
    long long p_power = 1;
    for (int i = 0; i < len; i++){
        //' ', space can be the first character we accept in ASCII code. Then we let space be 1.
        hash_value = (hash_value + (str[i] - ' ' +1) * p_power)%m;
        p_power = (p_power * p)%m;
    }
    return hash_value;
}

static inline uint32_t mmhash3_32_scramble(uint32_t k){
    k *= 0xcc9e2d51;
    k =(k << 15) | (k >> 17);
    k *=0x1b873593;
    return k;
}

// a 32 bits murmur hash function, for the bloom filter, code mianly from wiki
// use seed to generate different hash values. Good for bloom filter, since it need many different hash functions
uint32_t mmhash3_32(const char *str, int length, int seed){
    uint32_t hash = seed;
    uint32_t k;
    // read in 8bits per time
    for(int i = length >> 2; i; i--){
        memcpy(&k, str, sizeof(uint32_t));
        str += sizeof(uint32_t);
        hash ^= mmhash3_32_scramble(k);
        hash = (hash << 13) | (hash >> 19);
        hash = hash*5 + 0xe6546b64;
    }
    //read the rest
    k=0;
    for(int i = length&3; i; i--){
        k <<= 8;
        k |= str[i-1];
    }
    hash ^= mmhash3_32_scramble(k);
    //finalize
    hash ^= length;
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash;
}

//Highest Random Weight Hashing 
int rendezvous_hashing(const char *filename, const int length){
    struct proxy proxylist[LISTLEN];
    char buf[256];
    FILE *fp = NULL;
    char *result = NULL;
    fp = fopen("../src/proxylist.txt","r");
    // get a single line from the file
    for(int i=0;fgets(buf,256,fp)!=NULL; i++){
        // split the line
        result = strtok(buf,"\t");
        for(int j=0; j < 3; j++){
            //first string is port number
            if(j==0){
                proxylist[i].port = atoi(result);
                strncpy(proxylist[i].cport,result,strlen(result));
                // printf("%d\n",proxylist[i].port);
            }else if(j==1){
                //second string is address
                strncpy(proxylist[i].addr,result,strlen(result));
                // printf("%s\n",proxylist[i].addr);
            }else{
                //third string is proxy server name, doesn't work within localmachine
                strncpy(proxylist[i].name,result,strlen(result));
                // printf("%s\n",proxylist[i].name);
            }
            result = strtok(NULL,"\t");
        }
    }
    fclose(fp);
    printf("successfully read the proxylist\n");
    // find the max hash value
    long long max_value = 0;
    for(int i=0; i < LISTLEN; i++){
        if(proxylist[i].port != 0){
            char tmp[256];
            // tmp = filename + address + port
            strcpy(tmp,filename);
            strcat(tmp,proxylist[i].addr);
            strcat(tmp,proxylist[i].cport);
            // get the hash value
            proxylist[i].hash_value = polyhash(tmp,strlen(tmp));
            if(proxylist[i].hash_value > max_value){
                max_value = proxylist[i].hash_value;
            }
        }else{
            break;
        }
    }
    // return the port with highest weight
    for(int i=0; i < LISTLEN; i++){
        if(proxylist[i].hash_value == max_value){
            return proxylist[i].port;
        }
    }
    return 0; 
}