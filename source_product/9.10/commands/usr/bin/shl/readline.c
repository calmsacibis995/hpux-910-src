/* @(#) $Revision: 29.1 $ */     
/* readline() is used to read a line at a time from a terminal that may   */
/* be in raw mode. readline() removes the terminating newline. readline() */
/* returns the number of bytes read (not counting the newline) or -1 if   */
/* there was a read error or the EOF character was typed (terminal is not */
/* in raw mode in the latter case).                                       */

readline(fd,buf,buflen)
    int fd;
    char *buf;
    int buflen;
{
    int nread;
    int i;

    nread = 0;
    do {
	i = read(fd,&buf[nread],buflen - nread);
	if (i > 0)
	    nread += i;
	else
	    nread = -1;

    } while (nread != -1 && nread != buflen && buf[nread - 1] != '\n');

    if (nread > 0 && buf[nread - 1] == '\n') {
	--nread;
	buf[nread] = '\0';
    }
    else
	nread = -1;

    return(nread);
}













