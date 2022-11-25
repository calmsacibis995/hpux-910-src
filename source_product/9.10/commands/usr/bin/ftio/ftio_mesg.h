/* HPUX_ID: @(#) $Revision: 64.1 $  */

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : ftio_mesg.h 
 *	Purpose ............... : Defines ftio message constants. 
 *	Author ................ : David Williams. 
 *	Data Files Accessed ... : 
 *	Trace Flags ........... : 
 *
 *	Description:
 *	
 *
 *	Contents:
 *
 *		<no name>()
 *
 *-----------------------------------------------------------------------------
 */

#define	FM_TAPEN	1	/* please mount tape %d (tapeno) */
#define	FM_NOPEN	2	/* can't open %s, perror() (file_name) */
#define	FM_CONT		3	/* continuing... */
#define	FM_IOCTL	4	/* ioctl(2) call failed, perror() */
#define	FM_LASTT	5	/* found end of last tape, finishing up */
#define	FM_BADT		6	/* This tape is bad, entering restart mode */
#define	FM_UNEX		7	/* Unexpected value encountered in %s */
#define	FM_NDX		8	/* need -x option */
#define	FM_NREAD	9	/* error reading %s, perror() */
#define	FM_WSIZE	10	/*size of file does not match Stat into for %s*/
#define	FM_NMODT	11	/* error setting access/mod time for %s, p()*/
#define	FM_NSTAT	12	/* could not stat file %s, perror() */
#define	FM_NWRIT	13	/* error writing %s, perror() */
#define	FM_WPROT	14	/* no write protect ring on %s */
#define	FM_NREOF	15	/* error reading EOF mark */
#define	FM_RTHDR	16	/* error reading tape header */
#define	FM_WTHDR	17	/* error writing tape header */
#define	FM_NSEEK	18	/* error lseek(2)ing in file %s, perror() */
#define	FM_UNSPX	19	/* unsupported file type %s */
#define	FM_NCHMD	20	/* failed chmod(2) for %s */
#define	FM_NCHWN	21	/* failed chown(2) for %s */
#define	FM_RSYNC	22	/* Out of phase. Resync? [y] */
#define	FM_NMALL	23	/* call to malloc(2) failed */
#define	FM_NEW  	24	/* %s exists and is newer */
#define	FM_NREST	25	/* not restored */
#define	FM_SYSFL	26	/* script invocation failed, perror() */
#define	FM_USAGE	27	/* Usage: ftio -icdv /dev  .. etc */
#define	FM_SHMALLOC	28	/* shared memory request failed */
#define	FM_NSEMOP	29	/* semaphore request failed */
#define	FM_WRGMEDIA	30	/* wrong media mounted, continue? */
#define	FM_OTAPE	31	/*this tape created by another user,write over*/
#define	FM_WRGBCKUP	32	/* wrong backup set, continue? */
#define	FM_TAPEUSE	33	/* Warning: this tape has been used >100 times*/
#define	FM_WRGBLKSIZ	34	/* Wr blocksize specified at invocation, fix? */
#define	FM_WRGHTYPE	35	/* Wr headertype specified at invoaction,fix?*/
#define	FM_ARGC		36	/* Not enough arguments */
#define	FM_INVOPT	37	/* invalid option */
#define	FM_TOOBIG	38	/* is too big */
#define	FM_NOTIMP	39	/* option not implemted yet */
#define	FM_REQSU	40	/* requires su cap' */
#define	FM_BADTNOL	41	/* This tape is bad, data has been lost, cont?*/
#define	FM_FLIST	42	/* creating filelist - please wait. */
#define	FM_NALIAS	43	/* No alias file found - using defaults */
#define	FM_NHOME	44	/* Could not find home directory */
#define	FM_NKILL	45	/* Could not kill child process */
#define	FM_NLINK	46	/* Can't link %s to %s */
#define	FM_LINK		47	/* linked %s to %s */
#define	FM_NGETT	48	/* Could not get time of day */
#define	FM_NCHDIR	49	/* can't change directory to %s */
#define	FM_NRSEM	50	/* failed to remove semaphore id %d */
#define	FM_NRSHM	51	/* failed to remove shared memory seg, id %d */
#define	FM_NCREAT	52	/* could not create %s */
#define	FM_NPREALLOC	53	/* could not prealloc %s */
#define	FM_ALIEN	54	/* %s is an alien file - could not mknod */
#define	FM_NUNLINK	55	/* could not unlink %s */
#define	FM_NUNAME	56	/* uname request failed */
#define	FM_NMKNOD	57	/* could not mknod %s */
#define	FM_NMKDIR	59	/* could not make directory %s */
#define	FM_NDD		60	/* need -d option */
#define	FM_NDIRF	61	/*can't make directory %s, it exists as a file*/
#define	FM_NWAIT	62	/* wait on child process failed */
#define	FM_NCWD		63	/* unable to find current working directory */
#define	FM_NFORK	64	/* fork failed */
#define	FM_NLINK32	65	/* inode # too big, will be copied not linked */
#define	FM_NLINK32R	66	/* inode # was too big, copied not linked */
#define	FM_BIGNAME	67	/* %s has too many characters in it, %d max */	
#define	FM_HDRMAGIC	68	/* Err reading tape hdr: magic number invalid */
#define	FM_HDRCHECK	69	/* Err reading tape hdr: check sum invalid */
#define	FM_QUESTION	70	/* "%s", then prompt for response */
#define FM_SIGCLD       71      /* unexpected death of child - exiting */
#define FM_RDLINK       72      /* readlink(2) failed */
#define FM_SYMLINK      73      /* symlink(2) failed */
#define FM_SYMMB        74      /* can't lseek on symbolic link on media boundary */
#define FM_INVARG       75      /* invalid argument to -D option (use "hfs" or "nfs") */
#define FM_LOSTC	76	/* lost connection to remote host */
