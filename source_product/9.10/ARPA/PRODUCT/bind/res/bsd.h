
#include <memory.h>
#include <string.h>

#define bcopy(src,dst,n)  _memcpy(dst,src,n)
#define bcmp(s1,s2,n)     _memcmp(s1,s2,n)
#define bzero(s1,n)       _memset(s1,'\0',n)
#define index(s1,c)       _strchr(s1,c)
