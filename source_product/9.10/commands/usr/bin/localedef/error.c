/* @(#) $Revision: 70.4 $ */    
/* LINTLIBRARY */

#include <stdio.h>
#include "error.h"
#include "global.h"
extern int warn_flag;



/* error:p
** Error handling routine for errors encountered when reading buildlang script.
** print error message and exit.
*/
void
error(i)
int i;				/* index to error message array */
{
	extern int fprintf();
	extern void exit();
	extern int lineno;	/* line number from yylex() */

	switch (i) {
	  case KEY:
	  fprintf(stderr,(catgets(catd,NL_SETN,23, "localedef Error: invalid keyword on line %d\n")),lineno);
	  break;
	  case CHAR:
	  fprintf(stderr,(catgets(catd,NL_SETN,25, "localedef Error: invalid character on line %d\n")),lineno);
	  break;
	  case STATE:
	  fprintf(stderr,(catgets(catd,NL_SETN,26, "localedef Error: invalid statement on line %d\n")),lineno);
	  break;
	  case EXPR:
	  fprintf(stderr,(catgets(catd,NL_SETN,27, "localedef Error: invalid expression on line%d\n")),lineno);
	  break;
	  case RANGE:
	  fprintf(stderr,(catgets(catd,NL_SETN,28, "localedef Error: invalid range on line %d\n")),lineno);
	  break;
          case NAME_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,29, "localedef Error: invalid langname length on line  %d\n")),lineno);
	  exit(2);
	  case ID_RANGE:
	  fprintf(stderr,(catgets(catd,NL_SETN,30, "localedef Error: invalid langid number on line %d\n")),lineno);
	  break;
	case LANG_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,31, "localedef Error: invalid language name length in langname on %d\n")),lineno);
	  exit(2);
	case TERR_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,32, "localedef Error: invalid territory name length in langname %d\n")),lineno);
	  exit(2);
	case CODE_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,33, "localedef Error: invalid codeset name length in langname on line %d\n")),lineno);
	  exit(2);
	case REV_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,34, "localedef Error: invalid revision string length on line %d\n")),lineno);
	  exit(2);
	case CHR_CODE:
	  fprintf(stderr,(catgets(catd,NL_SETN,36, "localedef Error: invalid character code on line  %d\n")),lineno);
	  break;
	case FST_CODE:
	  fprintf(stderr,(catgets(catd,NL_SETN,37, "localedef Error: invalid isfirst character code on line %d\n")),lineno);
	  break;
	case SND_CODE:
	  fprintf(stderr,(catgets(catd,NL_SETN,38, "localedef Error: invalid issecond character code on line %d\n")),lineno);
	  break;
#ifdef EUC
	case CSCHM_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,39, "localedef Error: invalid code_scheme name length %d\n")),lineno);
	  exit(2);
	case CSCHM:
	  fprintf(stderr,(catgets(catd,NL_SETN,40, "localedef Error: invalid code_cscheme name %d\n")),lineno);
	  break;
	case CSWIDTH_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,41, "localedef Error: invalid cswidth string length %d\n")),lineno);
	  exit(2);
	case CSWIDTH_FMT:
	  fprintf(stderr,(catgets(catd,NL_SETN,42, "localedef Error: invalid cswidth string format %d\n")),lineno);
	  break;
#endif /* EUC */
	case INFO_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,43, "localedef Error: invalid langinfo string length %d\n")),lineno);
	  exit(2);
	case MNTRY_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,44, "localedef Error: invalid monetary string length  %d\n")),lineno);
	  exit(2);
	case NMRC_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,45, "localedef: invalid numeric string length %d\n")),lineno);
	  exit(2);
	case MOD_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,46, "localedef Error: invalid modifier string length %d\n")),lineno);
	  exit(2);
	case NOMEM:
	  fprintf(stderr,(catgets(catd,NL_SETN,47, "localedef Error: malloc() failed \n")));
	  exit(2);
	case Q_OVER:
	  fprintf(stderr,(catgets(catd,NL_SETN,48, "localedef Error: priority queue overflow \n")));
	  exit(2);
	case SAME_YESNO:
	  fprintf(stderr,(catgets(catd,NL_SETN,49, "localedef Error: yes and no strings can't have the same first char %d\n")),lineno);
	  exit(2);
	case ERA_LEN:
	  fprintf(stderr,(catgets(catd,NL_SETN,50, "localedef Error: invalid era string length %d\n")),lineno);
	  exit(2);
	case BAD_ERA_FMT:
	  fprintf(stderr,(catgets(catd,NL_SETN,51, "localedef Error: invalid era string format %d\n")),lineno);
	  break;
	case TOO_MANY_ERA:
	  fprintf(stderr,(catgets(catd,NL_SETN,52, "localedef Error: too many era strings are specified %d\n")),lineno);
	  exit(2);
	case SYM_NOT_FOUND:
	  fprintf(stderr,(catgets(catd,NL_SETN,53, "localedef Error: Specified symbol not found %d\n")),lineno);
	  break;
	case NO_CHARMAP:
	  fprintf(stderr,(catgets(catd,NL_SETN,58, "localedef Error: charmap file not found \n")));
	  break;
	case SYM_DUPLICATED:
	  fprintf(stderr,(catgets(catd,NL_SETN,60, "localedef Error: symbol duplicated %d\n")),lineno);
	  break;
	case WEIGHT_ELLIPSIS:
	  fprintf(stderr,(catgets(catd,NL_SETN,63, "localedef Error: more than COLL_WEIGHTS_MAX specified on line %d\n")),lineno);
	exit(2);
	case INV_COL_STMT:
	  fprintf(stderr,(catgets(catd,NL_SETN,82, "locale Error: invalid line in LC_COLLATE category: %d\n")), lineno);
	  break;
	}
	exit(4); /* changed from 1: Posix */
}


/* warnings
** warning handling routine for warnings encountered when reading 
** localedef script.
** print warning message and continue
*/
void
warning(i)
int i;				/* index to error message array */
{
	extern int fprintf();
	extern void exit();
	extern int lineno;	/* line number from yylex() */
	int stat=4;

	warn_flag++;
	switch (i) {
	case   SYM_NOT_FOUND:
	  fprintf(stderr,(catgets(catd,NL_SETN,68, "localedef warning: Specified symbol not found %d\n")),lineno);
	  break;
	case NOT_SUPP:
	  fprintf(stderr,(catgets(catd,NL_SETN,72, "localedef warning: keyword not supported %d\n")),lineno);
	  stat=2;
	  break;
	  case CTYPE_TAB:
          fprintf(stderr,(catgets(catd,NL_SETN,73, "localedef warning: Incorrect ctype initialization - corrective action taken \n")));
          break;
	 }

	/*
         * Posix -c option
         */
        if(!warn_flag)
	   exit(stat);
	else
	   exit_val = 1; /* warning and continue */

}



/* Error:
** Error handling routine for errors encountered other than reading script.
** print error message and exit.
*/
void
Error(msg, stat)
char *msg;			/* pointer to error message */
int stat;			/* exit stat: normally 2 or 4 
				 * 2 is used if a limit is exceeded
				 * 4 is used for other kinds of error
				 * according to P1003.2/D11.3
				 */
{
	extern int fprintf();
	extern void exit();

	fprintf(stderr, "localedef: ");
	fprintf(stderr, "%s\n", msg);
	/*
	 * Posix 
	 */

	exit(stat); /* exit value changed from 1 -> 4 means error: Posix */

}
