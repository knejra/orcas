#include "util.h"

void *memmove(void *dst, const void *src, size_t cnt)
{
    char *d = (char *)dst;
    const char *s = (const char *)src;
    while(cnt)
    {
        *d = *s;
        d++;
        s++;
        cnt--;
    }
    return dst;
}

int strcmp(const char *str1, const char *str2)
{
    char *p1 = str1;
    char *p2 = str2;
    while(*p1 == *p2)
    {
        if(*p1 == '\0' && *p2 == '\0')
        {
            return 0;
        }
        p1++;
        p2++;
    }

    return *p1 == '\0' ? -1 : 1; 
}

int strncpy(char *dst, const char *src, size_t cnt)
{
    char *d = dst;
    char *s = src;

    while(cnt > 0)
    {
        if(*s == '\0')
        {
            break;
        }
        *d++ = *s++;
        cnt--;
    }

    return cnt;
}

int strlen(char *str)
{
    int len = 0;
    
    char *p = str;
    while(*p != '\0')
    {
        p++;
        len++;
    }

    return len;
}

int strcat(char *dst, char *src)
{
    while(*dst != '\0')
    {
        dst++;
    }

    while(*src != '\0')
    {
        *dst++ = *src++;
    }
}

int split(const char *str, char c, char terms[NTERM][LTERM])
{
    char *p = (char *)str;

    // jump
    while(*p == c)
    {
        p++;
    }

    int i = 0, j = 0;
    while(*p != '\0')
    {
        if(*p == c)
        {
            terms[i++][j] = '\0';
            j = 0;
        }
        else
        {
            terms[i][j++] = *p;
        }
        p++;
    }

    return i;
}

void strTest()
{
    char text1[40] = "/Documents";
    char text2[] = "/Projects";
    strcat(text1, text2);
    printf("%s\n", text1);
    
    char text3[] = "/Documents/Projects/Orcas/";
    char terms[10][20];
    int n = 0;
    n = split(text3, '/', (char **)terms);
    printf("%d\n", n);
    for(int i = 0; i < n; i++)
    {
        printf("%s\n", terms[i]);
    }
}