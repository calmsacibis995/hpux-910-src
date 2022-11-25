/* HPUX_ID: @(#) $Revision: 70.1 $  */
/*
 *-----------------------------------------------------------------------------
 *
 *      Title ................. : ftio_mesg.c 
 *      Purpose ............... : Ftio message system. 
 *      Author ................ : David Williams. 
 *
 *      Description:
 *
 *      
 *
 *      Contents:
 *
 *              ftio_msg()
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <errno.h>	
#include "ftio_mesg.h"	

/*
 *      Defines for use with action field below 
 *
 *	BIT		DEFINE
 *	---		------
 *	0-1		terminal output mode
 *	2		perror flag
 *	3-4		action
 */
#define TERM    0x3	/* 00 0011 */
#define STDE    0x0
#define STDO    0x1
#define STDTTY  0x2

#define PERR    0x04	/* 00 0100 */

#define	ACTION	0x18	/* 01 1000 */
#define	A_NONE	0x0	/* x0 0xxx * default */
#define	A_WRET	0x8	/* x0 1xxx */
#define	A_WY_N	0x10	/* x1 0xxx */
#define	A_WSTR	0x18	/* x1 1xxx */

#define USAGE	0x20  	/* 10 0000 */

struct  f_msg
{
	short   no;             /* message number */
	char    *s;             /* the string */
	short   action;         /* see below */
}  msg[] =
{
FM_SYSFL,       "-S %s invocation failed",      STDE | PERR,
FM_TAPEN,       "please mount tape %d, then hit return", STDTTY | A_WRET,
FM_NOPEN,	"could not open %s",                STDE | PERR,
FM_CONT,        "continuing...\n",                STDTTY,
FM_IOCTL,       "ioctl(2) call failed",         STDE | PERR,
FM_BADT,        "a write error has occured, restart? ", STDTTY | A_WY_N,
FM_BADTNOL,     "a write error has occured, data has been lost, continue? ",
	STDTTY | A_WY_N,
FM_UNEX,        "Unexpected value encountered in %s\n", STDE,
FM_NDX,         "need -x option, %s not copied\n", STDE,
FM_NREAD,       "error reading %s",             STDE | PERR,
FM_WSIZE,       "warning: size of %s does not match stat info\n", STDE,
FM_NMODT,       "error setting access/modification time for %s", STDE | PERR,
FM_NSTAT,       "could not stat %s",            STDE | PERR,
FM_NWRIT,       "error writing to %s",          STDE | PERR,
FM_WPROT,       "device is not write enabled\n",  STDE,
FM_NREOF,       "error reading End of File mark\n", STDE,
FM_RTHDR,	"error reading tape header",    STDE | PERR,
FM_WTHDR,	"error writing tape header",    STDE | PERR,
FM_NSEEK,	"error lseek(2)ing in %s",      STDE | PERR,
FM_UNSPX,	"unsupported file type %s\n",     STDE,
FM_NCHMD,	"failed chmod of %s",           STDE | PERR,
FM_NCHWN,	"failed chown of %s",           STDE | PERR,
FM_RSYNC,       "Out of phase. Resync? ",       STDTTY | A_WY_N,
FM_NMALL,	"allocation of memory failed",  STDE | PERR,
FM_NEW,		"%s exists and is newer - not restored\n",      STDE,
FM_NREST,	"not restored.\n",                STDE,
FM_SHMALLOC,	"shared memory request failed",	STDE | PERR,
FM_NSEMOP,	"semaphore request failed",	STDE | PERR,
FM_WRGMEDIA,    "media is out of order, continue? ", STDTTY | A_WY_N,
FM_OTAPE, "media has an archive created by another user, continue? ",
	STDTTY | A_WY_N,
FM_WRGBCKUP,"media is not from this backup set, continue? ", STDTTY | A_WY_N,
FM_TAPEUSE,     "warning: this media has been used >100 times, continue? ",
	STDTTY | A_NONE,
FM_WRGBLKSIZ,"incorrect block_size specified at invocation - fixing\n", STDE,
FM_WRGHTYPE,"incorrect header type specified at invocation - fixing\n", STDE,
FM_INVOPT,	"invalid option: %c\n",	USAGE | STDE,
FM_ARGC,	"not enough arguments\n",	USAGE | STDE,
FM_TOOBIG,	"%s is too big!\n",		STDE,
FM_NOTIMP,      "option not implemented yet: %c\n",USAGE | STDE,
FM_REQSU,	"option %c requires super user capability\n",USAGE | STDE,
FM_FLIST,	"creating filelist - please wait\n",	STDTTY,
FM_NALIAS,	"no alias file found - using defaults\n",	STDE,
FM_NHOME,	"could not find home directory",	STDE | PERR,
FM_NKILL,	"could not kill child process",		STDE | PERR,
FM_NLINK,	"could not link %s to %s",		STDE | PERR,
FM_LINK,	"%s linked to %s\n",		STDE,
FM_NGETT,	"could not get time of day",	STDE | PERR,
FM_NCHDIR,	"could not change directory to %s",	STDE | PERR,
FM_NRSEM,	"failed to remove semaphore, id %d", STDE | PERR,
FM_NRSHM,	"failed to remove shared memory segment, id %d", STDE | PERR,
FM_NCREAT,	"could not create %s",		STDE | PERR,
FM_NPREALLOC,	"could not prealloc %s",	STDE | PERR,
FM_ALIEN,	"could not mknod %s : alien file\n", STDE,
FM_NUNLINK,	"could not unlink %s",		STDE | PERR,
FM_NUNAME,	"uname request failed",		STDE | PERR,
FM_NMKNOD,	"could not mknod %s",		STDE | PERR,
FM_NMKDIR,	"could not make directory %s", 	STDE | PERR,
FM_NDD,		"need -d option\n",		STDE,
FM_NDIRF,	"could make directory %s : it exists as a file\n", STDE,
FM_NWAIT,	"wait on child process failed",	STDE | PERR,
FM_NCWD,	"unable to find current working directory", STDE | PERR,
FM_NFORK,	"fork failed",			STDE | PERR,
FM_NLINK32,	"warning: inode number for %s is too big to archive,\n\
file will be copied not linked when restored.\n", STDE,
FM_NLINK32R,	"warning: inode for file %s was too big to archive,\n\
file will be copied not linked\n", STDE,
FM_BIGNAME,	"%s has too many characters in it, %d max please.\n", STDE,
FM_HDRMAGIC,	"error reading tape header : magic number is invalid\n", STDE,
FM_HDRCHECK,	"error reading tape header : check sum is invalid\n", STDE,
FM_QUESTION,	"%s", 				STDTTY | A_WY_N,
FM_SIGCLD,	"unexpected death of child process - exiting\n", STDE,
FM_RDLINK,      "could not readlink(2) %s",         STDE | PERR,
FM_SYMLINK,     "could not symlink(2) %s to %s",    STDE | PERR,
FM_SYMMB,       "warning: can't do lseek on symbolic link %s on media boundary", STDE,
FM_INVARG,      "invalid argument: %s (use \"hfs\" or \"nfs\")\n", USAGE | STDE,

FM_LOSTC,	"lost connection to remote host\n",	STDE,
0,    "ftio_mesg: unknown message no: %d\n", STDE /* must be last */
};

static	char	key_buf[20];
static	int	redirect = 0;


/*VARARGS 1*/
ftio_mesg(msg_number, s, t)
int     msg_number;
int	s, t;
{
	FILE	*ip,
		*op,
		*fopen();
	int	i;
	int	retval;
	int     errno_save;
	extern	errno;
	extern	Tty;

	errno_save = errno;

	/*
	 *	Search for the message.
	 */
	for (i = 0; msg[i].no != msg_number; i++)
	{
		if (!msg[i].no)
		{
			fprintf(stderr, msg[i].s, msg_number);
			return 1;
		}
	}

	/*
	 *	Set up Output pointers.
	 */
	switch	(msg[i].action & TERM)
	{
	case STDTTY:
		if (redirect)
		{
			op = stderr;
			retval = 0;
			break;
		}

		if ((op = fopen(Tty, "w")) == NULL)
		{
			(void)fprintf(stderr, "ftio: could not open %s", Tty);
			(void)perror(" for output");
			(void)fprintf(
				stderr, 
				"ftio: redirecting output to standard error\n"
			);
			redirect++;
			op = stderr;
			retval = 0;
		}
		break;

	case STDO:
		op = stdout;
		break;

	case STDE:
	default:
		retval = 1;
		op = stderr;
		break;
	}

	/*
	 *	If terminal interaction is required...
	 *	Attempt to open the terminal, if that fails
	 *	get out with an error to stderr.
	 */
	if (msg[i].action & ACTION)
	{
		if ((ip = fopen(Tty, "r")) == NULL)
		{
			(void)fprintf(stderr, "ftio: could not open %s", Tty);
			(void)perror(" for input");
			(void)myexit(1);
		}
	}

	/*
	 *	Output message..
	 */
	(void)fputs("ftio: ", op);
	(void)fprintf(op, msg[i].s, s, t);

	/*
	 *      If perror() output was requested.
	 */
	errno = errno_save;
	if (msg[i].action & PERR)
		(void)perror(" ");

	/*
	 *	If we are talking to the terminal, flush the output buffer.
	 */
	if ((msg[i].action & TERM) == STDTTY)
	{
		(void)fflush(op);
		if (op != stderr)
			(void)fclose(op);
	}
	
	/*
	 *	If terminal interaction is required...
	 *	Read the terminal. If we got to here we 
	 *	are able to read the terminal.
	 */
	if (msg[i].action & ACTION)
	{
		/*
		 *	If a response is required..
		 */
		switch (msg[i].action & ACTION)
		{
		case A_WRET:
				(void)get_string(ip);
				retval = 1;
				break;

		case A_WY_N:
				retval = get_yesno(ip);
				break;

		case A_WSTR:
				(void)get_string(ip);
				retval = (int)key_buf;
				break;

		default:
				retval = 1;
				break;
		}

		(void)fclose(ip);
	}
	
	/*
	 *	If Usage: then print the message.
	 */
	if (msg[i].action & USAGE) 
	{
		(void)usage();
		retval = 1;
	}
	
	return retval;
}

get_string(fp)
FILE	*fp;
{
	fgets(key_buf, sizeof(key_buf), fp);
}

get_yesno(fp)
FILE	*fp;
{
	int	retval = -1;
	
	/*
	 *	Loop until we get a valid response.
	 */
	while(retval == -1)
	{
		(void)get_string(fp);

		switch(*key_buf)
		{
		case 'Y':
		case 'y':
		case 'J':
		case 'j':
			retval = 1;
			break;

		case 'N':
		case 'n':
			retval = 0;
			break;

		default:
			fprintf(stderr, "invalid response - try again? ");
			break;	
		}
	}

	return retval;
}

usage()
{
	fprintf(stderr, "\
Usage:\n\
ftio -o|O[achpvxAEHLM] [-B blksize] [-D type] [-K comment] [-L filelist]\n\
\t[-N datefile] [-S script] [-T tty] [-Z nobufs] tapedev [pathnames]\n\
\t[-F ignorenames]\n\
ftio -i|I[cdfmptuvxAEMPR] [-Z nobufs] [-B blksize] [-S script] [-T tty]\n\
\ttapedev [patterns]\n\
ftio -g[v] tapedev [patterns]\n\
");
}
