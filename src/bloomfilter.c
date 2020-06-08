#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "hash.h"
#include "bloomfilter.h"

// init the bloom filter array
int bloomfilter_init(uint32_t hashtable[ARRAYSIZE]){
    for(int i = 0; i < ARRAYSIZE; i++){
        hashtable[i] = 0;
    }
    return 0;
}

//insert an object to bloom filter
int bloomfilter_insert(uint32_t hashtable[ARRAYSIZE], const char *filename, const int length){
    for (int i = 0; i < HASHNUM; i++){
        uint32_t hash_value;
        hash_value = mmhash3_32(filename, length, i);
        int bitindex = hash_value % ARRAYSIZE;
        if(hashtable[bitindex] == 0){
            hashtable[bitindex] = 1;
        }
    }
    printf("Inserted file:[%s] into bloom filter\n", filename);
    return 0;
}

//check if requested file is in bloom filter
int bloomfilter_query(uint32_t hashtable[ARRAYSIZE], const char *filename, const int length){
    for(int i=0; i<HASHNUM; i++){
        uint32_t hash_value;
        int bitindex;
        hash_value = mmhash3_32(filename,length,i);
        bitindex = hash_value % ARRAYSIZE;
        if(hashtable[bitindex] != 0){
            continue;
        }else{
            printf("Requested file [%s] not present!\n", filename);
            return -1;
        }
    }
    printf("Requested file [%s] probably present!\n",filename);
    return 0;
}