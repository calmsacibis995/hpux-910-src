%{

#ifdef BUILDFROMH
#include <h/types.h>
#include <h/param.h>
#include <h/sysmacros.h>
#include <h/signal.h>
#else
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#endif

#include "../standard/inc.h"
#include "../net/netkludge.h"
#include "../standard/defs.h"
#include "../standard/externs.h"


#define YY_CLEAR { \
			int err ; \
			do { \
				err == yylex(); \
			} while ((err > 0) && (err != '\n')); \
	}


#define MAXPATH 80

int regs[26];
int sv, rv, rd; 
int val1, val2, val3, num;
int secval1, secval2;
extern int scanlength;
extern char scanbuf[];
char redpath[MAXPATH];
char string[80];
int cur;
extern int get();

%}

%start command

%token TBUF TFRAME TPORT TINODE TPDIR TPROC TSHMEM TSWBUF
%token TTEXT TDUMP BADCHAR TSHOW TSNAP TSCAN TSYMBOL TVMSTATS
%token TVMADDR TIOSTATS TSYM TSCANOP NUMBER TPATH TGR TGR2 TBINARY
%token TREALTIME TLTOR TSYM THELP TNOPTION
%token TCMHASH TCMTOPG TPGTOCM TBTOC TCTOB TPTOB TBTOP TVFILE
%token TKMXTOB TBTOKMX TUPADD TUVADD TPGTOPDE TSETMASK TSEARCH
%token TSTRING TCLTOM TXDUMP 
%token TPTYINFO TPTTY TTTY TSEMA TMUXDATA TMUXCAMDATA TMUXHWDATA
%token TCMAP TWIZ TFIXBUG TLOOK TSETOPT TLISTDUMP TLISTCOUNT
%token TPREGION TREGION TSWAPTAB TDBD TPFDAT TSYSMAP TSHMMAP
%token TVTOI TITOV TSAVESTATE TDRIVER TPRINT TPDIRHASH TQUIT
%token TVNODE TRNODE TCRED TDNLCSTATS TFILE
%token TSTKTRC
/* *****
   Commented out as a temp hack for networking
%token TNET TNETWORK TDTOM 
***** */

%left '|'
%left '&'
%left '+' '-'
%left '*' '/' '%'
%right UMINUS  /* supplies precedence for unary minus */

%% /* beginning of rules section */

command	:	'\n'
				{
				 YYACCEPT;
				}
	|	cmds
				{
			 	 YYACCEPT;
				}
	|	error '\n'
				{
				 yyerrok;
				 yyclearin;
				 fprintf(stderr, "unknown command\n");
				 YYABORT;
				}
	;

cmds	:
	|	TSHOW showopt oredir '\n'
					{dumptable($2,$3,redpath);}

	|	TREALTIME '\n'		{togglerealtime();}

	|	TLOOK '\n'		{lok();}

	|	TWIZ '\n'		{wzrd();}

	|	TFIXBUG '\n'		{fxbg();}

	|	TSNAP '\n'
					{snapshot();}
	|	TQUIT '\n'
					{quit();}

	|	TSETOPT scanopt '\n'	{
					clearoptions();
					processoptions(scanbuf);
					saveoptions();
					}

	|	TSCAN scanopt oredir '\n'
					{
					saveoptions();
					clearoptions();
					if (!processoptions(scanbuf))
						scan($3,redpath);
					restoreoptions();
					}
	 
	|	TDRIVER string '\n'
					{
					an_mgr_decode(outf,string);
					}
	|	TDRIVER '\n'
					{
					string[0] = '\0';
					an_mgr_decode(outf,string);
					}

	|	TBUF devblk opt oredir '\n'
			{
			 display_buf(val1,val2,$3,$4,redpath);
			}
	|	TBUF error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TPFDAT devblk opt oredir '\n'
			{
			 display_pfdat(val1,val2,$3,$4,redpath);
			}
	|	TPFDAT error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TPREGION devblk opt oredir '\n'
			{
			 display_pregion(val1,val2,$3,$4,redpath);
			}
	|	TPREGION error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TSYSMAP devblk opt oredir '\n'
			{
			 display_sysmap(val1,val2,$3,$4,redpath);
			}
	|	TSYSMAP error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TSHMMAP devblk opt oredir '\n'
			{
			 display_shmmap(val1,val2,$3,$4,redpath);
			}
	|	TSHMMAP error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TREGION devblk opt oredir '\n'
			{
			 display_region(val1,val2,$3,$4,redpath);
			}
	|	TREGION error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TSWAPTAB devblk opt oredir '\n'
			{
			 display_swaptab(val1,val2,$3,$4,redpath);
			}
	|	TSWAPTAB error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TDBD devblk opt oredir '\n'
			{
			 display_dbd(val1,val2,$3,$4,redpath);
			}
	|	TDBD error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TSWBUF devblk opt oredir '\n'
			{
			 display_swbuf(val1,$3,$4,redpath);
			}
	|	TSWBUF error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TCMAP devblk opt oredir '\n'
			{
			 display_cmap(val1,val2,$3,$4,redpath);
			}
	|	TCMAP error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TFRAME oexpr opt oredir '\n'
			{
			 display_frame($2,$3,$4,redpath,outf);
			}
	|	TFRAME error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TSEMA  oexpr opt oredir '\n'
			{
			 display_sema($2,$3,$4,redpath);
			}
	|	TSEMA  error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TPORT oexpr opt oredir '\n'
			{
			display_imcport($2,$3,$4,redpath,outf);
			}
	|	TPORT error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TINODE devino opt oredir '\n'
			{
			display_inode(val1,val2,$3,$4,redpath);
			}
	|	TINODE error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TVNODE oexpr oredir '\n'
			{
			display_vnode($2,$3,redpath);
			}
	|	TVNODE error '\n'
			{
			fprintf(stderr, "bad usage\n");
			}
	|	TRNODE oexpr opt oredir '\n'
			{
			display_rnode($2,$3,$4,redpath);
			}
	|	TRNODE error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TCRED  oexpr oredir '\n'
			{
			display_cred($2,$3,redpath);
			}
	|	TCRED error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TFILE oexpr opt oredir '\n'
			{
			display_file($2,$3,$4,redpath);
			}
	|	TFILE error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TPDIR oexpr opt oredir
			{
			display_pdir($2,$3,$4,redpath);
			}
	|	TPDIR error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TPDIRHASH spcaddr opt oredir '\n'
			{
			displaypdirhash(val1,val2,$3,$4,redpath);
			}
	|	TPDIRHASH error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TPROC oexpr opt oredir '\n'
			{
			display_proc($2,$3,$4, redpath);
			}
	|	TPROC error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TPRINT string oexpr oredir '\n'
			{
			display_print($2,$3,$4, redpath);
			}
	|	TPRINT error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
/* *****
   Commented out as a temp hack for networking
	|	TNET string oexpr opt oredir '\n'
			{
			net_display($2,$3,$4,$5,redpath);
			}
	|	TNET TNOPTION string '\n'
			{
			netopt($3);
			}
	|	TNET TNOPTION '\n'
			{
			netopt(0);
			}
	|	TNET THELP oredir '\n'
			{
			nethelp($3,redpath);
			}
	|	TNET error '\n'
			{ 
			fprintf(stderr,"usage: net <type> [addr][n]\n");
			fprintf(stderr,"       net option [opt]\n");
			fprintf(stderr,"       net help\n");
			}
***** */

  	|	TLISTCOUNT spcaddr oexpr oredir '\n'
			{
			listdisplay(TLISTCOUNT,val1,val2,0,0,$3,$4,redpath);
			}
	|	TLISTCOUNT error '\n'
			{
			fprintf(stderr,"bad usage\n");
			}
	|	TLISTDUMP spcaddrcnt opt oexpr oredir '\n'
          		{
			listdisplay(TLISTDUMP,val1,val2,val3,$3,$4,$5,redpath);
			}
	|	TLISTDUMP error '\n'
			{
			fprintf(stderr,"bad usage\n");
			}
	|	TMUXDATA oexpr opt oredir '\n'
                        {
                        mux_display(TMUXDATA,$2,$3,$4,redpath);
                        }
	|	TMUXDATA error '\n'
  			{
			fprintf(stderr,"bad usage\n");
			}
	|	TMUXCAMDATA oexpr opt oredir '\n'
			{
			mux_display(TMUXCAMDATA,$2,$3,$4,redpath);
			}
	|	TMUXCAMDATA error '\n'
			{
			fprintf(stderr,"bad usage\n");
			}
	|	TMUXHWDATA oexpr opt oredir '\n'
			{
			mux_display(TMUXHWDATA,$2,$3,$4,redpath);
			}
	|	TMUXHWDATA error '\n'
			{
			fprintf(stderr,"bad usage\n");
			}
	|	TPTYINFO oexpr opt oredir '\n'
			{
			pty_display(TPTYINFO,$2,$3,$4,redpath);
			}
	|	TPTYINFO error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TPTTY oexpr opt oredir '\n'
			{
			pty_display(TPTTY,$2,$3,$4,redpath);
			}
	|	TPTTY error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TTTY oexpr opt oredir '\n'
			{
			pty_display(TTTY,$2,$3,$4,redpath);
			}
	|	TTTY error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TSHMEM oexpr opt oredir '\n'
			{
			display_shmem($2,$3,$4,redpath);
			}
	|	TSHMEM error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TTEXT oexpr opt oredir '\n'
			{
			display_text($2,$3,$4, redpath);
			}
	|	TTEXT error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TLTOR error '\n'
			{
			fprintf(stderr, "bad usage\n");
			}
	|	TLTOR spcaddr oredir '\n'
			{
			display_ltor(val1,val2,$3,redpath);
			}
	|	THELP error '\n'
			{
			display_help(0,0);
			}
	|	THELP oredir '\n'
			{
			display_help($2,redpath);
			}
	|	TSYM  error '\n'
			{
			fprintf(stderr, "bad usage\n");
			}
	|	TSYM  oexpr oredir '\n'
			{
			display_sym($2,$3,redpath);
			}
	|	TDUMP oredir '\n'
			{
			dumpmem(-1,-1,-1,-1,$2, redpath);
			}
	|	TDUMP spcaddrcnt opt oredir '\n'
			{
			dumpmem(val1,val2,val3,$3,$4,redpath);
			}
	|	TDUMP error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TSAVESTATE oredir '\n'
			{
			displaysavestate(-1,-1,-1,-1,$2, redpath);
			}
	|	TSAVESTATE spcaddr opt oredir '\n'
			{
			displaysavestate(val1,val2,$3,$4,redpath);
			}
	|	TSAVESTATE error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TXDUMP oredir '\n'
			{
			xdumpmem(-1,-1,-1,$2, redpath);
			}
	|	TXDUMP spcaddrcnt oredir '\n'
			{
			xdumpmem(val1,val2,val3,$3,redpath);
			}
	|	TXDUMP error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TSTKTRC spcaddr secspcaddr '\n'
			{
			arb_stktrc (val1,val2,secval1,secval2);
			}
	|	TSTKTRC error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TSETMASK oredir '\n'
			{
			setmask(0,0);
			}
	|	TSETMASK valmsk oredir '\n'
			{
			setmask(val1,val2);
			}
	|	TSETMASK error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TSEARCH oredir '\n'
			{
			search(-1,-1,-1,$2, redpath);
			}
	|	TSEARCH spcaddrcnt oredir '\n'
			{
			search(val1,val2,val3,$3,redpath);
			}
	|	TSEARCH error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TBINARY redir '\n'
			{
			dumpbinary(-1,-1,-1,$2, redpath);
			}
	|	TBINARY spcaddrcnt redir '\n'
			{
			dumpbinary(val1,val2,val3,$3,redpath);
			}
	|	TBINARY error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	TVFILE  redir '\n'
			{
			dumpkernelbinary($2, redpath);
			}
	|	TVFILE  error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	equal_c

	|	colon_c

	|	dollar_c

	|	q_c

	|	h_c
	;


h_c	:	'?' '\n'
				{display_help(0,0);}
	|	'h' error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	|	'?' error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
		
	;


q_c	:	'q' '\n'
				{quit();}
	|	'q' error '\n'
			{ 
			fprintf(stderr, "bad usage\n");
			}
	;

equal_c	:	'=' expr '\n'
				{display_num($2);}
	|	'=' error '\n'
			{ 
			fprintf(stderr, "usage: = <expr>\n");
			}

	;

colon_c	:	':' error '\n'
			{
			fprintf(stderr,"usage: :subcmd no longer supported\n");
			}

	;

dollar_c:	'$' lc '=' expr '\n'
					{regs[$2 - 'a'] = $4;}
	|	'$' error '\n'
			{ 
			fprintf(stderr, "usage: $<a-z> = <value>\n");
			}
	;

opt	:	/* empty */
			{$$ =  0;}
	|	lc
			{$$ = $1;}
	;


oredir	:	/* empty */
			{$$ = rd = 0;}
	|	redir
			{$$ = rd = $1;}
	;

redir	:	TGR TPATH
			{$$ = 1; strncpy(redpath, yytext, MAXPATH);}
	|	TGR2 TPATH
			{$$ = 2; strncpy(redpath, yytext, MAXPATH);}
	;

showopt	:	/* empty */
			{$$ = 0;}
	|	TSYMBOL
			{$$ = 1;}
	|	TVMSTATS
			{$$ = 2;}
	|	TVMADDR
			{$$ = 3;}
	|	TIOSTATS
			{$$ = 4;}
/* *****
   Commented out as a temp hack for networking
	|	TNETWORK
			{$$ = 5;}
***** */
	|	TDNLCSTATS
			{$$ = 6;}
	;


scanopt	:	/* empty */
			{
			scanbuf[0] = '\0';
			}
	|	TSCANOP
			{ strncpy(scanbuf,yytext,80);
			  scanbuf[yyleng] = '\0';
			  scanlength = yyleng;
#ifdef DEBUG
			  fprintf(stderr,"length %d str=%s\n",yyleng,scanbuf);
#endif
			}
			
	;

offcnt	:	term
			{val1 = $1; val2 = -1;}
	|	term term
			{val1 = $1; val2 = $2;}
	;

valmsk	:	term
			{val1 = $1; val2 = 0;}
	|	term term
			{val1 = $1; val2 = $2;}
	;

spcaddr :	term '.' term
			{val1 = $1; val2 = $3;}
        |	term
			{val1 = 0; val2 = $1;}
	;

secspcaddr :	term '.' term
			{secval1 = $1; secval2 = $3;}
        |	term
			{secval1 = 0; secval2 = $1;}
	;

spcaddrcnt :	term '.' term term
			{val1 = $1; val2 = $3; val3 = $4;}
        |	term term
			{val1 = 0; val2 = $1; val3 = $2;}
	;

devino	:	/* empty */
			{val1 = -1; val2 = -1;}
     	|	term
			{val1 = $1; val2 = -1;}
	|	term term
			{val1 = $1; val2 = $2;}
	;

devblk	:	/* empty */
			{val1 = -1; val2 = -1;}
     	|	term
			{val1 = $1; val2 = -1;}
	|	term term
			{val1 = $1; val2 = $2;}
	;

string	:	TSTRING
			{$$ = (int)string; strncpy(string,yytext,80);}
	;

oexpr	:	/* empty */
			{$$ = -1;}
	|	expr
			{$$ = $1;}
	;

expr	:	expr '+' expr
			{$$ = $1 + $3;}
	|	expr '-' expr
			{$$ = $1 - $3;}
	|	expr '*' expr
			{$$ = $1 * $3;}
	|	expr '/' expr
			{$$ = ($3 == 0 ? 0 : $1 / $3);}
	|	expr '%' expr
			{$$ = ($3 == 0 ? 0 : $1 % $3);}
	|	expr '&' expr
			{$$ = $1 & $3;}
	|	expr '|' expr
			{$$ = $1 | $3;}
	|	term
			{$$ = $1;}
	;

term	:	TCMHASH '(' expr ')'
			{$$ = CMHASH($3);}
	|	TCMTOPG '(' expr ')'
			{$$ = cmtopg($3);}
	|	TPTOB 	'(' expr ')'
			{$$ = ptob($3);}
	|	TBTOP	 '(' expr ')'
			{$$ = btop($3);}
	|	TPGTOCM '(' expr ')'
			{$$ = pgtocm($3);}
	|	TBTOC 	'(' expr ')'
			{$$ = btoc($3);}
	|	TCTOB 	'(' expr ')'
			{$$ = ctob($3);}
	|	TKMXTOB '(' expr ')'
			{$$ = kmxtob($3);}
	|	TBTOKMX '(' expr ')'
			{$$ = ($3);}
	|	TUPADD	 '(' expr ')'
			{$$ = ($3);}
	|	TUVADD	 '(' expr ')'
			{$$ = ($3);}
	|	TPGTOPDE '(' expr ')'
			{$$ = pgtopde($3);}
	|	TVTOI '(' expr ')'
			{$$ = vtoi($3);} 
	|	TITOV '(' expr ')'
			{$$ = itov($3);} 
/* *****
   Commented out as a temp hack for networking
	|	TDTOM '(' expr ')'
			{$$ = dtom($3);}
***** */
    	|	'-' term 	%prec UMINUS
			{$$ = - $2;}
	|	'$' lc
			{$$ = regs[$2 - 'a'];}
	|	'@' term
			{$$ = get($2);}
	| 	'(' expr ')'
			{$$ = $2;}
	|	NUMBER

	;

lc	:	'a'
			{$$ = 'a';}
	|	'b'
			{$$ = 'b';}
	|	'c'
			{$$ = 'c';}
	|	'd'
			{$$ = 'd';}
	|	'e'
			{$$ = 'e';}
	|	'f'
			{$$ = 'f';}
	|	'g'
			{$$ = 'g';}
	|	'h'
			{$$ = 'h';}
	|	'i'
			{$$ = 'i';}
	|	'j'
			{$$ = 'j';}
	|	'k'
			{$$ = 'k';}
	|	'l'
			{$$ = 'l';}
	|	'm'
			{$$ = 'm';}
	|	'n'
			{$$ = 'n';}
	|	'o'
			{$$ = 'o';}
	|	'p'
			{$$ = 'p';}
	|	'q'
			{$$ = 'q';}
	|	'r'
			{$$ = 'r';}
	|	's'
			{$$ = 's';}
	|	't'
			{$$ = 't';}
	|	'u'
			{$$ = 'u';}
	|	'v'
			{$$ = 'v';}
	|	'w'
			{$$ = 'w';}
	|	'x'
			{$$ = 'x';}
	|	'y'
			{$$ = 'y';}
	|	'z'
			{$$ = 'z';}
	;


%%
#include "lex.yy.c"
yyerror(s)
char *s;
{
	return;
	/* printf("%s\n", s); */
}
