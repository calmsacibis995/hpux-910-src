static char *HPUX_ID = "@(#) $Revision: 66.1 $";

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/signal.h>
#include	<sys/sysmacros.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<ctype.h>
#define	ERROR1	"Too many/few fields"
#define ERROR2	"Bad character(s) in logname"
#define ERROR2a "First char in logname not lower case alpha"
#define ERROR2b "Logname field NULL"
#define ERROR3	"Logname too long/short"
#define ERROR4	"Invalid UID"
#define S_ERROR4 "Invalid AID"
#define ERROR5	"Invalid GID"
#define S_ERROR5 "Invalid Audit Flag"
#define ERROR6	"Login directory not found"
#define ERROR6a	"Login directory null"
#define	ERROR7	"Optional shell file not found"
#ifdef HP_NFS
#define ERROR_NFS_1	"Illegal +/- yellow page entry"
#endif HP_NFS
#define SECUREPASS	"/.secure/etc/passwd"

int eflag, code=0;
int badc;
char buf[512];

main(argc,argv)

int argc;
char **argv;

{
	int delim[512];
	char logbuf[80];
	FILE *fopen(), *fptr, *sptr;
	char *fgets();
	int error();
	extern char *optarg;
	extern int optind, opterr;
	struct	stat obuf;
	long uid, gid; 
	unsigned long aid;
	int len;
	register int i, j, colons;
	register char *ptr;
#ifdef HP_NFS
	int namestart, ypentry;
#endif HP_NFS
	char *pw_file;
	register int secure = 0;

	if((i = getopt(argc, argv, "s")) != EOF) {
		if( i == '?') {
			fprintf(stderr,"usage: pwck [-s] [file]\n");
			exit(1);
		}
		secure = 1;
		argc--;
		if((sptr=fopen(SECUREPASS, "r"))==NULL) {
               		fprintf(stderr,"cannot open %s\n",SECUREPASS);
               		exit(1);
		}
	}

	if(argc == 1) pw_file="/etc/passwd";
	else pw_file=argv[optind];

	if((fptr=fopen(pw_file,"r"))==NULL) {
		fprintf(stderr,"cannot open %s\n",pw_file);
		exit(1);
	}

	while(fgets(buf,512,fptr)!=NULL) {

		logbuf[0] = '\0';

		colons=0;
		badc=0;
		uid=gid=0l;
		eflag=0;
#ifdef HP_NFS
		ypentry = 0;

		if ((buf[0] == '+') || (buf[0] == '-'))
		    ypentry = 1;
#endif HP_NFS

	/*  Check number of fields */

		for(i=0 ; buf[i]!=NULL; i++) {
			if(buf[i]==':') {
				delim[colons]=i;
				++colons;
			}
		delim[6]=i;
		delim[7]=NULL;
		}
#ifdef HP_NFS
		if (colons != 6) {
		    if ((ypentry == 1) && (colons < 6)) {
		/* If this is an imcomplete yp entry, fake the locations for
		   the rest of the fields. */

			while ((i > 0) && (buf[i-1] == '\n'))
			    i--;

			for (; colons < 6; i++, colons++)
			    delim[colons] = i;
		    }
		    else {
			error(ERROR1);
			continue;
		    }
		}
#else not HP_NFS
		if(colons != 6) {
			error(ERROR1);
			continue;
		}
#endif not HP_NFS

	/*  Check that first character is alpha and rest alphanumeric  */

#ifdef HP_NFS
	/* Check for "+", "+@", "-" and "-@" yellow page entries. */
		namestart = 0;

		if (ypentry == 1) {
		    if ((buf[1] == '@') && (islower(buf[2]))) {
			/* [+-]@ entries must have netgroup names. */
			namestart = 2;
		    }
		    else if (buf[0] == '+') {
			if (buf[1] == ':')
				/* An entry with logname of "+" is fine. */
			    goto checkuid;
			else if (buf[1] == '\n')
				/* An entry with only "+" on a line is fine. */
			    continue;
			else if (islower(buf[1])) 
				/* +name entry. */
			    namestart = 1;
			else {
			    error(ERROR_NFS_1);
			}
		    }
		    else if (islower(buf[1])) 
				/* -name entry. */
			namestart = 1;
		    else {
			error(ERROR_NFS_1);
		    }
		}
		else if(!(islower(buf[0]))) {
			error(ERROR2a);
		}
		if(buf[0] == ':') {
			error(ERROR2b);
		}
		for(i=0, ptr = &(buf[i+namestart]);
		    (*ptr !=':') && (*ptr!='\n'); i++, ptr++) {
			if(islower(*ptr));
			else if(isdigit(*ptr));
			else ++badc;
		}
		if(badc > 0) {
			error(ERROR2);
		}
#else not HP_NFS
		if(!(islower(buf[0]))) {
			error(ERROR2a);
		}
		if(buf[0] == ':') {
			error(ERROR2b);
		}
		for(i=0, ptr = &buf[0]; *ptr!=':'; i++, ptr++) {
			if(islower(*ptr));
			else if(isdigit(*ptr));
			else ++badc;
		}
		if(badc > 0) {
			error(ERROR2);
		}
#endif not HP_NFS

	/*  Check for valid number of characters in logname  */

		if(i <= 0  ||  i > 8) {
			error(ERROR3);
		}

#ifdef HP_NFS
checkuid:
#endif HP_NFS
	/*  Check that UID is numeric and <= 65535  */

		len = (delim[2]-delim[1])-1;
# ifndef HP_NFS
		if (len == 0) {
			error(ERROR4);
		}
#endif not HP_NFS
		if(len > 5) {
			error(ERROR4);
		}
		else {
#ifdef HP_NFS
				/* Look for -2 uid. */
		    i = delim[1]+1;

		    if (len == 0) {
		        if (ypentry == 0) /* null UID okay only if ypentry */
			    error(ERROR4);
		    }
		    else if ((len == 2) && (buf[i] == '-') && (buf[i+1] == '2'))
		        ;	/* Negative 2 as a uid is okay. */
		    else {
#endif HP_NFS
			for (i=(delim[1]+1); i < delim[2]; i++) {
			    if(!(isdigit(buf[i]))) {
				    error(ERROR4);
				    break;
			    }
			    uid = uid*10+(buf[i])-'0';
			}
			if(uid > 65535l  ||  uid < 0l) {
			    error(ERROR4);
			}
#ifdef HP_NFS
		    }
#endif HP_NFS
		}

	/*  Check that GID is numeric and <= 65535  */

		len = (delim[3]-delim[2])-1;
# ifndef HP_NFS
		if (len == 0) {
			error(ERROR5);
		}
#endif not HP_NFS
		if(len > 5) {
			error(ERROR5);
		}
#ifdef HP_NFS
		else if (len == 0) {
			if (ypentry == 0)  /* null GID okay only if ypentry */
				error(ERROR5);
		}
#endif HP_NFS
		else {
		    for(i=(delim[2]+1); i < delim[3]; i++) {
			if(!(isdigit(buf[i]))) {
				error(ERROR5);
				break;
			}
			gid = gid*10+(buf[i])-'0';
		    }
		    if(gid > 65535l  ||  gid < 0l) {
			error(ERROR5);
		    }
		}

	/*  Stat initial working directory  */

		for(j=0, i=(delim[4]+1); i<delim[5]; j++, i++) {
			logbuf[j]=buf[i];
		}
		logbuf[j]='\0';
#ifdef HP_NFS
	/* If this isn't a YP entry or a YP entry with a login directory. */
		if (((stat(logbuf,&obuf)) == -1) &&
			((ypentry == 0) ||
			((ypentry == 1) && (logbuf[0] != '\0')))) {
			error(ERROR6);
		}
		if ((logbuf[0] == NULL) && (ypentry == 0)) {
					  /* Currently OS translates */
			error(ERROR6a);   /*  "/" for NULL field */
		}
#else not HP_NFS
		if((stat(logbuf,&obuf)) == -1) {
			error(ERROR6);
		}
		if(logbuf[0] == NULL) { /* Currently OS translates */
			error(ERROR6a);   /*  "/" for NULL field */
		}
#endif not HP_NFS
		for(j=0;j<80;j++) logbuf[j]=NULL;

	/*  Stat of program to use as shell  */

		if((buf[(delim[5]+1)]) != '\n') {
			for(j=0, i=(delim[5]+1); i<delim[6]; j++, i++) {
				logbuf[j]=buf[i];
			}
			logbuf[j]='\0';
#ifdef HP_NFS
	/* If this isn't a YP entry or a YP entry with a login shell. */
			if (((stat(logbuf,&obuf)) == -1) &&
				((ypentry == 0) ||
				((ypentry == 1) && (logbuf[0] != '\0')))) {
				error(ERROR7);
			}
#else not HP_NFS
			if((stat(logbuf,&obuf)) == -1) {
				error(ERROR7);
			}
#endif not HP_NFS
			for(j=0;j<80;j++) logbuf[j]=NULL;
		}
	}
	fclose(fptr);

	if (secure) {		/* check the secure passwd file */
	   printf("Checking secure password file...\n");
	   while(fgets(buf,512,sptr)!=NULL) {
		colons=0;
                badc=0;
                aid=0l;
                eflag=0;

		/*  Check number of fields */

                for(i=0 ; buf[i]!=NULL; i++) {
                        if(buf[i]==':') 
                                delim[colons++]=i;
                delim[3]=i;
                delim[4]=NULL;
                }
		
		if (colons != 3) { 	/* invalid fields -- ignore entry */
			error(ERROR1);
			continue;
		}

	/*  Check that first character is alpha and rest alphanumeric  */

		if(!(islower(buf[0]))) {
                        error(ERROR2a);
                }
                if(buf[0] == ':') {
                        error(ERROR2b);
                }
                for(i=0, ptr = &buf[0]; *ptr!=':'; i++, ptr++) {
                        if(islower(*ptr));
                        else if(isdigit(*ptr));
                        else ++badc;
                }
                if(badc > 0) {
                        error(ERROR2);
                }

	 	/*  Check for valid number of characters in logname  */

                if(i <= 0  ||  i > 8) {
                        error(ERROR3);
                }
	
		/*  Check that AID is numeric and <= 2,147,483,647  */

                len = (delim[2]-delim[1])-1;
                if (len == 0) {
                        error(S_ERROR4);
                }

                if(len > 10) {
                        error(S_ERROR4);
                }
                else {
			for (i=(delim[1]+1); i < delim[2]; i++) {
                            if(!(isdigit(buf[i]))) {
                                    error(S_ERROR4);
                                    break;
                            }
                            aid = aid*10+(buf[i])-'0';
                        }
                        if(aid > 2147483647  ||  aid < 0l) {
                            error(S_ERROR4);
                        }
		}

		/*  Check that audit flag is numeric and 0 or 1 */

		aid = 0l;	/* use aid for audflg also */
		for(i=(delim[2]+1); i < delim[3]; i++) {
                        if(!(isdigit(buf[i]))) {
                                error(S_ERROR5);
                                break;
                        }
                        aid = aid*10+(buf[i])-'0';
                }
                if(aid > 1 ||  aid < 0l) {
                     error(S_ERROR5);
                }
	  }
	}
	if (secure)
		fclose(sptr);
	exit(code);
}
/*  Error printing routine  */

error(msg)

char *msg;
{
	if(!(eflag)) {
		fprintf(stderr,"\n%s",buf);
		code = 1;
		++eflag;
	}
	if(!(badc)) {
	fprintf(stderr,"\t%s\n",msg);
	return;
	}
	else {
	fprintf(stderr,"\t%d %s\n",badc,msg);
	badc=0;
	return;
	}
}
