/* @(#) $Revision: 64.1 $ */    

#ifndef _MSGCAT_INCLUDED /* allow multiple inclusions */
#define _MSGCAT_INCLUDED

#include <limits.h>

#define CATID		"msgcat01"	/* Catalog file ID. */
#define	NUM_MSG_POS	8	/* Byte pos of number of messages */
#define	DIR_START_POS	(NUM_MSG_POS+sizeof(long))    /* Byte pos of directory starts */
#define	OVERHEAD	DIR_START_POS	/* File overhead size */
#define IDLEN		NUM_MSG_POS	/* Catalog file ID. length */
#define SET		"$set"		/* Set command */
#define SETLEN		4		/* Length of "$set" command */
#define DELSET		"$delset"	/* Delete set command */
#define	DELSETLEN	7		/* Length of DELSET command */
#define QUOTE		"$quote"	/* Quote character command */
#define	QUOTELEN	6		/* Length of QUOTE command */
#ifndef L_SET
#define	L_SET		0	/* Whence of lseek, set to the address */
#endif
#define	ERROR		-1	/* Functional return to indicate error */
#define	TRUE		1	/* Use as logical value */
#define	FALSE		0	/* Use as logical value */
#define	BUFLEN		(100*DIRSIZE)/* The length of readbuf */
				/* This sould be multiple of DIRSIZE */
#define	DIRSIZE		(sizeof(struct dir))/* The size of dir */
#define MAX_SETNUM	NL_SETMAX           /* Maximum set number */
#define MAX_MSGNUM	NL_MSGMAX           /* Maximum message number */
#define MAX_MSGLEN	NL_TEXTMAX          /* Maximum message length */
#define MAX_BUFLEN	32767          /* Maximum buffer length */
#define MODE		0664	/* File mode of new catalog file */
#define	E_NO_SET	1001	/* Error - No specified set */
#define	E_NO_MSG	1002	/* Error - No specified message */
#define	E_ILLEGAL_SET	1003	/* Error - Illegal set number */
#define	E_ILLEGAL_MSG	1002	/* Error - Illegal message number */

struct dir {			/* Directory structure */
	short	setnum;		/* Set number */
	unsigned short	msgnum;		/* Message number */
	long	addr;		/* Message pointer */
	short	length;		/* The length of the message */
	short	reserved;	/* Padded with 2 bytes */
};
typedef	struct dir	DIR;

#endif /* _MSGCAT_INCLUDED */
