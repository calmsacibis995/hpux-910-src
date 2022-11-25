/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/doio.c,v $
 * $Revision: 1.7.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:30:32 $
 */


/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/


#ifdef BOBCAT
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define NBPG 0x800

/* globals */
int ddb_fildes = -1;
int hitCtrlC = 0;
int spec = 0;
extern int physmem;

extern int errno;
extern unsigned cur_address;

int
doopen(path, oflag, mode)
char *path;
int oflag, mode;
{
	int fildes = -1;
	if (strncmp(path, "/dev/ddb", 8) == 0){
		get_comm_link(&fildes, path);
		spec = ddb_fildes = fildes;
		cur_address = 0;
		return(fildes);
	} else {
		return(open(path, oflag, mode));
	}
}

int
doread(fildes, buf, nbyte)
int fildes;
char *buf;
int nbyte;
{
	int retval;
	int shortMsk;		/* Mask for short integer size */
	int intMsk;		/* Mask for integer size */

	shortMsk = sizeof(short) - 1;
				/* WORKs ONLY IF SHORT SIZE IS POWER OF 2 */
	intMsk = 3;		/* Integer size on Indigo is 4 bytes */

	if ((((int)buf & shortMsk) != 0) ||
		((nbyte % (intMsk + 1)) != 0) ||
		((int)nbyte == -1)){
		fprintf(stderr," alignment incorrect &buf 0x%x contact -joh\n",buf);
		return(-1);	/* alignment incorrect */
	}

	if (fildes == ddb_fildes){
		if ((cur_address + nbyte)  > physmem * NBPG){
			fprintf(stdout, " Read out of range, abort to avoid I/O hang\n");
			errno = EINVAL;
			return(-1);
		}
		retval = GpioRead(fildes, cur_address, buf, nbyte);
		cur_address += nbyte;
	} else {
		retval = read(fildes, buf, nbyte);
	}
	return(retval);
}

int
dowrite(fildes, buf, nbyte)
int fildes;
char *buf;
int nbyte;
{
	int retval;
	int shortMsk;		/* Mask for short integer size */
	int intMsk;		/* Mask for integer size */

	shortMsk = sizeof(short) - 1;
				/* WORKs ONLY IF SHORT SIZE IS POWER OF 2 */
	intMsk = 3;		/* Integer size on Indigo is 4 bytes */

	if ((((int)buf & shortMsk) != 0) ||
		((nbyte % (intMsk + 1)) != 0) ||
		((int)nbyte == -1)){
		fprintf(stderr," alignment incorrect &buf 0x%x contact -joh\n",buf);
		return(-1);	/* alignment incorrect */
	}


	if (fildes == ddb_fildes){
		if ((cur_address + nbyte)  > physmem * NBPG){
			fprintf(stdout, " Write out of range, abort to avoid I/O hang\n");
			errno = EINVAL;
			return(-1);
		}
		retval = GpioWrite(fildes, cur_address, buf, nbyte);
		cur_address += nbyte;
	} else {
		retval = write(fildes, buf, nbyte);
	}
	return(retval);
}


int
dolseek(fildes, offset, whence)
int fildes;
long offset;
int whence;
{
	if (fildes == ddb_fildes){
		if ((whence != 0) && (whence != 1)){
			fprintf(stderr,"Only whence of 0 or 1 allowed on /dev/ddb_link, whence was %d\n",whence);
			errno = EINVAL;
			return(-1);
		}
		if (whence == 0){
			if (offset < 0){
				errno = EINVAL;
				return(-1);
			}
			cur_address = offset;
		} else {
			if ((cur_address + offset) < 0){
				errno = EINVAL;
				return(-1);
			}
			cur_address += offset;
		}
		return(cur_address);
	} else {
		return(lseek(fildes, offset, whence));
	}
}


#endif
