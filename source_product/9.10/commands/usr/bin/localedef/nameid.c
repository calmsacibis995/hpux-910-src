/* @(#) $Revision: 70.8 $ */    
/* LINTLIBRARY */

#include	<stdio.h>
#include	<limits.h>
#include	"global.h"


int	gotstr, gotid;
char	lname[_POSIX_NAME_MAX],
	territory[_POSIX_NAME_MAX],
	codeset[_POSIX_NAME_MAX];
static int lstr=0, lid=0;
extern char *langname;
extern void getstr();

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
	unsigned char buf[NL_LANGMAX+1];

	if (gotstr) error(STATE);		/* only one string allowed */
						/* check the length */
	if (strlen(yytext) < 1+2 || strlen(yytext) > NL_LANGMAX+2)
		error(NAME_LEN);		
	(void) getstr(buf);                     /* process name string */

	process_name_str(buf);			/* process it */
	lstr++;					/* set flag to show langname
						 * is processed */
}

chk_name_str()
{
	if(lstr) return;			/* already processed */
	if(langname == (char *)0) {
		fprintf(stderr,(catgets(catd,NL_SETN,10, "no 'langname' specification\n")));
		exit(11);
	}
	gotid = FALSE;
	process_name_str(langname);		/* process it */
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
	/*
	 * POSIX 11.2: num_value is in lexer table
         */
	info_tab[LANGID] = (char *)plang;
	 
	if (strlen(plang) == 0) {
		newid = TRUE;
		if (num_value < MIDLANGID || num_value > MAXLANGID)
		    if (no_install)		/* give warning and continue */
		    	fprintf(stderr, (catgets(catd,NL_SETN,75, "WARNING: user defined language should have langid in the range of 901 to 999\n")));
		    else
		    	Error((catgets(catd,NL_SETN,76, "user defined language should have langid in the range of 901 to 999")),2);
	}
	else {
		if (gotstr && strcmp(plang, lctable_head.lang))
		    Error((catgets(catd,NL_SETN,77, "specification conflict in 'langname' and 'langid'")),4);
	}

	lctable_head.nl_langid = num_value;	/* store the number */

	gotid = TRUE;				/* have one number */
	lid++;					/* id already processed */
}

chk_id_num()
{
int id;
FILE *fdes;
char buf[LINE_MAX], *ptr;

	if(lid) return;				/* already processed */
	/* search for id if applicable, else allow
	 * unused id from user defined id 901 -999
	 */
	id = langtoid(lctable_head.lang);	/* get langid */
	if (id == N_COMPUTER)
	{
		newid = TRUE;
		if((fdes = fopen(NL_WHICHLANGS, "r")) == NULL) {
			fprintf(stderr,(catgets(catd,NL_SETN,9, "cant open input file %s\n")), NL_WHICHLANGS);
			exit(12);
		}
		id = buf[0] = 0;
		while(fgets(buf, LINE_MAX, fdes) != NULL);	/* go to end */
		ptr = buf;				/* contains last line */
		while(*ptr && *ptr != ' ')
			id = 10*id + *ptr++ - '0';	/* get last id */
		if(id < 901) id = 901;			/* user defined id */
		else id++;				/* bump up last id */
	}
	lctable_head.nl_langid = id;			/* store the number */

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
	     
		/*
		 * POSIX 11.2 code
		 */
		info_tab[REVISION] = (char *) buf;
		
	}
	else {					/* op == REVISION */
		if (strlen(yytext) < 1+2 || strlen(yytext) > MAXRSTRLEN-27)
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
	extern char *strcpy();
	unsigned char buf[_POSIX_NAME_MAX+1];
	int id;
	char *p1, *p2;
	unsigned char *cp;

	if (gotstr) error(STATE);		/* only one string allowed */
						/* check the length */
	if (strlen(yytext) < 2 || strlen(yytext) > (_POSIX_NAME_MAX+2))
		error(CSCHM_LEN);		
	(void) getstr(buf);			/* process name string */

	/* only "HP15" and "EUC" code scheme name are supported */
	if (strcmp(buf, "HP15") == 0)
	  {
		lctable_head.codeset = CODE_HP15;
if((cp =(unsigned char *)malloc((unsigned)strlen((char *)buf+1))) == NULL)
		   error(NOMEM);
		   else
		   {
		info_tab[CODE_SCHEME] = cp; /* CODE_HP15;  Posix.2 */
		(void) strcpy((char *)cp, (char *)buf);
	      }
	  }
	else if (strcmp(buf, "EUC") == 0)
	  {
		lctable_head.codeset = CODE_EUC;
if((cp =(unsigned char *)malloc((unsigned)strlen((char *)buf+1))) == NULL)
		   error(NOMEM);
		   else
		   {
		info_tab[CODE_SCHEME] = cp; /*  Posix.2 */
		(void) strcpy((char *)cp, (char *)buf);
	      }

	      }
	else if (strcmp(buf, "") == 0)
	  {
		lctable_head.codeset = CODE1BYTE;
		/*info_tab[CODE_SCHEME] = CODE1BYTE;
		*/
	      }
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
/* Posix.2 */
/* Revist this definition*/
void 
cswide_str()
{
/* Needs work in here: Revist */
/* info_tab[CSWIDTH] = cswidth; */
}

/* process_name_str: process language name string.
** Check length of the name string, and remove quote characters.
*/

process_name_str(buf)
unsigned char *buf;
{
int id;
char *p1, *p2;

	*lname = '\0';
	*territory = '\0';
	*codeset = '\0';
	info_tab[LANGNAME]= p1 = (char *)buf; /* POSIX code info_tab */
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
		    Error((catgets(catd,NL_SETN,74, "specification conflict in 'langname' and 'langid'")),4);

	strcpy(lctable_head.lang, (char *)buf);	/* store name string */

	gotstr = TRUE;

}

