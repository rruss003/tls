#ifndef __BLOOMFILTER_H__
#define __BLOOMFILTER_H__

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define OBJNUM 256
#define HASHNUM 4
#define ARRAYSIZE 24


void readarrayfromfile(uint32_t hashtable[ARRAYSIZE], const char *path);
void writearraytofile(uint32_t hashtable[ARRAYSIZE], const char *path);
int bloomfilter_init(uint32_t hashtable[ARRAYSIZE]);
int bloomfilter_insert(uint32_t hashtable[ARRAYSIZE], const char *filename, const int length);
int bloomfilter_query(uint32_t hashtable[ARRAYSIZE], const char *filename, const int length);

#endif // DEBUG