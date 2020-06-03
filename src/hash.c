#include "hash.h"
#define len(a) (sizeof(a)/sizeof(a[0]))
unsigned short hash(const char* s, const char* s2)
{
    const unsigned short ports[] = {4951};
    size_t temp;
    for(size_t i=0; i<strlen(s); i++)
    {
        temp += s[i];
    }
    for(size_t i=0; i<strlen(s2); i++)
    {
        temp += s2[i];
    }
    return ports[temp % len(ports)];
}