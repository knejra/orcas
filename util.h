#ifndef _UTIL_H
#define _UTIL_H

#include "types.h"

// memory

// memmove: move n bytes from src to dst
// parameters: dst-destination address
//             src-source address
//             cnt-bytes count
// outputs   : dst
void *memmove(void *dst, const void *src, size_t cnt);

// strcmp: compare string 1 with string 2
// parameters: str1-string 1
//             str2-string 2
// outputs   : 0-equal, -1-len(str1) < len(str2), 1 len(str1) > len(str2) 
int strcmp(const char *str1, const char *str2);

// strncpy: copy n char fron source string to destination string
// parameters: dst-destination string
//             src-source string
//             cnt-num to copy
// outputs   : 0
int strncpy(char *dst, const char *src, size_t cnt);

// strlen: get string length
// parameters: str-string
// outputs   : length of str
int strlen(char *str);

// strcat: connect string src with string dst
// parameters: dst-destination string
//             src-source string
// outputs  : 0
// warning  : virtual malloc has not been test, so dst must
//            keep enough space for strcat
int strcat(char *dst, char *src);

#define NTERM 10
#define LTERM 20
int split(const char *str, char c, char terms[NTERM][LTERM]);

void strTest();

// general double-linked list
struct dlistNode
{
    void *value;
    struct dlistNode *prev;
    struct dlistNode *next;
};

typedef struct dliskNode dliskNode;

// general hashtable

#endif // _UTIL_H