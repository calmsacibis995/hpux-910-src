/* @(#) $Revision: 29.1 $ */    
#include <fcntl.h>

static int fileflags;

extern void logerr();

setndelay(fd)
    int fd;
{
    if ((fileflags = fcntl(fd,F_GETFL,0)) == -1) {
	logerr("fcntl F_GETFL error.","");
	return(-1);
    }

    if (fcntl(fd,F_SETFL,(fileflags | O_NDELAY)) == -1) {
	logerr("fcntl F_SETFL error.","");
	return(-1);
    }
    return(0);
}

unsetndelay(fd)
    int fd;
{
    if ((fileflags = fcntl(fd,F_GETFL,0)) == -1) {
	logerr("fcntl F_GETFL error.","");
	return(-1);
    }

    if (fcntl(fd,F_SETFL,(fileflags & ~O_NDELAY)) == -1) {
	logerr("fcntl F_SETFL error.","");
	return(-1);
    }
    return(0);
}
