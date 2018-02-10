#include "helpers.h"

int split (const char *str, char c, char ***arr)
{
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;

    p = str;
    uint16_t times=0;
    while (times<3)
    {
        if (*p == c)
            count++;
        p++;
        if(*p == '\0')
        {
            times++;
        }
    }

    *arr = (char**) malloc(sizeof(char*) * count);
    if (*arr == NULL)
        exit(1);

    p = str;
    times = 0;
    while (times<3)
    {
        if (*p == c)
        {
            (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
            if ((*arr)[i] == NULL)
                exit(1);

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
        if(*p == '\0')
        {
            times++;
        }
    }
    (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
    if ((*arr)[i] == NULL)
        exit(1);

    i = 0;
    p = str;
    t = ((*arr)[i]);
    times=0;
    while (times<3)
    {
        if (*p != c)
        {
            *t = *p;
            t++;
        }
        else
        {
            *t = '\0';
            i++;
            t = ((*arr)[i]);
        }
        p++;
        if(*p == '\0')
        {
            times++;
        }
    }

    return count;
}