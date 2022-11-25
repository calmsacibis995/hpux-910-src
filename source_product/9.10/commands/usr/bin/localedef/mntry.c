/* @(#) $Revision: 70.4 $ */     
/* LINTLIBRARY */

#include	<stdio.h>
#include	<limits.h>
#include	"global.h"

int	gotstr;
int 	gotnmbr;

/* mntry_init: initialize monetary table.
** Get here when you see a lconv-item keyword.
*/
void
mntry_init(token)
int	token;				/* keyword token */
{
	extern void mntry_str();
	extern void mntry_nmbr();
	extern void mntry_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	string = mntry_str;		/* set string function for mntry.c */
	number = mntry_nmbr;		/* set string function for mntry.c */
	finish = mntry_finish;		/* set up mntry finish */

	gotstr = FALSE;			/* no monetary string yet */
	gotnmbr = FALSE;		/* no monetary number yet */
}

/* mntry_str: process one monetary string.
** Check length of the monetary string, and remove quote characters.
*/
void
mntry_str()
{
	extern char *strcpy(),
		    *malloc();
	extern void getstr();
	unsigned char buf[LEN_INFO_MSGS];
	unsigned char *cp;
	int i;

	if (gotstr) error(STATE);		/* only one string allowed */

	(void) getstr(buf);			/* process monetary string */
	if (strlen(buf) > LEN_INFO_MSGS-1)	/* check length */
		error(MNTRY_LEN);
	if ((cp = (unsigned char *)malloc((unsigned) strlen((char *)buf)+1)) == NULL)
		error(NOMEM);

	switch (op) {
	   case INT_FRAC:      /*These guys should have integer */
           case FRAC_DIGITS:   /*parameters, but since many of */
           case P_CS:          /*standard locales have strings*/
           case P_SEP:         /*(such as "3") as the operand,*/
           case N_CS:          /*put this code to handle them.*/
           case N_SEP:         /*POSIX compliant scripts should*/
           case P_SIGN:        /*specify only integers as opernad*/
           case N_SIGN:	       /*(i.e., they get handled by mntry_nmbr)*/
		if ( (i=atoi(buf)) == -1 )
		   cp[0]=CHAR_MAX;
		else
		   cp[0]=(unsigned char)i;
		cp[1]='\0';
		break;
	   default:
		(void) strcpy((char *)cp, (char *)buf);
		if (num_value <= N_SIGN && buf[0] == '\0') {
		/* num_value <= N_SIGN are really chars, got a null char */
			buf[0] = CHAR_MAX;
			buf[1] = '\0';
	     	}
	}
	mntry_tab[num_value] = cp;
	gotstr = TRUE;				/* have one string */
}

void
mntry_nmbr()
{
	unsigned char tcp[2];
	unsigned char *cp;

	if (gotnmbr) error(STATE);
	
	switch (op) {
	  case INT_CURR:
	  case CURRENCY_LC:
	  case CURRENCY_LI:
	  case MON_DECIMAL:
	  case MON_THOUSANDS:
	  case NEGATIVE_SIGN:
	  case POSITIVE_SIGN:
		error(CHAR);
		break;
	  default:
		if ( num_value > -1 )
		   tcp[0]=(unsigned char) num_value;
		else
		   tcp[0]=CHAR_MAX;
		tcp[1]='\0';
	        if ((cp = (unsigned char *)malloc(strlen(tcp))) == NULL)
			error(NOMEM);
		strcpy(cp,tcp);
		mntry_tab[op] = cp;
	}
	gotnmbr = TRUE;
}

/* mntry_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Make sure that there are no left-over metachars or item number.
*/
void
mntry_finish()
{
	if (META) error(EXPR);

	if (!gotstr && !gotnmbr) {
		lineno--;
		error(STATE);
	}
}
