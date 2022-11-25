
#include <memory.h>
#include <string.h>

#define bcopy(src,dst,n)  	memcpy(dst,src,n)
#define bcmp(s1,s2,n)     	memcmp(s1,s2,n)
#define bzero(s1,n)       	memset(s1,'\0',n)
#define index(s1,c)       	strchr(s1,c)
