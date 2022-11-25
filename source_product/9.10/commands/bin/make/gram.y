/* @(#) $Revision: 66.11 $ */     

%{#include "defs"
#ifdef NLS16
#include <nl_ctype.h>
#endif


/******************************************************/
/************* DECLARATIONS SECTION *******************/
/******************************************************/

%}

/*  term is an anachronistic form of token  */

%term NAME SHELLINE START COLON DOUBLECOLON EQUAL A_STRING VERSION

/*  declare the type union for the internal yacc stack which */
/*  will store values returned from actions associated with  */
/*  detection of valid tokens by the lexical analyzer        */

%union
	{
	SHBLOCK yshblock;
	DEPBLOCK ydepblock;
	NAMEBLOCK ynameblock;
	CHARSTAR ycharstring;
	}


/*  associate union members declared above with nonterminals  */
/*  used in the grammar so the values associated with the     */
/*  nonterminals can be typed properly on the stack           */

%type	<yshblock>	SHELLINE, shlist, shellist
%type	<ynameblock>	NAME, namelist, name
%type	<ydepblock>	deplist, dlist
%type	<ycharstring>	A_STRING


%%

/******************************************************/
/****************  RULES SECTION **********************/
/******************************************************/

%{
   
/*  The global variables declared below should          */
/*  really be declared in the DECLARATIONS section.     */
/*  Their location here follows an anachronistic form   */
/*  of yacc specification files.                        */
/*  These variables are used by the actions in the      */
/*  grammar rules and by the lexical analyzer           */

DEPBLOCK pp;                /* linked list of NAMEBLOCKS */
FSTATIC SHBLOCK prevshp;    /* linked list of strings to be executed? */             

FSTATIC NAMEBLOCK lefts[NLEFTS];  /* array of 512 NAMEBLOCKS */
NAMEBLOCK leftp;                  /* doubly linked list of filenames? */
FSTATIC int nlefts;     

LINEBLOCK lp, lpp;         /*  linked lists of DEPBLOCKS and SHBLOCKS  */
FSTATIC DEPBLOCK prevdep;  
FSTATIC int sepc;
int last_status;
%}
 

/*  The action rules below follow an old syntactical form of */
/*  name : definition = { actions }                          */
/*  The = sign is not necessary anymore                      */

file:
	| file comline
	;

comline:  START
	| START macrodef
	| START namelist deplist shellist = {

          /* this action sets mainname to the first target to be made */
          /* don't understand the rest yet                            */

	    if(mainname == NULL && IS_OFF(INTRULE))
#ifndef NLS16
		if(lefts[0]->namep[0] != DOT || any(lefts[0]->namep, SLASH) )
			mainname = lefts[0];
#else
	    {  /* NLS: get current status of char, if single byte ... */
		last_status = ONEBYTE;   /*  reset to use in following while
					   statement */
		last_status = BYTE_STATUS(lefts[0]->namep[0], last_status);
		if( (lefts[0]->namep[0] != DOT
		    && (last_status == ONEBYTE || last_status == FIRSTOF2) )
		    || any(lefts[0]->namep, SLASH) )
			mainname = lefts[0];
	    }
#endif
	    while( --nlefts >= 0)
	    {
		leftp = lefts[nlefts];
		if(leftp->septype == 0)
			leftp->septype = sepc;
		else if(leftp->septype != sepc)
			fprintf(stderr, "Inconsistent rules lines for `%s'\n",
				leftp->namep);
		else {  
#ifdef NLS16
		/* NLS: I don't think this is needed but it can't hurt */
			last_status = ONEBYTE;   /* reset status */
			last_status = BYTE_STATUS( *(leftp->namep), last_status);
			if(sepc==ALLDEPS && (*(leftp->namep)!=DOT && 
				last_status == ONEBYTE) && $4!=0)
#else
			if(sepc==ALLDEPS && *(leftp->namep)!=DOT && $4!=0)
#endif
		{
			for(lp=leftp->linep; lp->nextline!=0; lp=lp->nextline)
			    if(lp->shp)
				fprintf(stderr, "Multiple rules lines for `%s'\n",
				    leftp->namep);
		}
		}
                /* we have parsed a target block - assign the   */
                /* dependency list and shell lines into the     */
                /* new lineblock structure which will be passed */
                /* back to become part of make's global vars    */

		lp = ALLOC(lineblock);
		lp->nextline = 0;
		lp->depp = $3;
		lp->shp = $4;

#ifdef	PATH
		if((equal(leftp->namep, ".SUFFIXES") ||
		    equal(leftp->namep, ".PATH")) && $3==0)
#else	PATH
		if(equal(leftp->namep, ".SUFFIXES") && $3==0)
#endif	PATH
			leftp->linep = 0;
		else if(leftp->linep == 0)
			leftp->linep = lp;
		else
		{
			for(lpp = leftp->linep; lpp->nextline!=0;
				lpp = lpp->nextline) ;
#ifndef NLS16
				if(sepc==ALLDEPS && leftp->namep[0]==DOT)
#else
				last_status = ONEBYTE;   /* reset status */
				last_status = BYTE_STATUS( *(leftp->namep), last_status);
				if(sepc==ALLDEPS && (leftp->namep[0]==DOT &&
					last_status == ONEBYTE))
#endif
					lpp->shp = 0;
			lpp->nextline = lp;
		}
	    }
	}
	| error
	;

          /* the next rule is where the firstvar global variable   */
          /* is filled with all macro names and values specified   */
          /* in the makefile(s) being parsed.  setvar inserts a    */
          /* new node into the firstvar linked list which contains */
          /* the NAME of the macro and its value (A_STRING)        */

macrodef:	NAME EQUAL A_STRING =
	{
		setvar($1, $3);
	}
	;

          /* the next rule allows one or more names in either a    */
          /* target list or a dependency list                      */
          /* a limit of NLEFTS is imposed on this rule             */
          /* The names are put into the lefts array from right     */
          /* to left, so the first name seen will be the last one  */
          /* in the list                                           */

namelist: name	= { lefts[0] = $1; nlefts = 1; }
	| namelist name	= { lefts[nlefts++] = $2;
	    	if(nlefts>NLEFTS) fatal("Too many lefts"); }
	;

          /* the next rule defines what what the terminals in the  */
          /* name can be - I have no idea, nor does anyone else    */
          /* what the VERSION thing is - no man pages mention this */
          /* feature                                               */
          /* the routines srchname and makename are declared in    */
          /* make code and locate and insert the names into        */
          /* NAMEBLOCK structures and put them into firstname      */
          /* and the hashtable variables                           */

name:	NAME =
	{
	if(($$ = srchname($1)) == 0)
		$$ = makename($1);
	}
	| NAME VERSION = 
	{
	if(($$ = srchname($1)) == 0)
		$$ = makename($1);
	}
	;

deplist:	= { fatal1("Must be a separator on rules line %d", yylineno); }
	| dlist
	;

         /* the next rule defines the dependency part of a target line */
         /* This includes the single or double colon and the list of   */
         /* dependencies.  The rule allows (a target,) (double)colon,  */
         /* and an empty dependency list, or a (double)colon  and one  */
         /* or more names.  Names are parsed right to left and are     */
         /* inserted into a new DEPBLOCK structure                     */      
         /* I am not clear on how this value gets out of the yacc      */
         /* variables into make's global variables                     */

dlist:  sepchar	= { prevdep = 0;  $$ = 0; }
	| dlist name	= {
			  pp = ALLOC(depblock);
			  pp->nextdep = 0;
			  pp->depname = $2;
			  if(prevdep == 0) $$ = pp;
			  else  prevdep->nextdep = pp;
			  prevdep = pp;
			  }
	;

         /* the next rule allows for two separators on a target line */
         /* see the defs file for an explanation of the difference   */
         /* between the two - this feature is NOT documented in our  */
         /* man page - it should be                                  */

sepchar:  COLON 	= { sepc = ALLDEPS; }
	| DOUBLECOLON	= { sepc = SOMEDEPS; }
	;

         /* the next rule allows an empty command sequence after a   */
         /* target line, or if there is something there, it puts     */
         /* the whole thing into the current yacc variable for       */
         /* further processing (?)                                   */

shellist:	= {$$ = 0; }
	| shlist = { $$ = $1; }
	;

         /* the next rule sticks each command line after a target line */
         /* into the current yacc variable.  I don't understand the    */
         /* rest of it -                                               */

shlist:	SHELLINE   = { $$ = $1;  prevshp = $1; }
	| shlist SHELLINE = { $$ = $1;
			prevshp->nextsh = $2;
			prevshp = $2;
			}
	;


%%


/******************************************************/
/***************** PROGRAMS SECTION *******************/
/******************************************************/

#include "ctype.h"
CHARSTAR zznextc;  /*zero if need another line; otherwise points to next char*/
int yylineno;
static unsigned char inmacro = NO;
static unsigned char hit1stkolon = NO;  /* this is an indicator that you */
                                        /* are parsing a target - dependency */
                                        /* line */

yylex()
/*******************************************************/
/* This function defines the user specified lexical    */
/* analyzer incorporated into the final yacc program   */
/* It parses the input stream, putting appropriate     */
/* values of action statements in yylval and returning */
/* token numbers to the parser.  It uses several       */
/* locally defined functions to do some of its work    */
/* (retsh and nextlin)                                 */
/*******************************************************/

{
	register CHARSTAR p;
	register CHARSTAR q;
	static unsigned char word[256];
	CHARSTAR pword;

#ifdef NLS16
	register int last_stat = ONEBYTE;   	/* NLS: status of curr char */

#define BSTATUS		last_stat = BYTE_STATUS( *zznextc, last_stat)
			/* macro to check status of character */
#else
#define BSTATUS
#endif

	pword = word;
	if(zznextc == 0)              /* need another line */
        {
                hit1stkolon = NO;     /* clear dependency line indicator */
        	return( nextlin() ); 
        }

	while( isspace(*zznextc) )    /* eat blanks from input stream */
		++zznextc;

	if(inmacro == YES)            /* a = was found in the stream */
	{
		inmacro = NO;         /* reset macro flag                  */
		yylval.ycharstring = copys(zznextc);  /* copy out value of */
                                      /* string to right of = sign         */
		zznextc = 0;          /* indicate new line needed next     */
		return(A_STRING);     /* token for end of macrodef rule    */ 
	}

	if(*zznextc == CNULL)         /* end of string indicator ?  */
        {
                hit1stkolon = NO;     /* clear dependency line indicator */ 
	        return( nextlin() );   
        }

	BSTATUS;		/* NLS: get character status */

#ifndef NLS16
	if ((*zznextc == KOLON) && (hit1stkolon == NO))
                        /* on new dependency line - found a colon */
#else
	if ((*zznextc == KOLON) && (last_stat == ONEBYTE) && (hit1stkolon == NO))
			/* check for single byte char */
#endif
	{
                hit1stkolon = YES;    /* set dependency line indicator */
		++zznextc;            /* skip over first colon */
		BSTATUS;

#ifndef NLS16
		if(*zznextc == KOLON)   
#else
		if(*zznextc == KOLON && last_stat == ONEBYTE)
#endif
		{
			++zznextc;           /* skip over next colon */
			return(DOUBLECOLON); 
		}
		else
			return(COLON);
	}

#ifndef NLS16
	if((*zznextc == EQUALS) && (hit1stkolon == NO))
                        /* found a macro definition */
#else
	if((*zznextc == EQUALS) && (last_stat == ONEBYTE) && (hit1stkolon == NO))
#endif
	{
		inmacro = YES;             /* set macro flag */
		++zznextc;                 /* eat the =      */
		return(EQUAL);
	}

#ifndef NLS16
	if(*zznextc == SKOLON)           /* at end of shell statement */
#else
	if(*zznextc == SKOLON && last_stat == ONEBYTE)
#endif	
        {
                hit1stkolon = NO;             /* don't expect another */
                                   /* dependency line until proven by */
                                   /* parsing a colon in next line's  */
                                   /* input                           */ 
		return( retsh(zznextc) );  /* a mystery yet */
        }

	p = zznextc;
	q = word;

#ifndef NLS16
	while( !( funny[*p] & TERMINAL) || /* for new regex syntax ... */
                ((hit1stkolon == YES) && ((*p == KOLON) || (*p == EQUALS))))
		*q++ = *p++;
#else
	while( last_stat == SECOF2 || !(funny[*p] & TERMINAL) ||   /* for new regex syntax ... */
               ((hit1stkolon == YES) && (last_stat == ONEBYTE) && 
                ((*p == KOLON) || (*p == EQUALS))))
	{
		*q++ = *p++;
		last_stat = BYTE_STATUS( *p, last_stat);	/* NLS: due to re- */
							/* assignment to p, the
							   char classification
							   is now done on P to
							   keep the byte stream
							   correct */
	}
#endif

	if(p != zznextc)
	{
		*q = CNULL;
		yylval.ycharstring = copys(pword);
#ifndef NLS16
		if(*p == RCURLY)
#else
		if(*p == RCURLY && last_stat == ONEBYTE)
#endif
		{
			zznextc = p+1;
			return(VERSION);
		}
#ifndef NLS16
		if(*p == LCURLY)
#else
		if(*p == LCURLY && last_stat == ONEBYTE)
#endif
			p++;
		zznextc = p;
		return(NAME);
		}

	else
	{
		fprintf(stderr,"Bad character %c (octal %o), line %d",
			*zznextc,*zznextc,yylineno);
		fatal(Nullstr);
	}
	return(0);	/* never executed */
}


retsh(q)
register CHARSTAR q;
/********************************************************/
/* This function takes the current input stream ? and   */
/* copies it into a new SHBLOCK structure and returns   */
/* a SHELLINE token.  I don't understand all the inner  */
/* workings of this yet.                                */
/* This thing is called in two places:                  */
/* by nextlin() when it detects a text line beginning   */
/* with a tab or a space (OK) OR by yylex when a ; is   */
/* detected in the stream.  The first one makes sense   */
/* but the second one doesn't yet                       */
/********************************************************/

{
	register CHARSTAR p;
	register int c;
	extern CHARSTAR *linesptr;
	SHBLOCK sp;

	for(p=q+1 ; *p==BLANK||*p==TAB ; ++p)  ;
               /* strip off leading tabs and blanks */

	sp = ALLOC(shblock);
	sp->nextsh = 0;
	sp->shbp = (fin == NULL ? p : copys(p) );  /* don't get this */
	yylval.yshblock = sp;                /* return new block */
	zznextc = 0;             /* now we need a new line */

/*  The following if-else statement eats up newlines within the   */
/*  remaining shell block in the input stream, preparing the      */
/*  input for further processing.                                 */

	if(fin == NULL)          /* input file pointer null */
	{                        /* clear data in linesptr array */
		if(linesptr[0])
			while(linesptr[1] && equal(linesptr[1], "\n"))
			{
				yylineno++;
				linesptr++;
			}
	}
	else                   /* input file pointer not NULL */
	{                      /* clear data in input file */
		while((c = GETC()) == NEWLINE)
			yylineno++;    /* eat newlines from input stream */
		if(c != EOF)           /* if not at end of file */
			ungetc(c, fin);  /* put the "next" character back in */
	}
	return(SHELLINE);
}

nextlin()
/************************************************************************/
/* This function is called by the lexical analyzer under two conditions,*/
/* either when zznextc is detected to be an empty pointer or when it    */
/* points to CNULL - the end of a string just analyzed                  */
/*                                                                      */
/* The function reads characters one at a time from the input.  The     */
/* input is either the global file pointer "fin", or special input in   */
/* a global string "linesptr" which is set up by callyacc when handling */
/* macro input from the command line.                                   */
/*                                                                      */
/* There are three basic parts to nextlin.  The first part determines   */
/* the input source and reads (or copies) the input source until it     */
/* detects a logical end of line.  The resulting "line" of input is     */
/* put into a local variable "text" as well as "yytext".  The second    */
/* part checks to see if the "line" thus read is an include line, and   */
/* sets up the new file to be included, if so. In this case, the new    */
/* file is opened for input and the routine jumps to the first part     */
/* again.  The third part does macro expansion on dependency lines.     */
/* Macro expansion of shell lines is done elsewhere.                    */
/*                                                                      */
/* I will add more comments later if I see that this needs to be more   */
/* detailed or corrected.                                               */
/* The routine returns two TERMINAL symbols, START directly, and        */
/* SHELLINE indirectly via a call to retsh()                            */
/* External variables (visible by yacc) affected by this routine are:   */
/*   zznextc                                                            */    
/************************************************************************/
{
    register unsigned char c;    /* where is this used?  */    
    register CHARSTAR p, t;      /* working pointer variables */
    static unsigned char yytext[INMAX]; /* global variable for yacc */
    CHARSTAR yytextl = yytext+INMAX;    /* keep the limit away from the array */
                                        /* else it(?) will overwrite the */
                                        /* limit and the test will fail */
    CHARSTAR text;               /* working variable holding yytext contents */
    unsigned char templin[INMAX];/* array holding macro expanded yytext */
    unsigned char lastch;        /* last character of original line */
    CHARSTAR lastchp;            /* pointer to last character in */
                                   /* unexpanded yytext          */
    extern CHARSTAR *linesptr;   /* holds input to yacc not from */
                                   /* regular input file but from  */
                                   /* command line input (callyacc)*/
    int incom;                   /* count of lines processed */
    int indeplist = 0;           /* on if KOLON detected in input */
    int kc;                      /* holds next character from input */
    int nflg;                    /* indicator of continued shell lines ? */ 
    int poundflg;                /* on if # detected in input */

#ifdef NLS16
    register int prev_stat = ONEBYTE;    /* status of previous char */
    register int last_stat = ONEBYTE;    /* status of current char */
#endif

again:
    incom = 0;          /* clear flag variables for a new line */
    zznextc = 0;
    poundflg = 0;

    if(fin == NULL)     /* input not coming from input file */
                        /* input provided in global variable linesptr */
    {
	if( (text = *linesptr++) == 0)  /* input string empty */
	    return(0);                 
	++yylineno;              /* input there */
	copstr(yytext, text);    /* copy input from linesptr into yytext */ 
    }
    else
    {
	yytext[0] = CNULL;      /* begin yytext with CNULL - why? */
	for(p=yytext ; ; ++p)   /* loop for input characters */
	{
	    if(p > yytextl)
		fatal("line too long");
 	    kc = GETC();        /* get next character from input */

#ifdef NLS16
	    if (kc != EOF)
            {
		prev_stat = last_stat;
		last_stat = BYTE_STATUS( kc, last_stat);
	    }
#endif
	    if(kc == EOF)      
	    {
		*p = CNULL;     /* terminate yytext */
		return(0);      /* RETURN - end of file found */
	    }

#ifndef NLS16
	    else
		if(kc == SKOLON)
#else
	    else
		if(kc == SKOLON && last_stat == ONEBYTE)
#endif
		    ++incom;    /* found a dependency line */
		else
		    if (kc == TAB && p == yytext)
			++incom;  /* found a shell line */
#ifndef NLS16
		    else
			if (kc==POUND && !incom && yytext[0] != TAB)
#else
		    else
			if ((kc==POUND && last_stat == ONEBYTE) 
				  && !incom && yytext[0] != TAB)
#endif
			{
			    poundflg++;  /* found a # - a comment? */
			    kc = CNULL;  /* null it out of the input */
			}
			else
			    if (kc == NEWLINE)
			    {
			        ++yylineno; /* increment line count for yacc */
#ifdef NLS16
				if(p==yytext || ((p[-1] == BACKSLASH)
                                  && prev_stat == SECOF2) || p[-1]!=BACKSLASH)
#else NLS16
                      /* determine if this line is a continuation of */
                      /* the previous one and if it is, continue on, */
                      /* if it isn't, break out - you are done with  */
                      /* the line                                    */
				if(p==yytext || p[-1] != BACKSLASH)
#endif NLS16
				    break;

                      /* you get here only if you are parsing a line */
                      /* with a continuation character on it         */
                      /* if you have a dependency or shell line,     */
                      /* add a newline to the input; otherwise       */
                      /* blank out the backslash and keep going      */ 

				if(incom || yytext[0] == TAB)
				    *p++ = NEWLINE;
				else
				    p[-1] = BLANK;

				nflg = YES;      /* indicate continued line? */
				while( kc = GETC())
				{
				    if(kc != TAB && kc != BLANK && kc != NEWLINE)
					break;
				    if(incom || yytext[0] == TAB)
				    {
					if(nflg == YES && kc == TAB)
					{
					    nflg = NO;
					    continue;
					}
					if(kc == NEWLINE)
				        {
					    nflg = YES;
					}

					*p++ = kc;
				    }
				    if(kc == NEWLINE)
					++yylineno;
				}

				if(kc == EOF)
				{
					*p = CNULL;
					return(0);
				}
			    }
	    *p = kc;  /* now stick the current character in yytext */
                      /* and loop for the next one                 */
	}
	*p = CNULL;     /* first pass processing of line finished  */
	text = yytext;  /* save working copy of yytext             */
    }

    c = text[0];       /* extract first character of text          */
                       /* they set yytext[0] to CNULL but the      */
                       /* next test expects to maybe find a tab    */
                       /* or blank in it - haven't figured out yet */
                       /* what the significance of all this is     */

    if(c == TAB)  
		return( retsh(text) ); /* retsh returns terminal SHELLINE    */
                               /* this test seems to indicate that   */
                               /* a shelline can begin with a space  */
                               /* which is contrary to the man page  */
                               /* which specified it must begin with */
                               /* a tab.  Obviously something else   */
                               /* is being defined here which I      */
                               /* haven't figured out yet            */ 

/*****************************************************/
/*  DO include FILES LINES HERE                      */
/*  The next "line" is parsed into yytext and text   */
/*  Now, check to see if the line is an include line */
/*  and if so, expand any macros on it, and then     */
/*  set up the input file so that the parser will    */
/*  now read input from that file instead of the     */
/*  current file being read - this provides the      */
/*  include feature of C in makefiles                */
/*                                                   */
/*  The syntax for an include line is rigidly        */
/*  enforced by the first if statement - the first   */
/*  seven characters must be "include" followed by   */
/*  at least one blank or tab.                       */
/*****************************************************/

    if(sindex(text, "include") == 0 && (text[7] == BLANK || text[7] == TAB))
    {
	CHARSTAR pfile;
	for(p = &text[8]; *p != CNULL; p++) /*for rest of line after include*/
	    if(*p != TAB && *p != BLANK)    /*strip blanks and tabs   */
                                            /*after the include word  */
                                            /*bug fix - used to be || */
		break;
        pfile = p;           /* save pointer to name to be included */

        for(; *p != CNULL && *p != NEWLINE &&
	      *p != TAB   && *p != BLANK; p++);  /* find end of include line */
	if(*p != CNULL)
	    *p = CNULL;            /* null terminate it if not already so */


        /* Start using new file, after doing macro expansion of the line */

	subst(pfile, templin, &templin[INMAX]); /* macro substitution        */
	fstack(templin, &fin, &yylineno);       /* stack new filename        */
	goto again;                             /* get next line in new file */
    }


/********************************************************************/
/* You are here if the line was not an include line or a shell line */
/* which leaves dependency lines or macro lines.  These lines may   */
/* contain macros which make knows about, either from reading in    */
/* macros from the command line, from the environment, or from      */
/* previous parses of MACRO terminals.  These lines must be run     */
/* through subst to expand the macros and some other string mucking */
/* done to clean up the final contents of the "current line" in     */
/* yytext.                                                          */
/********************************************************************/

   /* this first if statement determines if we have a dependency */
   /* line to expand. Dependency lines can begin with an alpha-  */
   /* numeric, a blank(?), a . for the internal macros make      */
   /* defines, or an 8-bit character                             */
   /* if a colon is detected in the line, indeplist is set       */

#ifndef NLS16
    if(isalpha(c) || isdigit(c) || c==BLANK || c==DOT ||
       (c & 0200) || c=='_') 
	for(p=text+1; *p!=CNULL; p++)
	    if(!indeplist && (*p == KOLON || *p == EQUALS))
            {
		if (*p == KOLON)
			++indeplist;
		break;
	    }
#else
    last_stat = FIRSTof2(c);	/* NLS: if first byte of 2 byte char */
    if (last_stat == 1)
	last_stat = BYTE_STATUS( text[1], last_stat);
    if((last_stat == SECOF2) || isalpha(c) || isdigit(c) || c==BLANK   
	   || c==DOT || (c & 0200) || c=='_' )                    /* NLS */
    {
		p = text;
		ADVANCE(p);           	/* NLS: put p to start of next char */
		for(; *p!=CNULL; ADVANCE(p))
	    	if(!indeplist && (*p == KOLON || *p == EQUALS))
         	{
				if (*p == KOLON)
	                ++indeplist;
				break;
			}
    }
#endif


    if(p == NULL || *p != EQUALS)      /* this condition is not understood */
    {
#ifndef NLS16
	for(t = yytext ; *t!=CNULL && *t!=SKOLON ; ++t);
#else
	for(t = yytext ; *t!=CNULL && *t!=SKOLON ; ADVANCE(t));
		/* advance character to start of next char */
#endif

	    lastchp = t;   /* save pointer to end of unexpanded yytext */
	lastch = *t;       /* save last character in yytext            */
	*t = CNULL;        /* overwrite last character in yytext       */
                           /* for subst call                           */

                           /* do macro expansion of yytext             */
	subst(yytext, templin, &templin[INMAX]);

	if(lastch)         /* if last char of yytext was a semicolon   */
	{
	    for(t = templin ; *t ; ++t); /* find end of expanded line  */
	    *t = lastch;                 /* tack last character on it  */
	    while( *++t = *++lastchp ) ; /* what ?                     */
	}

	p = templin;
	t = yytext;
	while( *t++ = *p++ );  /* copy expanded line into yytext       */
    }

    if(poundflg == 0 || yytext[0] != CNULL)
    {
	zznextc = text;        /* save old unexpanded line - why?      */
	return(START);
    }
    else
	goto again;
}

#include "stdio.h"
static int morefiles;       /* external static variables */
static struct sfiles        /* for use in GETC           */
{
	unsigned char sfname[64];
	FILE *sfilep;
	int syylno;
} sfiles[20];

GETC()
/****************************************************************************/
/* GETC automatically unravels stacked include files.  That is, during      */
/* include file processing, when a new file is encountered fstack will      */
/* stack the FILE pointer argument.  Subsequent calls to GETC with the new  */
/* FILE pointer will get characters from the new file.  When an EOF is      */
/* encountered, GETC will check to see if the file pointer has been         */
/* stacked.  If so, a character from the previous file will be returned.    */
/* The external references are "GETC()" and "fstack(fname,stream,lno)".     */
/* "Fstack(stfname,ream,lno)" is used to stack an old file pointer before   */
/* the new file is assigned to the same variable.  Also stacked are the     */
/* file name and the old current lineno, generally, yylineno.               */
/****************************************************************************/
{
    register int c;

    c = getc(fin);
    while(c == EOF && morefiles)
    {
	fclose(fin);
	yylineno = sfiles[--morefiles].syylno;
	fin = sfiles[morefiles].sfilep;
	c = getc(fin);
    }
    return(c);
}


fstack(newname, oldfp, oldlno)
register unsigned char *newname;
register FILE **oldfp;
register int *oldlno;
/*************************************************************/
/* set up proper input file for analyzer and parser          */
/*************************************************************/
{
    if(access(newname, 4) != 0)
   /* This get line can be removed if used elsewhere than make. */
    if(get(newname, CD, Nullstr) == NO)
	fatal1("Cannot read or get %s", newname);
    if(IS_ON(DBUG))
	printf("Include file: \"%s\"\n", newname);

/*  Stack the new file name, the old file pointer and the old yylineno */

    strcat(sfiles[morefiles].sfname, newname);
    sfiles[morefiles].sfilep = *oldfp;
    sfiles[morefiles++].syylno = *oldlno;
    yylineno = 0;
    if((*oldfp=fopen(newname, "r")) == NULL)
	fatal1("Cannot open %s", newname);
}
