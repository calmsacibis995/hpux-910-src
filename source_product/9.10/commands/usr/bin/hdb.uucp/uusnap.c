/*	@(#) $Revision: 70.2 $	*/
static char *RCS_ID="@(#)$Revision: 70.2 $ $Date: 91/11/07 10:49:20 $";
#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	
#include <nl_types.h>
#endif NLS

/* uusnap - HoneyDanBer version */
/* This was HP-UX Revision: 27.6  */   
/*
 *	Uusnap - displays a snapshot of the uucp system.
 *					RJKing WECo-MG6565 May 83
 */
#include "uucp.h"

#ifndef	SYSBUF
char SYSBUF[BUFSIZ];
#endif

#define	NSYSTEM	100				/* max # of systems queued */
#define	SPOOLDR	"/usr/spool/uucp/.Status"	/* Where STST files are */
#define LOCKDIR "/usr/spool/uucp"
#ifdef	UUDIR
#define	CMDSDIR	"/usr/spool/uucp/C."		/* Name of commands dir */
#define	DATADIR	"/usr/spool/uucp/D."		/* Name of data directory */
#define	XEQTDIR	"/usr/spool/uucp/X."		/* Name of execute dir */
#else
#define	CMDSDIR	"/usr/spool/uucp"		/* Name of commands dir */
#define	DATADIR	"/usr/spool/uucp"		/* Name of data directory */
#define	XEQTDIR	"/usr/spool/uucp"		/* Name of execute dir */
#endif

#define	CMDSLEN	5				/* Length of trailer */
#define	DATALEN	8				/* Length of trailer */
/* rti!trt: XEQTLEN was 0, for reverse search, but that did not work. */
#define	XEQTLEN	5				/* Length of trailer */
#define	NUMCTRS	3				/* # file types to count */
#define	CMDTYPE	0				/* Index into scnt.cntr */
#define	DATTYPE	1				/* Index into scnt.cntr */
#define	XEQTYPE	2				/* Index into scnt.cntr */

#ifdef HDBuucp
void	Scandir(), GetLck();
#endif
void	scandir(), getstst();
extern	char *index(), *rindex(), *strcpy(), *strncpy();;
extern	long atol();

struct	scnt {					/* System count structure */
		char	name[16];		/* Name of system */
		short	cntr[NUMCTRS];		/* Count */
		char	stst[32];		/* STST Message */
		short	locked;			/* If LCK..sys present */
		int	st_type;		/* STST Type */
		int	st_count;		/* STST Count */
		time_t	st_lastime;		/* STST Last time tried */
		time_t	st_retry;		/* STST Secs to retry */
	     };

int	sndx;					/* Number of systems */
struct	scnt	sys[NSYSTEM];			/* Systems queued */

main()
{	register int i, j, nlen = 0;
	time_t	curtime, t;


#ifdef NLS
	nlmsg_fd = catopen("uucp",0);
#endif
	setbuf(stdout, SYSBUF);
#ifdef HDBuucp
	Scandir(CMDSDIR, "C.", CMDSLEN, NULL, CMDTYPE);
	Scandir(DATADIR, "D.", DATALEN, NULL, DATTYPE);
	Scandir(XEQTDIR, "X.", XEQTLEN, 'X', XEQTYPE);
	GetLck(LOCKDIR);
#else
	scandir(CMDSDIR, "C.", CMDSLEN, NULL, CMDTYPE);
	scandir(DATADIR, "D.", DATALEN, NULL, DATTYPE);
	scandir(XEQTDIR, "X.", XEQTLEN, 'X', XEQTYPE);
#endif

	getstst(SPOOLDR);
	time(&curtime);
	for(i=0; i<sndx; ++i)
		if((j = strlen(sys[i].name)) > nlen)
			nlen = j;
	for(i=0; i<sndx; ++i)
	{	printf((catgets(nlmsg_fd,NL_SETN,902, "%-*.*s ")), nlen, nlen, sys[i].name);
		if(sys[i].cntr[CMDTYPE])
			printf((catgets(nlmsg_fd,NL_SETN,903, "%3.d Cmd%s ")), sys[i].cntr[CMDTYPE],
				sys[i].cntr[CMDTYPE]>1?"s":" ");
		else	printf((catgets(nlmsg_fd,NL_SETN,904, "   ---   ")));
		if(sys[i].cntr[DATTYPE])
			printf((catgets(nlmsg_fd,NL_SETN,905, "%3.d Data ")), sys[i].cntr[DATTYPE]);
		else	printf((catgets(nlmsg_fd,NL_SETN,906, "   ---   ")));
		if(sys[i].cntr[XEQTYPE])
			printf((catgets(nlmsg_fd,NL_SETN,907, "%3.d Xqt%s ")), sys[i].cntr[XEQTYPE],
				sys[i].cntr[XEQTYPE]>1?"s":" ");
		else	printf((catgets(nlmsg_fd,NL_SETN,908, "   ---   ")));
#ifndef HDBuucp
		if(*sys[i].stst == NULL)
#else
		if((*sys[i].stst == NULL) || (strncmp(sys[i].stst,"SUCCESSFUL",10) == 0))
#endif
		{       if(sys[i].locked) {
				printf((catgets(nlmsg_fd,NL_SETN,909, "LOCKED   ")));
				printpid(sys[i].name);
				printf("\n");
			}
			else    printf("\n");
			continue;
		}
		printf("%s  ", sys[i].stst);
		if (strncmp(sys[i].stst, (catgets(nlmsg_fd,NL_SETN,910, "TALKING")), 7) == 0)
			printpid(sys[i].name);
		if(sys[i].st_type == SS_INPROGRESS)
		{	printf("\n");
			continue;
		}
		t = (sys[i].st_lastime +sys[i].st_retry) - curtime;
		if(t <= 0)
			printf((catgets(nlmsg_fd,NL_SETN,911, "Retry time reached  ")));
		else
		{	if(t < 60)
				printf((catgets(nlmsg_fd,NL_SETN,912, "Retry time %ld sec%s  ")), (long)(t%60),
					(t%60)!=1? "s": "");
			else	printf((catgets(nlmsg_fd,NL_SETN,913, "Retry time %ld min%s  ")), (long)(t/60),
					(t/60)!=1? "s": "");
		}
		if(sys[i].st_count > 1)
			printf((catgets(nlmsg_fd,NL_SETN,914, "Count: %d\n")), sys[i].st_count);
		else	printf("\n");
	}
#ifdef NLS
	catclose(nlmsg_fd);
#endif
        exit(0);
}
void scandir(dnam, prfx, flen, fchr, type)
char *dnam, *prfx, fchr;
{	register int i, plen;
	char	fnam[MAXNAMLEN+1];
	register struct direct *dentp;
	register DIR *dirp;

	plen = strlen(prfx);
	if(chdir(dnam) < 0)
	{	perror(dnam);
		exit(1);
	}
	if ((dirp = opendir(".")) == NULL)
	{	perror(dnam);
		exit(1);
	}
	while((dentp = readdir(dirp)) != NULL)
	{	if(*dentp->d_name == '.' || dentp->d_ino == 0)
			continue;
		if(strncmp(dentp->d_name, prfx, plen) != SAME) {
#ifdef	UUDIR
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,915, "strange file (%s) in %s\n")),
				dentp->d_name, dnam);
#endif
			continue;
		}
#ifdef HDBuucp  /* HDB */
                if (strcmp(prfx,"D.") == SAME ||
                    strcmp(prfx,"X.") == SAME ||
                    strcmp(prfx,"C.") == SAME ){
                     strcpy(fnam,dnam);
                } 
#else
		strcpy(fnam, &dentp->d_name[plen]);
		i = strlen(fnam);
		if(flen > 0)
			fnam[i -flen] = NULL;
		else
		for(; i>0; --i)
		{	if(fnam[i] == fchr)
			{	fnam[i] = NULL;
				break;
			}
		}
#endif
		for(i=0; i<sndx; ++i)
		{	if(strcmp(fnam, sys[i].name) == SAME)
			{	++sys[i].cntr[type];
				break;
			}
		}
		if(i == sndx)
		{	strcpy(sys[i].name, fnam);
			++sys[i].cntr[type];
			++sndx;
		}
	}
	closedir(dirp);
}
void getstst(sdir)
char *sdir;
{	register int i, csys;
	register char *tp;
	char	fnam[MAXNAMLEN+1], buff[128];
	register struct	direct *dentp;
	register DIR *dirp;
	register FILE *st;

	if(chdir(sdir) < 0)
	{	perror(sdir);
		exit(1);
	}
	if((dirp = opendir(".")) == NULL)
	{	perror(sdir);
		exit(1);
	}
	while((dentp = readdir(dirp)) != NULL)
	{	if(dentp->d_ino == 0)
			continue;
                if(dentp->d_name[0] == '.') continue;
		if(strncmp(dentp->d_name, "LCK..", 5) == SAME)
		{	if(strncmp(&dentp->d_name[5], "tty", 3) == SAME ||
			   strncmp(&dentp->d_name[5], "cua", 3) == SAME ||
			   strncmp(&dentp->d_name[5], "cul", 3) == SAME)
				continue;
			strcpy(fnam, dentp->d_name);
			for(csys=0; csys<sndx; ++csys)
			{	if(strcmp(&fnam[5], sys[csys].name) == SAME)
					break;
			}
			if(csys == sndx)
			{	strcpy(sys[csys].name, &fnam[5]);
				++sndx;
			}
			++sys[csys].locked;
			continue;
		}
#ifdef HDBuucp  /* HDB */
                else
#else
		if(strncmp(dentp->d_name, "STST.", 5) == SAME)
#endif
		{	strcpy(fnam, dentp->d_name);
			for(csys=0; csys<sndx; ++csys)
#ifdef HDBuucp /* HDB */  
			{	if(strcmp(&fnam[0], sys[csys].name) == SAME)
#else
			{	if(strcmp(&fnam[5], sys[csys].name) == SAME)
#endif
					break;
			}
			if(csys == sndx)
#ifdef HDBuucp /* HDB */ 
			{	strcpy(sys[csys].name, &fnam[0]);
#else
			{	strcpy(sys[csys].name, &fnam[5]);
#endif
				++sndx;
			}
			if((st = fopen(fnam, "r")) == NULL)
			{	strncpy(sys[csys].stst, "",
					sizeof(sys[csys].stst));
				continue;
			}
			strncpy(buff, "", sizeof(buff));
			fgets(buff, sizeof(buff), st);
			fclose(st);
			if(tp = rindex(buff, ' '))
				*tp = NULL;		/* drop system name */
			else	continue;
			for(i=0, tp=buff;  i<4;  ++i, ++tp)
				if((tp = index(tp, ' ')) == NULL)
					break;
			if(i != 4)
				continue;
			strncpy(sys[csys].stst, tp, sizeof(sys[csys].stst));
			tp = buff;
			sys[csys].st_type = atoi(tp);
			tp = index(tp+1, ' ');
			sys[csys].st_count = atoi(tp+1);
			tp = index(tp+1, ' ');
			sys[csys].st_lastime = (time_t)atol(tp+1);
			tp = index(tp+1, ' ');
			sys[csys].st_retry = (time_t)atol(tp+1);
		}
	}
	closedir(dirp);
}
/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 */


char *
index(sp, c)
register char *sp, c;
{
	do {
		if (*sp == c)
			return(sp);
	} while (*sp++);
	return(NULL);
}

/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
*/


char *
rindex(sp, c)
register char *sp, c;
{
	register char *r;

	r = NULL;
	do {
		if (*sp == c)
			r = sp;
	} while (*sp++);
	return(r);
}

printpid(sname)
char *sname;
{
   char lokfil[30];
   int ret, pid, fd;

	strcpy(lokfil, "/usr/spool/uucp/LCK..");
	strcat(lokfil, sname);

	fd=open(lokfil, 0);
	if (fd < 0) {
		printf((catgets(nlmsg_fd,NL_SETN,916, "   \t(pid ??)")));
		return;
	}
	ret=read(fd, (char *) &pid, sizeof(pid));
	if ( ret <= 0 ) {
		printf((catgets(nlmsg_fd,NL_SETN,917, "   \t(pid ??)")));
		close(fd);
		return;
	}
	printf((catgets(nlmsg_fd,NL_SETN,918, "   \t(pid %d)")), pid);
	close(fd);

	return;
}
#ifdef HDBuucp
void Scandir(dnam,prfx,flen,fchr,type)
char *dnam,*prfx,fchr;
int flen,type;
{
  register DIR *dirp;
  register struct direct *dp;
  char tmp[1024];
  struct stat sb;
 
  
  /* for HDB uucp directory structure */ 
  if (chdir(dnam)<0){
        perror(dnam);
        exit(1);
  }

  if ((dirp = opendir(".")) == NULL) {
         perror(dnam);
         exit(1);
  }
  /*printf("In Scandir\n");*/

  while ((dp = readdir(dirp)) != NULL)
  {
        if ((dp->d_name[0] == '.' )|| dp->d_ino == 0)
            continue;
        else{
               /*printf("Calling scandir%s\n",dp->d_name);*/
               strcpy(tmp,dp->d_name);
               stat(tmp,&sb);
               if (!stat(tmp,&sb) && ((sb.st_mode & S_IFMT) == S_IFDIR))
               		scandir(tmp,prfx,flen,fchr,type);
               chdir(dnam); /* change back to /usr/spool/uucp */
        } 
  }
  closedir(dirp);
}
         
    
void GetLck(sdir)   /* we use this to get the LCK files */
char *sdir;
{	register int i, csys;
	register char *tp;
	char	fnam[MAXNAMLEN+1], buff[128];
	register struct	direct *dentp;
	register DIR *dirp;
	register FILE *st;

	if(chdir(sdir) < 0)
	{	perror(sdir);
		exit(1);
	}
	if((dirp = opendir(".")) == NULL)
	{	perror(sdir);
		exit(1);
	}
	while((dentp = readdir(dirp)) != NULL)
	{	if(dentp->d_ino == 0)
			continue;
                if(dentp->d_name[0] == '.') continue;
		if(strncmp(dentp->d_name, "LCK..", 5) == SAME)
		{	if(strncmp(&dentp->d_name[5], "tty", 3) == SAME ||
			   strncmp(&dentp->d_name[5], "cul", 3) == SAME)
				continue;
			strcpy(fnam, dentp->d_name);
			for(csys=0; csys<sndx; ++csys)
			{	if(strcmp(&fnam[5], sys[csys].name) == SAME)
					break;
			}
			if(csys == sndx)
			{	strcpy(sys[csys].name, &fnam[5]);
				++sndx;
			}
			++sys[csys].locked;
			continue;
		}
	}
	closedir(dirp);
}
#endif
