/* @(#) $Revision: 66.3 $ */    
/* LINTLIBRARY */

#include	<stdio.h>
#include	<limits.h>
#include	"global.h"

int	gotstr, gotid;
char	lname[_POSIX_NAME_MAX],
	territory[_POSIX_NAME_MAX],
	codeset[_POSIX_NAME_MAX];

/* name_init: initialize the process of language name.
** Get here when you see a 'langname' keyword.
*/
void
name_init(token)
int token;				/* keyword token */
{
	extern void name_str();
	extern void name_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	string = name_str;		/* set sting function for langname */
	finish = name_finish;		/* set up langname finish */

	gotstr = FALSE;			/* no language name initially */
}

/* name_str: process language name string.
** Check length of the name string, and remove quote characters.
*/
void
name_str()
{
 	extern void getstr();
	extern char *strcpy();
	unsigned char buf[NL_LANGMAX+1];
	int id;
	char *p1, *p2;

	if (gotstr) error(STATE);		/* only one string allowed */
						/* check the length */
	if (strlen(yytext) < 1+2 || strlen(yytext) > NL_LANGMAX+2)
		error(NAME_LEN);		
	(void) getstr(buf);			/* process name string */

	*lname = '\0';
	*territory = '\0';
	*codeset = '\0';
	p1 = (char *)buf;
	p2 = lname;

	while (*p1 && *p1 != '_' && *p1 != '.')
		*p2++ = *p1++;
	*p2 = '\0';
	if (*p1 == '_') {
		p2 = territory;
		if (*p1++) {
			while (*p1 && *p1 != '.')
				*p2++ = *p1++;
			*p2 = '\0';
			p2 = codeset;
		}
	} else if (*p1 == '.')
			p2 = codeset;

	if (*p1++) {
		while (*p1)
			*p2++ = *p1++;
		*p2 = '\0';
	}

	/* more length checking */
	if (strlen(lname) < 1 || strlen(lname) > _POSIX_NAME_MAX)
		error(LANG_LEN);		
	if (strlen(territory) > _POSIX_NAME_MAX)
		error(TERR_LEN);		
	if (strlen(codeset) > _POSIX_NAME_MAX)
		error(CODE_LEN);		


	id = langtoid((char *)buf);		/* check langname & langid */
	if (id == N_COMPUTER || id == C_LOCALE)
		newlang = TRUE;
	else
		if (gotid && id != lctable_head.nl_langid)
		    Error("specification conflict in 'langname' and 'langid'");

	(void) strcpy(lctable_head.lang, (char *)buf);	/* store name string */

	gotstr = TRUE;				/* have one string */
}

/* name_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Just make sure that there are no meta-chars and one name string. 
*/
void
name_finish()
{
	if (META) error(EXPR);

	if (!gotstr) {
		lineno--;
		error(STATE);
	}
}

/* id_init: initialize the process of language id.
** Get here when you see a 'langid' keyword.
*/
void
id_init(token)
int token;				/* keyword token */
{
	extern void id_num();
	extern void id_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}
	op = token;			/* save off the token */
	number = id_num;		/* set number function for langid */
	finish = id_finish;		/* set up langid finish */

	gotid = FALSE;			/* no language id initially */
}

/* id_num: process language id number.
** Check range of the id number.
*/
void
id_num()
{
	extern char *idtolang();
	extern int strcmp();
	char *plang;

	if (gotid) error(STATE); 		/* only one number allowed */
						/* check range */
	if (num_value < MINLANGID || num_value > MAXLANGID)
		error(ID_RANGE);

	plang = idtolang(num_value);		/* check langname & langid */
	if (strlen(plang) == 0) {
		newid = TRUE;
		if (num_value < MIDLANGID || num_value > MAXLANGID)
		    if (no_install)		/* give warning and continue */
		    	fprintf(stderr, "WARNING: user defined language should have langid in the range of 901 to 999\n");
		    else
		    	Error("user defined language should have langid in the range of 901 to 999");
	}
	else {
		if (gotstr && strcmp(plang, lctable_head.lang))
		    Error("specification conflict in 'langname' and 'langid'");
	}

	lctable_head.nl_langid = num_value;	/* store the number */

	gotid = TRUE;				/* have one number */
}

/* id_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Just make sure that there are no meta-chars and one id number. 
*/
void
id_finish()
{
	if (META) error(EXPR);

	if (!gotid) {
		lineno--;
		error(STATE);
	}
}

/* rev_init: initialize the process of revision string.
** Get here when you see a 'hprevision' or 'revision' keyword.
*/
void
rev_init(token)
int token;				/* keyword token */
{
	extern void rev_str();
	extern void rev_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	string = rev_str;		/* set sting function for revision */
	finish = rev_finish;		/* set up revision finish */

	gotstr = FALSE;			/* no revision string initially */
}

/* rev_str: process revision string.
** Check length of the revision string, and remove quote characters.
** Store the revision string in lctable_head.
*/
void
rev_str()
{
 	extern void getstr();
	extern char *strcpy(),
		    *strcat();
	extern char *HPUX_ID;
	unsigned char buf[MAXRSTRLEN]; 

	if (gotstr) error(STATE);		/* only one string allowed */

	if (op == HPREVISION) {			/* op == HPREVISION */
		if (strlen(yytext) < 1+2 || strlen(yytext) > MAXRSTRLEN+2-18)
			error(REV_LEN);		/* check the length */
		(void) getstr(buf);		/* process revision string */
						/* store rev string */
		lctable_head.rev_flag = TRUE;	/* set HP defined flag */
		(void) strcpy(lctable_head.rev_str, "@(#) HP ");
		(void) strcat(lctable_head.rev_str, (char *)buf);
		(void) strcat(lctable_head.rev_str, (char *)(HPUX_ID+15));

	}
	else {					/* op == REVISION */
		if (strlen(yytext) < 1+2 || strlen(yytext) > 7+2)
			error(REV_LEN);		/* check the length */
		(void) getstr(buf);		/* process revision string */
		lctable_head.rev_flag = FALSE;	/* set non-HP defined flag */
		(void) strcpy(lctable_head.rev_str, "@(#) $Revision: ");
		(void) strcat(lctable_head.rev_str, (char *)buf);
		(void) strcat(lctable_head.rev_str, " $");
		(void) strcat(lctable_head.rev_str, (char *)(HPUX_ID+15));
	}

	gotstr = TRUE;				/* have one string */
}

/* rev_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Just make sure that there are no meta-chars and one name string. 
*/
void
rev_finish()
{
	if (META) error(EXPR);

	if (!gotstr) {
		lineno--;
		error(STATE);
	}
}

#ifdef EUC
/* cscheme_init: initialize the process of code scheme name.
** Get here when you see a 'code_scheme' keyword.
*/
void
cscheme_init(token)
int token;				/* keyword token */
{
	extern void cschm_str();
	extern void cschm_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	string = cschm_str;		/* set sting function for code_scheme */
	number = NULL;			/* no numbers allowed */
	finish = cschm_finish;		/* set up code_scheme finish */

	gotstr = FALSE;			/* no code scheme name initially */
}

/* cschm_str: process code_scheme string.
** Remove quote characters, and validate the code scheme name.
*/
void
cschm_str()
{
 	extern void getstr();
	extern char *strcpy();
	unsigned char buf[_POSIX_NAME_MAX+1];
	int id;
	char *p1, *p2;

	if (gotstr) error(STATE);		/* only one string allowed */
						/* check the length */
	if (strlen(yytext) < 2 || strlen(yytext) > (_POSIX_NAME_MAX+2))
		error(CSCHM_LEN);		
	(void) getstr(buf);			/* process name string */

	/* only "HP15" and "EUC" code scheme name are supported */
	if (strcmp(buf, "HP15") == 0)
		lctable_head.codeset = CODE_HP15;
	else if (strcmp(buf, "EUC") == 0)
		lctable_head.codeset = CODE_EUC;
	else if (strcmp(buf, "") == 0)
		lctable_head.codeset = CODE1BYTE;
	else
		error(CSCHM);		

	gotstr = TRUE;				/* have one string */
}

/* cschm_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Just make sure that there are no meta-chars and one name string. 
*/
void
cschm_finish()
{
	if (META) error(EXPR);

	if (!gotstr) {
		lineno--;
		error(STATE);
	}
}
#endif /* EUC */
