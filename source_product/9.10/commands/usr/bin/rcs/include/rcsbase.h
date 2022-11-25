/*
 *                     RCS common definitions and data structures
 */
#define RCSBASE "$Header: rcsbase.h,v 70.1 93/09/06 17:43:36 ssa Exp $"
/*****************************************************************************
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

/* $Log:	rcsbase.h,v $
 * Revision 70.1  93/09/06  17:43:36  17:43:36  ssa
 * Author: glenn@hpucsb2.cup.hp.com
 * fixes DSDe412076 to allow the rcs command to act on a file/file,v filename
 * without it checking to see if the workfile ("file" in this case) is a
 * regular file.  rcs ignore the workfile, so no check is required.
 * 
 * Revision 66.1  89/09/18  10:42:46  10:42:46  smp
 * increased the value of hshsize from 239 to 719.  This is a temporary
 * solution to the problem of rcs commands running out of space in their
 * hash table for revisions from an rcs file.  Hopefully, I will get a chance
 * to re-implement the hashing algorithm so these limits won't be 
 * obvious.
 * 
 * Revision 51.1  87/06/15  16:27:57  16:27:57  ajf
 * Added NUM_REVS.  This determines the maximum number of revisions that
 * co checks when processing multiple -r arguments.
 * 
 * Revision 4.5  86/06/05  08:25:54  08:25:54  bob
 * Changed "logsize" to 2048 to be compatible with ITG.
 * 
 * Revision 4.4  86/04/17  14:45:40  14:45:40  bob (Bob Boothby)
 * Changed definition of NCPPN to 1024 to be sure pathname buffers will be
 * big enough even if the file is on another system.  Changed definition
 * of RCSDIR to "RCS".  Also deleted static char array copyright, since it
 * is now defined in header file "copyright.h".
 * 
 * Revision 4.3  86/02/24  09:26:34  09:26:34  bob (Bob Boothby)
 * Defined RCSSIGMASK, used in "reliable signal" modules.
 * 
 * Revision 4.2  86/01/28  14:51:17  14:51:17  bob (Bob Boothby)
 * Defined RCSFILESUF for use by RCSfile name truncatation changes in
 * pairfilenames() & findpairfile() - in file rcsfnms.c
 * 
 * Revision 4.1  85/08/14  15:48:51  15:48:51  scm (Software Configuration Management)
 * Forced revision for a place to store state=HP_UXRel2
 * 
 * Revision 4.0  85/07/19  10:28:55  10:28:55  bob ( Bob Boothby)
 * Restructured RCS into a SPMS project.  System dependent defines etc. that
 * were in rcsbase.h have been moved to a new header file "system.h".  The
 * installation instructions have been moved to the project README file.
 * 
 */

#undef putc         /* will be redefined */

#define PRINTDATE(file,date) fprintf(file,"%.2s/%.2s/%.2s",date,date+3,date+6)
#define PRINTTIME(file,date) fprintf(file,"%.2s:%.2s:%.2s",date+9,date+12,date+15)
/* print RCS format date and time in nice format from a string              */

/*
 * Parameters
 */

#define STRICT_LOCKING      1 /* 0 sets the default locking to non-strict;  */
                              /* used in experimental environments.         */
                              /* 1 sets the default locking to strict;      */
                              /* used in production environments.           */
#define hshsize           719 /* hashtable size; MUST be prime and -1 mod 4 */
                              /* some choices: 239, 547, 719                */
#define strtsize (hshsize * 50) /* string table size                        */
#define logsize           2048 /* size of logmessage                         */
#define revlength          30 /* max. length of revision numbers            */
#define datelength         20 /* length of a date in RCS format             */
#define joinlength         20 /* number of joined revisions permitted       */
#define NCPPN		1024  /* number of characters per pathname	    */
#define RCSDIR          "RCS" /* subdirectory for RCS files                 */
#define RCSSUF            'v' /* suffix for RCS files                       */
#define RCSSEP            ',' /* separator for RCSSUF                       */
#define RCSFILESUF	",v"  /* RCS file suffix			    */
#define RCSSIGMASK	   0L /* RCS signal mask - allow all signals        */
#define KDELIM            '$' /* delimiter for keywords                     */
#define VDELIM            ':' /* separates keywords from values             */
#define DEFAULTSTATE    "Exp" /* default state of revisions                 */
#define keylength          20 /* buffer length for expansion keywords       */
#define keyvallength NCPPN+revlength+datelength+60
                              /* buffer length for keyword expansion        */
#define NUM_REVS	20    /* number of revisions allowed on command line */	

#define true     1
#define false    0
#define nil      0
#define elsif    else if
#define elif     else if
#define no_pipe  0x2            /* indicates that no -p option was specified */

/* temporary file names */

#define NEWRCSFILE  ",RCSnewXXXXXX"
#define DIFFILE     ",RCSciXXXXXX"
#define TMPFILE1    ",RCSt1XXXXXX"
#define TMPFILE2    ",RCSt2XXXXXX"
#define TMPFILE3    ",RCSt3XXXXXX"
#define JOINFIL2    ",RCSj2XXXXXX"
#define JOINFIL3    ",RCSj3XXXXXX"

#define putc(x,p) (--(p)->_cnt>=0? ((int)(*(p)->_ptr++=(unsigned)(x))):fflsbuf((unsigned)(x),p))
/* This version of putc prints a char, but aborts on write error            */

#define GETC(in,out,echo) (echo?putc(getc(in),out):getc(in))
/* GETC writes a del-character (octal 177) on end of file                   */

#define WORKMODE(RCSmode) (RCSmode&~0222)|((lockflag||!StrictLocks)?0600:0000)
/* computes mode of working file: same as RCSmode, but write permission     */
/* determined by lockflag and StrictLocks.                                  */

/* character classes and token codes */
enum tokens {
/* char classes*/  DIGIT, IDCHAR, NEWLN, LETTER, PERIOD, SBEGIN, SPACE, UNKN,
/* tokens */       COLON, DATE, EOFILE, ID, KEYW, NUM, SEMI, STRING,
};

#define AT      SBEGIN  /* class SBEGIN (string begin) is returned by lex. anal. */
#define SDELIM  '@'     /* the actual character is needed for string handling*/
/* these must be changed consistently, for instance to:
 * #define DQUOTE       SBEGIN
 * #define SDELIM       '"'
 * #define AT           IDCHAR
 * there should be no overlap among SDELIM, KDELIM, and VDELIM
 */

/* other characters */

#define ACCENT   IDCHAR
#define AMPER    IDCHAR
#define BACKSL   IDCHAR
#define BAR      IDCHAR
#define COMMA    UNKN
#define DIVIDE   IDCHAR
#define DOLLAR   IDCHAR
#define DQUOTE   IDCHAR
#define EQUAL    IDCHAR
#define EXCLA    IDCHAR
#define GREAT    IDCHAR
#define HASH     IDCHAR
#define INSERT   UNKN
#define LBRACE   IDCHAR
#define LBRACK   IDCHAR
#define LESS     IDCHAR
#define LPARN    IDCHAR
#define MINUS    IDCHAR
#define PERCNT   IDCHAR
#define PLUS     IDCHAR
#define QUEST    IDCHAR
#define RBRACE   IDCHAR
#define RBRACK   IDCHAR
#define RPARN    IDCHAR
#define SQUOTE   IDCHAR
#define TILDE    IDCHAR
#define TIMES    IDCHAR
#define UNDER    IDCHAR
#define UPARR    IDCHAR

/***************************************
 * Data structures for the symbol table
 ***************************************/

/* Hash table entry */
struct hshentry {
        char              * num;      /* pointer to revision number (ASCIZ) */
        char              * date;     /* pointer to date of checking        */
        char              * author;   /* login of person checking in        */
        char              * lockedby; /* who locks the revision             */
        char              * log;      /* log message requested at checkin   */
        char              * state;    /* state of revision (Exp by default) */
        struct branchhead * branches; /* list of first revisions on branches*/
        struct hshentry   * next;     /* next revision on same branch       */
        int                 insertlns;/* lines inserted (computed by rlog)  */
        int                 deletelns;/* lines deleted  (computed by rlog)  */
        char                selector; /* marks entry for selection/deletion */
};

/* list element for branch lists */
struct branchhead {
        struct hshentry   * hsh;
        struct branchhead * nextbranch;
};

/* accesslist element */
struct access {
        char              * login;
        struct access     * nextaccess;
};

/* list element for locks  */
struct lock {
        char              * login;
        struct hshentry   * delta;
        struct lock       * nextlock;
};

/* list element for symbolic names */
struct assoc {
        char              * symbol;
        struct hshentry   * delta;
        struct assoc      * nextassoc;
};

/* common variables (getadmin and getdelta())*/
extern char            * Comment;
extern struct access   * AccessList;
extern struct assoc    * Symbols;
extern struct lock     * Locks;
extern struct hshentry * Head;
extern int               StrictLocks;
extern int               TotalDeltas;
/* common variables (lexical analyzer)*/
extern enum tokens map[];
#define ctab (&map[1])
extern struct hshentry   hshtab[];
extern struct hshentry * nexthsh;
extern enum tokens       nexttok;
extern int               hshenter;
extern char            * NextString;
extern char            * cmdid;

/* common routines */
extern int serror();
extern int faterror();
extern int fatserror();

/*
 * Markers for keyword expansion (used in co and ident)
 */
#define AUTHOR          "Author"
#define DATE            "Date"
#define HEADER          "Header"
#define LOCKER          "Locker"
#define LOG             "Log"
#define REVISION        "Revision"
#define SOURCE          "Source"
#define STATE           "State"

enum markers { Nomatch, Author, Date, Header,
               Locker, Log, Revision, Source, State };

#define DELNUMFORM      "\n\n%s\n%s\n"
/* used by putdtext and scanlogtext */
#define DELETE          'D'
/* set by rcs -o and used by puttree() in rcssyn */
