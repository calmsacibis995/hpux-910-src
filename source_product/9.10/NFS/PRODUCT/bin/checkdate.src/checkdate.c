/*	   @(#)1.1	87/02/02	*/
/* HPUX_ID: @(#)checkdate.c	1.2		87/01/07 */

/* Buzz word:
	Psudeo SCCS file:  A file whose basename is leaded by "s.".  The
			content of the file is "%%  machine  pathname".
			Example: %%  hpfcls /source/sys/sccs/h/s.param.h
			Note: the first two characters of the files have
				to be "%%".

	Pseudo RCS file:   Same thing, only the name looks like an RCS file.
	Target file:  The psudeo file has the path to the file we are
			really interested.

    Function:
	Checkdate will traverse all the directories/files in the arguments list.
        If the file is a Psudeo SCCS file, use the contents of the file
to get the "last modefication time" of the target file.  If the target
file is newer than the psudeo file, touch the psudeo file so that the
"last modification time" is same as the target file.
*/
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ftw.h>
#include <string.h>
int netconnect();
int fn();
extern	int	errno;
int	debuglevel;

main(argc, argv)
register int	argc;
register char	**argv;
{
	extern char *optarg;
	extern int optind, opterr;
	int c, errflg;

	c = errflg = 0;

	while ((c = getopt(argc, argv, "r:")) != EOF) {
		switch(c) {
		case 'r':
			debuglevel = atoi(optarg);
			break;
		case '?':
			errflg++;
		}
	}
	argc -= optind;
	if (argc < 1 || errflg) {
		fprintf(stderr,"Usage: checkdate  directory [directories...]\n");
		fprintf(stderr, "argc = %d, errflg = %d\n", argc, errflg);
		exit(1);
	}

/*It is said: a good C program doesn't need comments.*/
	for (argv+=optind; argc; argc--, argv++) {
		ftw(*argv, fn, 10); 
	}
}

fn(pathname, status, flag)
char	*pathname;
struct stat *status;
int	flag;
{
	register char *cp1, *cp2;
	register int	i, j;
	char	linebuf[512];
	char	magic[3];
	char	machine[25];
	char	rmotpath[215];
	char	rmotmach[sizeof(machine) + 5]; /* 5 is "/net/" */
	char	rpath[sizeof(rmotpath) + sizeof (rmotmach)];
	FILE	*fp;
	struct	stat	rfstat, lfstat;
	struct	utimbuf	times;
	
	switch(flag) {
	default:
		fprintf(stderr, "checkdate: Unknown flag passed by ftw()\n");
		break;
	case FTW_D:
		break;
	case FTW_DNR:
		fprintf(stderr,"checkdate: Can't read directory %s\n", pathname);
		break;
	case FTW_NS:
		fprintf(stderr, "Checkdate: Can't stat file %s\n", pathname);
		break;;
	case FTW_F:
		/*Check to see if its basename is s.* or *,v */
		if ((cp1 = strrchr(pathname, '/')) == NULL) cp1 = pathname;
		else cp1++;;
		if(strncmp(cp1, "s.", 2)) {
			int len = strlen(cp1);
			if ( (cp1[len-2] != ',') || (cp1[len-1] != 'v' ) )
				break;	/* do nothing */
		}

		if ((fp = fopen(pathname, "r")) == NULL) {
			fprintf(stderr, "checkdate: Can't open %s, errno = 0d%d\n", pathname, errno);
			break;
		}
		if (fstat(fileno(fp), &lfstat) == EOF) {
			fprintf(stderr, "checkdate: Can't stat %s, errno = 0d%d\n", pathname, errno);
			break;
		}
		if(fread(linebuf, 1, sizeof(linebuf), fp) < 2) {
			fclose(fp);
			break;
		}
		fclose(fp);

/* if the file is not pseudo sccs, do nothing */
		if(sscanf(linebuf, "%%%% %s %s", machine, rmotpath) != 2) break;

/*construct the pathname of network special file*/
		strcpy (rmotmach, "/net/");
		strcat (rmotmach, machine);
		strcpy (rpath, rmotmach);
		strcat (rpath, rmotpath);
		if (debuglevel >=1) {
			printf("remote machine is %s\nremote file is %s\n", rmotmach, rpath);
		}
		if (stat(rpath, &rfstat) == EOF) {
			if((i = netconnect(rmotmach)) == NULL) break;
			if(i == EOF) { /*fatal error*/
				return(EOF);
			}
			if (stat(rpath, &rfstat) == EOF) {
				fprintf(stderr, "checkdate: Can't stat %s, errno = 0d%d\n",rpath, errno);
				break;
			}
		}
		if (rfstat.st_mtime > lfstat.st_mtime) {
			times.actime = rfstat.st_atime;
			times.modtime = rfstat.st_mtime;
			if(utime(pathname, &times) == NULL) {
				printf("checkdate: touch %s\n", pathname);
			}
			else {
				fprintf(stderr, "checkdate: Fail to touch %s. errno = 0d%d\n",
					pathname, errno);
			}
		}
		break;
	}
	return(0);
}
