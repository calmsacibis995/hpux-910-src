/* @(#) $Revision: 26.1 $ */      
# include <stdio.h>

yyerror( s ) char *s; {
	fprintf(  stderr, "%s\n", s );
	}
