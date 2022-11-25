/* @(#) $Revision: 70.2 $ */      

/* Posix Functionality / NLS wchar_t support
 *      For the 9.0 release the size of these library files significantly
 *    increased.  This is due to two factors:  (1) The support for a wide
 *    character (wchar_t) declaration of yytext for NLS and (2) the
 *    support for %array and %pointer as required by POSIX.
 *
 *      Both of these enhancements cause the defining declaration of yytext
 *    in lex.yy.c to change.  The default declaration of yytext is as
 *    an array of unsigned characters.  When wide character support is
 *    turned on it changes to an array of wchar_t.  When %pointer is
 *    turned on it is a pointer to an unsigned character.  Our problem
 *    is that this library is already compiled when delevered to our users.
 *    We don't have the luxury of changing the declarations in this
 *    library.  We're stuck with a single declaration.
 *
 *      What we do here is make multiple copies of the code and only go
 *    through the code that properly references the data in yytext in
 *    a manner consistent with its declaration.  That is why we have so
 *    much extra almost duplicate code.  The code we go through is determined
 *    by the values of yynls_wcharr and yyposix_point.
 *
 *               Case                      yytext declaration in lex.yy.c
 *     --------------------------------  ---------------------------------
 *     yynls_wchar=0, yyposix_point=0 :   unsigned char yytext[];
 *     yynls_wchar=0, yyposix_point=1 :   unsigned char *yytext;
 *     yynls_wchar=1, yyposix_point=0 :   wchar_t yytext[];
 *     yynls_wchar=1, yyposix_point=1 :   wchar_t *yytext;
*/

# include <stdio.h>
extern FILE *yyout, *yyin;
extern int yyprevious , *yyfnd;
extern char yyextra[];
extern char yytext[], yytextarr[];
extern int yyleng;
extern struct {int *yyaa, *yybb; int *yystops;} *yylstate [], **yylsp, **yyolsp;

/* NLS 16 wchar_t support */
char yytextuc[];
int yylenguc;
int yynls_wchar;

/* Posix Support */
int yyposix_point;

yyreject ()
{
if (yynls_wchar) {
   for( ; yylsp < yyolsp; yylsp++)
           yytextuc[yylenguc++] = yyinput();
   if (*yyfnd > 0)
           return(yyracc(*yyfnd++));
   while (yylsp-- > yylstate)
           {
           yyunput(yytextuc[yylenguc-1]);
           yytextuc[--yylenguc] = 0;
           if (*yylsp != 0 && (yyfnd= (*yylsp)->yystops) && *yyfnd > 0)
                   return(yyracc(*yyfnd++));
           }
   if (yytextuc[0] == 0)
           return(0);
   yyoutput(yyprevious = yyinput());
   yyleng=0;
   yylenguc=0;
   return(-1);
   }
else { /* non-NLS uchar code */
      if (yyposix_point==0) { /* %array */
            for( ; yylsp < yyolsp; yylsp++)
	            yytext[yyleng++] = yyinput();
            if (*yyfnd > 0)
	            return(yyracc(*yyfnd++));
            while (yylsp-- > yylstate)
	            {
	            yyunput(yytext[yyleng-1]);
	            yytext[--yyleng] = 0;
	            if (*yylsp != 0 && (yyfnd= (*yylsp)->yystops) && *yyfnd > 0)
		            return(yyracc(*yyfnd++));
	            }
            if (yytext[0] == 0)
	            return(0);
            yyoutput(yyprevious = yyinput());
            yyleng=0;
            return(-1);
            }
      else {   /* %pointer */
            for( ; yylsp < yyolsp; yylsp++)
                    yytextarr[yyleng++] = yyinput();
            if (*yyfnd > 0)
                    return(yyracc(*yyfnd++));
            while (yylsp-- > yylstate)
                    {
                    yyunput(yytextarr[yyleng-1]);
                    yytextarr[--yyleng] = 0;
                    if (*yylsp != 0 && (yyfnd= (*yylsp)->yystops) && *yyfnd > 0)
                            return(yyracc(*yyfnd++));
                    }
            if (yytextarr[0] == 0)
                    return(0);
            yyoutput(yyprevious = yyinput());
            yyleng=0;
            return(-1);
            }
     }
}

yyracc(m)
{
if (yynls_wchar) {
   /* NLS wchar_t support */
   yyolsp = yylsp;
   if (yyextra[m])
           {
           while (yyback((*yylsp)->yystops, -m) != 1 && yylsp>yylstate)
                   {
                   yylsp--;
                   yyunput(yytextuc[--yylenguc]);
                   }
           }
   yyprevious = yytextuc[yylenguc-1];
   yytextuc[yylenguc] = 0;
   yytext[yyleng] = 0;
   return(m);
   }
else {
   if (yyposix_point==0) {
        /* "Normal" code. */
        yyolsp = yylsp;
        if (yyextra[m])
                {
                while (yyback((*yylsp)->yystops, -m) != 1 && yylsp>yylstate)
                        {
                        yylsp--;
                        yyunput(yytext[--yyleng]);
                        }
                }
        yyprevious = yytext[yyleng-1];
        yytext[yyleng] = 0;
        return(m);
        }
   else {
        /* Support for lex-time "%pointer" directive. */
        yyolsp = yylsp;
        if (yyextra[m])
	        {
	        while (yyback((*yylsp)->yystops, -m) != 1 && yylsp>yylstate)
		        {
		        yylsp--;
		        yyunput(yytextarr[--yyleng]);
		        }
	        }
        yyprevious = yytextarr[yyleng-1];
        yytextarr[yyleng] = 0;
        return(m);
        }
   }
}
