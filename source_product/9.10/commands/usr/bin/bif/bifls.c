static char *HPUX_ID = "@(#) $Revision: 64.1 $";
#include <pwd.h>
#include <grp.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef NLS || NLS16
#include <locale.h>
#endif NLS || NLS16

typedef int boolean;
#define false (0)
#define true (1)
#define eq(a,b) (strcmp((a),(b))==0)
#define flip(x) (x) = !(x)

char *ctime(), *strcat(), *strcpy(), *strncpy(), *malloc(), *strrchr();

char *pname;
boolean debug		= false;
boolean list_inumber	= false;
boolean long_listing	= false;
boolean directory_only	= false;
boolean use_passwd_file	= true;
boolean list_all	= false;
boolean list_dots	= false;
boolean flag_type	= false;
boolean outline		= false;

main(argc, argv)
char **argv;
{

#ifdef NLS || NLS16			/* initialize to the correct locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("bifls"), stderr);
		putenv("LANG=");
	}
#endif NLS || NLS16

	pname = *argv++;

	if (pname[strlen(pname)-1] == 'l')
		long_listing = true;

	if (getuid()==0) 			/* if super-user */
		list_all = true;		/* list everything */


	while (*argv!=NULL && **argv=='-') {
		register char *s;

		for (s = *argv++ + 1; *s; s++) {
			switch (*s) {
			case 'A': flip(list_all);		argc--; break;
			case 'a': list_all = list_dots = true;	argc--; break;
			case 'd': flip(directory_only);		argc--; break;
			case 'F': flip(flag_type);		argc--; break;
			case 'i': flip(list_inumber);		argc--; break;
			case 'l': flip(long_listing);		argc--; break;
			case 'p': flip(use_passwd_file);	argc--; break;
			case 'x': flip(debug);			argc--; break;
			default: goto usage;
			}
		}
	}

	if (*argv==NULL) {
usage:		fprintf(stderr, "usage: %s -AadFilp device:file ...\n",
				pname);
		exit(1);
	}

	if(argc > 2)	outline = true;		/* if more than one directory */
						/* to be listed then give     */
						/* directory name before each */

	for ( ; *argv!=NULL; argv++) {
		process(*argv);
		flushit();
	}

	exit(0);
}

char * biflspath(path)
char *path;	/* return past a /dev/<hd>: else return original path*/
{
	char *ppath;	/* temp */
	register int len;
	register boolean okbif = false;

/*	path = normalize(path); */
	ppath = path; 		/* save away in case no colon found */
	len = strlen(path);
	while(*path != ':' && *path != '\0')
	{
		if(path - ppath < len)
			if(*path == '/' && *(path+1) == 'd' && *(path+2) == 'e' && *(path+3) == 'v' && *(path+4) == '/')  okbif = true;
		path++;
	}
	if(*path == ':' && okbif == true)	/* then ':' found -- else restore path */
	{
		path++;		/* skip over the colon */
		if(*path != '/')	/* if nothing beyond root '/' then supply one */
		{
			path--;		/* overwrite the ':' */
			*path = '/';
		}
	}
	else
		path = ppath;

	return(path);
}

char * biflstail(path)
char *path;	/* return last component of path name */
{
	register int len,i,j;
	char *ppath;

	path = biflspath(path);
	if((len = strlen(path)) > 0)
	{
		/* find last '/' */

		for(i=len - 1; i>=0; i--)
		{
			if( *(path+i) == '/')	/* found it */
			{
				return(path+i+1);	
			}
		}
	}
	return(path);
}

extern char * normalize();

process(path)
char *path;
{
	struct stat statbuf;
	register int dp;
	register char * ppath;

	path = normalize(path);
	dp = bfsopen(path, 0);				/* open file/dir */
	if (dp == -1) {
		fprintf(stderr, "%s: can't open %s\n", pname, path);
		return;
	}

	bfsfstat(dp, &statbuf);
	if ((statbuf.st_mode & 0170000) != 0040000 || directory_only) {
		bfsclose(dp);
		if(outline) fprintf(stderr,"\n");
		enqueue(path);
	} else {
		char buf[256], new_name[15];
		struct dir {
			unsigned short dir_ino;
			char dir_name[14];
		} dir;

		if(outline)
		{
			fprintf(stderr,"\n%s:\n",path);
		}
		while (bfsread(dp, (char *) &dir, sizeof(dir))==sizeof(dir)) {
			if (dir.dir_ino==0)		/* deleted entry */
				continue;

			if (eq(dir.dir_name,".") || eq(dir.dir_name,"..")) {
				if (!list_dots)
					continue;
			} else if (dir.dir_name[0]=='.' && !list_all)
				continue;

			strncpy(new_name, dir.dir_name, 14);
			new_name[14] = '\0';		/* seal off long name */
			strcpy(buf, path);		/* directory name     */
			if (buf[strlen(buf)-1]!=':')	/* if native file     */
				strcat(buf, "/");	/* separate with /    */
			strcat(buf, new_name);		/* complete the name  */
			enqueue(buf);
		}
		bfsclose(dp);
	}
}


list_file(path)
char *path;
{
	register int ret, mode;
	register char *ppath;
	boolean pr_majmin;
	long now, time(), time_limit;
	char *t;
	struct stat statbuf;
	struct group *getgrgid(), *gptr;
	struct passwd *getpwuid(), *uptr;

	if (!long_listing && !flag_type && !list_inumber) {
		if(directory_only)
			path = biflspath(path);
		else
			path = biflstail(path);
		puts(path);
		return;
	}

	ret = bfsstat(path, &statbuf);
	if (ret == -1) {
		fprintf(stderr, "%s: can't stat %s\n", pname, path);
		return;
	}

	if (list_inumber)
		printf("%5d ", statbuf.st_ino);

	/* strip off the /dev/fd: portion of path if it exists */
	if(directory_only)
		path = biflspath(path);
	else
		path = biflstail(path);
	if (!long_listing) {
		printf("%s",path);
		print_flag_char(statbuf.st_mode);
		puts("");
		return;
	}

	mode = statbuf.st_mode;

	pr_majmin = false;
	switch(mode & 0170000) {
	case 0010000: putchar('p'); break;			/* pipe */
	case 0020000: putchar('c'); pr_majmin=true; break;	/* character */
	case 0040000: putchar('d'); break;			/* directory */
	case 0060000: putchar('b'); pr_majmin=true; break;	/* block */
	case 0110000: putchar('n'); pr_majmin=true; break;	/* network */
	case 0150000: putchar('s'); pr_majmin=true; break;	/* srm */
	case 0000000:						/* regular? */
	case 0100000: putchar('-'); break;			/* regular */
	default:      putchar('?'); break;			/* wierd */
	}


	putchar(mode & 0400 ? 'r' : '-');
	putchar(mode & 0200 ? 'w' : '-');
	putchar (mode & 04000				/* if set-user-id */
			? mode & 0100 ? 's' : 'S'	/* s if execute set */
			: mode & 0100 ? 'x' : '-');	/* x if execute set */
	putchar(mode & 0040 ? 'r' : '-');
	putchar(mode & 0020 ? 'w' : '-');
	putchar (mode & 02000				/* if set-group-id */
			? mode & 0010 ? 's' : 'S'	/* s if execute set */
			: mode & 0010 ? 'x' : '-');	/* x if execute set */
	putchar(mode & 0004 ? 'r' : '-');
	putchar(mode & 0002 ? 'w' : '-');
	putchar (mode & 01000				/* if sticky */
			? mode & 0001 ? 't' : 'T'	/* t if execute set */
			: mode & 0001 ? 'x' : '-');	/* x if execute set */
/*
drwxr-xr-x  25 root     other       1056 May  2 10:37 /
*/
	printf(" %3d", statbuf.st_nlink);	/* number of links */

	if (use_passwd_file && (uptr=getpwuid(statbuf.st_uid))!=NULL)
		printf(" %-8s", uptr->pw_name);	/* symbolic userid */
	else
		printf(" %-8d", statbuf.st_uid);	/* numeric userid */

	if (use_passwd_file && (gptr=getgrgid(statbuf.st_gid))!=NULL)
		printf(" %-8s", gptr->gr_name);	/* symbolic groupid */
	else
		printf(" %-8d", statbuf.st_gid);	/* numeric groupid */

	if (pr_majmin)
		printf(" %3d %3d",
			(statbuf.st_dev>>8) & 0xff, statbuf.st_dev & 0xff);
	else
		printf(" %7d", statbuf.st_size);	/* file size */
	now = time((long *) 0);
	time_limit = now - 6L*30L*24L*60L*60L; 		/* 6 months ago */
	t = ctime(&statbuf.st_mtime);
	if (statbuf.st_mtime > time_limit)
		printf(" %.6s %.5s ", t+4, t+11);	/* recent, no year */
	else
		printf(" %.6s  %.4s ", t+4, t+20);	/* ancient, add year */

	printf("%s", path);				/* the name itself */
	print_flag_char(statbuf.st_mode);
	puts("");
}

print_flag_char(mode)
register int mode;
{
	if (!flag_type)
		return;

	if ((mode & 0170000) == 0040000)	/* directory? */
		putchar('/');
	else if (mode & 0111)			/* executable? */
		putchar('*');
	else					/* boring */
		putchar(' ');
}


#define BUFSIZE 200
char *ptrbuf[BUFSIZE];
int ptrcount=0;

enqueue(path)
register char *path;
{
	static boolean message_printed=false;
	char *p;

	if (ptrcount>=BUFSIZE) {
		if (!message_printed) {
			fprintf(stderr, "%s: too many files to sort\n",
				pname);
			message_printed = true;
		}
		flushit();
	}

	p = malloc((unsigned) (strlen(path)+1));
	if (p==NULL) {
		fprintf("%s: can't allocate memory\n");
		list_file(path);
	}
	strcpy(p, path);
	ptrbuf[ptrcount++] = p;
}

compare(a,b)
char **a, **b;
{
	return strcoll(*a,*b);			       /* compare the names */
}


flushit()
{
	char **p;

	qsort((char *) ptrbuf, ptrcount, sizeof(ptrbuf[0]), compare);
	for (p=ptrbuf; ptrcount>0; ptrcount--, p++) {
		list_file(*p);
		free(*p);
	}
}

