#ifndef __HASH_H__
#define __HASH_H__


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define LISTLEN 64

struct proxy{
    int port;
    char cport[32];
    char name[256];
    char addr[256];
    long long hash_value;
};

long long polyhash(const char* str, const int len);
uint32_t mmhash3_32(const char* str, const int length, int seed);
int rendezvous_hashing();


#endif // DEBUG