/* @(#) $Revision: 70.11 $ */

#include <stdio.h>
#include <errno.h>
#ifdef NLS
# include <nl_ctype.h>
#endif

#ifndef PWB
# define PWB   1
#endif

#ifndef _PWB
# define _PWB  1 
#endif

#ifdef NLS
# define NL_SETN 1   /* set number */
# include <nl_types.h>
# include <locale.h>
# include <msgbuf.h>
  nl_catd nl_fn;
#else
# define catgets(i,mn,sn,s) (s)
#endif

/* C command written by John F. Reiser July/August 1978 */

#define BOOLEAN unsigned int
#define FALSE 0
#define TRUE 1


#define STATIC

#define READ  "r"
#define WRITE "w"
#define SALT  '#'

#define LINEBUFSIZE 4096
#define NAMESIZE 255

extern int errno;

int ncps = 255;       /* variable for name size */

#ifdef OSF
#define LINESIZE 80
static int linesize;
BOOLEAN ShowDependents = FALSE;
BOOLEAN ShowFiles      = FALSE;
#endif /* OSF */

char *pbeg, *pbuf, *pend, *outp,*inp, *newp, cinit;

#ifdef CXREF
   char       *xcopy();
   FILE       *outfp;
   static int  ready = 0;
   int         xline;
   int         doCXREFdata = 0;
#endif

/* Some code depends on whether characters are sign or zero extended */
/* #if '\377' < 0  not used here, old cpp doesn't understand */

#if defined (hpux) || defined (_HPUX_SOURCE) || defined (OSF)
# define COFF 128
#else
# define COFF   0
#endif

#define ALFSIZ 256   /* alphabet size */

char macbit [ALFSIZ + 11];
char toktyp [ALFSIZ];

#define BLANK 1
#define IDENT 2
#define NUMBR 3

/* a superimposed code is used to reduce the number of calls to the
/* symbol table lookup routine.  (if the kth character of an identifier
/* is 'a' and there are no macro names whose kth character is 'a'
/* then the identifier cannot be a macro name, hence there is no need
/* to look in the symbol table.)  'scw1' enables the test based on
/* single characters and their position in the identifier.  'scw2'
/* enables the test based on adjacent pairs of characters and their
/* position in the identifier.  scw1 typically costs 1 indexed fetch,
/* an AND, and a jump per character of identifier, until the identifier
/* is known as a non-macro name or until the end of the identifier.
/* scw1 is inexpensive.  scw2 typically costs 4 indexed fetches,
/* an add, an AND, and a jump per character of identifier, but it is also
/* slightly more effective at reducing symbol table searches.
/* scw2 usually costs too much because the symbol table search is
/* usually short; but if symbol table search should become expensive,
/* the code is here.
/* using both scw1 and scw2 is of dubious value.
*/
#define scw1 1
#define scw2 0

#if scw2
   char t21 [ALFSIZ], t22 [ALFSIZ], t23 [ALFSIZ + 8];
#endif

#if scw1
#  define b0   1
#  define b1   2
#  define b2   4
#  define b3   8
#  define b4  16
#  define b5  32
#  define b6  64
#  define b7 128
#endif

#define IB  1
#define SB  2
#define NB  4
#define CB  8
#define QB 16
#define WB 32

char fastab [ALFSIZ], slotab [ALFSIZ], *ptrtab;

#define isslo     (ptrtab == (slotab + COFF))
#define isspc(a)  ((ptrtab       ) [a] & SB)
#define isid(a)   ((fastab + COFF) [a] & IB)
#define isnum(a)  ((fastab + COFF) [a] & NB)
#define iscom(a)  ((fastab + COFF) [a] & CB)
#define isquo(a)  ((fastab + COFF) [a] & QB)
#define iswarn(a) ((fastab + COFF) [a] & WB)

#define eob(a) ((a) >= pend)
#define bob(a) (pbeg >= (a))

char buffer [NAMESIZE + LINEBUFSIZE + LINEBUFSIZE + NAMESIZE];

/* increased from 24000 with Sys5.2, possibly linked to long names? */
/* The SBSIZE must be an even number of 32 bit words.               */

# define SBSIZE 128000
char *sbf;             /* now allocated in init(), by malloc */
int   sbsize = SBSIZE; /* real size, can be changed with -H  */
char *savch;

#if defined(hpux) || defined(_HPUX_SOURCE) || defined(OSF)
#  define DROP '\177' /* special character not used in HP-15 */
#else
#  define DROP '\376' /* special character not legal ASCII or EBCDIC */
#endif

#define WARN DROP
#define SAME 0

/*  increased from 10 with Sys5.2 changes */

# define MAXINC 36 /* max nested includes */
# define MAXFRE 42 /* max buffers of macro pushback */
# define MAXFRM 127 /* max number of formals/actuals to a macro */

static char warnc = WARN;

int mactop, fretop;
char *instack [MAXFRE], *bufstack [MAXFRE], *endbuf [MAXFRE];

int   plvl;    /* parenthesis level during scan for macro actuals */
int   maclvl;  /* # calls since last decrease in nesting level */
int   maclin;  /* line number of macro call requiring actuals */
char *macfil;  /* file name of macro call requiring actuals */
char *macnam;  /* name of macro requiring actuals */
char *macforw; /* pointer which must be exceeded to decrease nesting level */
int   macdam;  /* offset to macforw due to buffer shifting */
#ifdef PROTOGEN
unsigned char protogenflag = 0;
#endif /* PROTOGEN */
#if defined ( tgp)
   int tgpscan;   /* flag for dump(); */
#endif

STATIC int   inctop  [MAXINC];
STATIC char *fnames  [MAXINC];
STATIC char *dirnams [MAXINC]; /* actual directory of #include files */
STATIC FILE *fins    [MAXINC]; /* was int before Sys5.2 */
STATIC int   lineno  [MAXINC];
#ifdef OSF
STATIC char *sys_dirs    [MAXINC]; /* -I and/or -y and <> directories */
STATIC char *pri_dirs    [MAXINC]; /* -I and/or -p and <> directories */
#else
STATIC char *dirs    [MAXINC]; /* -I and <> directories */
#endif /* OSF */

char *copy(), *subst(), *trmdir(), *strchr(), *getenv();

STATIC FILE *fin  = stdin;
STATIC FILE *fout = stdout;
#ifdef OSF
STATIC int  sys_nd   = 1;
STATIC int  pri_nd   = 1;
#else
STATIC int  nd   = 1;
#endif /* OSF */
STATIC int  ifno;
STATIC int  pflag;   /* don't put out lines "# 12 foo.c" */
STATIC int  passcom; /* don't delete comments */
STATIC int  incomm;  /* set while in comment so that EOF is caught */
STATIC int  rflag;   /* allow macro recursion */
STATIC int  posflag; /* provide POSIX's clean namespace */

#if defined (C_PLUS_PLUS)
STATIC int   c_plus_plus; /* treat "//" as comment-to-EOL */
STATIC int   in_define;   /* for handling "//" comments in #defines */
#endif  /* C_PLUS_PLUS */

#define NPREDEF 180

STATIC short   jseen; /* flag to indicate the J option was parsed */
STATIC char   *prespc [NPREDEF];
STATIC char  **predef = prespc;
STATIC char   *punspc [NPREDEF];
STATIC char  **prund = punspc;
STATIC int     exfail;

struct symtab {
   char   *name;
   char   *value;
} *lastsym, *lookup(), *slookup(), *stsym();

struct symtab *stab; /* now allocated by init()  */

#define SYMSIZ 6400   
int symsiz  = SYMSIZ;
#define LINEFORM1 "# %d \"%s\"\n"
#define LINEFORM2 "# %d\n"
#define ERRFORM  "*%c*   \"%s\", line "

STATIC struct symtab *defloc;
STATIC struct symtab *udfloc;
STATIC struct symtab *incloc;
STATIC struct symtab *ifloc;
STATIC struct symtab *elsloc;
STATIC struct symtab *elifloc;
STATIC struct symtab *eifloc;
STATIC struct symtab *ifdloc;
STATIC struct symtab *ifnloc;
STATIC struct symtab *ysysloc;
STATIC struct symtab *varloc;
STATIC struct symtab *lneloc;
STATIC struct symtab *ulnloc;
STATIC struct symtab *uflloc;
STATIC struct symtab *clsloc;
STATIC struct symtab *idtloc;
STATIC struct symtab *pragloc;

STATIC int trulvl, flslvl, iflvl;
#if defined (STAT_ANAL) /* Code to support excalibur static analysis. */
int emit_SA   = 0; /* Remembers if option was present. */
FILE *SA_file     = (FILE *)0;
char *SA_filename = (char *)0;
int predefs_done  = 0;

      /* Remembers where a macro name is defined, used, had actuals, etc: */
struct {
      char *name;       /* Macro name; assumed that it isn't freed!. */
      char *start;      /* Remembers address where last newline char is. */
      char *last_fname; /* File name to print; '@' if same as last time. */
      char *fname;      /* File   where macro name is defined or used. */
      int   line;       /* Line     "     "     "  "     "     "   "   */
      int   col;        /* Column   "     "     "  "     "     "   "   */
   } sa;

   /* These set values for some of the above members: */

#  define SA_MACRO_NAME(s) (predefs_done ? (sa.name  = (s)) : 0)
#  define SA_LINE_START(p) (predefs_done ? (sa.start = (p)) : 0)
#  define SA_MACRO_LINE(l) (predefs_done ? (sa.line  = (l)) : 0)
#  define SA_MACRO_COL(p)  (predefs_done ? (sa.col   = (p) - sa.start + 1) : 0)

   /* Print macro info to the SA_file like so:
    *    kind macro_name file_name line column
    * where kind is one of the characters D, U, A, L, and E; meaning 
    * definition, use, actual name, local name inside definition, and end of 
    * the macro's expanded text.  (only supporting D, U, and A for now).
    * Print '@' if the file name is the same as the last one. */

#  define SA_FILENAME() \
      (sa.fname = (fnames[ifno] == sa.last_fname) \
         ? "@" \
         : (sa.last_fname = fnames[ifno]))

#  define SA_MACRO_PRINT(kindch) \
      ((predefs_done && sa.name) \
         ? (fprintf (SA_file, "%c %s \"%s\" %d %d\n", \
                  kindch, sa.name, SA_FILENAME(), sa.line, sa.col)) \
         : 0)

   extern void sa_macro_use_in_if (name)
         char                     *name;
   {
      SA_MACRO_NAME(name);
      SA_MACRO_LINE(lineno[ifno]);
      SA_MACRO_COL (name);
      SA_MACRO_PRINT('U');
   }

#else

#  define SA_LINE_START(p)
#  define SA_MACRO_COL(p)
#  define SA_MACRO_NAME(s)
#  define SA_MACRO_LINE(l)
#  define SA_MACRO_PRINT(kind)

#endif /* STAT_ANAL */

sayline() 
{
static char *last_fname = 0;
char *fp;
#ifdef PROTOGEN
if(protogenflag)
  return;
#endif /* PROTOGEN */
if (last_fname != fnames [ifno])
  {
  last_fname = fnames [ifno];
  fp = last_fname;
#ifdef CXREF
  if(doCXREFdata && (*fp == '.') && (*(fp+1) == '/'))
    fp += 2;
#endif
  if (pflag == 0) 
     fprintf (fout, LINEFORM1, lineno [ifno], fp);
  }
else
  {
  if (pflag == 0) 
     fprintf (fout, LINEFORM2, lineno [ifno]            );
  }
}
/* data structure guide
/*
/* most of the scanning takes place in the buffer:
/*
/* (low address)                                              (high address)
/* pbeg                              pbuf                               pend
/* |    <-- LINEBUFSIZE chars -->      |    <-- LINEBUFSIZE chars -->      |
/*  _______________________________________________________________________
/* |_______________________________________________________________________|
/*          |               |               |
/*          |<-- waiting -->|               |<-- waiting -->
/*          |    to be      |<-- current -->|    to be
/*          |    written    |    token      |    scanned
/*          |               |               |
/*          outp            inp             p
/*
/*  *outp   first char not yet written to output file
/*  *inp    first char of current token
/*  *p      first char not yet scanned
/*
/* macro expansion: write from *outp to *inp (chars waiting to be written),
/* ignore from *inp to *p (chars of the macro call), place generated
/* characters in front of *p (in reverse order), update pointers,
/* resume scanning.
/*
/* symbol table pointers point to just beyond the end of macro definitions;
/* the first preceding character is the number of formal parameters.
/* the appearance of a formal in the body of a definition is marked by
/* 2 chars: the char WARN, and a char containing the parameter number.
/* the first char of a definition is preceded by a zero character.
/*
/* when macro expansion attempts to back up over the beginning of the
/* buffer, some characters preceding *pend are saved in a side buffer,
/* the address of the side buffer is put on 'instack', and the rest
/* of the main buffer is moved to the right.  the end of the saved buffer
/* is kept in 'endbuf' since there may be nulls in the saved buffer.
/*
/* similar action is taken when an 'include' statement is processed,
/* except that the main buffer must be completely emptied.  the array
/* element 'inctop[ifno]' records the last side buffer saved when
/* file 'ifno' was included.  these buffers remain dormant while
/* the file is being read, and are reactivated at end-of-file.
/*
/* instack[0 : mactop] holds the addresses of all pending side buffers.
/* instack[inctop[ifno]+1 : mactop-1] holds the addresses of the side
/* buffers which are "live"; the side buffers instack[0 : inctop[ifno]]
/* are dormant, waiting for end-of-file on the current file.
/*
/* space for side buffers is obtained from 'savch' and is never returned.
/* bufstack[0:fretop-1] holds addresses of side buffers which
/* are available for use.
*/

void dump()
   /* Write part of buffer which lies between outp and inp.
    * This should be a direct call to 'write', but the system slows to a crawl
    * if it has to do an unaligned copy.  Thus we buffer.  This silly loop
    * is 15% of the total time, thus even the 'putc' macro is too slow. */
{
      register char *p1,*p2;
      register FILE *f;

   if ((p1=outp)==inp || flslvl!=0) return;
#if defined ( tgp)
#  define MAXOUT 80
   if (! tgpscan) {
      /* scan again to insure <= MAXOUT chars between linefeeds */
         register char c, *pblank, stopc;
         char          savc, brk;

      tgpscan = 1;
      brk     = stopc = pblank = 0;
      p2      = inp;
      savc    =  *p2;
      *p2     = '\0';
      while (c =  *p1++) {
         if (c == '\\') c = *p1++;
         if (stopc == c) stopc = 0;
         else if (c == '"' || c == '\'') stopc = c;
         if (p1 - outp > MAXOUT && pblank != 0) {
            *pblank++ = '\n';
            inp       = pblank;
            dump();
            brk       = 1;
            pblank    = 0;
         }
         if (c == ' ' && stopc == 0) pblank = p1 - 1;
      }
      if (brk) sayline();
      *p2     = savc;
      inp     = p2;
      p1      = outp;
      tgpscan = 0;
   }
#endif
   f = fout;
   /*  while (p1 < inp) putc (*p1++, f); */     /* before Sys5.2 */
   if (p1 < inp)
     {
#ifdef PROTOGEN
     if (protogenflag)
        p1 +=  (sizeof (char))*(inp - p1);
     else
#endif /* PROTOGEN */
        p1 += fwrite (p1, sizeof (char), inp - p1, f);
     }
   outp = p1;
} /* dump */

char *refill (p) 
   register char *p;
   /* Dump buffer.  Save chars from inp to p.  Read into buffer at pbuf,
    * contiguous with p.  Update pointers, return new p.  */
{
      register char *np, *op;
      register int   ninbuf;

   dump();
   np = pbuf - (p - inp);
   op = inp;
   if (bob (np + 1)) {
      pperror ((catgets(nl_fn,NL_SETN,1, "token too long")));
      np = pbeg;
      p = inp + LINEBUFSIZE;
   }
   macdam += np - inp;
   outp = np;
   inp = np;
   while (op < p) *np++ = *op++;
   p = np;
   for (;;) {
      if (mactop > inctop [ifno]) {
         /* Retrieve hunk of pushed-back macro text */
         op = instack [--mactop];
         np = pbuf;
         do {
            while (*np++ = *op++);
         } while (op < endbuf [mactop]);
         pend = np - 1;
         /* make buffer space avail for 'include' processing */
         if (fretop < MAXFRE) bufstack [fretop++] = instack [mactop];
         return (p);
      } else { /* get more text from file(s) */
         maclvl = 0;
      /* if (0 < (ninbuf =  read (fin, pbuf, LINEBUFSIZE))) { */
         if (0 < (ninbuf = fread (pbuf, sizeof (char), LINEBUFSIZE, fin))) {
            pend = pbuf + ninbuf;
            *pend = '\0';
            return (p);
         }
         /* end of #include file */
#ifdef PROTOGEN
         if (protogenflag)
           fprintf(fout,"# %d %s ", lineno [ifno], fnames[ifno]);
#endif /* PROTOGEN */
         if (ifno == 0) {/* end of input */
            if (plvl != 0) {
                  int n = plvl, tlin = lineno [ifno];
                  char *tfil = fnames [ifno];

               lineno [ifno] = maclin;
               fnames [ifno] = macfil;
               pperror ((catgets(nl_fn,NL_SETN,2, "%s: unterminated macro call")), macnam);
               lineno [ifno] = tlin;
               fnames [ifno] = tfil;
               np = p;
               *np++ = '\n';   /* shut off unterminated quoted string */
	       if (incomm)     /* if in comment, terminate it */
		 {*np++='*'; *np++='/';}
               while (--n >= 0) *np++=')';   /* supply missing parens */
	       if (incomm)     /* restart terminated comment  */
		 {*np++='/'; *np++='*';}
               pend = np;
               *np = '\0';
               if (plvl < 0) plvl = 0;
               return (p);
            }
            if (incomm) pperror ((catgets(nl_fn,NL_SETN,3, "Unexpected EOF in comment")));
            inp = p;
            dump ();
            if (fout && ferror (fout))
               pperror ((catgets(nl_fn,NL_SETN,4, "Problems with writing output file; probably out of temp space")));
            if (iflvl > 0) 
               ppwarn  ((catgets(nl_fn,NL_SETN,5, "missing #endif statement(s)")));
#ifdef OSF
            if (ShowDependents)
               fprintf(stderr, "\n");
#endif /* OSF */
            exit (exfail);
         }
         fclose (fin);
         fin      = fins [--ifno];
#ifdef OSF
         sys_dirs [0] = dirnams [ifno];
         pri_dirs [0] = dirnams [ifno];
#else
         dirs [0] = dirnams [ifno];
#endif /* OSF */
         sayline ();
#ifdef CXREF
	 if(doCXREFdata)
		{
		char *fp;
		fp = fnames[ifno];
		if( (*fp == '.') && (*(fp+1) == '/'))
		  fp += 2;
		fprintf (outfp, "\"%s\"\n", fp);
		}
#endif

      }
   }
} /* refill */

#define BEG 0
#define LF  1

#ifdef __hp9000s300
#pragma OPT_LEVEL 1
#endif
char *cotoken (p) 
   register char *p;
{
      register int c, i;
      char         quoc;
      static int   state   = BEG;
      static int   speakup = 0;

   if (state != BEG) goto prevlf;
   for (;;) { /* loop forever */
again:
      while (! isspc (*p++));
      switch (*(inp = p - 1)) {
      case 0: {
            if (eob (--p)) {
               p = refill (p);
               goto again;
            }
            else 
               ++p; /* ignore null byte */
         } break; /* 0 */

      case '|':
      case '&':
         for (;;) {/* sloscan only */
            if (*p++ == *inp) break;
            if (eob (--p)) p = refill (p);
            else           break;
         } break; /* '|' '&' */

      case '=':
      case '!':
         for (;;) {/* sloscan only */
            if (*p++ == '=') break;
            if (eob (--p)) p = refill (p);
            else           break;
         } break; /* '=' '!' */

      case '<':
      case '>':
         for (;;) { /* sloscan only */
            if (*p++ == '=' || p [-2] == p [-1]) break;
            if (eob(--p)) p = refill (p);
            else          break;
         } break; /* '<' '>' */

      case '\\':
         for (;;) {
            if (*p++ == '\n') {
               ++lineno [ifno];
               break;
            }
            if (eob (--p)) 
               p = refill (p);
            else {
               ++p;
               break;
            }
         }
         break; /* '\\' */

      case '/':
         for (;;) {
#if defined(C_PLUS_PLUS)
            if (c_plus_plus && *p == '/') { /* C++ comment */
               if (in_define) {
                   passcom = 0;
                   /*
                    * OK to do this unconditionally - since in_define is set,
                    * the "real" value of passcom must be saved away in
                    * dodef(). 
                    * The reason for doing this is that "//" comments 
                    * must not be sucked up into the macro definition,
                    * to avoid unexpected deletion of tokens during macro
                    * expansion, as in
                    *   #define x y // comment
                    *   a x b
                    * expanding to
                    *   a y // comment b
                    * (and then to "a y")
                    */
               }
               p++;
               incomm = 1;
               if (! passcom) {
                  inp = p - 2;
                  dump();
                  ++flslvl;
               }
	       for (;;) {
#ifdef NLS
		   while (*p && *p != '\n') {
		      p++;
		      /* HP-15: if 2 byte char, skip 2nd byte */
		      if (FIRSTof2 ((unsigned char) p [-1])) {
			 /* check for eob */
			 if (eob (p)) {
			    if (! passcom) {
			       inp = p;
			    }
			    p   = refill (p);
			 }
			 /* HP-15 doesn't allow a control char as 2nd byte: */
			 if (! SECof2 ((unsigned char)*p)) 
			    ppwarn ((catgets(nl_fn,NL_SETN,6, "(NLS) illegal second byte in 16-bit character")));
			 p++;
		      } /* HP15 */
		   }
#else
		   while (*p != '\n') p++;
#endif  /* NLS */
		   if (! *p) {
		      if (eob(p)) {
			  if (! passcom) {
			     inp = p;
			  }
			  p   = refill (p);
		      } else {
			  /* ignore null byte */
		      }
		   } else {
		      break;  /* out of for loop */
		   }
	       }
               incomm = 0;
               if (! passcom) {
                  inp = p;
                  outp = p;
                  --flslvl;
                  goto again;
               }
               break;
            }
            else
#endif  /* C_PLUS_PLUS */
            if (*p++ == '*') { /* comment */
               incomm = 1;
               if (! passcom) {
                  inp = p - 2;
                  dump ();
                  ++flslvl;
               }
               for (;;) {
#ifdef NLS
                     while (! iscom (*p++)) {
                        /* HP-15: if 2 byte char, skip 2nd byte */
                        if (FIRSTof2 ((unsigned char) p [-1])) {
                           /* check for eob */
                           if (eob (p)) {
                              if (! passcom) {
                                 inp = p;
                                 p   = refill (p);
                              } else if ((p - inp) >= LINEBUFSIZE) {
                                 /* split long comment */
                                 inp = p;
                                 p   = refill (p);
                                 /* last char written is '*' */
                                 /* Split long comment into  */
                                 /* parts. Terminate first   */
                                 /* bit & start second.      */
                                 outp = inp = p -= 4;
                                 *p++ = '/';
                                 *p++ = '/';
                                 *p++ = '*';
                                 *p++ = '*';
                              } else 
                                 p = refill (p);
                           }
                           /* HP-15 doesn't allow a control char as 2nd byte: */
                           if (! SECof2 ((unsigned char)*p)) 
                              ppwarn ((catgets(nl_fn,NL_SETN,6, "(NLS) illegal second byte in 16-bit character")));
                           p++;
                        } /* if (FIRSTof2... */
                     } /* while (! iscom... */
#else
		     while (! iscom (*p++));
#endif /* NLS */
                  if (p [-1] == '*') {
                     for (;;) {
                        if (*p++ == '/')
                           goto endcom;
                        if (eob (--p)) {
                           if (! passcom) {
                              inp = p;
                              p   = refill (p);
                           } else if ((p - inp) >= LINEBUFSIZE) {
                              inp =   p;
                              p     = refill (p);
                              /* last char written is '*' */
                              /* Split long comment into  */
                              /* parts. Terminate first   */
                              /* bit & start second.      */
                              outp = inp = p -= 4;
                              *p++ = '/';
                              *p++ = '/';
                              *p++ = '*';
                              *p++ = '*';
                           } else 
                              p = refill(p);
                        } else 
                           break;
                     } /* for (;;) */
                  } else if (p [-1] == '\n') {
                     ++lineno [ifno];
                     if (! passcom)
#ifdef PROTOGEN
                        if(!protogenflag)
#endif /* PROTOGEN */
			  putc ('\n',fout);
                  } else if (eob (--p)) {
                     if (! passcom) {
                        inp = p;
                        p   = refill (p);
                     }
                     else if ((p - inp) >= LINEBUFSIZE) { 
                        inp = p;                     /* split long comment */
                        p   = refill (p);
                        /* Split long comment into  */
                        /* parts. Terminate first   */
                        /* bit & start second.      */
                        outp = inp = p -= 4;
                        *p++ = '*';
                        *p++ = '/';
                        *p++ = '/';
                        *p++ = '*';
                     } else 
                        p = refill (p);
                  } else 
                     ++p; /* ignore null byte */
               } /* for (;;) */
endcom:
               incomm = 0;
               if (! passcom) {
                  outp = p;
                  inp  = p;
                  --flslvl;
                  goto again;
               }
               break;
            }
            if (eob (--p)) p = refill (p);
            else           break;
         } break; /* '/' */

      case '"':
      case '\'':
         quoc = p [-1];
         for (;;) {
#ifdef NLS
               while (! isquo (*p)){
                  if(FIRSTof2 ((unsigned char)*p)) {
                     p++;
                     if (eob (p)) p = refill (p);
                     /* HP-15 doesn't allow a control char as 2nd byte. */
                     if (! SECof2 ((unsigned char)*p)) 
                        pperror ((catgets(nl_fn,NL_SETN,7, "(NLS) illegal second byte in 16-bit character")));
                  }
                  p++;
               }
               p++;
#else
	    while (!isquo (*p++));
#endif /* NLS */
            if (p [-1] == quoc) break;
            if (p [-1] == '\n') {
               --p;
               break;
            } /* bare \n terminates quotation */
            if (p [-1] == '\\') for (;;) {
               if (*p++ == '\n') {
                  ++lineno [ifno];
                  break;
               } /* escaped \n ignored */
               if (eob (--p)) p = refill (p);
               /* Do not ignore escaped first byte */
		else {
#ifdef NLS
               if (!(FIRSTof2 ((unsigned char)*p)))
		   ++p;
               break;
	       }
#else
               ++p;
               break;
               }
#endif /* NLS */
            } else if (eob (--p)) 
               p = refill (p);
            else 
               ++p; /* it was a different quote character */
         } /* for (;;) */
         break; /* '"' '\'' */

      case WARN: {
            int ii;
         dump ();
         speakup = 0;
         for (ii = sizeof (int) / sizeof (char); --ii >= 0;) {
            if (eob(p)) p=refill(p);
            speakup |= (*p++ & 0xFF) << (ii * 8);
         }
         inp  = p;
         outp = p;
      } break; /* WARN */

      case '\n':
         ++lineno [ifno];
         if (isslo) {
            state = LF;
            return (p);
         }
prevlf:
         if (speakup) {
	    char savec;

            savec = *inp;
	    inp = p;
            dump ();
            lineno [ifno] = speakup + 1;
            if (savec == '\n')
	      sayline ();
            speakup = 0;
         }
         state = BEG;
         for (;;) {
            SA_LINE_START (((*inp=='\n') ? (inp + 1) : inp));
            /* Ingore formfeeds and vertical tab which may be just before 
             * the SALT.   Sys 5 change. */
            if (*p == '\f' || *p == '\v') {
               char *s = p;
               while (*++s == '\f' || *s == '\v') ;
               if (*s == SALT) { /* get SALT to front */
                  *s = *p;
                  *p = SALT;
               }
            }
            if (*p++ == '#') return (p);
            if (eob(inp = --p)) p = refill (p);
            else                goto again;
         }
         break; /* '\n' */

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
         for (;;) {
            while (isnum (*p++));
            if (eob (--p)) p = refill (p);
            else           break;
         }
         break; /* decimal digits */

      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
      case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
      case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
      case 'V': case 'W': case 'X': case 'Y': case 'Z':
      case '_':
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
      case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
      case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
      case 'v': case 'w': case 'x': case 'y': case 'z':
#if scw1
#  define tmac1(c,bit)    if (! xmac1 (c, bit, &)) goto nomac
#  define xmac1(c,bit,op) ((macbit + COFF) [c] op (bit))
#else
#  define tmac1(c,bit)
#  define xmac1(c,bit,op)
#endif

#if scw2
#  define tmac2(c0,c1,cpos) if (! xmac2 (c0, c1, cpos, &)) goto nomac
#  define xmac2(c0,c1,cpos,op) \
   ((macbit + COFF) [(t21+COFF)[c0] + (t22+COFF)[c1]] op (t23+COFF+cpos)[c0])
#else
#  define tmac2(c0,c1,cpos)
#  define xmac2(c0,c1,cpos,op)
#endif

         if (flslvl) goto nomac;
         for (;;) {
            c = p [-1]; tmac1 (c, b0);
            i = *p++; if (! isid (i)) goto endid; 
            tmac1 (i, b1); tmac2 (c, i, 0);
            c = *p++; if (! isid (c)) goto endid; 
            tmac1 (c, b2); tmac2 (i, c, 1);
            i = *p++; if (! isid (i)) goto endid;
            tmac1 (i, b3); tmac2 (c, i, 2);
            c = *p++; if (!isid (c)) goto endid; 
            tmac1 (c, b4); tmac2 (i, c, 3);
            i = *p++; if (!isid (i)) goto endid;
            tmac1 (i, b5); tmac2 (c, i, 4);
            c = *p++; if (! isid (c)) goto endid;
            tmac1 (c, b6); tmac2 (i, c, 5);
            i = *p++; if (! isid (i)) goto endid;
            tmac1 (i, b7); tmac2 (c, i, 6); tmac2 (i, 0, 7);
            while (isid (*p++));
            if (eob (--p)) {
               refill (p);
               p = inp + 1;
               continue;
            }
            goto lokid;
endid:
            if (eob (--p)) {
               refill (p);
               p = inp + 1;
               continue;
            }
            tmac2 (p [-1], 0, -1 + (p - inp));
lokid:
            slookup (inp, p, 0);
            if (newp) {
               p = newp;
               goto again;
            } else 
               break;
nomac:
            while (isid (*p++));
            if (eob (--p)) {
               p = refill (p);
               goto nomac;
            } else 
               break;
         }
         break; /* letters and underscore. */

      } /* switch (*(inp = p - 1)) */

      if (isslo) return(p);
   } /* for (;;) */
} /* cotoken */
#ifdef __hp9000s300
#pragma OPT_LEVEL 2
#endif

char *skipbl (p) /* get next non-blank token */
   register char *p;
{
   do {
      outp = p;
      inp  = p;
      p    = cotoken (p);
   } while ((toktyp + COFF) [*inp] == BLANK);
   return (p);
}

char *unfill (p) 
   register char *p;
   /* take <= LINEBUFSIZE chars from right end of buffer and put them on 
    * instack.  Slide rest of buffer to the right, update pointers, return 
    * new p. */
{
      register char *np, *op;
      register int   d;

   if (mactop >= MAXFRE) {
      pperror ((catgets(nl_fn,NL_SETN,8, "%s: too much pushback")), macnam);
      inp = p = pend;
      dump ();   /* begin flushing pushback */
      while (mactop > inctop [ifno]) {
         p   = refill (p);
         inp = p = pend;
         dump ();
      }
   }
   if (fretop > 0) 
      np = bufstack [--fretop];
   else {
      np     = savch;
      savch += LINEBUFSIZE;
      if (savch >= sbf + sbsize)
      {
         pperror ((catgets(nl_fn,NL_SETN,9, "no space")));
         exit (exfail);
      }
      *savch++ = '\0';
   }
   instack [mactop] = np;
   op = pend - LINEBUFSIZE;
   if (op < p) op = p;
   for (;;) {
      while (*np++ = *op++);
      if (eob (op)) break;
   } /* out with old */
   endbuf [mactop++] = np;   /* mark end of saved text */
   np   = pbuf + LINEBUFSIZE;
   op   = pend - LINEBUFSIZE;
   pend = np;
   if (op < p) op = p;
   while (outp < op) 
      *--np = *--op; /* slide over new */
   if (bob(np)) 
      pperror ((catgets(nl_fn,NL_SETN,10, "token too long")));
   d       = np - outp;
   outp   += d;
   inp    += d;
   macdam += d;
   return (p + d);
} /* unfill */

char *doincl (p) 
   register char *p;
{
      int            filok, inctype;
      register char *cp, **dirp;
      char            *nfil;
      char            filname [LINEBUFSIZE];

   p  = skipbl (p);
   cp = filname;
   if (*inp++ == '<') { /* special <> syntax */
      inctype = 1;
      ++flslvl;   /* prevent macro expansion */
      for (;;) {
         outp = p;
         inp  = p;
         p    = cotoken (p);
         if (*inp == '\n') {
            --p;
            *cp = '\0';
            /* Missing closing >; Include but give an error */
	    pperror ((catgets(nl_fn,NL_SETN,11, "bad include syntax")), 0);
            break;
         }
         if (*inp == '>') {
            *cp = '\0';
            break;
         }
         while (inp < p) *cp++ = *inp++;
      }
      --flslvl;   /* reenable macro expansion */
   } else if (inp [-1] == '"') { /* regular "" syntax */
      inctype = 0;
      while (inp<p) *cp++= *inp++;
      if (*--cp == '"') 
         *cp = '\0';
      else {
	 /* Missing closing "; Include but give an error */
	 *(++cp) = '\0'; /* Null terminated  */
	 pperror ((catgets(nl_fn,NL_SETN,11, "bad include syntax")), 0);
      }
   } else {
      pperror ((catgets(nl_fn,NL_SETN,11, "bad include syntax")), 0);
      inctype = 2;
   }
   /* flush current file to \n , then write \n */
   ++flslvl;
   do {
      outp = p;
      inp  = p;
      p    = cotoken (p);
   } while (*inp != '\n');
   --flslvl;
   inp = p;
   dump ();
   if (inctype == 2) return (p);
   /* look for included file */
   if (ifno + 1 >= MAXINC) {
      pperror ((catgets(nl_fn,NL_SETN,12, "Unreasonable include nesting")), 0);
      return (p);
   }
   if ((nfil = savch) > sbf + sbsize - LINEBUFSIZE) {
      pperror ((catgets(nl_fn,NL_SETN,13, "no space")));
      exit (exfail);
   }
   filok = 0;
#ifdef OSF
   if (inctype == 1)
      dirp = sys_dirs + inctype;
   else
      dirp = pri_dirs;
   for ( ; *dirp; ++dirp) {
#else
   for (dirp = dirs + inctype; *dirp; ++dirp) {
#endif /* OSF */
      if (
          filname [0] == '/'
          || **dirp == '\0') 
         strcpy (nfil, filname);
      else {
         strcpy (nfil, *dirp);
#if (defined unix) || (defined __unix) || (defined _HPUX_SOURCE)
         strcat (nfil, "/"); 
#endif
#if defined ( ibm)
         strcat (nfil, ".");
#endif
         strcat (nfil, filname);
      }
      if (NULL != (fins [ifno + 1] = fopen (nfil, READ))) {
#ifdef OSF
         char *filename;
         if ( strncmp(nfil,"./",2) == 0 ) /* ignore "./" if exists */
            filename = nfil+2;
         else
            filename = nfil;
         if (ShowFiles)
            fprintf(stderr, "%*s%s\n", (ifno + 1) * 4, "", filename);
         if (ShowDependents) {
            int len;
            len = strlen(filename);
            /* the last 4 bytes reserved for a backslash and a newline */
            if (linesize - 1 - 4 >= len) { /* enough room left */
               fprintf(stderr, " %s", filename);
               linesize = linesize - 1 - len;
               }
            else {
               fprintf(stderr, " \\\n  %s", filename);
               linesize = LINESIZE - 2 - len; /* reset the linesize */
            }
         }
#endif /* OSF */
         filok = 1;
         fin   = fins [++ifno];
         break;
      }
   }
   if (filok == 0) 
      pperror ((catgets(nl_fn,NL_SETN,14, "Can't find include file %s")), filname);
   else {
      lineno [ifno]  = 1;
      fnames [ifno]  = cp = nfil;
      while (*cp++);
      savch          = cp;
#ifdef OSF
      dirnams [ifno] = sys_dirs [0] = pri_dirs [0] = trmdir (copy (nfil));
#else
      dirnams [ifno] = dirs [0] = trmdir (copy (nfil));
#endif /* OSF */
#ifdef PROTOGEN
      if (protogenflag)
        fprintf(fout,"$ %d %s ", lineno [ifno], fnames[ifno]);
#endif /* PROTOGEN */
      sayline ();
#ifdef CXREF
      if(doCXREFdata)
	{
	char *fp;
	fp = fnames[ifno];
	if( (*fp == '.') && (*(fp+1) == '/'))
	  fp += 2;
	fprintf (outfp, "\"%s\"\n", fp);
	}
#endif
      /* save current contents of buffer */
      while (! eob (p)) p = unfill (p);
      inctop [ifno] = mactop;
   }
   return (p);
} /* doincl */

equfrm (a, p1, p2) 
   register char *a, *p1, *p2;
{
      register char c;
      int flag;

   c    = *p2;
   *p2  = '\0';
   flag = strcmp (a, p1);
   *p2  = c;
   return (flag == SAME);
} /* equfrm */

char *dodef (p)    /* process '#define' */
      char  *p;
{
      register char  *pin, *psav, *cf;
      char          **pf, **qf;
      int             b, c, params;
      struct symtab  *np;
      int             sav_passcom = passcom;
      int             ex_blank;              /* ignore extra blanks var */
      char           *oldval, *oldsavch;
      char           *formal  [MAXFRM];      /* Name of n'th formal. */
      char            formtxt [LINEBUFSIZE]; /* Space for formal names */

   if (savch > sbf + sbsize - LINEBUFSIZE) {
      pperror ((catgets(nl_fn,NL_SETN,15, "too much defining - use -H option")));
      return (p);
   }
   oldsavch = savch; /* to reclaim space if redefinition */
   ++flslvl; /* prevent macro expansion during 'define' */
   p = skipbl (p);
   pin = inp;
   SA_MACRO_COL (inp);
   if ((toktyp + COFF) [*pin] != IDENT) {
      ppwarn ((catgets(nl_fn,NL_SETN,16, "illegal macro name")));
      while (*inp != '\n') p = skipbl (p);
      return (p);
   }
   np=slookup (pin, p, 1);
   SA_MACRO_NAME (np->name);
   SA_MACRO_LINE (lineno [ifno]);
   SA_MACRO_PRINT ('D');
   if (oldval = np->value) savch = oldsavch;   /* was previously defined */
#ifdef CXREF
      if(doCXREFdata)
	  def (np->name, lineno [ifno]);
#endif
   b  = 1;
   cf = pin;
   while (cf < p) { /* update macbit */
      c = *cf++;
      xmac1 (c, b, |=);
      b = (b + b) & 0xFF;
      if (cf != p) xmac2 (c, *cf, -1 + (cf - pin), |=);
      else         xmac2 (c, 0,   -1 + (cf - pin), |=);
   }
   params = 0;
   outp   = p;
   inp    = p;
   p      = cotoken (p);
   pin    = inp;
   if (*pin == '\\' && pin [1] == '\n')
     {
#ifdef PROTOGEN
     if(!protogenflag)
#endif /* PROTOGEN */
       putc ('\n', fout);
     }
   if (*pin == '(') { /* With parameters; identify the formals */
#ifdef CXREF
    if(doCXREFdata)
	newf (np->name, lineno[ifno]);
#endif
      cf = formtxt;
      pf = formal;
      for (;;) {
         p   = skipbl (p);
         pin = inp;
         if (*pin == '\n') {
            --lineno [ifno];
            --p;
            pperror ((catgets(nl_fn,NL_SETN,17, "%s: missing )")), np->name);
            break;
         }
         if (*pin==')') break;
         if (*pin==',') continue;
         if ((toktyp + COFF) [*pin] != IDENT) {
            c  = *p;
            *p = '\0';
            pperror ((catgets(nl_fn,NL_SETN,18, "bad formal: %s")), pin);
            *p = c;
         } else if (pf >= &formal [MAXFRM]) {
            c  = *p;
            *p = '\0';
            pperror ((catgets(nl_fn,NL_SETN,19, "too many formals: %s")), pin);
            *p = c;
         } else {
            *pf++ = cf;
            while (pin < p) *cf++ = *pin++;
            *cf++ = '\0';
            ++params;
#ifdef CXREF
	    if(doCXREFdata)
		def (*(pf - 1), lineno [ifno]);
#endif
         }
      }
      if (params == 0) --params; /* #define foo() ... */
   } else if (*pin == '\n') {
      --lineno [ifno];
      --p;
   }
   /* Remember beginning of macro body, so that we can warn if a 
    * redefinition is different from old value: */
   oldsavch = psav = savch;
   passcom  = 1; /* Make cotoken() return comments as tokens */
#if defined(C_PLUS_PLUS)
   in_define = 1; /* for cotoken() to handle "//" comments properly */
#endif
   ex_blank = 1; /* Must have some delimiter - use blank */
   for (;;) { /* Accumulate definition until linefeed */
      outp = p;
      inp  = p;
      p    = cotoken (p);
      pin  = inp;
      if (*pin == '\\' && pin [1] == '\n') {
         if (! ex_blank) {  /* replace with a blank */
            *psav++  = ' ';
            ex_blank = 1;
         }
#ifdef PROTOGEN
         if(!protogenflag)
#endif /* PROTOGEN */  
	   putc ('\n', fout);
         continue;   /* ignore escaped lf */
      }
      if (*pin == '\n') break;

      if ((toktyp + COFF) [*pin] == BLANK) { /* skip extra blanks */
         if (ex_blank)  continue;
         *pin     = ' ';  /* fource it to be a "real" blank */
         ex_blank = 1;
      } else
         ex_blank = 0;
      if (*pin == '/' && pin[1] == '*') { /* skip comment except for \n */
         while (pin < p)
            if (*pin++ == '\n')
#ifdef PROTOGEN
                if(!protogenflag)
#endif /* PROTOGEN */
		 putc ('\n', fout);
         continue;
      }
      if (params) { /* mark the appearance of formals in the definiton */
         if ((toktyp + COFF) [*pin] == IDENT) {
            for (qf = pf; --qf >= formal;) {
               if (equfrm (*qf, pin, p)) {
#if defined (CXREF) && (! defined (NO_MACRO_FORMAL))
		  if(doCXREFdata)
		    ref (*qf, lineno [ifno]);
#endif
                  *psav++ = qf - formal + 1;
                  *psav++ = WARN;
                  pin     = p;
                  break;
               }
            }
         } else if (*pin == '"' || *pin == '\'')
	 { /* inside quotation marks, too */
               char quoc = *pin;

            for (*psav++ = *pin++; pin < p && *pin != quoc;) {
               while (pin < p && ! isid (*pin)) {
                  if (*pin == '\n' && pin [-1] == '\\') {
#ifdef PROTOGEN
                     if(!protogenflag)
#endif /* PROTOGEN */
                       putc ('\n', fout);
                     psav--;
                     pin++;
                  }
                  else
                     *psav++ = *pin++;
               }
               cf = pin;
               while (cf < p && isid (*cf)) ++cf;
               for (qf = pf; --qf >= formal;) {
                  if (equfrm (*qf, pin, cf)) {
                     *psav++ = qf - formal + 1;
                     *psav++ = WARN;
                     pin     = cf;
                     break;
                  }
               }
               while (pin < cf) *psav++ = *pin++;
            } /* for (*psav... */
         } /* else if (*pin... inside quotation marks... */
      } /* if (params) */
      while (pin < p)
         if (*pin == '\n' && pin [-1] == '\\') {
#ifdef PROTOGEN
            if(!protogenflag)
#endif /* PROTOGEN */
              putc ('\n', fout);
            psav--;
            pin++;
         }
         else
            *psav++ = *pin++;
   } /* done with definition */
   passcom = sav_passcom;       /* restore to "real" value     */
#if defined(C_PLUS_PLUS)
   in_define = 0;
#endif
   if (psav[-1] == ' ') psav--; /* if throw away tailing blank */
   *psav++ = params;
   *psav++ = '\0';
   if ((cf = oldval) != NULL) { /* redefinition */
      --cf;          /* skip no. of params, which may be zero */
      while (*--cf); /* go back to the beginning */
      if (0 != strcmp (++cf, oldsavch)) { /* redefinition different from old */
         --lineno [ifno];
         ppwarn ((catgets(nl_fn,NL_SETN,20, "%s redefined")), np->name);
         ++lineno [ifno];
         np->value = psav - 1;
      } else 
         psav = oldsavch; /* identical redef.; reclaim space */
   } else 
      np->value = psav - 1;
   --flslvl;
   inp   = pin;
   savch = psav;
   return (p);
} /* dodef */

#define fasscan() ptrtab=fastab+COFF
#define sloscan() ptrtab=slotab+COFF

char *control (p) /* Find and handle preprocessor control lines */
      register char *p;
{ 
      register struct symtab *np;
      int savelineno;

   for (;;) {
      fasscan ();
      p      = cotoken (p);
      if (*inp == '\n') ++inp;
      dump ();
      sloscan ();
      p      = skipbl (p);
      *--inp = SALT;
      outp   = inp;
      ++flslvl;
      np     = slookup (inp, p, 0);
      --flslvl;
      if (np == defloc) { /* define */
         if (flslvl == 0) {
            p = dodef (p);
            continue;
         }
      } else if (np == udfloc) { /* undefine */
         if (flslvl == 0) {
            ++flslvl;
            p = skipbl (p);
            slookup (inp, p, DROP);
#if defined (STAT_ANAL)
            {
               char sa_savech;
            SA_MACRO_LINE(lineno[ifno]);
            SA_MACRO_COL(inp);
            /* temporarily null-terminate the name: */
            if (emit_SA) { sa_savech = *p; *p = '\0'; }
            SA_MACRO_NAME(inp);
            SA_MACRO_PRINT('U');
            if (emit_SA) { *p = sa_savech; }
            }
#endif /* STAT_ANAL */
            --flslvl;
#ifdef CXREF
	    if(doCXREFdata)
		ref (xcopy (inp, p), lineno [ifno]);
#endif
         }
      } else if (np == incloc) { /* include */
         if (flslvl == 0) {
            p = doincl (p);
            continue;
         }
      } else if (np == ifnloc) { /* ifndef */
         ++iflvl;
         ++flslvl;
         savelineno = lineno[ifno];
         p  = skipbl (p);
         if (savelineno != lineno[ifno]) /* no other token on line */
           pperror (catgets(nl_fn, NL_SETN, 30003, "syntax error"));
         np = slookup (inp, p, 0);
         SA_MACRO_LINE(lineno[ifno]);
         SA_MACRO_COL(inp);
         SA_MACRO_NAME(np->name);
         SA_MACRO_PRINT('U');
         --flslvl;
         if (flslvl == 0 && np->value == 0)
	   {
	   ++trulvl;
#ifdef PROTOGEN
            if (protogenflag)
              fprintf(fout," @ %d T ",lineno[ifno]);
#endif /* PROTOGEN */
	   }
	 else
	   {
	   ++flslvl;
#ifdef PROTOGEN
           if (protogenflag)
             fprintf(fout," @ %d F ",lineno[ifno]);
#endif /* PROTOGEN */
	   }
#ifdef CXREF
	 if(doCXREFdata)
	     ref (xcopy (inp, p), lineno [ifno]);
#endif
      } else if (np == ifdloc) { /* ifdef */
         ++iflvl;
         ++flslvl;
         savelineno = lineno[ifno];
         p  = skipbl (p);
         if (savelineno != lineno[ifno]) /* no other token on line */
           pperror (catgets(nl_fn, NL_SETN, 30003, "syntax error"));
         np = slookup (inp, p, 0);
         SA_MACRO_LINE(lineno[ifno]);
         SA_MACRO_COL(inp);
         SA_MACRO_NAME(np->name);
         SA_MACRO_PRINT('U');
         --flslvl;
         if (flslvl == 0 && np->value != 0)
           {
           ++trulvl;
#ifdef PROTOGEN
            if (protogenflag)
              fprintf(fout," @ %d T ",lineno[ifno]);
#endif /* PROTOGEN */
           }
         else
           {
           ++flslvl;
#ifdef PROTOGEN
           if (protogenflag)
             fprintf(fout," @ %d F ",lineno[ifno]);
#endif /* PROTOGEN */
           }
#ifdef CXREF
	 if(doCXREFdata)
	     ref (xcopy (inp, p), lineno [ifno]);
#endif
      } else if (np == ifloc) { /* if */
         ++iflvl;
#if defined ( tgp)
         pperror ((catgets(nl_fn,NL_SETN,23, " IF not implemented, true assumed")), 0);
         if (flslvl == 0) ++trulvl; else ++flslvl;
#else
         newp  = p;
#ifdef CXREF
	 if(doCXREFdata)
	     xline = lineno [ifno];
#endif
         if (flslvl == 0 && yyparse ())
           {
           ++trulvl;
#ifdef PROTOGEN
           if (protogenflag)
             fprintf(fout," @ %d T ",lineno[ifno]);
#endif /* PROTOGEN */
           }
         else
           {
           ++flslvl;
#ifdef PROTOGEN
           if (protogenflag)
             fprintf(fout," @ %d F ",lineno[ifno]);
#endif /* PROTOGEN */
           }
         p = newp;
#endif
      } else if (np == elifloc) { /* elif */
         if (flslvl) {
            if (--flslvl != 0) 
               ++flslvl;
            else {
               ++trulvl;
               sayline ();
            }
         } else if (trulvl) {
            ++flslvl;
            --trulvl;
         }
         else 
            pperror ((catgets(nl_fn,NL_SETN,22, "If-less elif")), 0);
         newp  = p;
#ifdef CXREF
	 if(doCXREFdata)
	     xline = lineno [ifno];
#endif
         if (flslvl == 0 && yyparse ())
           {
           ++trulvl;
#ifdef PROTOGEN
           if (protogenflag)
             fprintf(fout," @ %d T ",lineno[ifno]);
#endif /* PROTOGEN */
           }
         else
           {
           ++flslvl;
#ifdef PROTOGEN
           if (protogenflag)
             fprintf(fout," @ %d F ",lineno[ifno]);
#endif /* PROTOGEN */
           }
         p = newp;
      } else if (np == elsloc) { /* else */
         if (flslvl) {
            if (--flslvl != 0) 
               ++flslvl;
            else {
               ++trulvl;
               sayline ();
            }
         } else if (trulvl) {
            ++flslvl;
            --trulvl;
         }
         else 
            pperror ((catgets(nl_fn,NL_SETN,22, "If-less else")), 0);
      } else if (np == eifloc) { /* endif */
         --iflvl;
         if (flslvl) {
            if (--flslvl==0) sayline ();
         } else if (trulvl) 
            --trulvl;
         else {
            pperror ((catgets(nl_fn,NL_SETN,21, "If-less endif")), 0);
            ++iflvl;
         }
      } else if (np == lneloc) { /* line */
         if (flslvl == 0 && pflag == 0) {
               register char *s;
               register int   ln;

            outp = inp = p;
do_line:
            *--outp = '#';
            /*  make sure that the whole
            /*  directive has been read */
            s = p;
            while (*s && *s != '\n') s++;
            if (eob (s)) p = refill (s);

            /* eat the line number */
            s = inp;
            while ((toktyp + COFF) [*s] == BLANK) s++;
            ln = 0;
            while (isdigit (*s)) ln = ln * 10 + *s++ - '0';
            if (ln)
               lineno [ifno] = ln - 1;
            else
               pperror ((catgets(nl_fn,NL_SETN,24, "bad number for #line")));

            /* eat the optional "filename" */
            while ((toktyp + COFF) [*s] == BLANK) s++;
            if (*s != '\n') {
               if (*s != '"')
                  pperror ((catgets(nl_fn,NL_SETN,25, "bad file for #line")));
               else {
                     register char *t = savch;
                  for (;;) {
                     if (*++s == '"')
                        break;
                     else if (*s == '\n' || *s == '\0') {
                        pperror((catgets(nl_fn,NL_SETN,26, "bad file for #line")));
                        break;
                     }
                     *t++ = *s;
                  }
                  *t++ = '\0';
                  if (strcmp (savch, fnames [ifno])) {
                     fnames [ifno] = savch;
                     savch         = t;
                  }
               }
            }
            /*  push it all along to be eventually printed */
            while (*inp != '\n') p = cotoken (p);
            continue;
         }
      } else if (np == idtloc) { /* ident */
#ifdef PAXDEV
         /* pass pragma through to compiler */
         while (*inp != '\n') p = cotoken (p);
         continue;
#endif
         /* Othw ignore the #ident */
      } else if (np == pragloc) { /* pragma */
         /* pass pragma through to compiler */
         while (*inp != '\n') p = cotoken (p);
         continue;
      } else if (*++inp == '\n')   /* allows blank line after # */
         outp = inp;
      else if (isdigit (*inp)) { /* pass thru line directives */
         outp = p = inp;
         goto do_line;
      } else 
      if (flslvl == 0)
/* change the following from an error to a warning, if it is within a #define */
/* which is evaluated to FALSE.  CLLca01170 & CLLca01923.  pkwan  920110 */
         pperror ((catgets(nl_fn,NL_SETN,27, "undefined preprocessor directive")), 0);
      else
         ppwarn ((catgets(nl_fn,NL_SETN,27, "undefined preprocessor directive")), 0);
/* CLLca01170.  pkwan  920110 */
      ++flslvl; /* flush to lf */
      while (*inp != '\n') {
         outp = p;
         inp  = p;
         p    = cotoken (p);
      }
      --flslvl;
   }
} /* control */

struct symtab *stsym (s) 
      register char  *s;
{
      char           buf [LINEBUFSIZE];
      register char *p;

   /* Make definition look exactly like end of #define line */
   /* Copy to avoid running off end of world when param list is at end */

   p = buf;
   while (*p++ = *s++);
   p = buf;
   while (isid (*p++)); /* skip first identifier */
   if (*--p == '=') {
      *p++ = ' ';
      while (*p++);
   } else {
      s = " 1";
      while (*p++ = *s++);
   }
   pend = p;
   *--p = '\n';
   sloscan ();
   dodef (buf);
   return (lastsym);
} /* stsym */

struct symtab *ppsym (s) /* kluge */
      char           *s;
{
      register struct symtab *sp;

   cinit    = SALT;
   *savch++ = SALT;
   sp       = stsym (s);
   --sp->name;
   cinit    = 0;
   return (sp);
} /* ppsym */

int yy_errflag;     /* TRUE when pperror called by yyerror()  */

/* VARARGS1 */
pperror (s, x, y) 
   char *s;
   int      x, y;
{
   if (fnames [ifno] [0]) fprintf (stderr,"%s: ",fnames [ifno]);
   fprintf (stderr, "%d: ", lineno [ifno]);
   fprintf (stderr, s, x, y);
   if (yy_errflag) fprintf (stderr, (catgets(nl_fn,NL_SETN,28, " (in preprocessor if)")));
   fprintf (stderr, "\n");
   if(((++exfail) % 256) == 0) /* don't set exfail to mod 256 value, as only */
     ++exfail;                 /* low 8 bits of exit value are significant.  */
} /* pperror */

yyerror (s, a, b) 
   char *s;
   int      a, b;
{
   yy_errflag = 1;
   pperror (s, a, b);
   yy_errflag = 0;
} /* yyerror */

ppwarn (s, x)
   char *s;
   int     x;
{
      int fail = exfail;
/* CLLca01948 */
#define WARNING "warning- "
      extern char *malloc();
      char *warning;

      if (! (warning = malloc (strlen(WARNING)+strlen(s)+1))) { 
         pperror ((catgets(nl_fn,NL_SETN,50,"Can't allocate warning message")));
         exit (exfail);
      }
/* CLLca01948 */

   exfail = -1;

/* CLLca01948.  Enhancement request to put "warning" in front of warning */
/* messages.  This is consistent with cpp.ansi.  pkwan 920115 */
   strcpy (warning, WARNING);
   strcat (warning, s);
   pperror (warning, x);
/* CLLca01948 */

   exfail = fail;
} /* ppwarn */

struct symtab *lookup (namep, enterf)
      char            *namep;
      int                     enterf;
{
      register char          *np, *snp;
      register int            c, i;
      register struct symtab *sp;
      int                     around;

   /* Namep had better not be too long (currently, <= NAMESIZE chars) */
   np     = namep;
   around = 0;
   i      = cinit;
   while (c = *np++) i += i + c;
   c = i;
   c %= symsiz;
   if (c < 0) c += symsiz;
   sp = stab + c;
   while (snp = sp->name) {
      np = namep;
      while (*snp++ == *np) {
         if (* np++ == '\0') {
            if (enterf == DROP) {
               sp->name [0] = DROP;
               sp->value    = 0;
            }
            return (lastsym = sp);
         }
      }
      if (--sp < stab)
         if (around) {
            pperror ((catgets(nl_fn,NL_SETN,29, "too many defines - use -H option")), 0);
            exit (exfail);
         } else {
            ++around;
            sp = stab + (symsiz - 1);
         }
   }
   if (enterf == 1) sp->name = namep;
   return (lastsym = sp);
} /* lookup */

struct symtab *slookup (p1, p2, enterf) 
      register char    *p1,*p2;
      int                       enterf;
{
      register char *p3;
      char           c2,c3;
      struct symtab *np;

   c2   = *p2;
   *p2  = '\0';   /* mark end of token */
   p3   = ((p2 - p1) > ncps) ? p1 + ncps : p2;
   c3   = *p3;
   *p3  = '\0';   /* truncate to NAMESIZE chars or less */
   p1   = (enterf == 1) ? copy (p1) : p1;
   np   = lookup (p1, enterf);
   *p3  = c3;
   *p2  = c2;
   newp = (np->value != 0 && flslvl == 0) ? subst (p2, np) : 0;
   return (np);
} /* slookup */

char *subst (p, sp) 
register char *p;
struct symtab *sp;
{
register char *ca,*vp;
register int   params;
char  *actual [MAXFRM];     /* is text of each actual */
char  acttxt [LINEBUFSIZE]; /* space for actuals */
unsigned char zerop = 0;  /* flag to denote zero-parameter macro */
unsigned char havep = 0;  /* flag to denote detection of actual parameter */

SA_MACRO_NAME(sp->name);
SA_MACRO_LINE(lineno[ifno]);
SA_MACRO_COL(inp);
if (0 == (vp = sp->value)) return (p);
if ((p - macforw) <= macdam)
  {
  if (++maclvl > SYMSIZ && ! rflag)
    {
    pperror ((catgets(nl_fn,NL_SETN,30, "%s: macro recursion")), sp->name);
    return (p);
    }
 }
else 
  maclvl = 0; /* level decreased */
macforw = p;
macdam  = 0;   /* new target for decrease in level */
macnam  = sp->name;

#ifdef CXREF
if(doCXREFdata)
  ref (macnam, lineno [ifno]);
#endif

dump ();
if (sp == ulnloc)
  {
  vp    = acttxt;
  *vp++ = '\0';
  sprintf (vp, "%d", lineno [ifno]);
  while (*vp++);
  }
else if (sp == uflloc)
  {
  vp    = acttxt;
  *vp++ = '\0';
  sprintf (vp,"\"%s\"", fnames [ifno]);
  while (*vp++);
  }

if (0 == (params = *--vp & 0xFF)) /* no parameters */
  SA_MACRO_PRINT ('U');
else
  {                            /* definition calls for params */
  register char **pa;

  SA_MACRO_PRINT ('C');
  ca = acttxt;
  pa = actual;
  if (params == 0xFF)
    zerop = params = 1;   /* #define foo() ... */
  sloscan ();
  ++flslvl; /* no expansion during search for actuals */
  plvl   = -1;
  maclin = lineno [ifno];
  macfil = fnames [ifno];
  do 
     p = skipbl (p);
  while (*inp == '\n');   /* skip \n too */
  if (*inp == '(')
    {
    for (plvl = 1; plvl != 0;)
      {
      *ca++ = '\0';
      for (;;)
	{
        outp = inp = p;
        p = cotoken(p);
        if (*inp == '(') ++plvl;
        if (*inp == ')' && --plvl == 0) { --params; break; }
        if (plvl == 1 && *inp == ',') { --params; break; }
        if ( !(((toktyp+COFF)[*inp] == BLANK)
	        || (*inp == '\n')
	        || ((*inp == '/') && (*(inp+1) == '*'))
	        || ((*inp == '\\') && (*(inp+1)== '\n'))) )
	   havep = 1;
        while (inp < p)
	  { /* remove newlines in arguments, unless they are escaped */
          if (*inp == '\n' && inp [-1] != '\\') *inp = ' ';
          *ca++= *inp++;
          }
        if (ca > &acttxt [LINEBUFSIZE])
          pperror((catgets(nl_fn,NL_SETN,31,"%s: actuals too long")),sp->name);
        } /* for (;;) */
      if (pa >= &actual [MAXFRM])
        ppwarn ((catgets(nl_fn,NL_SETN,33, "%s: argument mismatch")), sp->name);
      else 
        *pa++=ca;
      } /* for (plvl... */
    } /* if (*inp == '(') */
  if (maclin != lineno [ifno])
    { /* embedded linefeeds in macro call */
    int   i;
    int   j = lineno [ifno];
    for (i = sizeof (int) / sizeof (char); --i >= 0;)
      {
      if (bob (p))
        {
        outp = p;
        inp  = p;
        p    = unfill (p);
        }
      *--p = j;
      j  >>= 8;
      }
    if (bob (p))
      {
      outp = inp = p;
      p    = unfill (p);
      }
    *--p = warnc;
    }
  if((params != 0) || (zerop && havep) || (!zerop && !havep))
    ppwarn ((catgets(nl_fn,NL_SETN,33, "%s: argument mismatch")), sp->name);
  while (--params >= 0)
    *pa++ = "" +1; /* null string for missing actuals */
  --flslvl;
  fasscan ();
  }

for (;;)
  { /* push definition onto front of input stack */
  while (! iswarn (*--vp))
    {
    if (bob (p))
      {
      outp = p;
      inp  = p;
      p    = unfill (p);
      }
    *--p = *vp;
    }
  if (*vp == warnc)
    { /* insert actual param */
    ca = actual [*--vp - 1];
    while (*--ca)
      {
      if (bob (p))
        {
        outp = inp = p;
        p    = unfill (p);
        }
      *--p = *ca;
      }
    }
  else 
    break;
  } /* for(;;) */
outp = p;
inp  = p;
return (p);
} /* subst */

char *trmdir (s) 
   register char *s;
{
      register char *p = s;

   while (*p++);
   --p;
   while (p > s && *--p != '/');
#if defined (unix) || defined (__unix) || defined(_HPUX_SOURCE)
   if (p == s) *p++ = '.';
#endif
   *p = '\0';
   return (s);
} /* trmdir */

STATIC char *copy (s) 
   register char *s;
{
      register char *old;

   old = savch;
   while (*savch++ = *s++);
   return (old);
} /* copy */

yywrap ()
{
   return (1);
}

main (argc, argv)
      int argc;
      char *argv [];
{
      register int   i, c;
      register char *p, **cp2;
      BOOLEAN recognize_dollar_sign = FALSE; 
      char          *tf, *lang, *ifilename = 0;
      char          *basename = (char *)strrchr(argv[0], '/');

   sbf = NULL;

#ifdef NLS
		/* initialize to the right language */
		if (!setlocale(LC_ALL,"")) {
			fputs(_errlocale(),stderr);
			nl_fn = (nl_catd)-1;
		}
		else
			nl_fn = catopen("cpp",0);
#endif /* NLS */

#if defined(C_PLUS_PLUS)
   /* Set up c_plus_plus if base name starts with "Cpp" */
   if (basename)
      basename++;
   else
      basename = argv[0];
   if (strncmp (basename, "Cpp", 3) == 0)
      c_plus_plus = 1;
#endif /* C_PLUS_PLUS */

#if scw2
   for ((t23 + COFF) [i = ALFSIZ + 7 - COFF] = 1; --i >= -COFF;)
   if (((t23 + COFF) [i] = (t23 + COFF + 1) [i] << 1) == 0)
      (t23 + COFF) [i] = 1;
#endif
#if defined (unix) || defined (__unix) || defined (_HPUX_SOURCE)
   fnames [ifno = 0] = ""; 
#ifdef OSF
   dirnams [0] = sys_dirs [0] = pri_dirs [0] = ".";
#else
   dirnams [0] = dirs [0] = ".";
#endif /* OSF */
#endif
#if defined ( ibm)
   fnames [ifno = 0] = "";
#endif
   for (i = 1; i < argc; i++) {
      switch (argv [i] [0]) {
         case '-':
            switch (argv [i][1]) {
	       case 'A': posflag++; continue;
               case 'P': pflag++;
               case 'Y':  /* -Y and -n are NLS which is handled by default */
               case 'n':
               case 'E':            continue;
#ifdef PROTOGEN
               case 'Q': protogenflag++;   continue;
#endif /* PROTOGEN */
               case 'R': rflag++;   continue;
               case 'C': passcom++; continue;
#ifdef CXREF
               case 'F':
                  if ((outfp = fopen (argv[++i], WRITE)) == NULL) {
                     fprintf (stderr, (catgets(nl_fn,NL_SETN,36, "Can't open %s\n")), argv[i]);
                     exit(1);
                  }
		  doCXREFdata = 1;
                  continue;
#endif
               case 'D':
                  if (predef>=prespc+NPREDEF) {
                     pperror ((catgets(nl_fn,NL_SETN,37, "too many -D options, ignoring %s")),
                              argv[i]);
                     continue;
                  }
                  /* ignore "-D" (no argument) */
                  if (*(argv [i] + 2)) *predef++ = argv [i] + 2;
                  continue;
               case 'U':
                  if (prund >= punspc + NPREDEF) {
                     pperror ((catgets(nl_fn,NL_SETN,38, "too many -U options, ignoring %s")),
                              argv[i]);
                     continue;
                  }
                  /* ignore "-U" (no argument) */
                  if (*(argv [i] + 2)) *prund++ = argv [i] + 2;
                  continue;
#ifdef OSF
               case 'h':
                  ShowFiles = TRUE;
                  continue;
               case 'M': /* print the dependents */
                  ShowDependents = TRUE;
                  continue;
	       case 'v':
		  print_version();
		  continue;
               case 'y':
                  if (sys_nd > MAXINC - 4) 
                     pperror ((catgets(nl_fn,NL_SETN,56, "excessive -y file (%s) ignored")), 
                              argv[i]);
                  else 
                     sys_dirs [sys_nd++] = argv [i] + 2;
                  continue;
               case 'p':
                  if (pri_nd > MAXINC - 4) 
                     pperror ((catgets(nl_fn,NL_SETN,57, "excessive -p file (%s) ignored")), 
                              argv[i]);
                  else 
                     pri_dirs [pri_nd++] = argv [i] + 2;
                  continue;
               case 'I':
                  if ((sys_nd > MAXINC - 4) || (pri_nd > MAXINC - 4) ) 
                     pperror ((catgets(nl_fn,NL_SETN,39, "excessive -I file (%s) ignored")), 
                              argv[i]);
                  else 
                     {
                     sys_dirs [sys_nd++] = argv [i] + 2;
                     pri_dirs [pri_nd++] = argv [i] + 2;
                     }
                  continue;
#else
               case 'I':
                  if (nd > MAXINC - 4) 
                     pperror ((catgets(nl_fn,NL_SETN,39, "excessive -I file (%s) ignored")), 
                              argv[i]);
                  else 
                     dirs [nd++] = argv [i] + 2;
                  continue;
#endif /* OSF */
               case 'J': /* remove /usr/include from directory search list: */
                  jseen = 1;
                  continue;
               case '\0': continue;
               case 'T':        /* backward name comp. */
                  ncps = 8;
                  continue;
               case 'H': /* modify define table size */
                  if (argv[i][2] != 0)
                     sbsize = atoi (argv[i] + 2);
                  else if (i + 1 > argc) {
                     pperror 
                       ((catgets(nl_fn,NL_SETN,40, "missing table size after -H option, -H option ignored")));
                     continue;
                  } else 
                     sbsize = atoi(argv[++i]);
                  if (sbsize == 0) {
                     pperror 
                       ((catgets(nl_fn,NL_SETN,41, "Invalid define table size specified")));
                     sbsize = SBSIZE;
                  }
                  /* if -H sets it to less than default, ignore the -H: */
                  if (sbsize < SBSIZE) sbsize = SBSIZE;
                  if ((sbsize % 4) != 0) /* round to words */
                  sbsize += 4 - (sbsize % 4);
                  /* if we've made it siginificantly larger
                   * then increase the symbol table size: */
                  if (sbsize > (SBSIZE * 1.15)) symsiz = sbsize / 20;
                  continue;
#if defined (STAT_ANAL)
               case 'X':
		  emit_SA++;
		  SA_filename = argv[++i];
		  if ((SA_file = fopen (SA_filename, WRITE)) == NULL)
		    {
		    fprintf (stderr,
			     (catgets(nl_fn,NL_SETN,36, "Can't open %s\n")),
			     SA_filename);
		    exit(1);
		    }
#endif
#if defined (C_PLUS_PLUS)
               case 'Z':
                   c_plus_plus = 1;
                   continue;
#endif  /* C_PLUS_PLUS */
               case '$':
                  recognize_dollar_sign = TRUE; /* Recognize $ in identifier */
                  continue;                     /* SR#4700916783 - choang    */
               default: 
                  pperror ((catgets(nl_fn,NL_SETN,42, "unknown flag %s")), argv[i]);
                  continue;
            } /* switch (argv [i] [1]) */
         default:
            if (fin==stdin) {
               if (NULL == (fin=fopen(argv[i], READ))) {
                  switch (errno) {
                     case ENOENT:
                          pperror ((catgets(nl_fn,NL_SETN,43, "No source file %s")), argv[i]); 
                          break;
                     case EACCES: 
                          pperror ((catgets(nl_fn,NL_SETN,51, "Access permission denied for %s")), argv[i]);
                          break;
                     case ENAMETOOLONG: 
                          pperror ((catgets(nl_fn,NL_SETN,52, "Pathname too long for file %s")), argv[i]);
                          break;
                     case ENFILE: 
                          pperror ((catgets(nl_fn,NL_SETN,53, "System File Table full.  Can't open %s")), argv[i]);
                          break;
                     case ENOTDIR: 
                          pperror ((catgets(nl_fn,NL_SETN,54, "Invalid Pathname for %s")), argv[i]);
                          break;
                     default:   
                          pperror ((catgets(nl_fn,NL_SETN,55, "Can't open %s")), argv[i]);
                          break;
                  }
                  exit(8);
               }
               ifilename = argv[i];
            } else if (fout == stdout) {
               /* Too dangerous to have a file name in the same syntactic 
                * position be input or output file depending on file 
                * redirections, so force output to stdout, willy-nilly
                * [i don't see what the problem is.  jfr] */
                  static char _sobuf [BUFSIZ];

               if (NULL == (fout = fopen (argv [i], "w"))) {
                  pperror ((catgets(nl_fn,NL_SETN,44, "Can't create %s")), argv[i]); 
                  exit(8);
               } else {
                  fclose (stdout); 
                  setbuf (fout, _sobuf);
               }
            } else 
               pperror((catgets(nl_fn,NL_SETN,45, "extraneous name %s")), argv[i]);
      } /* switch (argv[i][0]) */
   } /* for each arg */

   p = "$_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
/* in OSF we handle $ by default */
/* 910906 curtw.  No longer.  Reqire "-$" to handle '$' */
/* #ifndef OSF */
   if (!recognize_dollar_sign) /* skip over the $ char */
      p++;
/* #endif */
   i = 0;

   while (c = *p++) {
      (fastab + COFF) [c] |= IB | NB | SB;
      (toktyp + COFF) [c]  = IDENT;
#if scw2
      /* 53 == 63 - 10; digits rarely appear in identifiers,
       * and can never be the first char of an identifier.
       * 11 == 53 * 53 / sizeof (macbit). */
      ++i;
      (t21 + COFF) [c] = (53 * i) / 11;
      (t22 + COFF) [c] = i % 11;
#endif
   }
   p = "0123456789.";
   while (c =  *p++) {
      (fastab + COFF) [c] |= NB | SB;
      (toktyp + COFF) [c]  = NUMBR;
   }
   p = "\n\"'/\\";
   while (c =  *p++) (fastab + COFF) [c] |= SB;
   p = "\n\"'\\";
   while (c =  *p++) (fastab + COFF) [c] |= QB;
   p = "*\n"; while (c =  *p++) (fastab + COFF) [c] |= CB;
   (fastab + COFF) [warnc] |= WB | SB;
   (fastab + COFF) [ '\0'] |= CB | QB | SB | WB;
   for (i = ALFSIZ; --i >= 0;) slotab [i] = fastab [i] | SB;
   p = " \t\013\f\r"; /* note no \n; \v not legal for vertical tab? */
   while (c =  *p++) (toktyp + COFF) [c] = BLANK;
#if scw2
   for ((t23 + COFF) [i = ALFSIZ + 7 - COFF] = 1; --i >= -COFF;)
   if (((t23 + COFF) [i] = (t23 + COFF + 1) [i] << 1) == 0)
      (t23 + COFF) [i] = 1;
#endif
            

   if (sbf == NULL) init();

#ifdef OSF
   if (ShowFiles)
      fprintf(stderr, "%*s%s\n", ifno*4, "", ifilename);
   if (ShowDependents) {
      char *object_file;
      extern char *malloc();
      int len = strlen(ifilename);

      if (! (object_file = malloc (len+1+2))) { 
         pperror ((catgets(nl_fn,NL_SETN,50, "Can't allocate object file")));
         exit (8);
      }
      /* we needs two extra bytes for the filename which
         is not ended with ".c" */
      strcpy(object_file,ifilename);
      if( strcmp(object_file+len-2, ".c") == 0 )
              *(object_file+len-1) = 'o';
      else
              strcat(object_file,".o");
      fprintf(stderr, "%s : %s", object_file,ifilename);
      linesize = LINESIZE - strlen(object_file) - 2 - 1 - strlen(ifilename);
      }
#endif /* OSF */

   if (ifilename) {
      fnames [ifno] = copy(ifilename);
#ifdef CXREF
	 if(doCXREFdata)
		{
		char *fp;
		fp = fnames[ifno];
		if( (*fp == '.') && (*(fp+1) == '/'))
		  fp += 2;
		fprintf (outfp, "\"%s\"\n", fp);
		}
#endif
#ifdef OSF
      sys_dirs [0] = pri_dirs [0] = dirnams [ifno] = trmdir (ifilename);
#else
      dirs [0] = dirnams [ifno] = trmdir (ifilename);
#endif /* OSF */
   }
   fins [ifno] = fin;
   exfail      = 0;
   /* After user -I files here are the standard include libraries: */
#if (defined (unix)  || defined (__unix) || defined (_HPUX_SOURCE)) && defined (ENVINCL)
#ifdef OSF
   pri_dirs[pri_nd++] = sys_dirs[sys_nd++] = getenv ("INCLUDIR");
   if (! pri_dirs[pri_nd])
      if (jseen == 0) {
         sys_dirs [--sys_nd] = "/usr/include";
         pri_dirs [--pri_nd] = "/usr/include";
         sys_nd++;
         pri_nd++;
      }
#else
   if (! (dirs [nd++] = getenv ("INCLUDIR")))
      if (jseen == 0) {
         dirs [--nd] = "/usr/include";
         nd++;
      }
#endif /* OSF */
#else
#ifdef OSF
   if (jseen == 0)
      sys_dirs [sys_nd++] = pri_dirs [pri_nd++] = "/usr/include";
#else
   if (jseen == 0) dirs [nd++] = "/usr/include";
#endif /* OSF */
#endif
#if defined ( ibm)
#ifdef OSF
   sys_dirs [sys_nd++] = pri_dirs [pri_nd++] = "BTL$CLIB";
#else
   dirs [nd++] = "BTL$CLIB";
#endif /* OSF */
#endif
#ifdef OSF
   sys_dirs [sys_nd++] = pri_dirs [pri_nd++] = 0;
#else
   dirs [nd++] = 0;
#endif /* OSF */
   defloc  = ppsym ("define");
   udfloc  = ppsym ("undef");
   incloc  = ppsym ("include");
   elsloc  = ppsym ("else");
   elifloc = ppsym ("elif");
   eifloc  = ppsym ("endif");
   ifdloc  = ppsym ("ifdef");
   ifnloc  = ppsym ("ifndef");
   ifloc   = ppsym ("if");
   lneloc  = ppsym ("line");
   idtloc  = ppsym ("ident");
   pragloc = ppsym ("pragma");
   for (i = sizeof (macbit) / sizeof (macbit [0]); --i >= 0;) 
      macbit[i] = 0;
#if defined (ENVDEF) && (defined (hp9000s800) || defined (__hp9000s800))
   {     char *importpath(), *cp, **define_flag;
         int   flag_count = 0;
      if (cp = getenv ("ST_DEFINE")) {
         define_flag = (char **)importpath (cp);
         while (define_flag [flag_count])
            stsym(define_flag [flag_count++]);
      }
   }
#endif
   if(posflag)
    {
	/* provide the POSIX clean name space */
#if defined(hp9000s200) || defined(hp9000s300) || defined(__hp9000s200) || defined(__hp9000s300)  
	varloc = stsym ("__hp9000s200");
	varloc = stsym ("__hp9000s300");
#endif
#if defined (unix) 
	ysysloc = stsym ("__unix");
#endif
#if defined (hp9000s800) || defined (__hp9000s800)
	varloc = stsym ("__hp9000s800");
	varloc = stsym ("__hppa");
#endif
#if defined (__hpux) || defined (hpux)
	varloc = stsym ("__hpux");
#endif
    }
   else
    {
#if defined (unix) || defined (_HPUX_SOURCE)
	ysysloc = stsym ("unix");
	ysysloc = stsym ("__unix");
#endif
#if defined ( ibm)
	ysysloc = stsym ("ibm");
#endif
#if defined ( __hppa)    /* Added 2/28/89 - Christopher Hoang */
	varloc = stsym ("__hppa");
#endif
#if defined ( PWB) || defined ( _PWB)
	varloc = stsym ("PWB");
  	varloc = stsym ("_PWB");
#endif
#if defined ( hp9000s500)
	varloc = stsym ("hp9000s500");
#endif
#if defined(hp9000s200) || defined (__hp9000s300)
	varloc = stsym ("hp9000s200");
	varloc = stsym ("hp9000s300");
	varloc = stsym ("__hp9000s200");
	varloc = stsym ("__hp9000s300");
#endif
#if defined ( HFS)
	varloc = stsym ("HFS");
#endif
#if defined(hp9000s800) || defined (__hp9000s800)
	varloc = stsym ("__hp9000s800");
	varloc = stsym ("hp9000s800");
	varloc = stsym ("hppa");
#endif
#if defined(hpux) || defined(_hpux)
	varloc = stsym ("hpux");
	varloc = stsym ("__hpux");
#endif
#if defined ( RES)
	varloc = stsym ("RES");
#endif
    }
   ulnloc = stsym ("__LINE__");
   uflloc = stsym ("__FILE__");
      
   tf = fnames [ifno];
#ifndef NLS
   fnames [ifno]="command line";
#else
   {     static char buf[40];
      strcpy (buf, (catgets(nl_fn,NL_SETN,46, "command line")));
   }
#endif
   lineno [ifno] = 1;
   cp2 = prespc;
   while (cp2 < predef) stsym (*cp2++);
   cp2 = punspc;
   while (cp2 < prund) {
      if (p = strchr (*cp2, '=')) 
         *p++ = '\0';
      /* Truncate to ncps characters if necessary */
      if (strlen (*cp2) > ncps)
         (*cp2) [ncps] = '\0';
      lookup (*cp2++, DROP);
   }
   fnames [ifno] = tf;
   pbeg       = buffer + NAMESIZE; 
   pbuf       = pbeg   + LINEBUFSIZE;
   pend       = pbuf   + LINEBUFSIZE;
   trulvl     = flslvl = iflvl = 0;
   lineno [0] = 1; 

#ifdef STAT_ANAL
   if (predefs_done = emit_SA)
     fprintf (fout, "#pragma macrofile \"%s\"\n", SA_filename);
#endif
#ifdef PROTOGEN
   if (protogenflag)
     fprintf(fout,"# %d %s ", lineno [ifno], fnames[ifno]);
#endif /* PROTOGEN */
   sayline();
   outp       = inp = pend;
#ifdef CXREF
      if(doCXREFdata)
	  ready = 1;
#endif
   control(pend);
   if (fout && ferror(fout)) 
      pperror 
        ((catgets(nl_fn,NL_SETN,47, "Problems with output file; probably out of temp space")));
      return (exfail); 

} /* main */

#ifdef CXREF
ref (name, line)
   char *name;
   int     line;
{
   fprintf (outfp, "R%s\t%05d\n", name, line);
}

def (name, line)
   char *name;
   int line;
{
   if (ready) fprintf(outfp, "D%s\t%05d\n", name, line);
}

newf (name, line)
   char *name;
   int line;
{
   fprintf (outfp, "F%s\t%05d\n", name, line);
}
            
char *xcopy (ptr1, ptr2)
   register char *ptr1, *ptr2;
{
      static char    name [NAMESIZE];
      char          *saveptr, ch;
      register char *ptr3 = name;

   /* Locate end of name; save character there */
   if ((ptr2 - ptr1) > ncps) {
      saveptr = ptr1 + ncps;
      ch = *saveptr;
      *saveptr = '\0';
   } else {
      ch = *ptr2;
      *ptr2 = '\0';
      saveptr = ptr2;
   }
   while (*ptr3++ = *ptr1++); /* copy name */
   *saveptr = ch;             /* replace character */
   return (name);
} /* xcopy */
#endif /* CXREF */

init ()
   /* Set up the #define table, the symbol table and any other initialization 
    * types of things that may come along.  Both tables are filled with zeros,
    * just in case. */
{  
      extern char *malloc();
            
   /* Allocate define byte table */
            
   if (! (sbf = malloc (sbsize))) { 
      pperror ((catgets(nl_fn,NL_SETN,48, "Can't allocate define table")));
      exit (8);
   }
   savch = sbf;
   fillbytes ('\0', (char *) sbf, sbsize);
            
   /* Allocate symbol table */
            
   if (! (stab = (struct symtab *) malloc (symsiz * sizeof (struct symtab)))) {
      pperror ((catgets(nl_fn,NL_SETN,49, "Can't allocate symbol table ")));
      exit (8);
   }
   fillbytes ('\0', (char *) stab, symsiz * sizeof (struct symtab));
} /* init */

#if defined ( hp9000s500)
            
/* Simple fillbytes routine in focus assembly for the s500 */
            
fillbytes (byte, dest, cnt)
   char    byte,*dest;
   long int            cnt;
{
    asm ("
          ldb   Q-31  # load byte to fill with
          lds   Q-35  # load destination pointer
          lds   Q-39  # load number of bytes to fill
          filb  3     # fill bytes and pop stack
       ");
}
#else
            
   /* Simple fillbytes routine for 32-bit mc68000 and hp9000s800 */
   /* This version of fillbytes assumes the character to be filled is '\0' */

fillbytes (byte, dest, cnt)
   char    byte,*dest;
   long int            cnt;
{
   register int cnt4;
   register int *p = (int *) dest;

   cnt4 = cnt >> 2;   /* # 32-bit longs */
   for (; cnt4 > 0; cnt4--) *p++ = '\0';
}
#endif


#ifdef OSF

/*
**      print_version:  Print out version string and exit.
*/

print_version()
{
   printf ("\tcpp version: $Revision: 70.11 $\n");
   exit (0);
}

#endif /* OSF */
