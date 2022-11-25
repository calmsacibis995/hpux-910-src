/* HPUX_ID: @(#) $Revision: 70.1 $  */
#include <stdio.h>
#include <fcntl.h>
#include <varargs.h>
#include <ndir.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rbootd.h"
#include "rmp_proto.h"

#define CDFCHAR '+'

int loglevel = EL1;

static struct {
    char *machine_type;    /* As provided by boot client	  */
    char *machine_context; /* context to use for processing cdf's */
} context_lookup[] = {
	{ "HPS300",	"HP-MC68881 HP-MC68020 HP-MC68010"	},
	{ "HPS700",	"PA-RISC1.1 HP-PA"			},
	{ "HPS800",	"HP-PA"					},
	{ (char *)0,	(char *)0				}
    };

extern int errno;
extern FILE *errorfile;
#ifdef DTC
void log_char();
#endif
void errexit();
void log();
char *getnbootfile();
static char *getcdfname(), *cdfexpand();

/*
 * openbootfile() --
 *    This routine checks to see if the client has access to the
 *    requested bootfile. It then attempts to open that file (for
 *    reading) and returns a valid file descriptor if the file can
 *    be opened.  Otherwise it returns -1 for any errors along the
 *    way. It also returns by address an appropriate error code to
 *    send in a boot reply if an error occurs.
 */
openbootfile(pathname,client,mach_type,cdfpathnameptr,errorptr)
    char *pathname;
    struct cinfo *client;
    char *mach_type;
    char **cdfpathnameptr;
    int *errorptr;
{
    int bootfd;
    int fileokflag;
    int defaultflag;
    struct bootinfo *bootinfoptr;
    struct stat sbuf;
    char *cdfpathname;

    /* Check for existence of a FILENAME in request */

    if (pathname[0] != '\0') {

#ifdef DEBUG
	log(EL4,"Specific Filename (%s)\n",pathname);
#endif DEBUG

	defaultflag = FALSE;

	/* There is a filename specified by this request */
	/* see if it is in the boot info list for this   */
	/* client (either scanname or cmpname).		 */

	bootinfoptr = client->bootlist;
	while(bootinfoptr != (struct bootinfo *)0
	  && (bootinfoptr->scanflag == FALSE
	      || strcmp(pathname,bootinfoptr->scanname) != 0)
	  && strcmp(pathname,bootinfoptr->cmpname) != 0 )
	    bootinfoptr = bootinfoptr->next;

#ifdef DTC
	/*
	 * The while loop above will not be executed for the DTC as
	 * bootinfoptr would point to a null struct. For the same
	 * reason we need to skip the following if statement and the
	 * setting of pathname for the case of a request from a DTC.
	 */
	if (client->cl_type != BF_DTC) {
	    if (bootinfoptr == (struct bootinfo *)0) {
		*errorptr = SCANNOTOPEN;
		return(-1);
	    }
	    pathname = bootinfoptr->fullname;
	}
#else
	if (bootinfoptr == (struct bootinfo *)0) {
	    *errorptr = SCANNOTOPEN;
	    return(-1);
	}
	pathname = bootinfoptr->fullname;
#endif /* DTC */

    }
    else {  /* no pathname was specified in boot request */

	defaultflag = TRUE;

	if ((bootinfoptr = client->bootlist) == (struct bootinfo *)0) {
	    *errorptr = DCANNOTOPEN;
	    return(-1);
	}
	pathname = bootinfoptr->fullname;
    }

#ifdef DEBUG
    log(EL4,"Requested file: %s\n",pathname);
#endif DEBUG

#ifdef DTC
    /*
     * For the case of a request from a DTC we do not want to do a
     * getcdfname, and therefore set cdfpathname to pathname.
     */
    if (client->cl_type == BF_DTC) {
	cdfpathname = pathname;
    }
    else {
	cdfpathname = getcdfname(pathname,client,mach_type);
    }
#else
    cdfpathname = getcdfname(pathname,client,mach_type);
#endif /* DTC */

    if (cdfpathname == (char *)0) {
	log(EL1,"Could not expand %s for client %s.\n",pathname,client->name);
	if (defaultflag == FALSE)
	    *errorptr = SFILENOTFOUND;
	else
	    *errorptr = DFILENOTFOUND;

	return(-1);
    }

#ifdef DEBUG
    log(EL4,"Expanded name:  %s\n",cdfpathname);
#endif DEBUG

    /* Try to open pathname for reading */
    if ((bootfd = open(cdfpathname,O_RDONLY)) < 0) {
	log(EL1,"open failed on %s. errno=%d.\n",cdfpathname,errno);
	if (defaultflag == FALSE)
	    *errorptr = SFILENOTFOUND;
	else
	    *errorptr = DFILENOTFOUND;

	return(-1);
    }

#ifdef DTC
    if (client->cl_type == BF_DTC)
    {
	extern char *strrchr();
	char *basename;

	/*
	 * If the basename of cdfpathname matches either global.dnld
	 * or tioconf.dnld or acclist.dnld then unlink the file. Saves
	 * disc space and as this file is always recreated on receiving
	 * a boot request, receiving duplicate boot requests does not
	 * cause a problem.
	 */

	/* obtain basename of cdfpathname */

	if ((basename = strrchr(cdfpathname, '/')) == NULL)
	    basename = cdfpathname;
	else
	    basename++;


	if (strcmp(basename, "global.dnld") == 0 ||
	    strcmp(basename, "global1.dnld") == 0 ||
	    strcmp(basename, "tioconf.dnld") == 0 ||
	    strcmp(basename, "acclist.dnld") == 0)
	{
	    unlink(cdfpathname);
#ifdef DEBUG
	    log(EL5, "openbotfile(): unlinked %s\n", cdfpathname);
#endif /* DEBUG */
	}
    }
#endif /* DTC */

    /* check mode to be sure this is an ordinary file */

    if (fstat(bootfd, &sbuf) != 0)
	fileokflag = FALSE;
    else
    {
	if ((sbuf.st_mode & S_IFMT) != S_IFREG)
	{
	    log(EL1,"Requested boot file %s is not an ordinary file.\n",
		cdfpathname);
	    fileokflag = FALSE;
	}
	else
	    fileokflag = TRUE;
    }

    if (fileokflag == FALSE) {
	(void) close(bootfd);

	if (defaultflag == FALSE)
	    *errorptr = SCANNOTOPEN;
	else
	    *errorptr = DCANNOTOPEN;

	return(-1);
    }

    *cdfpathnameptr = cdfpathname;
    return(bootfd);
}

static char *
getcdfname(pathname,client,mach_type)
    char *pathname;
    struct cinfo *client;
    char *mach_type;
{
    register int i;
    char *cdfpathname;
    char mtype[MACH_TYPE_LEN + 1];
    char *mach_context;

    /* Map machine type */

    memcpy(mtype,mach_type,MACH_TYPE_LEN);
    mtype[MACH_TYPE_LEN] = '\0';

    mach_context = mtype; /* temporarily use mach_context as a temp pointer */
    while (*mach_context != ' ' && *mach_context != '\0')
	mach_context++;

    *mach_context = '\0';

    mach_context = ""; /* point to empty context if no match is found */
    for (i = 0; context_lookup[i].machine_type != (char *)0; i++) {

	if (strcmp(mtype,context_lookup[i].machine_type) == 0) {
	    mach_context = context_lookup[i].machine_context;
	    break;
	}
    }

    /* call cdfexpand() to expand pathname based on context. We call */
    /* cdfexpand() with different arguments based on whether or not  */
    /* the client is in /etc/clusterconf. This is determined by      */
    /* looking at the clients cnode_id. If it is zero then the client*/
    /* is not in /etc/clusterconf.				     */

    if (client->cnode_id == 0)
	cdfpathname = cdfexpand(pathname,client->name,mach_context,
				"default",(char *)0);
    else
	cdfpathname = cdfexpand(pathname,client->name,mach_context,
				"remoteroot","default",(char *)0);

    return(cdfpathname);
}

static char *
cdfexpand(va_alist)
va_dcl
{
    va_list args;
    register char *s1, *s2;
    char *pathname;
    char *cxt;
    static char cdfpathname[MAXPATHLEN];
    int cdflen;
    int cxtlen;
    int matchflag;
    DIR *dfptr;

    va_start(args);
    pathname = va_arg(args, char *);
    va_end(args);

    s1 = pathname;
    s2 = cdfpathname;

    if (s1 == (char *)0 || *s1 == '\0')
	return (char *)0;

    /*
     * Copy first character into cdfpathname as a special case. This
     * is due to the fact that "/" can not be a cdf (At least not
     * at the time this code was written. If "/" can be a cdf then
     * we would have to open "/..+" to see if it is.
     */
    *s2++ = *s1++;
    cdflen = 1;

    while (*s1 != '\0')
    {
	/*
	 * Copy until we find a '/', a null or we overflow cdfpathname
	 */
	while (*s1 != '/' && *s1 != '\0' && cdflen < (MAXPATHLEN - 1))
	{
	    *s2++ = *s1++;
	    cdflen++;
	}

	/* need room for CDFCHAR */

	if (cdflen >= (MAXPATHLEN - 1))
	    return (char *)0;

	do
	{
	    *s2++ = CDFCHAR;
	    *s2 = '\0';

	    /*
	     * Try to open escaped pathname to see if this component
	     * of the path is context dependent.
	     */
	    if ((dfptr = opendir(cdfpathname)) == (DIR *)0)
	    {
		s2--;
	    }
	    else
	    {
		va_start(args);
		pathname = va_arg(args, char *);    /* skip pathname */
		matchflag = FALSE;

		/*
		 * loop, trying to match cxt.
		 * The context in cxt can actually be any number of
		 * context strings, blank seperated.  This is to
		 * support machines that should have multiple
		 * strings for the "machine type" portion of their
		 * context (i.e. "HP-MC68881 HP-MC68020 HP-MC68010").
		 */
		while (matchflag == FALSE &&
		       (cxt = va_arg(args, char *)) != (char *)0)
		{
		    while (*cxt != '\0')
		    {
			extern char *strchr();
			struct direct *dir;
			char *endptr;

			/*
			 * Extract the next word in this cxt string
			 */
			while (*cxt == ' ')
			    cxt++;
			if (*cxt == '\0')
			    break;
			if ((endptr = strchr(cxt, ' ')) == NULL)
			    endptr = cxt + strlen(cxt);
			cxtlen = endptr - cxt;

			while ((dir = readdir(dfptr)) !=
						     (struct direct *)0)
			{

			    if (cxtlen == dir->d_namlen &&
				strncmp(cxt, dir->d_name, cxtlen) == 0)
			    {
				matchflag = TRUE;
				break;
			    }
			}

			if (matchflag == TRUE)
			    break;

			cxt = endptr;
			(void)rewinddir(dfptr);
		    }
		}

		va_end(args);
		(void)closedir(dfptr);

		/*
		 * If matchflag is FALSE or we will overflow
		 * cdfpathname by adding cxt then return
		 * a null pointer since either the cxt does
		 * not match any file or we can't fit the
		 * cxt.
		 */
		if (matchflag == FALSE ||
		    cdflen + cxtlen + 2 >= (MAXPATHLEN - 1))
		    return (char *)0;

		*s2++ = '/';
		strncpy(s2, cxt, cxtlen);
		s2 += cxtlen;
		*s2 = '\0';
		cdflen += (cxtlen + 2);	/* CDFCHAR + '/' + cxt */
	    }
	} while (dfptr != (DIR *)0);

	if (*s1 != '\0')
	{
	    *s2++ = *s1++;
	    cdflen++;
	}
    }

    *s2 = '\0';
    return cdfpathname;
}

char *
getnbootfile(client,filenum,mach_type)
    struct cinfo *client;
    long filenum;
    char *mach_type;
{
    struct bootinfo *bootinfoptr;
    char *cdfpathname;
    struct stat sbuf;
    long n;


#ifdef DEBUG
    log(EL4,"In getnbootfile. filenum=%d\n",filenum);
#endif DEBUG

    n = 1;
    bootinfoptr = client->bootlist;
    while (bootinfoptr != (struct bootinfo *)0) {
	if (bootinfoptr->scanflag == TRUE) {

	    /* Check to see if file currently exists */

	    cdfpathname = getcdfname(bootinfoptr->fullname,client,mach_type);

	    if (cdfpathname != (char *)0 && stat(cdfpathname,&sbuf) == 0) {
#ifdef DEBUG
		log(EL4,"n=%d filename=%s cmpname=%s fullname=%s\n",n,
			 bootinfoptr->scanname,bootinfoptr->cmpname,bootinfoptr->fullname);
#endif DEBUG
		if (filenum == n++)
		    return(bootinfoptr->scanname);
	    }
#ifdef DEBUG
	    else {
		log(EL4,"Skipping due to missing file filename=%s cmpname=%s fullname=%s\n",
			 bootinfoptr->scanname,bootinfoptr->cmpname,bootinfoptr->fullname);

	    }
#endif DEBUG
	}
#ifdef DEBUG
	else {
	    log(EL4,"Skipping due to scanflag = FALSE filename=%s cmpname=%s fullname=%s\n",
		     bootinfoptr->scanname,bootinfoptr->cmpname,bootinfoptr->fullname);

	}
#endif DEBUG

	bootinfoptr = bootinfoptr->next;
    }

    return((char *)0);
}

void
log(va_alist)
    va_dcl
{
    static time_t last_time;
    static char datestring[32];
    time_t curtime;
    va_list args;
    char *fmt;
    int loglev;

#ifdef DEBUG
    static int times_called;
    static int new_date_calls;

    times_called++;
#endif

    /*
     * Get the current time.  If it is the same as the last time
     * that we are called, we use the formatted time from the last
     * call.  Otherwise, we must call ctime() to convert the time
     * to its printable representation.
     */
    if ((curtime = time((time_t *)0)) != last_time)
    {
	extern char *ctime();

	last_time = curtime;
	strcpy(datestring, ctime(&curtime));
	datestring[strlen(datestring) - 1] = '\0';
#ifdef DEBUG
	new_date_calls++;
#endif
    }

    va_start(args);
    loglev = va_arg(args, int);
    if (loglev <= loglevel)
    {
	(void)fprintf(errorfile, "%s pid=%d: ", datestring, getpid());
	fmt = va_arg(args, char *);
	(void)vfprintf(errorfile, fmt, args);
    }
    va_end(args);
}

void
errexit(va_alist)
    va_dcl
{
    va_list args;
    char *fmt;
    char *datestring;
    extern char *ctime();
    long curtime;

    curtime = time(0);
    datestring = ctime(&curtime);
    datestring[strlen(datestring) - 1] = '\0';

    va_start(args);
    fmt = va_arg(args,char *);
    (void) fprintf(errorfile,"%s pid=%d: ",datestring,getpid());
    (void) vfprintf(errorfile, fmt, args);
    (void) fprintf(errorfile,"%s pid=%d: TERMINATING\n",datestring,getpid());
    va_end(args);

    exit(1);
}

#ifdef DTC
void
log_char(v)
unsigned char v;
{
    /*
     * This function is called by print_packet(). It converts a
     * character into its 8 bit representation. Eg: log_char(w) writes
     * 01110111 into errorfile.
     */
    int i;
    int mask = 1 << 7;  /* shift the mask bit to the high-order end */

    for (i=1; i<=8; ++i) {
	fputc((((v & mask) == 0) ? '0' : '1'), errorfile);
	v <<= 1;
    }
    fputc('\n', errorfile);
}
#endif /* DTC */
