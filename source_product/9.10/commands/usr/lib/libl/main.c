/* @(#) $Revision: 70.5 $ */    
# include "stdio.h"
# include "locale.h"

unsigned char *yylocale; /* declared by lex in lex.yy.c files   */
                         /* consists of the locale at lex time. */

main(){
   char * stat;

   stat = setlocale(LC_ALL, yylocale ? yylocale : (unsigned char *)"");
   yylex();
   exit(0);
}
