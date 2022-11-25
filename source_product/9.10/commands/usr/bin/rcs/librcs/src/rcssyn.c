/*
 *                     RCS file input
 *
 *  $Header: rcssyn.c,v 66.1 90/11/14 14:47:04 ssa Exp $ Purdue CS
 */
/******************************************************************************
 *                       Syntax Analysis.
 *                       Keyword table
 *                       Testprogram: define SYNDB
 *                       Compatibility with Release 2: define COMPAT2
 ******************************************************************************
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

/*
#define COMPAT2
/* version COMPAT2 reads files of the format of release 2 and 3, but
 * generates files of release 3 format. Need not be defined if no
 * old RCS files generated with release 2 exist.
 */
/*
#define SYNDB
/* version SYNDB is for debugging the syntax analysis for RCS files.
 * SYNDB performs additional error checks.
 */
/*
#define SYNTEST
/* version SYNTEST inputs a RCS file and then prints out its internal
 * data structures.
*/

#include <stdio.h>
#include "rcsbase.h"
extern FILE * finptr;        /*RCS input file*/
extern char * getid();
extern struct hshentry * getnum();
extern int    getkey();
extern int    getlex();
extern char * malloc();
extern        readstring();
extern int    savestring();

/* forward */
char * getkeyval();
int delta();

/* keyword table */

char * Kaccess     = "access";
char * Kauthor     = "author";
char * Kbranches   = "branches";
char * Kcomment    = "comment";
char * Kdate       = "date";
char * Kdesc       = "desc";
char * Khead       = "head";
char * Klocks      = "locks";
char * Klog        = "log";
char * Knext       = "next";
char * Kstate      = "state";
char * Kstrict     = "strict";
char * Ksuffix     = "suffix";
char * Ksymbols    = "symbols";
char * Ktext       = "text";

#define COMMLENGTH 20
char              Commleader[COMMLENGTH];
char            * Comment    = "";
struct access   * AccessList =nil;
struct access   * LastAccess =nil;
struct assoc    * Symbols    =nil;
struct assoc    * LastSymbol =nil;
struct lock     * Locks      =nil;
struct lock     * LastLock   =nil;
int               StrictLocks=false;
struct hshentry * Head       =nil;
int               TotalDeltas=0;



getadmin()
/* Function: Reads an <admin> and initializes the globals
 * AccessList, LastAccess, Symbols, LastSymbol,
 * Locks, LastLock, StrictLocks, Head, Comment, TotalDeltas;
 */
{
        register char   * id;
        struct access   * newaccess;
        struct assoc    * newassoc;
        struct lock     * newlock;
        struct hshentry * delta;

        Comment="";
        AccessList=LastAccess=nil;
        Symbols=LastSymbol=nil;
        Locks=LastLock=nil;
        Head = nil;
        TotalDeltas=0;

        if (!getkey(Khead)) fatserror("Missing head");
        Head=getnum();
#       ifdef SYNDB
        if (Head&&((countnumflds(Head->num)%2)!=0))
                serror("Delta number required for head");
#       endif
        if (!getlex(SEMI)) serror("Missing ';' after head");

#ifdef COMPAT2
        /* read suffix. Only in release 2 format */
        if (getkey(Ksuffix)) {
                if (nexttok==STRING) {
                        readstring(); nextlex(); /*through away the suffix*/
                } elsif(nexttok==ID) {
                        nextlex();
                }
                if (!getlex(SEMI)) serror1s("Missing ';' after %s",Ksuffix);
        }
#endif

        if (!getkey(Kaccess)) fatserror("Missing access list");
        while (id=getid()) {
                newaccess = (struct access *)malloc(sizeof(struct access));
                newaccess->login = id;
                newaccess->nextaccess = nil;
                if (AccessList == nil) {
                        AccessList=LastAccess=newaccess;
                } else {
                        LastAccess=LastAccess->nextaccess=newaccess;
                }
        }
        if (!getlex(SEMI)) serror("Missing ';' after access list");

        if (!getkey(Ksymbols)) fatserror("Missing symbols");
        while (id = getid()) {
                if (!getlex(COLON))
                        serror("Missing ':' in symbolic name definition");
                if (!(delta=getnum())) {
                        serror("Missing number in symbolic name definition");
                } else { /*add new pair to association list*/
                        newassoc=(struct assoc *)malloc(sizeof(struct assoc));
                        newassoc->symbol=id;
                        newassoc->delta=delta;
                        newassoc->nextassoc=nil;
                        if (Symbols == nil) {
                                Symbols=LastSymbol=newassoc;
                        } else {
                                LastSymbol=LastSymbol->nextassoc=newassoc;
                        }
                }
        }
        if (!getlex(SEMI)) serror("Missing ';' after symbolic names");

        if (!getkey(Klocks)) serror("Missing locks");
        while (id = getid()) {
                if (!getlex(COLON))
                        serror("Missing ':' in lock");
                if (!(delta=getnum())) {
                        serror("Missing number in lock");
                } else { /*add new pair to lock list*/
#                       ifdef SYNDB
                        if ((countnumflds(delta->num)%2)!=0)
                                serror("Delta number required for lock");
#                       endif
                        newlock=(struct lock *)malloc(sizeof(struct lock));
                        newlock->login=id;
                        newlock->delta=delta;
                        newlock->nextlock=nil;
                        if (Locks == nil) {
                                Locks=LastLock=newlock;
                        } else {
                                LastLock=LastLock->nextlock=newlock;
                        }
                }
        }
        if (!getlex(SEMI)) serror("Missing ';' after locks");
        if (!getkey(Kstrict)) {
                StrictLocks = false;
        } else {
                StrictLocks = true;
                if (!getlex(SEMI)) serror1s("Missing ';' after keyword %s",Kstrict);
        }
        if (getkey(Kcomment) && (nexttok==STRING)) {
                savestring(Commleader,COMMLENGTH);nextlex();
                Comment=Commleader;
                if (!getlex(SEMI)) serror1s("Missing ';' after %s",Kcomment);
        }
}



getdelta()
/* Function: reads a delta block.
 * returns false if the current block does not start with a number.
 */
{
        register struct hshentry * Delta, * num;
        struct branchhead * LastBranch, * NewBranch;

        if (!(Delta=getnum())) return false;
#       ifdef SYNDB
        if ((countnumflds(Delta->num)%2)!=0)
                serror("Delta number required");
#       endif

        hshenter = false; /*Don't enter dates into hashtable*/
        Delta->date = getkeyval(Kdate, NUM, false);
        hshenter=true;    /*reset hshenter for revision numbers.*/

        Delta->author = getkeyval(Kauthor, ID, false);

        Delta->state = getkeyval(Kstate, ID, true);

        if (!getkey(Kbranches)) fatserror("Missing branches");
        Delta->branches = LastBranch=nil;
        while (num=getnum()) {
#               ifdef SYNDB
                if ((countnumflds(num->num)%2)!=0)
                        serror("Delta number required");
#               endif
                NewBranch = (struct branchhead *)malloc(sizeof(struct branchhead));
                NewBranch->hsh = num;
                NewBranch->nextbranch = nil;
                if (LastBranch == nil) {
                        Delta->branches=LastBranch=NewBranch;
                } else {
                        LastBranch=LastBranch->nextbranch=NewBranch;
                }
        }
        if (!getlex(SEMI)) serror("Missing ';' after branches");

        if (!getkey(Knext)) fatserror("Missing next");
        Delta->next=num=getnum();
#       ifdef SYNDB
        if (num&&((countnumflds(num->num)%2)!=0))
                serror("Delta number required");
#       endif
        if (!getlex(SEMI)) serror("Missing ';' after next");
        Delta->log=Delta->lockedby = nil;
        Delta->selector = '\0';
        TotalDeltas++;
        return (true);
}


gettree()
/* Function: Reads in the delta tree with getdelta(), then
 * updates the lockedby fields. Returns the total number of deltas read.
 */
{       struct lock * currlock;
        while (getdelta());
        currlock=Locks;
        while (currlock) {
                currlock->delta->lockedby = currlock->login;
                currlock = currlock->nextlock;
        }
        return TotalDeltas;
}


getdesc(prdesc)
int  prdesc;
/* Function: read in descriptive text
 * nexttok is not advanced afterwards.
 * if prdesc==true, the text is printed to stdout.
 */
{

        if (!getkey(Kdesc) || (nexttok!=STRING)) fatserror("Missing descriptive text");
        if (prdesc)
                printstring();  /*echo string*/
        else    readstring();   /*skip string*/
}






char * getkeyval(keyword, token, optional)
enum tokens token; char * keyword; int optional;
/* reads a pair of the form
 * <keyword> <token> ;
 * where token is one of <id> or <num>. optional indicates whether
 * <token> is optional. A pointer to
 * the acutal character string of <id> or <num) is returned.
 * Getkeyval terminates the program on missing keyword or token, continues
 * on missing ;.
 */
{
        register char * val;

        if (!getkey(keyword)) {
                fatserror1s("Missing %s", keyword);
        }
        if (nexttok==token) {
                val = NextString;
                nextlex();
        } else {
                if (!optional) {fatserror1s("Missing %s", keyword); }
                else val = nil;
        }
        if (!getlex(SEMI)) serror1s("Missing ';' after %s",keyword);
        return(val);
}




putadmin(fout)
register FILE * fout;
/* Function: Print the <admin> node read with getadmin() to file fout.
 * Assumption: Variables AccessList, Symbols, Locks, StrictLocks,
 * and Head have been set.
 */
{       struct assoc  * curassoc;
        struct lock   * curlock;
        struct access * curaccess;
        register char * sp;

        fputs(Khead,fout); fputs("     ",fout);
        if (Head) fputs(Head->num,fout);

        fprintf(fout,";\n%s  ",Kaccess);
	if (ferror(fout)) faterror("write error");
        curaccess = AccessList;
        if (curaccess==nil) putc(' ',fout);
        while (curaccess) {
               putc(' ',fout);
               fputs(curaccess->login,fout);
               curaccess = curaccess->nextaccess;
        }
        fprintf(fout,";\n%s ",Ksymbols);
	if (ferror(fout)) faterror("write error");
        curassoc = Symbols;
        if (curassoc==nil) putc(' ',fout);
        while (curassoc) {
               fprintf(fout," %s:%s",curassoc->symbol, curassoc->delta->num);
	       if (ferror(fout)) faterror("write error");
               curassoc = curassoc->nextassoc;
        }
        fprintf(fout,";\n%s   ",Klocks);
	if (ferror(fout)) faterror("write error");
        curlock = Locks;
        if (curlock==nil) putc(' ',fout);
        while (curlock) {
               fprintf(fout," %s:%s",curlock->login, curlock->delta->num);
	       if (ferror(fout)) faterror("write error");
               curlock = curlock->nextlock;
        }
        if (StrictLocks) 
		{
		fprintf(fout,"; %s",Kstrict);
		if (ferror(fout)) faterror("write error");
		}
        fprintf(fout,";\n%s  %c",Kcomment,SDELIM);
	if (ferror(fout)) faterror("write error");
        if((sp=Comment)!=nil) {
               while (*sp) if (putc(*sp++,fout)==SDELIM) putc(SDELIM,fout);
        }
        fprintf(fout,"%c;\n\n",SDELIM);
	if (ferror(fout)) faterror("write error");
}




putdelta(node,fout)
register struct hshentry * node;
register FILE * fout;
/* Function: prints a <delta> node to fout;
 */
{      struct branchhead * nextbranch;

        if (node == nil) return;

        fprintf(fout,"\n%s\n",node->num);
	if (ferror(fout)) faterror("write error");
        fprintf(fout,"%s     %s;  %s %s;  %s ",
                Kdate,node->date,Kauthor,node->author,Kstate);
	if (ferror(fout)) faterror("write error");
        if (node->state!=nil) fputs(node->state,fout);
        fputs(";\nbranches",fout);
        nextbranch = node->branches;
        if (nextbranch==nil) putc(' ',fout);
        while (nextbranch) {
               putc(' ',fout);
               fputs(nextbranch->hsh->num,fout);
               nextbranch = nextbranch->nextbranch;
        }

        fprintf(fout,";\n%s     ",Knext);
	if (ferror(fout)) faterror("write error");
        if (node->next!=nil) fputs(node->next->num,fout);
        fputs(";\n",fout);

}




puttree(root,fout)
struct hshentry * root;
register FILE * fout;
/* Function: prints the delta tree in preorder to fout, starting with root.
 */
{       struct branchhead * nextbranch;

        if (root==nil) return;

        if (root->selector !=DELETE)putdelta(root,fout);
        /* selector DELETE means deleted; set by rcs -o */

        puttree(root->next,fout);

        nextbranch = root->branches;
        while (nextbranch) {
             puttree(nextbranch->hsh,fout);
             nextbranch = nextbranch->nextbranch;
        }
}



int putdtext(num,log,srcfilename,fout)
char * num, * log, * srcfilename; FILE * fout;
/* Function: write a deltatext-node to fout.
 * num points to the deltanumber, log to the logmessage, and
 * sourcefile contains the text. Doubles up all SDELIMs in both the
 * log and the text; Makes sure the log message ends in \n.
 * returns false on error.
 */
{
        register char * sp;
	register int c;
        register FILE * fin;

        fprintf(fout,DELNUMFORM,num,Klog);
	if (ferror(fout)) faterror("write error");
        /* put log */
        putc(SDELIM,fout);
        sp=log;
	while (*sp) if (putc(*sp++,fout)==SDELIM) putc(SDELIM,fout);
	if (*(sp-1)!='\n') putc('\n', fout); /*append \n if necessary*/
	/* put text */
	fprintf(fout, "%c\n%s\n%c",SDELIM,Ktext,SDELIM);
	if (ferror(fout)) faterror("write error");
	setruser();
	if ((fin=fopen(srcfilename,"r"))==NULL) {
		seteuser();
		error1s("Can't open source file %s",srcfilename);
		return false;
	}
	seteuser();
        while ((c=fgetc(fin))!=EOF) {
                if (c==SDELIM) putc(SDELIM,fout);   /*double up SDELIM*/
                putc(c,fout);
        }
        putc(SDELIM,fout);putc('\n',fout);
        fclose(fin);
        return true;
}
