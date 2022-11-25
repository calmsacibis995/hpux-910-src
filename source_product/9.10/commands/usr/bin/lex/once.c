/* @(#) $Revision: 70.3 $ */     
/* once.c */
	/* because of external definitions, this code should occur only once */

# ifdef ASCII

#if defined NLS || defined NLS16
int	ctable[2*NCH] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
100,101,102,103,104,105,106,107,108,109,
110,111,112,113,114,115,116,117,118,119,
120,121,122,123,124,125,126,127,128,129,
130,131,132,133,134,135,136,137,138,139,
140,141,142,143,144,145,146,147,148,149,
150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,168,169,
170,171,172,173,174,175,176,177,178,179,
180,181,182,183,184,185,186,187,188,189,
190,191,192,193,194,195,196,197,198,199,
200,201,202,203,204,205,206,207,208,209,
210,211,212,213,214,215,216,217,218,219,
220,221,222,223,224,225,226,227,228,229,
230,231,232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,247,248,249,
250,251,252,253,254,255};
#else
int	ctable[2*NCH] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
 100,101,102,103,104,105,106,107,108,109,
 110,111,112,113,114,115,116,117,118,119,
 120,121,122,123,124,125,126,127};
#endif

# endif

# ifdef EBCDIC
int ctable[2*NCH] {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
100,101,102,103,104,105,106,107,108,109,
110,111,112,113,114,115,116,117,118,119,
120,121,122,123,124,125,126,127,128,129,
130,131,132,133,134,135,136,137,138,139,
140,141,142,143,144,145,146,147,148,149,
150,151,152,153,154,155,156,157,158,159,
160,161,162,163,164,165,166,167,168,169,
170,171,172,173,174,175,176,177,178,179,
180,181,182,183,184,185,186,187,188,189,
190,191,192,193,194,195,196,197,198,199,
200,201,202,203,204,205,206,207,208,209,
210,211,212,213,214,215,216,217,218,219,
220,221,222,223,224,225,226,227,228,229,
230,231,232,233,234,235,236,237,238,239,
240,241,242,243,244,245,246,247,248,249,
250,251,252,253,254,255};
# endif
int	ZCH = NCH;
FILE	*fout = NULL, *errorf = {stdout};
int	sect = DEFSECTION;
int	prev = '\n';	/* previous input character */
int	pres = '\n';	/* present input character */
int	peek = '\n';	/* next input character */
uchar	*pushptr = pushc;
uchar	*slptr;

#ifdef OSF
#ifdef PAXDEV
char	*cname = "/paXdev/lib/ncform";
char	*ratname = "/paXdev/lib/nrform";
#else
char	*cname = "/usr/ccs/lib/ncform";
char	*ratname = "/usr/ccs/lib/nrform";
#endif /* PAXDEV */
#else
char	*cname = "/usr/lib/lex/ncform";
char	*ratname = "/usr/lib/lex/nrform";
#endif /* OSF */

# ifdef gcos
char *cname "pounce/lexcform";
char *ratname "pounce/lexrform";
# endif
int ccount = 1;
int casecount = 1;
int aptr = 1;
int nstates = NSTATES, maxpos = MAXPOS;
int maxpos_state = MAXPOS_STATE;
int treesize = TREESIZE, ntrans = NTRANS;
int yytop;
int outsize = NOUTPUT;
int sptr = 1;
int optim = TRUE;
int report = 2;
#ifdef NLS16
int nls16 = 0;
int opt_w_set = 0;
#endif
int nls_wchar = 0;
int debug;		/* 1 = on */
int charc;
int sargc;
char **sargv;
uchar buf[520];
int ratfor;		/* 1 = ratfor, 0 = C */
int yyline;		/* line number of file */
int eof;
int lgatflg;
int divflg;
int funcflag;
int pflag;
int chset;		/* 1 = char set modified */
FILE *fin, *fother;
int fptr;
int *name;
int *left;
int *right;
int *parent;
uchar *nullstr;
int tptr;
uchar pushc[TOKENSIZE];
uchar *slist;
uchar **def, **subs, *dchar;
uchar **sname, *schar;
uchar *stype;
uchar *ccl;
uchar *ccptr;
uchar *dp, *sp;
int dptr;
uchar *bptr;		/* store input position */
uchar *tmpstat;
int count;
int **foll;
int *nxtpos;
int *positions;
int *actemp, *acneg;	/* ptrs to space used in acompute */
int *gotof;
int *nexts;
uchar *nchar;
int **state;
int *sfall;		/* fallback state num */
uchar *cpackflg;	/* true if state has been character packed */
int *atable;
int nptr;
uchar symbol[NCH];
uchar cindex[NCH];
int xstate;
int stnum;
uchar match[NCH];
uchar *extra;
uchar *pchar, *pcptr;
int pchlen = TOKENSIZE;
 long rcount;
int *verify, *advance, *stoff;
int scon;
uchar *psave;
int buserr(), segviol();
int yytextarr=1;         /* default=1. 0=%pointer, 1=%array (from POSIX) */

/* sizes of internal tables, initialized to defaults */
int defsize = DEFSIZE;
int defchar = DEFCHAR;
int startsize = STARTSIZE;
int startchar = STARTCHAR;
int cclsize = CCLSIZE;
int nactions = NACTIONS;

int col_syms = None;

char ncformfile[256];/* name of alternate for /usr/lib/lex/ncform */
int alt_ncform = 0;  /* 1 if -L has been used (an internal undocumented opt.)*/

/* Used in error/warning message printing */
char b_[15],  b_1[15], b_2[15], b_3[15], b_4[15], b_5[15], b_6[15], b_7[15];
