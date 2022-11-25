/*  $Revision: 66.5 $
 *
 *                     RCS checkout operation
 */
/*****************************************************************************
 *                       check out revisions from RCS files
 *****************************************************************************
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "system.h"
#include "bin.h"
#include "rcsbase.h"
#include "time.h"
#include "copyright.h"

extern FILE * fopen();
extern char * getancestor();
extern void rcssigs();
extern void rcsholdsigs();
extern void rcsallowsigs();
extern int    RENAME();
extern char * malloc();
extern struct hshentry * genrevs(); /*generate delta numbers                */
extern int check_in;		       /*check-in operation flag (true/false)*/
extern int  nextc;                  /*next input character                  */
extern int  nerror;                 /*counter for errors                    */
extern char * Kdesc;                /*keyword for description               */
extern char * maketempfile();       /*temporary file name                   */
extern char * buildrevision();      /*constructs desired revision           */
extern int    buildjoin();          /*join several revisions                */
extern char * getuser();	    /*get current user name (login)	    */
extern char * mktempfile();         /*temporary file name generator         */
extern struct lock * addlock();     /*add a new lock                        */
extern long   maketime();           /*convert parsed time to unix time.     */
extern struct tm * gmtime();        /*convert unixtime into a tm-structure  */
extern int execstdout();	    /* execute a command & redirect stdout  */
extern char * rindex();
extern int StrictLocks;
extern FILE * finptr;               /* RCS input file                       */
extern FILE * frewrite;             /* new RCS file                         */

int neutral = 2;
char * RCSfilename, * workfilename;
char * newRCSfilename, * neworkfilename;
int    rewriteflag; /* indicates whether input should be echoed to frewrite */

char * date, *rev[NUM_REVS], * state, * author, * join;
char finaldate[datelength];

int lockflag, tostdout, expandflag;
char * caller;                        /* caller's login;                    */
extern quietflag;

char numericrev[revlength];           /* holds expanded revision number     */
struct hshentry * gendeltas[hshsize]; /* stores deltas to be generated      */
struct hshentry * targetdelta;        /* final delta to be generated        */

char * joinlist[joinlength];          /* pointers to revisions to be joined */
int lastjoin;                         /* index of last element in joinlist  */

main (argc, argv)
int argc;
char * argv[];
{
        register c;
        char * cmdusage;
        struct stat RCSstat;
        struct tm parseddate, *ftm;
        char * rawdate;
        long unixtime;
	int revs_indx = 0;
	int rv_exists;
	register i;

	numericrev[0] = '\0';
	rcssigs();
        cmdid = "co";
	check_in = false;
        cmdusage = "command format:\nco -l[rev] -p[rev] -q[rev] -r[rev] -ddate -sstate -w[login] -jjoinlist file ...";
        date =  state = author = join = nil;
        lockflag = tostdout = quietflag = false;
	expandflag = true;
        caller=getuser();

        while (--argc,++argv, argc>=1 && ((*argv)[0] == '-')) {
                switch ((*argv)[1]) {

                case 'l':
                        lockflag=true;
                case 'r':
                revno:  if ((*argv)[2]!='\0') {
                                rev[revs_indx++] = (*argv)+2;
                        }
                        break;

                case 'p':
                        tostdout=true;
                        goto revno;

                case 'q':
                        quietflag=true;
                        goto revno;

                case 'd':
                        if ((*argv)[2]!='\0') {
                                if (date!=nil) warn("Redefinition of -d option");
                                rawdate=(*argv)+2;
                        }
                        /* process date/time */
                        if (partime(rawdate,&parseddate)==0)
                                faterror1s("Can't parse date/time: %s",rawdate);
                        if ((unixtime=maketime(&parseddate))== 0L)
                                faterror1s("Inconsistent date/time: %s",rawdate);
                        ftm=gmtime(&unixtime);
                        sprintf(finaldate,DATEFORM,
                        ftm->tm_year,ftm->tm_mon+1,ftm->tm_mday,ftm->tm_hour,ftm->tm_min,ftm->tm_sec);
                        date=finaldate;
                        break;

                case 'j':
                        if ((*argv)[2]!='\0'){
                                if (join!=nil)warn("Redefinition of -j option");
                                join = (*argv)+2;
                        }
                        break;

                case 's':
                        if ((*argv)[2]!='\0'){
                                if (state!=nil)warn("Redefinition of -s option");
                                state = (*argv)+2;
                        }
                        break;

                case 'w':
                        if (author!=nil)warn("Redefinition of -w option");
                        if ((*argv)[2]!='\0')
                                author = (*argv)+2;
                        else    author = caller;
                        break;

                case 'z':
			expandflag=neutral;
                        goto revno;

                default:
                        faterror2s("unknown option: %s\n%s", *argv,cmdusage);

                };
        } /* end of option processing */

        if (argc<1) faterror1s("No input file\n%s",cmdusage);

        /* now handle all filenames */
        do {
        rewriteflag=false;
        finptr=frewrite=NULL;
        neworkfilename=nil;

        if (!pairfilenames(argc,argv,true,tostdout)) continue;

        /* now RCSfilename contains the name of the RCS file, and finptr
         * the file descriptor. If tostdout is false, workfilename contains
         * the name of the working file, otherwise undefined (not nil!).
         */
        diagnose2s("%s  -->  %s", RCSfilename,tostdout?"stdout":workfilename);

        fstat(fileno(finptr),&RCSstat); /* get file status, esp. the mode  */

	setruser();
        if (!tostdout && !trydiraccess(workfilename)) continue; /* give up */
	seteuser();
	if (lockflag && !trydiraccess(RCSfilename)) continue;   /* give up */
        if (lockflag && !checkaccesslist(caller)) continue;     /* give up */
        if (!trysema(RCSfilename,lockflag)) continue;           /* give up */


        gettree();  /* reads in the delta tree */

        if (Head==nil) {
                /* no revisions; create empty file, or do nothing if tostdout */
                diagnose("no revisions present; generating empty revision 0.0");
                if (!tostdout)
			{
			if (!creatempty(workfilename)) continue;
			}
                /* Can't reserve a delta, so don't call addlock */
        } else {
		/* step thru all revisions to find first that exists, if any */
		i = 0;
		do {
                /* expand symbolic revision number */
			if ((rv_exists = expandsym(rev[i],numericrev)) == true)
				break;
			if (i < (revs_indx -1))
				nerror--;
			i++;
		} while (i < revs_indx);
			
/*		for (i = 0; i < revs_indx; i++) {	*/
                /* expand symbolic revision number */
		/*	if ((rv_exists = expandsym(rev[i],numericrev)) == true)
				break;
			if (i < (revs_indx -1))
				nerror--;
		}	*/
		if (!rv_exists)
			continue;
                /* get numbers of deltas to be generated */
                if (!(targetdelta=genrevs(numericrev,date,author,state,gendeltas)))
                        continue;
                /* check reservations */
                if (lockflag && !addlock(targetdelta,caller))
                        continue;

                if (join && !preparejoin()) continue;

                diagnose2s("revision %s %s",targetdelta->num,
                         lockflag?"(locked)":"");

                /* remove old working file if necessary */
                if (!tostdout)
                        if (!rmoldfile(workfilename)) continue;

                /* prepare for rewriting the RCS file */
                if (lockflag) {
                        newRCSfilename=mktempfile(RCSfilename,NEWRCSFILE);
                        if ((frewrite=fopen(newRCSfilename, "w"))==NULL) {
                                error1s("Can't open file %s",newRCSfilename);
                                continue;
                        }
                        putadmin(frewrite);
                        puttree(Head,frewrite);
                        fprintf(frewrite, "\n\n%s%c",Kdesc,nextc);
			if (ferror(frewrite)) faterror("write error");
                        rewriteflag=true;
                }

                /* skip description */
                getdesc(false); /* don't echo*/

                if (!(neworkfilename=buildrevision(gendeltas,targetdelta,
                      tostdout?(join!=nil?"/tmp/":nil):workfilename,expandflag)))
                                continue;

                if (lockflag&&nerror==0) {
                        /* rewrite the rest of the RCSfile */
                        fastcopy(finptr,frewrite);
                        ffclose(frewrite); frewrite=NULL;
			rcsholdsigs();
                        if (RENAME(newRCSfilename,RCSfilename)<0) {
                                error2s("Can't rewrite %s; saved in: %s",
                                RCSfilename, newRCSfilename);
                                newRCSfilename[0]='\0'; /* avoid deletion*/
				rcsallowsigs();
                                break;
                        }
                        newRCSfilename[0]='\0'; /* avoid re-deletion by cleanup()*/
                        if (chmod(RCSfilename,RCSstat.st_mode & ~0222)<0)
                            warn1s("Can't preserve mode of %s",RCSfilename);
			rcsallowsigs();
                }

                logcommand("co",targetdelta,gendeltas,caller);

                if (join) {
                        rmsema(); /* kill semaphore file so other co's can proceed */
                        if (!buildjoin(neworkfilename,tostdout)) continue;
                }
                if (!tostdout) {
			setruser();
                        if (link(neworkfilename,workfilename) <0) {
				seteuser();
                                error2s("Can't create %s; see %s",workfilename,neworkfilename);
                                neworkfilename[0]= '\0'; /*avoid deletion*/
                                continue;
                        }
			seteuser();
		}
        }
	if (!tostdout) {
	    setruser();
            if (chmod(workfilename, WORKMODE(RCSstat.st_mode))<0)
                warn1s("Can't adjust mode of %s",workfilename);
	    seteuser();
	}

        if (!tostdout) diagnose("done");
        } while (cleanup(),
                 ++argv, --argc >=1);

        exit(nerror!=0);

}       /* end of main (co) */


/*****************************************************************
 * The following routines are auxiliary routines
 *****************************************************************/

int rmoldfile(ofile)
char * ofile;
/* Function: unlinks ofile, if it exists, under the following conditions:
 * If the file is read-only, file is unlinked.
 * Otherwise (file writable):
 *   if !quietmode asks the user whether to really delete it (default: fail);
 *   otherwise failure.
 * Returns false on failure to unlink, true otherwise.
 */
{
        int response, c;    /* holds user response to queries */
        struct stat buf;

        if (stat (ofile, &buf) < 0)         /* File doesn't exist */
            return (true);                  /* No problem         */

        if (buf.st_mode & 0222) {            /* File is writable */
            if (!quietflag) {
                fprintf(stderr,"writable %s exists; overwrite? [ny](n): ",ofile);
                /* must be stderr in case of IO redirect */
                c=response=getchar();
                while (!(c==EOF || c=='\n')) c=getchar(); /*skip rest*/
                if (!(response=='y'||response=='Y')) {
                        warn("checkout aborted.");
                        return false;
                }
            } else {
                error1s("writable %s exists; checkout aborted.",ofile);
                return false;
            }
        }
        /* now unlink: either not writable, or permission given */
	setruser();
        if (unlink(ofile) != 0) {            /* Remove failed   */
	    seteuser();
            error1s("Can't unlink %s",ofile);
            return false;
        }
	seteuser();
        return true;
}


creatempty(file)
char * file;
/* Function: creates an empty file named file.
 * Removes an existing file with the same name with rmoldfile().
 */
{
        int  fdesc;              /* file descriptor */

        if (!rmoldfile(file)) return false;
	setruser();
        fdesc=creat(file,0666);
	seteuser();
        if (fdesc < 0) {
                faterror1s("Cannot create %s",file);
                return false;
        } else {
                close(fdesc); /* empty file */
                return true;
        }
}



/*****************************************************************
 * The rest of the routines are for handling joins
 *****************************************************************/

char * getrev(sp, tp, buffsize)
register char * sp, *tp; int buffsize;
/* Function: copies a symbolic revision number from sp to tp,
 * appends a '\0', and returns a pointer to the character following
 * the revision number; returns nil if the revision number is more than
 * buffsize characters long.
 * The revision number is terminated by space, tab, comma, colon,
 * semicolon, newline, or '\0'.
 * used for parsing the -j option.
 */
{
        register char c;
        register int length;

        length = 0;
        while (((c= *sp)!=' ')&&(c!='\t')&&(c!='\n')&&(c!=':')&&(c!=',')
                &&(c!=';')&&(c!='\0')) {
                if (length>=buffsize) return false;
                *tp++= *sp++;
                length++;
        }
        *tp= '\0';
        return sp;
}



int preparejoin()
/* Function: Parses a join list pointed to by join and places pointers to the
 * revision numbers into joinlist.
 */
{
        struct hshentry * (* joindeltas)[];
        struct hshentry * tmpdelta;
        register char * j;
        char symbolrev[revlength],numrev[revlength];

        joindeltas = (struct hshentry * (*)[])malloc(hshsize*sizeof(struct hshentry *));
        j=join;
        lastjoin= -1;
        for (;;) {
                while ((*j==' ')||(*j=='\t')||(*j==',')) j++;
                if (*j=='\0') break;
                if (lastjoin>=joinlength-2) {
                        error("too many joins");
                        return(false);
                }
                if(!(j=getrev(j,symbolrev,revlength))) return false;
                if (!expandsym(symbolrev,numrev)) return false;
                tmpdelta=genrevs(numrev,nil,nil,nil,joindeltas);
                if (tmpdelta==nil)
                        return false;
                else    joinlist[++lastjoin]=tmpdelta->num;
                while ((*j==' ') || (*j=='\t')) j++;
                if (*j == ':') {
                        j++;
                        while((*j==' ') || (*j=='\t')) j++;
                        if (*j!='\0') {
                                if(!(j=getrev(j,symbolrev,revlength))) return false;
                                if (!expandsym(symbolrev,numrev)) return false;
                                tmpdelta=genrevs(numrev,nil,nil,nil,joindeltas);
                                if (tmpdelta==nil)
                                        return false;
                                else    joinlist[++lastjoin]=tmpdelta->num;
                        } else {
                                error("join pair incomplete");
                                return false;
                        }
                } else {
                        if (lastjoin==0) { /* first pair */
                                /* common ancestor missing */
                                joinlist[1]=joinlist[0];
                                lastjoin=1;
                                /*derive common ancestor*/
                                joinlist[0]=malloc(revlength);
                                if (!getancestor(targetdelta->num,joinlist[1],joinlist[0]))
                                       return false;
                        } else {
                                error("join pair incomplete");
                                return false;
                        }
                }
        }
        if (lastjoin<1) {
                error("empty join");
                return false;
        } else  return true;
}



buildjoin(initialfile, tostdout)
char * initialfile; int tostdout;
/* Function: merge pairs of elements in joinlist into initialfile
 * If tostdout==true, copy result to stdout.
 * All unlinking of initialfile, rev2, and rev3 should be done by cleanup().
 */
{
	char command[NCPPN+80];
	char *co = CO;
        char *cmd_args[8];	 /* holds co & merge command arguments	    */
	char p_parm[NCPPN];
        char subs[revlength];
        char * rev2, * rev3;
        int i;

        rev2=mktempfile("/tmp/",JOINFIL2);
        rev3=mktempfile("/tmp/",JOINFIL3);

        i=0;
        while (i<lastjoin) {
                /*prepare marker for merge*/
                if (i==0)
                        strcpy(subs,targetdelta->num);
                else    sprintf(subs, "merge%d",i/2);
                diagnose1s("revision %s",joinlist[i]);
		if ((cmd_args[0] = rindex(co, '/')) != NULL) cmd_args[0]++;
		else cmd_args[0] = co;
		cmd_args[1] = strcat(strcpy(p_parm, "-p"), joinlist[i]);
		cmd_args[2] = "-q";
		cmd_args[3] = RCSfilename;
		cmd_args[4] = NULL;
		if (execstdout(co, cmd_args, rev2)) {
                        nerror++;return false;
                }
                diagnose1s("revision %s",joinlist[i+1]);
		if ((cmd_args[0] = rindex(co, '/')) != NULL) cmd_args[0]++;
		else cmd_args[0] = co;
		cmd_args[1] = strcat(strcpy(p_parm, "-p"), joinlist[i+1]);
		cmd_args[2] = "-q";
		cmd_args[3] = RCSfilename;
		cmd_args[4] = NULL;
		if (execstdout(co, cmd_args, rev3)) {
                        nerror++; return false;
                }
                diagnose("merging...");
                sprintf(command,"%s %s%s %s %s %s %s\n", MERGE,
                        ((i+2)>=lastjoin && tostdout)?"-p ":"",
                        initialfile,rev2,rev3,subs,joinlist[i+1]);
                if (system(command)) {
                        nerror++; return false;
                }
                i=i+2;
        }
        return true;
}
