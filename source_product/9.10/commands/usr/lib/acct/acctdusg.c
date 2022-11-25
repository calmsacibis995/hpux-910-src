/* @(#) $Revision: 66.3 $ */      
/*
 *	acctdusg [-u ufile] [-p pfile]
 *
 *	Acctdusg reads its standard input (usually from find / -print)
 *	and computes disk resource consumption by login. If -u is given,
 *	records consisting of those files names for which acctdusg
 *	charges no one are placed in ufile (a potential source for
 *	finding users trying to avoid disk charges). If -p is given,
 *	pfile is the name of the password file. This option is not 
 *	needed if the password file is /etc/passwd.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <unistd.h>
#include "acctdef.h"

#define NAMESZ UTSLEN

struct	disk{
	char	dsk_name[NAMESZ];	/* login name */
	unsigned dsk_uid;		/* user id of login name */
	int	dsk_dirsz;	/* # letters in pathname of login directory */
	char	*dsk_logdir;	/* ptr to path name of login directory */
	int	dsk_ns;			/* no of slashes in path name */
	long	dsk_du;			/* disk usage */
	struct	disk *dsk_np;		/* ptr to next struct */

};
char	*pfile ={ "/etc/passwd" };
char	*afile, *nfile;
 
struct disk	*Dp;	/* ptr to 1st entry in dirlist */
struct stat	statb;

FILE	*pswd, *nchrg;
FILE	*names ={stdin};
FILE	*acct  ={stdout};

char	fbuf[BUFSIZ];
char	*calloc();

char	*getcwd();
char	*cwd_buf;

main(argc, argv)
char	**argv;

{
	while(--argc > 0){
		++argv;
		if(**argv == '-') switch((*argv)[1]) {
		case 'u':
			if (--argc <= 0)
				break;
			if ((nchrg = fopen(*(++argv),"w")) == NULL)
				openerr(*argv);
			chmod(*argv, 0644);
			continue;
		case 'p':
			if (--argc <= 0)
				break;
			pfile = *(++argv);
			continue;
		}
		fprintf(stderr,"Usage: acctdusg [-u file] [-p file]\n");
		exit(1);
	}
	if ((pswd = fopen(pfile, "r")) == NULL)
		openerr(pfile);

	while (fgets(fbuf,sizeof fbuf, pswd) != NULL) {
		makdlst(fbuf);	/* make a list of home directory names
				for every entry in password file */
	}
	fclose(pswd);

	dsort();
	
	if( (cwd_buf = getcwd((char *) NULL, BUFSIZ)) == NULL)
		fprintf(stderr, "acctdusg: cannot get current working directory\n");
	else
		strcat(cwd_buf, "/");

	while( fgets(fbuf, sizeof fbuf, names) != NULL) {
		fbuf[strndx(fbuf, '\n') ] = '\0';
		if(clean(fbuf))
			charge(fbuf);
	}
	if (names != stdin)
		fclose(names);

	output();

	if (acct != stdout)
		fclose(acct);
	if (nchrg)
		fclose(nchrg);
#ifdef DEBUG
		pdisk();
#endif

	exit(0);
}
openerr(file)
char	*file;
{
	fprintf(stderr, "acctdusg: Cannot open %s\n", file);
	exit(1);
}

output()
{

	register struct disk *dp;

	for(dp = Dp; dp != NULL; dp=dp->dsk_np) {
		if(dp->dsk_du)
			fprintf(acct,
				"%05u\t%-8.8s\t%7lu\n",
				dp->dsk_uid,
				dp->dsk_name,
				dp->dsk_du);
	}
}

strndx(str, chr)
register char *str;
register char chr;
{

	register index;

	for (index=0; *str; str++,index++)
		if (*str == chr)
			return index;
	return -1;
}

/*
 *	make a list of home directory names
 *	for every entry in password file
 */

makdlst(p)
register char *p;
{

	static struct	disk *dl = {NULL};
	struct disk	*dp;
	int    i;

	if( (dp = (struct disk *)calloc(sizeof(struct disk), 1)) == NULL) {
	nocore:
		fprintf(stderr, "acctdusg: Out of memory\n");
		exit(2);
	}
	for(i=0; i<NAMESZ; i++) {	/* copy login name from 
				password file to structure */
		if((dp->dsk_name[i] = *p++) == ':') {
			dp->dsk_name[i] = '\0';
			break;
		}
	}
	while(*p++ != ':')	/* skip password field */
		;
	sscanf(p, "%u", &(dp->dsk_uid));

	for(i=ccount(p, ':')-1; i>0; i--) /* read to end of 
			line counting field delimited by : */
		while(*p++ != ':')  /* move up to path name */
			;

	dp->dsk_dirsz = strndx(p, ':'); /* length of path name */
	if((dp->dsk_logdir = calloc(dp->dsk_dirsz + 1, 1)) == NULL)
		goto nocore;

	strncpy(dp->dsk_logdir, p, dp->dsk_dirsz);

	if(stat(dp->dsk_logdir,&statb)== -1 ||
			(statb.st_mode & S_IFMT) != S_IFDIR) {
		cfree(dp->dsk_logdir);
		cfree(dp);
		return;
	}
	for(i=0; dp->dsk_logdir[i]; i++)
		if(dp->dsk_logdir[i] == '/')
			dp->dsk_ns++; /* count # of slashes */

	if(dl == NULL) { /* link ptrs */
		Dp = dl = dp;
	} else {
		dl->dsk_np = dp;
		dl = dp;
	}
	return;
}

/*
 *	read to end of line counting
 *	fields (delimited by :
 */
ccount(p,c)
register char *p, c;
{

	register i;

	i = 0;
	while(*p)
		if(*p++ == c)
			i++;
	return i;
}
/*
 *	sort by decreasing # of levels in login
 *	pathname and then by increasing uid
 */
dsort()
{

	register struct disk *dp1, *dp2, *pdp;
	int	change;

	if(Dp == NULL || Dp->dsk_np == NULL)
		return;

	change = 0;
	pdp = NULL;

	for(dp1 = Dp; ;) {
		if((dp2 = dp1->dsk_np) == NULL) {
			if(!change)
				break;
			dp1 = Dp;
			pdp = NULL;
			change = 0;
			continue;
		}
		if((dp1->dsk_ns < dp2->dsk_ns) ||
		   (dp1->dsk_ns==dp2->dsk_ns && dp1->dsk_uid > dp2->dsk_uid)) {
			swapd(pdp, dp1, dp2);
			change = 1;
			dp1 = dp2;
			continue;
		}
		pdp = dp1;
		dp1 = dp2;
	}
}

swapd(p,d1,d2)

register struct disk *p, *d1, *d2;
{
	struct disk *t;

	switch((int) p) {
	default:
		p->dsk_np = d2;
		t = d2->dsk_np;
		d2->dsk_np = d1;
		d1->dsk_np = t;
		break;
	case NULL:
		t = d2->dsk_np;
		d2->dsk_np = d1;
		d1->dsk_np = t;
		Dp = d2;
		break;
	}
}

charge(n)
register char *n;
{
	register struct disk *dp;
	register i;
	long	blks;

	if(stat(n,&statb) == -1)
		return;

	i = strlen(n);
	for(dp = Dp; dp != NULL; dp = dp->dsk_np) {
		if(i < dp->dsk_dirsz)
			continue;
		if(strncmp(dp->dsk_logdir, n, dp->dsk_dirsz) == 0 &&
		   (n[dp->dsk_dirsz] == '/' || n[dp->dsk_dirsz] == '\0'))
			break;
	}


	blks = BLOCKS(statb.st_blocks);

	if(dp == NULL) {
		if(nchrg && (statb.st_size)) {
		   	if ((statb.st_mode&S_IFMT) == S_IFDIR)
				fprintf(nchrg, "%5u\t%7lu\t%s\n",
				statb.st_uid,blks,n);
		   	else if ((statb.st_mode&S_IFMT) == S_IFREG)
				fprintf(nchrg, "%5u\t%7lu\t%s\n",
				statb.st_uid, blks / statb.st_nlink, n);
		};
		return;
	}

	/* Check if the file belongs to the owner or not...
	   Fix for Bug #: FSDlj05492
	*/
	if (dp->dsk_uid == statb.st_uid)
	{
		dp->dsk_du += (statb.st_mode&S_IFMT) == S_IFDIR ? blks
			:((statb.st_mode&S_IFMT)==S_IFREG ?
			  (blks / statb.st_nlink) : 0L);
	}
	else
	{
		/* File does not belong to the owner of this users
		 * home directory.  Flag it if the -u option specified
		 */
		if ((statb.st_mode&S_IFMT) == S_IFDIR)
			fprintf(nchrg, "%5u\t%7lu\t%s\n",
			statb.st_uid,blks,n);
		else if ((statb.st_mode&S_IFMT) == S_IFREG)
			fprintf(nchrg, "%5u\t%7lu\t%s\n",
			statb.st_uid, blks / statb.st_nlink, n);
	}
}

#ifdef DEBUG
pdisk()
{
	register struct disk *dp;

	for(dp=Dp; dp!=NULL; dp=dp->dsk_np)
		printf("%.8s\t%5u\t%7lu\t%5u\t%5u\t%s\n",
			dp->dsk_name,
			dp->dsk_uid,
			dp->dsk_du,
			dp->dsk_dirsz,
			dp->dsk_ns,
			dp->dsk_logdir);
}
#endif

int
clean(p)
register char *p;
{
	register char *s1, *s2;
	char	temp[2 * BUFSIZ];

	s1 = p;
	if( (*s1 != '/') && (*cwd_buf != NULL) )
	{
		strcpy(temp, cwd_buf);
		strcat(temp, s1);
		if(strlen(temp) > BUFSIZ)
		{
			fprintf(stderr, "acctdusg: Pathname %s too long\n",
			temp);
			return(0);
		}
		strcpy(s1, temp);
	}
			

	for(s1=p; *s1; ) {
		s2 = s1;
		while(*s1 == '/')
			s1++;
		s1 = s1<= s2 ? s2 : s1-1;
		if(s1 != s2) {
			strcpy(s2,s1);
			s1 = s2;
		}
		if(*++s2 == '.') 
		switch(*++s2) {
		case '/':
			strcpy(s1,s2);
			continue;
		case '.':
			if(*++s2 != '/')
				break;
			if(s1 > p)
				while(*--s1 != '/' && s1 > p)
					;
			strcpy(s1,s2);
			continue;
		}
		while(*s2 && *++s2 != '/')
			;
		s1 = s2;
	}
	return(1);
}
