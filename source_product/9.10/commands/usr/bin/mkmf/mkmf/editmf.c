/* $Header: editmf.c,v 70.1 91/08/08 11:11:09 ssa Exp $ */

/*
 * Author: Peter J. Nicklin
 */
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include "Mkmf.h"
#include "dlist.h"
#include "hash.h"
#include "macro.h"
#include "null.h"
#include "slist.h"
#include "system.h"
#include "yesno.h"

#if SEQ68K
#   include <malloc.h>
#endif

static char *Mftemp;			/* temporary makefile */

/*
 * editmf() replaces macro definitions within a makefile.
 */
void
editmf(mfname, mfpath)
	char *mfname;			/* makefile name */
	char *mfpath;			/* makefile template pathname */
{
	register char *bp;		/* buffer pointer */
	extern char IOBUF[];		/* I/O buffer line */
	extern int DEPEND;		/* dependency analysis? */
	extern SLIST *EXTLIST;		/* external header file name list */
	extern SLIST *HEADLIST;		/* header file name list */
	extern SLIST *LIBLIST;		/* library pathname list */
	extern SLIST *SRCLIST;		/* source file name list */
	extern SLIST *SYSLIST;		/* system header file name list */
	extern HASH *MDEFTABLE;		/* macro definition table */
	char *findmacro();		/* is the line a macro definition? */
	char *getlin();			/* get a line from input stream */
	char *mktemp();			/* make file name */
	char mnam[MACRONAMSIZE];	/* macro name buffer */
	DLIST *dlp;			/* dependency list */
	DLIST *mkdepend();		/* generate object-include file deps */
	FILE *ifp;			/* input stream */
	FILE *mustfopen();		/* must open file or die */
	FILE *ofp;			/* output stream */
	HASHBLK *htb;			/* hash table block */
	HASHBLK *htlookup();		/* find hash table entry */
	void cleanup();			/* remove temporary makefile and exit */
	void dlprint();			/* print dependency list */
	void purgcontinue();		/* get rid of continuation lines */
	void putmacro();		/* put macro defs from table */
	void putlin();			/* put a makefile line */
	void putobjmacro();		/* put object file name macro def */
	void putslmacro();		/* put macro defs from linked list */
#if SEQ68K
	extern int system();            /* There is no system call rename on
					 * the series 1200, so use the system 
					 * call with a /bin/mv.
					 */
	char *sys_string;
	int sys_err_stat;
	unsigned str_len;
#endif

	ifp = mustfopen(mfpath, "r");
	Mftemp = mktemp("mkmfXXXXXX");

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		{
		signal(SIGINT, cleanup);
		signal(SIGHUP, cleanup);
		signal(SIGQUIT, cleanup);
		}

	ofp = mustfopen(Mftemp, "w");
	if (DEPEND)
		{
		dlp = mkdepend();
		}

	while (getlin(ifp) != NULL)
		{
		if (DEPEND && EQUAL(IOBUF, DEPENDMARK))
			break;
		for (bp = IOBUF; *bp == ' '; bp++)
			continue;
		if (isalnum(*bp) && findmacro(mnam, bp) != NULL)
			{
			if (EQUAL(mnam, MHEADERS))
				{
				putslmacro(HEADLIST, ofp);
				purgcontinue(ifp);
				}
			else if (EQUAL(mnam, MOBJECTS))
				{
				putobjmacro(ofp);
				purgcontinue(ifp);
				}
			else if (EQUAL(mnam, MSOURCES))
				{
				putslmacro(SRCLIST, ofp);
				purgcontinue(ifp);
				}
			else if (EQUAL(mnam, MSYSHDRS))
				{
				putslmacro(SYSLIST, ofp);
				purgcontinue(ifp);
				}
			else if (EQUAL(mnam, MEXTERNALS))
				{
				if (DEPEND)
					{
					putslmacro(EXTLIST, ofp);
					purgcontinue(ifp);
					}
				else	{
					putlin(ofp);
					}
				}
			else if (EQUAL(mnam, MLIBLIST) && LIBLIST != NULL)
				{
				putslmacro(LIBLIST, ofp);
				purgcontinue(ifp);
				}
			else if ((htb = htlookup(mnam, MDEFTABLE)) != NULL)
				{
				if (htb->h_val == VREADWRITE)
					{
					putmacro(htb->h_def, ofp);
					purgcontinue(ifp);
					}
				else	{
					putlin(ofp);
					}
				}
			else	{
				putlin(ofp);
				}
			}
		else	{
			putlin(ofp);
			}
		}
	fclose(ifp);
	if (DEPEND)
		{
		dlprint(dlp, ofp);
		}
	fclose(ofp);

	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

#if SEQ68K
        str_len = strlen(Mftemp) + strlen(mfname) + 9;
	sys_string=malloc(str_len);
	if (sys_string == 0) {
	   pperror("Unable to malloc space for file rename.\n");
	   exit(1);
	   }
	sprintf(sys_string, "%s%s%s%s","/bin/mv ", Mftemp, " ", mfname);
	sys_err_stat=system(sys_string);
	if (sys_err_stat != 0) {
	   pperror("Unable to rename temp file.\n");
	   exit(1);
	   }
	free(sys_string);
#else
	RENAME(Mftemp, mfname);
#endif
}



/*
 * cleanup() removes the temporary makefile and dependency file, and
 * calls exit(1).
 */
void
cleanup(sig)
	int sig;			/* value of signal causing cleanup */
{
	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGQUIT, cleanup);

	unlink(Mftemp);
	exit(1);
}
