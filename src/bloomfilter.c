#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "bloomfilter.h"


static inline uint32_t mmhash3_32_scramble(uint32_t k)
{
    k *= 0xcc9e2d51;
    k =(k << 15) | (k >> 17);
    k *=0x1b873593;
    return k;
}

// a 32 bits murmur hash function, for the bloom filter, code mianly from wiki
// use seed to generate different hash values. Good for bloom filter, since it need many different hash functions
uint32_t mmhash3_32(const char *str, int length, int seed)
{
    uint32_t hash = seed;
    uint32_t k;
    // read in 8bits per time
    for(int i = length >> 2; i; i--) {
        memcpy(&k, str, sizeof(uint32_t));
        str += sizeof(uint32_t);
        hash ^= mmhash3_32_scramble(k);
        hash = (hash << 13) | (hash >> 19);
        hash = hash*5 + 0xe6546b64;
    }
    //read the rest
    k=0;
    for(int i = length&3; i; i--) {
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

// init the bloom filter array
int bloomfilter_init(uint32_t hashtable[ARRAYSIZE]){
    for(int i = 0; i < ARRAYSIZE; i++){
        hashtable[i] = 0;
    }
    return 0;
}
void writearraytofile(uint32_t array[ARRAYSIZE], const char *path) {
    FILE *fp = fopen(path, "w+");
    for(int i = 0; i < ARRAYSIZE; i++){
        fprintf(fp, "%u\n", array[i]);
    }
    fclose(fp);
}

void readarrayfromfile(uint32_t array[ARRAYSIZE], const char *path) {
    FILE *fp = fopen(path, "r");
    for(int i = 0; i < ARRAYSIZE; i++){
        int ret = fscanf(fp, "%d", &array[i]);
    }
    fclose(fp);
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