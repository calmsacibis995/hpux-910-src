/* HPUX_ID:  @(#) $Revision: 64.9 $ */

#ifdef _NAMESPACE_CLEAN
#define lseek _lseek
#define prealloc _prealloc
#define fstat _fstat
#define write _write
#define fsync _fsync
#define ftruncate _ftruncate
#       ifdef   _ANSIC_CLEAN
#define free _free
#define malloc _malloc
#       endif  /* _ANSIC_CLEAN */
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
long lseek();
char *malloc();
extern errno;

/*
 *  prealloc allocates files space.
 *    The file must be of zero length and
 *      a regular file.
 *    The file is zeroed, written to disc
 *      and fsync'ed with disc.
 *
 */

#define FAILURE -1
#define PREBUFSIZ 128

#define	min(a,b)	((a) < (b) ? (a) : (b))

#ifdef _NAMESPACE_CLEAN
#undef prealloc
#pragma _HP_SECONDARY_DEF _prealloc prealloc
#define prealloc _prealloc
#endif

prealloc(fd,size)
	int fd;
	unsigned size;
 {
	char *buf;
	char emergency_buf[PREBUFSIZ];
	struct stat fdbuf;
	long i,saverrno,thistime;
	unsigned sofar;
	unsigned prebufsiz;

	errno = 0;

	/*
	 * Get file status
	 */
	if(fstat(fd,&fdbuf) == FAILURE){
		return(FAILURE);
	}

	/* Zero length ? */
	if(fdbuf.st_size !=0){
		errno = ENOTEMPTY;
		return(FAILURE);
	}

	/* Regular file ? */
	if((fdbuf.st_mode & S_IFMT)!=(S_IFREG)){
		errno = EINVAL;
		return(FAILURE);
	}

	/* Get an optimal size buffer */
	prebufsiz = fdbuf.st_blksize;
	buf = malloc(prebufsiz);

	/* Only if malloc didn't work do we do this */
	if(buf == 0){
		buf = emergency_buf;
		prebufsiz = PREBUFSIZ;
	}


	/* zero buffer */
	for(i=0;i<prebufsiz;i++)
		buf[i] = 0;


	/* fill file with zeroes */ 
	for(sofar = thistime = 0; sofar < size; sofar += thistime)
		if ((thistime=write(fd,buf,min(prebufsiz,(size-sofar))))
								     <= 0)
				goto failure;		/* error in write */

	/* Synchronize file with disc image */
	if(fsync(fd)==FAILURE){
		goto failure;
	}

	/* Set pointer to beginning of file */
	if(lseek(fd,(long)0,L_SET)!=0){
		goto failure;
	}

	/* return buffer */
	if(buf != emergency_buf)
		free(buf);

	return(0);

failure:
	/* return buffer */
	if(buf != emergency_buf)
		free(buf);

	/*  
	 *  Save the error of the offending call.
	 *  Attempt to truncate the file, even though
	 *  we got an error. Then restore the error
	 *  and return.
	 */

	saverrno = errno;
	(void)ftruncate(fd,0);  /* attempt to truncate, even though error */
	errno = saverrno;
	return(FAILURE);
 }
