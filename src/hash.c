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