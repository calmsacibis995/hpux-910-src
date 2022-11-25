/* @(#) $Revision: 70.3 $ */      

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
 *
 *   If yynls_wchar is set, the actual data used by the state machine will
 *   be kept in the array yytextuc.  All operations are performed on this
 *   array.  It is not converted into wchar_t data until just before it is
 *   returned to the user in yylex().  mbstowcs is used to accomplish this.
 * 
 *   yylenguc (unsigned char value for yyleng) will also be used to keep
 *   track of the actual internal value. 
 *  
 *   If yyposix_point is set, the actual data used by the state machine will
 *   be held in the array yytextarr.  All operations are performed on this
 *   array.  The pointer yytext will be set to the beginning of this array
 *   at the beginning of yylex().
*/

#include <stdlib.h>
int yynls_wchar;
int yyposix_point;
char yytextarr[];

yyless(x)
{
extern char yytext[];
register char *lastch, *ptr;
extern int yyleng;
extern int yyprevious;

if (yynls_wchar == 1) { 
        /* NLS16 wchar_t support */
	extern unsigned char yytextuc[];
	extern int yylenguc;
	int xuchar=0,j;
	/* yytext is an array of wchar_t and is not what we want to look at. */
	/* when yynls_wchar is set, we want to use yytextuc and yylenguc.    */

	/* First determine the number of unsigned chars to keep. */
        for (j = 0; j < x; j++) 
		switch (mblen(&yytextuc[xuchar], sizeof(wchar_t))) {
			case  1: xuchar++;   /* this is a 1 byte character. */
				 break;
			case  2: xuchar = xuchar +2; /* two byte character. */
				 break;
			default: j = x;  /* null char or error, quit trying. */
				 break; 
			}

        if (x<0 || x > (int) yytext) 
		  /* x is a pointer into yytext.  The user has no business    */
                  /* giving us this but we'll try to do what he wants anyway. */
		x = (int) yytextuc + (x - (int) yytext);
	else if (x > yyleng && x < (int) yytext)
		  /* no idea what x is.  just forget the whole thing. */
		return(1);

	ptr = (char *) (xuchar +  yytextuc);
	lastch = (char *) (yytextuc + yylenguc);
	while (lastch > ptr)
		yyunput(*--lastch);
	yytext[x] = *lastch = 0;
	if ( (unsigned char *) ptr > yytextuc)
		yyprevious = *--lastch;
	yyleng = x;
	yylenguc = (unsigned char *) ptr - yytextuc;
	}
else { 
      /* yynls==0, no NLS, "normal" unsigned char code */

      if (yyposix_point == 0) {  /* standard uchar array code */
	       lastch = yytext+yyleng;
	       if (x>=0 && x <= yyleng)
		       ptr = x + yytext;
	       else
	       /*
	        * The cast on the next line papers over an unconscionable 
                * nonportable glitch to allow the caller to hand the function 
                * a pointer instead of an integer and hope that it gets 
                * figured out properly.  But it's that way on all systems .   
	        */
		       ptr = (char *) x;
	       while (lastch > ptr)
		       yyunput(*--lastch);
	       *lastch = 0;
	       if (ptr >yytext)
		       yyprevious = *--lastch;
	       yyleng = ptr-yytext;
	       }
       else {  /* %pointer was used, *uchar code. yytextarr points to yytext */
	       lastch = yytextarr+yyleng;
	       if (x>=0 && x <= yyleng)
		       ptr = x + yytextarr;
	       else
		       ptr = (char *) x;
	       while (lastch > ptr)
		       yyunput(*--lastch);
	       *lastch = 0;
	       if (ptr >yytextarr)
		       yyprevious = *--lastch;
	       yyleng = ptr-yytextarr;
	       }
       } /* end uchar code */
}
