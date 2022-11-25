/*
 *                     RCS file name handling
 *
 * $Header: rcsfnms.c,v 70.1 93/09/06 17:43:31 ssa Exp $ Purdue CS
 */
/****************************************************************************
 *                     creation and deletion of semaphorefile,
 *                     creation of temporary filenames and cleanup()
 *                     pairing of RCS file names and working file names.
 *                     Testprogram: define PAIRTEST
 ****************************************************************************
 *
 * Copyright (C) 1982 by Walter F. Tichy
 *                       Purdue University
 *                       Computer Science Department
 *                       West Lafayette, IN 47907
 *
 * All rights reserved. No part of this software may be sold or distributed
 * in any form or by any means without the prior written permission of the
 * author.
 * Report problems and direct all inquiries to Tichy@purdue (ARPA net).
 */

#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "system.h"
#include "dir.h"
#include "rcsbase.h"

extern char * rindex();
extern char * pathncat();	/* concatenate path names		   */
extern void free();
extern char * mktemp();
extern char * malloc();
extern FILE * fopen();
extern char * getcwd();         /* get working directory		*/
extern char * strsav();
extern char * getdirlink();	/*  get directory links */
char * bindex();			/*  forward declaration */
extern int errno;		/* system error codes */
extern FILE * finptr;          /* RCS input file descriptor                 */
extern FILE * frewrite;        /* New RCS file descriptor                   */
extern char * RCSfilename, * workfilename; /* filenames                     */

char *RCSdirname = NULL;	/* RCS directory w/directory links resolved */
char tempfilename[NCPFN+NCPFN];	/*  used for derived file names             */
char subfilename[NCPPN];	/*  used for files RCS/file.sfx,v           */
char semafilename [NCPPN];     /* name of semaphore file                    */
int  madesema;                 /* indicates whether a semaphore file has been set */
char * tfnames[10] =           /* temp. file names to be unlinked when finished   */
       {nil,nil,nil,nil,nil,nil,nil,nil,nil,nil};
int  lastfilename         = -1;/* index of last file name in tfnames[]      */


struct compair {
        char * suffix, * comlead;
};

struct compair comtable[] = {
/* comtable pairs each filename suffix with a comment leader. The comment   */
/* leader is placed before each line generated by the $Log keyword. This    */
/* table is used to guess the proper comment leader from the working file's */
/* suffix during initial ci (see InitAdmin()). Comment leaders are needed   */
/* for languages without multiline comments; for others they are optional.  */
        "c",   " * ",   /* C           */
        "h",   " * ",   /* C-header    */
        "p",   " * ",   /* pascal      */
        "sh",  "# ",    /* shell       */
        "s",   "# ",    /* assembler   */
        "r",   "# ",    /* ratfor      */
        "e",   "# ",    /* efl         */
        "l",   " * ",   /* lex         NOTE: conflict between lex and franzlisp*/
        "y",   " * ",   /* yacc        */
        "yr",  " * ",   /* yacc-ratfor */
        "ye",  " * ",   /* yacc-efl    */
        "ml",  "; ",    /* mocklisp    */
        "mac", "; ",    /* macro       vms or dec-20 or pdp-11 macro */
        "f",   "c ",    /* fortran     */
        "ms",  "\\\" ", /* ms-macros   t/nroff*/
        "me",  "\\\" ", /* me-macros   t/nroff*/
        "",    "# ",    /* default for empty suffix */
        nil,   ""       /* default for unknown suffix; must always be last */
};



ffclose(fptr)
FILE * fptr;
/* Function: checks ferror(fptr) and aborts the program if there were
 * errors; otherwise closes fptr.
 */
{       if (ferror(fptr) || fclose(fptr)==EOF)
                faterror("File read or write error; file system full?");
}



int trysema(RCSfilename,makesema)
char * RCSfilename; int makesema;
/* Function: Checks whether a semaphore file exists for RCSfilename. If yes,
 * returns false. If not, creates one if makesema==true and returns true
 * if successful. If a semaphore file was created, madesema is set to true.
 * The name of the semaphore file is put into variable semafilename.
 */
{
        register char * tp, *sp, *lp;
        int fdesc;

        sp=RCSfilename;
        lp = rindex(sp,'/');
        if (lp==0) {
                semafilename[0]='.'; semafilename[1]='/';
                tp= &semafilename[2];
        } else {
                /* copy path */
                tp=semafilename;
                do *tp++ = *sp++; while (sp<=lp);
        }
        /*now insert `,' and append file name */
        *tp++ = ',';
        lp = rindex(sp, RCSSEP);
        while (sp<lp) *tp++ = *sp++;
        *tp++ = ','; *tp++ = '\0'; /* will be the same length as RCSfilename*/

        madesema = false;
        if (access(semafilename, 0) == 0) {
                error1s("RCS file %s is in use",RCSfilename);
                return false;
        }
        if (makesema) {
                if ((fdesc=open(semafilename, O_WRONLY|O_CREAT|O_EXCL, 000)) == -1) {
			if (errno == EEXIST)
				error1s("RCS file %s is in use",RCSfilename);
			else
				error1s("Can't create semaphore file for RCS file %s",RCSfilename);
			return false;
                }
		close(fdesc);
		madesema = true;
        }
        return true;
}


int rmsema()
/* Function: delete the semaphore file if madeseam==true;
 * sets madesema to false.
 */
{
        if (madesema) {
                madesema=false;
                if (unlink(semafilename) == -1) {
                        error1s("Can't find semaphore file %s",semafilename);
                        return false;
                }
        }
        return true;
}



InitCleanup()
{       lastfilename =  -1;  /* initialize pointer */
}


cleanup()
/* Function: closes input file and rewrite file.
 * Unlinks files in tfnames[], deletes semaphore file.
 */
{
        register int i;

        if (finptr!=NULL)   fclose(finptr);
        if (frewrite!=NULL) fclose(frewrite);
	setruser();
        for (i=0; i<=lastfilename; i++) {
            if (tfnames[i][0]!='\0')  unlink(tfnames[i]);
        }
	seteuser();
        InitCleanup();
        return (rmsema());
}


char * mktempfile(fullpath,filename)
register char * fullpath, * filename;
/* Function: Creates a unique filename using the process id and stores it
 * into a free slot in tfnames. The filename consists of the path contained
 * in fullpath concatenated with filename. filename should end in "XXXXXX".
 * Because of storage in tfnames, cleanup() can unlink the file later.
 * lastfilename indicates the highest occupied slot in tfnames.
 * Returns a pointer to the filename created.
 * Example use: mktempfile("/tmp/", somefilename)
 */
{
        register char * lastslash, *tp;
        lastfilename++;
        if ((tp=tfnames[lastfilename])==nil)
              tp=tfnames[lastfilename] = malloc(NCPPN);
        if (fullpath!=nil && (lastslash=rindex(fullpath,'/'))!=0) {
                /* copy path */
                while (fullpath<=lastslash) *tp++ = *fullpath++;
        }
        while (*tp++ = *filename++);
        return (mktemp(tfnames[lastfilename]));
}




char * bindex(sp,c)
register char * sp, c;
/* Function: Finds the last occurrence of character c in string sp
 * and returns a pointer to the character just beyond it. If the
 * character doesn't occur in the string, sp is returned.
 */
{       register char * r;
        r = sp;
        while (*sp) {
                if (*sp++ == c) r=sp;
        }
        return r;
}



InitAdmin()
/* function: initializes an admin node */
{       register char * Suffix;
        register int i;

	Head=nil; AccessList=nil; Symbols=nil; Locks=nil;
        StrictLocks=STRICT_LOCKING;

        /* guess the comment leader from the suffix*/
        Suffix=bindex(workfilename, '.');
        if (Suffix==workfilename) Suffix= ""; /* empty suffix; will get default*/
        for (i=0;;i++) {
                if (comtable[i].suffix==nil) {
                        Comment=comtable[i].comlead; /*default*/
                        break;
                } elsif (strcmp(Suffix,comtable[i].suffix)==0) {
                        Comment=comtable[i].comlead; /*default*/
                        break;
                }
        }
        Lexinit(); /* Note: if finptr==NULL, reads nothing; only initializes*/
}

char * findpairfile(argc, argv, fname, findworkfile)
int argc, findworkfile; char * argv[], *fname;
/* Function: Given a filename fname, findpairfile scans argv for a pathname
 * ending in fname. If found, returns a pointer to the pathname, and sets
 * the corresponding pointer in argv to nil. Otherwise returns fname.
 * argc indicates the number of entries in argv. Some of them may be nil.
 *
 * 1/28/86 - changed search for pairs to match a truncated RCSfile name to
 * its corresponding (untruncated) workfile name.
 */
{
        register char * * next, * lastsep, * match;
        register int count;

        for (next = argv, count = argc; count>0; next++,count--) {
                if (*next != nil) {
                        /* bindex finds the beginning of the file name stem */
			match = bindex(*next,'/');
			/* Ignore RCS files when looking for a working file */
			if (findworkfile) {
				lastsep = rindex( match, RCSSEP);
				if ( lastsep != 0 &&
					 *(lastsep+1) == RCSSUF &&
					 *(lastsep+2) == '\0' )
					continue;
			}
			/* Now see if we have a working file name */
			if (strcmp(match, fname) == 0) {
				match = *next;
				*next = nil;
				return match;
			}
			else if ((strlen(match) >= NAME_MAX-2) &&
			     (strlen(fname) >= NAME_MAX-2) &&
			     (strncmp(match, fname, NAME_MAX-2) == 0)) {
				match = *next;
				*next = nil;
				return match;
			}
		}
        }
        return fname;
}

int pairfilenames(argc, argv, mustread, tostdout)
int argc; char ** argv; int mustread, tostdout;
/* Function: Pairs the filenames pointed to by argv; argc indicates
 * how many there are.
 * Places a pointer to the RCS filename into RCSfilename,
 * and a pointer to the name of the working file into workfilename.
 * If both the workfilename and the RCS filename are given, and tostdout
 * is true, a warning is printed.
 * If RCSfilename is toolong(), it is regenerated by truncating it to NAME_MAX-2 and
 * re-appending ",v" and a warning message is sent to "stderr".
 *
 * If the RCS file exists, it is opened for reading, the file pointer
 * is placed into finptr, and the admin-node is read in; returns 1.
 * If the RCS file does not exist and mustread==true, an error is printed
 * and 0 returned.
 * If the RCS file does not exist and mustread==false, the admin node
 * is initialized to empty (Head, AccessList, Locks, Symbols, StrictLocks),
 * and -1 returned.
 *
 * 0 is returned on all errors.
 * Also calls InitCleanup();
 */
{
        register char * tp;
        char * lastsep, * purefname, * pureRCSname;
        int opened, returncode;
        char * RCS1;
	char *longRCSname;
	char Err_Str[BUFSIZ];	        /* Error Reporting string  */
	struct stat filestatus;		/* for checking file type */

        if (*argv == nil) return 0; /* already paired filename */

        InitCleanup();

        /* first check suffix to see whether it is an RCS file or not */
        purefname=bindex(*argv, '/'); /* skip path */
        lastsep=rindex(purefname, RCSSEP);
        if (lastsep!= 0 && *(lastsep+1)==RCSSUF && *(lastsep+2)=='\0') {
                /* RCS file name given*/
                RCS1=(*argv); pureRCSname=purefname;
                /* derive workfilename*/
		strcpy(tempfilename, purefname);
		tp = rindex(tempfilename, RCSSEP);
		*tp = '\0';	/*  convert RCSfilename to workfilename * /
                /* try to find workfile name among arguments */
                workfilename=findpairfile(argc-1,argv+1,tempfilename,true);
        } else {
                /* working file given; now try to find RCS file */
                workfilename= *argv;
                /* derive RCS file name*/
		strcat(strcpy(tempfilename, purefname), RCSFILESUF);
                /* Try to find RCS file name among arguments*/
                RCS1=findpairfile(argc-1,argv+1,tempfilename,false);
                pureRCSname=bindex(RCS1, '/');
        }

        /* now we have a (tentative) RCS filename in RCS1 and workfilename  */
	/* Run toolong if given full pathname on command line, else wait    */
	/* until we have expanded the RCS files/links.                      */

        /* in addition to calling toolong() (which fails over a read only   */
        /* filesystem), use access() to deterimine if file exists           */

        if (pureRCSname!=RCS1) {
           if (access(RCS1, F_OK)) {
              if ( toolong(RCS1)) {
                 /* error out */
                 sprintf(Err_Str,
                         "RCS filename, %s, is too long. Please resolve",
                         RCS1);
                 faterror(Err_Str);
              }
	   }
        }

	/* stat local file only if tostdout is false */
	if (!tostdout) {
	  if (stat(workfilename, &filestatus) == 0 )
	    {
	    if ( !(filestatus.st_mode & S_IFREG) )
	      {
	      error1s("%s is not a regular file", workfilename);
	      return 0;
	      }
	    }
	  }
	if (stat(RCS1, &filestatus) == 0 )
	  {
	  if ( !(filestatus.st_mode & S_IFREG) )
	    {
	    error1s("%s is not a regular file", RCS1);
	    return 0;
	    }
	  }

        if (pureRCSname!=RCS1) {
                /* a path for RCSfile is given; single RCS file to look for */
                finptr=fopen(RCSfilename=RCS1, "r");
                if (finptr!=NULL) {
                    Lexinit(); getadmin();
                    returncode=1;
                } else { /* could not open */
                    if (access(RCSfilename,0)==0) {
                        error1s("Can't open existing %s", RCSfilename);
                        return 0;
                    }
                    if (mustread) {
                        error1s("Can't find %s", RCSfilename);
                        return 0;
                    } else {
                        /* initialize if not mustread */
                        InitAdmin();
                        returncode = -1;
                    }
                }
        } else {
                /* build second RCS file name by prefixing it with */
                /* RCSDIR or the resolved directory link from */
		/* RCSDIR then try to open one of them */

		if (RCSdirname == NULL)
			RCSdirname = getdirlink(RCSDIR);

		if (pathncat(strcpy(subfilename, RCSdirname),
                             RCS1,
                             sizeof(subfilename)) == NULL)
			faterror("pathname too long");
		
		/* now we can check if expanded filename is toolong for FS */

                if (access(subfilename, F_OK)) {
                   if ( toolong(subfilename) ) {
                      /* error out */
                      sprintf(Err_Str,
                              "RCS filename, %s, is too long. Please resolve",
                              RCS1);
                      faterror(Err_Str);
                   }
                }

		/* this one has to be a regular file, too! */
		if ( stat(subfilename, &filestatus) == 0 )
		  {
		  if ( !(filestatus.st_mode & S_IFREG) )
		    {
		    error1s("%s is not a regular file", subfilename);
		    return 0;
		    }
		  }

                opened=(
                ((finptr=fopen(RCSfilename=subfilename, "r"))!=NULL) ||
                ((finptr=fopen(RCSfilename=RCS1,"r"))!=NULL) );

                if (opened) {
                        /* open succeeded */
                        Lexinit(); getadmin();
                        returncode=1;
                } else {
                        /* open failed; may be read protected */
                        if ((access(RCSfilename=subfilename,0)==0) ||
                            (access(RCSfilename=RCS1,0)==0)) {
                                error1s("Can't open existing %s",RCSfilename);
                                return 0;
                        }
                        if (mustread) {
				if (strcmp(subfilename,RCS1)==0)
                                     error2s("Can't find %s",subfilename);
				else
                                     error2s("Can't find %s nor %s",subfilename,RCS1);
                                return 0;
                        } else {
                                /* Initialize new file.  Put into RCS */
				/* directory if possible.             */
				/* Strip off suffix.		      */

                                RCSfilename= (access(RCSDIR,0)==0)?subfilename:RCS1;
                                InitAdmin();
                                returncode= -1;
                                }
                        }
                }
                if (tostdout && !(no_pipe&tostdout) && /* footnote 1 */
                    !(RCS1==tempfilename||workfilename==tempfilename))
                        /*The last term determines whether a pair of        */
                        /* file names was given in the argument list        */
                        warn1s("Option -p is set; ignoring output file %s",workfilename);

        return returncode;
}


char * getfullRCSname()
/* Function: returns a pointer to the full path name of the RCS file.
 * Calls getcwd(), but only once.
 * removes leading "../" and "./".
 */
{       static char pathbuf[NCPPN];
        static char namebuf[NCPPN];
        static int  pathlength =0;

        register char * realname, * lastpathchar;
        register int  dotdotcounter, realpathlength;

        if (*RCSfilename=='/') {
                return(RCSfilename);
        } else {
                if (pathlength==0) { /*call curdir for the first time*/
                    if (getcwd(pathbuf, sizeof(pathbuf))==NULL)
                        faterror("Can't build current directory path");
                    pathlength=strlen(pathbuf);
                    if (!((pathlength==1) && (pathbuf[0]=='/'))) {
                        pathbuf[pathlength++]='/';
                        /* Check needed because some getcwd implementations */
                        /* generate "/" for the root.                      */
                    }
                }
                /*the following must be redone since RCSfilename may change*/
                /* find how many ../ to remvove from RCSfilename */
                dotdotcounter =0;
                realname = RCSfilename;
                while( realname[0]=='.' &&
                      (realname[1]=='/'||(realname[1]=='.'&&realname[2]=='/'))){
                        if (realname[1]=='/') {
                            /* drop leading ./ */
                            realname += 2;
                        } else {
                            /* drop leading ../ and remember */
                            dotdotcounter++;
                            realname += 3;
                        }
                }
                /* now remove dotdotcounter trailing directories from pathbuf*/
                lastpathchar=pathbuf + pathlength-1;
                while (dotdotcounter>0 && lastpathchar>pathbuf) {
                    /* move pointer backwards over trailing directory */
                    lastpathchar--;
                    if (*lastpathchar=='/') {
                        dotdotcounter--;
                    }
                }
                if (dotdotcounter>0) {
                    error("Can't generate full path name for RCS file");
                    return RCSfilename;
                } else {
                    /* build full path name */
                    realpathlength=lastpathchar-pathbuf+1;
                    strncpy(namebuf,pathbuf,realpathlength);
                    strcpy(&namebuf[realpathlength],realname);
                    return(namebuf);
                }
        }
}



int trydiraccess(filename)
char * filename;
/* checks write permission in directory of filename and returns
 * true if writable, false otherwise
 */
{
        char pathname[NCPPN];
        register char * tp, *sp, *lp;
        lp = rindex(filename,'/');
        if (lp==0) {
                /* check current directory */
		if (eaccess(".",2)==0)
                        return true;
                else {
                        error("Current directory not writable");
                        return false;
                }
        }
        /* copy path */
        sp=filename;
        tp=pathname;
        do *tp++ = *sp++; while (sp<=lp);
        *tp='\0';
	if (eaccess(pathname,2)==0)
                return true;
        else {
                error1s("Directory %s not writable", pathname);
                return false;
        }
}

int eaccess(name,mode)              /* same semantics as access, except */
char *name;                         /* use effective uid and gid.       */
int mode;
{
    struct stat buf;
    int euid;
    int modetest;
	int i, ngroups, foundgid, gidset[NGROUPS];

    mode &= 07;

    if (stat(name, &buf) != 0)      /* if stat fails, interpret that */
	return(-1);                 /* as "does not exist"           */

    euid = geteuid();

    if (mode == 0 || euid == 0)
	return(0);

    modetest = mode << 6;
    if (buf.st_uid == euid)         /* if owner of file */
	if ((buf.st_mode & modetest) == modetest)
	    return(0);
	else                        /* if owner does not have   */
	    return(-1);             /* permission, then fail    */

    /* check the group access list and effective gid */
    modetest = mode << 3;
    if ( !(foundgid=(buf.st_gid == getegid())) ) { /* if growner of file */
	  ngroups = getgroups(NGROUPS, gidset);     /* or in graccess list */
	  for (i = 0, foundgid = 0; i < ngroups; i++ )
		if ( foundgid = (gidset[i] == buf.st_gid))
			break;
	}

	if (foundgid)
	if ((buf.st_mode & modetest) == modetest)
	    return(0);
	else                        /* if growner does not have */
	    return(-1);             /* permission, then fail    */
				    /* also.                    */

    if ((buf.st_mode & mode) == mode)           /* if stranger */
	return(0);
    else
	return(-1);
}

/* footnote 1
 * The rcs command has no -p option, so the "no_pipe" flag prevents the rcs
 * command from printing the warning.  It would be okay for the rcs command to
 * print the warning message since it ignores the workfile, but it doesn't use
 * the -p option to do this.  To keep the error message the same for all the
 * rcs commands, maintain GNU compatibility and prevent the rcs command
 * from emitting a warning message, the "no_pipe" flag was added.
 */
