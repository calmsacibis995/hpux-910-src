static char *HPUX_ID = "@(#) $Revision: 70.1 $";

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <limits.h>

#define MAXLINE LINE_MAX+1	/* This matches getgrent() and friends */
#define ERROR1 "Too many/few fields"
#define ERROR2a "No group name"
#define ERROR2b "Bad character(s) in group name"
#define ERROR3  "Invalid GID"
#define ERROR4a "Null login name"
#define ERROR4b "Logname not found in password file"
#ifdef HP_NFS
#define ERROR_NFS_1	"Illegal +/- yellow page entry"
#endif HP_NFS

int eflag, badchar, baddigit,badlognam,colons,len,i;
char buf[MAXLINE];
char tmpbuf[MAXLINE];

struct passwd *getpwnam();
char *strchr();
void setpwent(), exit();
void error();
char *nptr;
int found_error=0;

main (argc,argv)
int argc;
char *argv[];
{
    FILE *fptr;
    char *cptr;
    int delim[MAXLINE];
    long gid;
#ifdef HP_NFS
    int namestart, ypentry;
#endif HP_NFS

    if ( argc == 1)
	argv[1] = "/etc/group";
    else if ( argc != 2 )
    {
	printf ("\nusage: %s [filename]\n\n",*argv);
	exit(1);
    }

    if ( ( fptr = fopen (argv[1],"r" ) ) == NULL )
    { 
	printf ("\ncannot open file %s\n\n",argv[1]);
	exit(1);
    }

    while(fgets(buf,MAXLINE,fptr) != NULL )
    {
	if ( buf[0] == '\n' )    /* blank lines are ignored */
	    continue;

#ifdef HP_NFS
	ypentry = 0;

	if ((buf[0] == '+') || (buf[0] == '-'))
	    ypentry = 1;
#endif HP_NFS

	for (i=0; buf[i]!=NULL; i++)
	{
	    tmpbuf[i]=buf[i];          /* tmpbuf is a work area */
	    if (tmpbuf[i] == '\n')     /* newline changed to comma */  
		tmpbuf[i] = ',';
	}

	for ( ; i < MAXLINE; ++i)     /* blanks out rest of tmpbuf */ 
	{
	    tmpbuf[i] = NULL;
	}
	colons=0;
	eflag=0;
	badchar=0;
	baddigit=0;
	badlognam=0;
	gid=0l;

    /*	Check number of fields	*/

	for (i=0 ; buf[i]!=NULL ; i++)
	{
	    if (buf[i]==':')
	    {
		delim[colons]=i;
		++colons;
	    }
	}
#ifdef HP_NFS
	if (colons != 3 )
	{
	    if (ypentry == 1)
	    {
	/* If this is an incomplete yp entry, fake the locations for
	   the rest of the fields. */
		while ((i > 0) && (buf[i-1] == '\0'))
		    i--;

		for (i++; colons < 3; i++, colons++)
		    delim[colons] = i;
	    }
	    else
	    {
		error(ERROR1);
		continue;
	    }
	}
#else not HP_NFS
	if (colons != 3 )
	{
	    error(ERROR1);
	    continue;
	}
#endif not HP_NFS

    /*	check to see that group name is at least 1 character	*/
    /*		and that all characters are printable.		*/
 
#ifdef HP_NFS
	/* Check for "+", "+name" and "-name" yp entries. */
	namestart = 0;

	if (ypentry == 1)
	{
	    if ((buf[0] == '-') && (islower(buf[1])))
		namestart = 1;
	    else if (buf[0] == '+')
	    {
		switch (buf[1])
		{
	    case ':':
		    goto checkgid;
		    break;
	    case '\n':
	    case '\0':
		    continue;
		    break;
	    default:
		    if (islower(buf[1]))
			namestart = 1;
		    else
			error(ERROR_NFS_1);
		    break;
		}
	    }
	    else
		error(ERROR_NFS_1);
	}
	else if ( buf[0] == ':' )
	    error(ERROR2a);
	else
	{
	    for ( i=0, cptr = &(buf[i + namestart]);
			(*cptr != ':') && (*cptr != '\n'); i++, cptr++ )
	    {
		if ( ! ( isprint(*cptr)))
		    badchar++;
	    }
	    if ( badchar > 0 )
		error(ERROR2b);
	}
#else not HP_NFS
	if ( buf[0] == ':' )
	    error(ERROR2a);
	else
	{
	    for ( i=0; buf[i] != ':'; i++ )
	    {
		if ( ! ( isprint(buf[i])))
		    badchar++;
	    }
	    if ( badchar > 0 )
		error(ERROR2b);
	}
#endif not HP_NFS

#ifdef HP_NFS
checkgid:
#endif HP_NFS
    /*	check that GID is numeric and <= 65535	*/

	len = ( delim[2] - delim[1] ) - 1;

#ifdef HP_NFS
	if ( len > 5 || ((ypentry == 0) && (len == 0 )))
#else not HP_NFS
	if ( len > 5 || len == 0 )
#endif not HP_NFS
	    error(ERROR3);
	else
	{
	    for ( i=(delim[1]+1); i < delim[2]; i++ )
	    {
		if ( ! (isdigit(buf[i])))
		    baddigit++;
		else if ( baddigit == 0 )
		    gid=gid * 10 + (buf[i]) - '0';    /* converts ascii */
						      /* GID to decimal */
	    }
	    if ( baddigit > 0 )
		error(ERROR3);
	    else if ( gid > 65535l || gid < 0l )
		error(ERROR3);
	}

     /*  check that logname appears in the passwd file  */

	nptr = &tmpbuf[delim[2]];
	nptr++;
	while ( ( cptr = strchr(nptr,',') ) != NULL )
	{
	    *cptr=NULL;
	    if ( *nptr == NULL )
	    {
#ifdef HP_NFS
	    if (ypentry == 0)
#endif HP_NFS
		error(ERROR4a);
		nptr++;
		continue;
	    }
	    if (  getpwnam(nptr) == NULL )
	    {
		badlognam=1;
		error(ERROR4b);
	    }
	    nptr = ++cptr;
	    setpwent();
	}
	
    }
    exit(found_error?2:0);
}

    /*	Error printing routine	*/
void
error(msg)

char *msg;
{
    found_error = 1;  /* global flag to signify that something's wrong */

    if ( eflag==0 )
    {
	fprintf(stderr,"\n\n%s",buf);
	eflag=1;
    }

    if ( badchar != 0 )
    {
	fprintf (stderr,"\t%d %s\n",badchar,msg);
	badchar=0;
	return;
    }
    else if ( baddigit != 0 )
    {
	fprintf (stderr,"\t%s\n",msg);
	baddigit=0;
	return;
    }
    else if ( badlognam != 0 )
    {
	fprintf (stderr,"\t%s - %s\n",nptr,msg);
	badlognam=0;
	return;
    }
    else
    {
	fprintf (stderr,"\t%s\n",msg);
	return;
    }
}

