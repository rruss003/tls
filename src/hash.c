#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "hash.h"

#define LISTLEN 64

//polynomial rolling hash function, for the rendezvous_hashing
long long polyhash(const char* str, const int len)
{
    //input is the filename, which is lowercase, uppercase letters and some punctuation; the total kinds of characters used will be less than 100;
    int p = 101;
    int m = 1E9+9;
    long long hash_value = 0;
    long long p_power = 1;
    for (int i = 0; i < len; i++) {
        //' ', space can be the first character we accept in ASCII code. Then we let space be 1.
        hash_value = (hash_value + (str[i] - ' ' +1) * p_power)%m;
        p_power = (p_power * p)%m;
    }
    return hash_value;
}

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

void er()
{
    exit(1);
}

//Highest Random Weight Hashing 
u_short rendezvous_hash(const char *filename)
{
    char* proxylist[LISTLEN];
    char buf[10];
    FILE *fp = NULL;
    fp = fopen("./src/proxylist.txt","r");
	if(fp==NULL)
		errx(1, "missing proxylist.txt");

    // read file line by line
    for (int i=0; fgets(buf, 10, fp); i++)
        proxylist[i] = buf;
    fclose(fp);
    
    // find the max hash value
    long long max_hash = 0;
    int proxy = 0;
    for(int i=0; i < LISTLEN && proxylist[i]; i++) {
        char tmp[100];
        // tmp = filename + port
        snprintf(tmp, 99, "%s%s", filename, proxylist[i]);
        // get the hash value
        long long hash = polyhash(tmp,strlen(tmp));
        if (hash > max_hash)
        {
            max_hash = hash;
            proxy = i;
        }
    }
    // return the port with highest weight
    return get_port(proxylist[proxy], &er);
}