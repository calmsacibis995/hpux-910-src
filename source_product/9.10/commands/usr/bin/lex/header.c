/* @(#) $Revision: 72.1 $ */      
/* header.c */
# include "ldefs.c"
# include "msgs.h"
# include <strings.h>

phead1(){
	ratfor ? rhd1() : chd1();
	}

chd1(){
        int i;
        char *yylocale = setlocale(LC_ALL,0);

	fprintf(fout,"# include \"stdio.h\"\n");
#ifdef NLS16
        if (nls16) {
	   fprintf(fout,"# ifndef _HPUX_SOURCE\n");
	   fprintf(fout,"#  define _HPUX_SOURCE\n");
	   fprintf(fout,"# endif\n");
	   fprintf(fout,"# ifndef _INCLUDE_XOPEN_SOURCE\n");
	   fprintf(fout,"#  define _INCLUDE_XOPEN_SOURCE\n");
	   fprintf(fout,"# endif\n");
	   fprintf(fout,"# define YYNLS16\n");
           if (nls_wchar) {
		fprintf(fout,"# define YYNLS16_WCHAR\n");
	   	fprintf(fout,"# include <stdlib.h>\n");
		}
	   fprintf(fout,"# include <nl_ctype.h>\n");
        }
#endif
	fprintf(fout,"#ifdef __cplusplus\n");
	fprintf(fout,"   extern \"C\" {\n");
	fprintf(fout,"     extern int yyreject();\n");
	fprintf(fout,"     extern int yywrap();\n");
	fprintf(fout,"     extern int yylook();\n");
	fprintf(fout,"     extern int main();\n");
	fprintf(fout,"     extern int yyback(int *, int);\n");
	fprintf(fout,"     extern int yyinput();\n");
	fprintf(fout,"     extern void yyoutput(int);\n");
	fprintf(fout,"     extern void yyunput(int);\n");
	fprintf(fout,"     extern int yylex();\n");
	fprintf(fout,"   }\n");
	fprintf(fout,"#endif	/* __cplusplus */\n");
	if (ZCH>NCH)
	fprintf(fout, "# define U(x) ((x)&0377)\n");
	else
	fprintf(fout, "# define U(x) x\n");
	fprintf(fout, "# define NLSTATE yyprevious=YYNEWLINE\n");
	fprintf(fout,"# define BEGIN yybgin = yysvec + 1 +\n");
	fprintf(fout,"# define INITIAL 0\n");
	fprintf(fout,"# define YYLERR yysvec\n");
	fprintf(fout,"# define YYSTATE (yyestate-yysvec-1)\n");
	if(optim)
		fprintf(fout,"# define YYOPTIM 1\n");
# ifdef DEBUG
	fprintf(fout,"# define LEXDEBUG 1\n");
# endif

#ifdef DOMAIN_OS
	fprintf(fout,"# define YYLMAX 400\n");
#else
# ifdef NLS16
        if (nls16 && !nls_wchar) /* only use with -m option. */
	   fprintf(fout,"# define YYLMAX 1024\n");
        else
# endif
	   fprintf(fout,"# define YYLMAX 200\n");
#endif

	fprintf(fout,"# define output(c) putc(c,yyout)\n");
	fprintf(fout, "%s%d%s\n",
  "# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==",
	ctable['\n'],
 "?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)");
	fprintf(fout,
"# define unput(c) {yytchar= (c);if(yytchar=='\\n')yylineno--;*yysptr++=yytchar;}\n");
	fprintf(fout,"# define yymore() (yymorfg=1)\n");
#ifdef NLS16
        if (nls_wchar)
	      fprintf(fout,"# define ECHO fprintf(yyout, \"%%s\",yytextuc)\n");
	else
#endif
	      fprintf(fout,"# define ECHO fprintf(yyout, \"%%s\",yytext)\n");
	fprintf(fout,"# define REJECT { nstr = yyreject(); goto yyfussy;}\n");
#if defined NLS || defined NLS16 || defined PAXDEV
 	fprintf(fout,"int yyleng;\n");
	fprintf(fout,"int yylenguc;\n");
	if (nls_wchar) {
 	       fprintf(fout,"extern unsigned char yytextuc[];\n");
 	       fprintf(fout,"extern wchar_t yytextarr[];\n");
               if (yytextarr == 1) {
 	               fprintf(fout,"extern wchar_t yytext[];\n");
 	               fprintf(fout,"int yyposix_point=0;\n");
                    }
               else { 
                       /* Support for POSIX %array / %pointer.  8/27/91  */
 	               fprintf(fout,"#define YY_PCT_POINT\n");
 	               fprintf(fout,"int yyposix_point=1;\n");
 	               fprintf(fout,"extern wchar_t *yytext;\n");
                    }
	       }
	else {
 	       fprintf(fout,"extern unsigned char yytextarr[];\n");
               if (yytextarr == 1) {
 	               fprintf(fout,"extern unsigned char yytext[];\n");
 	               fprintf(fout,"int yyposix_point=0;\n");
                    }
               else {
                       /* Support for POSIX %array / %pointer.  8/27/91  */
 	               fprintf(fout,"#define YY_PCT_POINT\n");
 	               fprintf(fout,"int yyposix_point=1;\n");
 	               fprintf(fout,"extern unsigned char *yytext;\n");
                    }
	       }
#ifdef NLS16
        if(nls16) {
 	   	fprintf(fout,"int yynls16=1;\n");
		if (nls_wchar) 
                   fprintf(fout,"int yynls_wchar=1;\n");
		else 
                   fprintf(fout,"int yynls_wchar=0;\n");
		}
        else
#endif
		{
 	   	fprintf(fout,"int yynls16=0;\n");
 	   	fprintf(fout,"int yynls_wchar=0;\n");
		}
#else
 	fprintf(fout,"int yyleng; extern unsigned char yytext[];\n");
 	fprintf(fout,"int yynls16=0, yynls_wchar=0;\n");
#endif
        /* yylocale output: To work around a cpp.ansi deficiency, translate  */
        /* ^A, ^B, ^C, and ^D into \001, \201, \003 and \004.              */
        fprintf(fout,"char *yylocale = \"");
        for (i=0; i < strlen(yylocale); i++)
            if ((yylocale[i] > 4) || (yylocale[i] < 1))
               fprintf(fout,"%c", yylocale[i]);
            else
               switch (yylocale[i]) {
                  case 1: fprintf(fout,"\\001"); break;
                  case 2: fprintf(fout,"\\002"); break;
                  case 3: fprintf(fout,"\\003"); break;
                  case 4: fprintf(fout,"\\004"); break;
                  }
        fprintf(fout,"\";\n");

	fprintf(fout,"int yymorfg;\n");
 	fprintf(fout,"extern unsigned char *yysptr, yysbuf[];\n");
	fprintf(fout,"int yytchar;\n");
	fprintf(fout,"FILE *yyin = {stdin}, *yyout = {stdout};\n");
	fprintf(fout,"extern int yylineno;\n");
	fprintf(fout,"struct yysvf { \n");
	fprintf(fout,"\tint yystoff;\n");
	fprintf(fout,"\tstruct yysvf *yyother;\n");
	fprintf(fout,"\tint *yystops;};\n");
	fprintf(fout,"struct yysvf *yyestate;\n");
	fprintf(fout,"extern struct yysvf yysvec[], *yybgin;\n");
	}

rhd1(){
	fprintf(fout,"integer function yylex(dummy)\n");

#ifdef DOMAIN_OS
	fprintf(fout,"define YYLMAX 1024\n");
#else
# ifdef NLS16
        if (nls16 && !nls_wchar) /* only use with -m option. */
	   fprintf(fout,"define YYLMAX 1024\n");
        else
# endif
	fprintf(fout,"define YYLMAX 200\n");
#endif

	fprintf(fout,"define ECHO call yyecho(yytext,yyleng)\n");
	fprintf(fout,"define REJECT nstr = yyrjct(yytext,yyleng);goto 30998\n");
	fprintf(fout,"integer nstr,yylook,yywrap\n");
	fprintf(fout,"integer yyleng, yytext(YYLMAX)\n");
	fprintf(fout,"common /yyxel/ yyleng, yytext\n");
	fprintf(fout,"common /yyldat/ yyfnd, yymorf, yyprev, yybgin, yylsp, yylsta\n");
	fprintf(fout,"integer yyfnd, yymorf, yyprev, yybgin, yylsp, yylsta(YYLMAX)\n");
	fprintf(fout,"for(;;){\n");
	fprintf(fout,"\t30999 nstr = yylook(dummy)\n");
	fprintf(fout,"\tgoto 30998\n");
	fprintf(fout,"\t30000 k = yywrap(dummy)\n");
	fprintf(fout,"\tif(k .ne. 0){\n");
	fprintf(fout,"\tyylex=0; return; }\n");
	fprintf(fout,"\t\telse goto 30998\n");
	}

phead2(){
	if(!ratfor)chd2();
	}

chd2(){
        if (yytextarr == 0) {
                /* support for %pointer / %array in POSIX 8/27/91 */
                fprintf(fout,"   yytext=yytextarr;\n");
                }
	fprintf(fout,"   while((nstr = yylook()) >= 0)\n");
#ifdef NLS16
 	if (nls_wchar) {  /* matching '}' is found in ctail() */
		fprintf(fout,"   { \n");
		/* code for making yytext an array of wchar_t */
		fprintf(fout,"#ifdef YYNLS16_WCHAR\n");
		fprintf(fout,"      yyleng = mbstowcs(yytext, (char *)yytextuc,yylenguc);\n");
		fprintf(fout,"      yytext[yyleng] = 0;\n");
		fprintf(fout,"#endif\n");
		}
#endif
	fprintf(fout,"yyfussy: switch(nstr){\n");
	fprintf(fout,"case 0:\n");
	fprintf(fout,"   if(yywrap()) return(0); break;\n");
	}

ptail(){
	if(!pflag)
		ratfor ? rtail() : ctail();
	pflag = 1;
	}

ctail(){
	fprintf(fout,"case -1:\nbreak;\n");		/* for reject */
	fprintf(fout,"default:\n");
	fprintf(fout,"   fprintf(yyout,\"bad switch yylook %%d\",nstr);\n");
#ifdef NLS16
	if (nls_wchar)
		fprintf(fout,"}\n"); /* matches '{' in chd2()*/
#endif
	fprintf(fout,"} return(0); }\n");
	fprintf(fout,"/* end of yylex */\n");
#ifdef hp9000s800
	fprintf(fout,"\nstatic void __yy__unused() { main(); }\n");
#endif /* hp9000s800 */
	}

rtail(){
	register int i;
	fprintf(fout,"\n30998 if(nstr .lt. 0 .or. nstr .gt. %d)goto 30999\n",casecount);
	fprintf(fout,"nstr = nstr + 1\n");
	fprintf(fout,"goto(\n");
	for(i=0; i<casecount; i++)
		fprintf(fout,"%d,\n",30000+i);
	fprintf(fout,"30999),nstr\n");
	fprintf(fout,"30997 continue\n");
	fprintf(fout,"}\nend\n");
	}
statistics(){
        message(STATISTIC, itos(b_1,tptr), itos(b_2,treesize), 
                itos(b_3,nxtpos-positions), itos(b_4,maxpos), itos(b_5,stnum+1),
                itos(b_6,nstates), itos(b_7,rcount));

        message(STATISTIC2, itos(b_1,pcptr-pchar), itos(b_2,pchlen));
	if(optim)  
           message(STATISTIC3, itos(b_1,nptr), itos(b_2,ntrans));
        message(STATISTIC4, itos(b_1,yytop), itos(b_2,outsize));
	}
