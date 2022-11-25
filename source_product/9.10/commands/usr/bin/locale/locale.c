static char *HPUX_ID = "@(#) $Revision: 70.11 $";

/*
**       The locale command displays table information associated
**       with a given locale.  These tables are used to control
**       language sensitive operations in commands and libraries.
**       The major features of the command include:
**
**             1.  Table information displayed by setlocale(LIBC) categories.
**             2.  A list of installed locales.
**             3.  A list of characters defined in the code set
**                 associated with a language.
**
**       Design Objectives:
**             1. Implement Posix functionality
**             2. Maximize code reuse from nlsinfo - Objective not met
**                due to functionality being orthogonal
**             3. Maximize quality
*/

#include <stdio.h>		/* input - output */
#include <stdlib.h>
#include <ctype.h>		/* character classification */
#include <string.h>		/* string function declarations */
#include <fcntl.h>
#include <sys/stat.h>
#include <nl_types.h>		/* for nl_catd */
#include <nl_ctype.h>		/* for 16-bit macros */
#include <langinfo.h> 		/* for local customs */
#include <locale.h>		/* for setlocale & localeconv */
#include <setlocale.h>		/* for setlocale & localeconv */
#include <limits.h>		/* for UINT_MAX */
#include <dirent.h>		/* read directories */

/* DO NOT USE 0 IN THIS LIST. */
#define CTYPE                 5
#define LIST_KEY              10
#define ERA_KEY               20
#define CATEGORY              30
#define STRING                40
#define MON_STR               50
#define MON_NUM               60
#define CONTEXT_NUM           65
#define LANG_NAM              70
#define LANG_ID               80
#define GROUPING	      90

#define NL_SETN 1

/*
 * locale table constants
 */
#define MAX_FNAME    1024      		/* maximum length of file name */
#define NLSDIR       "/usr/lib/nls/"
#define LOCALE       "/locale.inf"
#define CONFIG       "config"
#define CHARMAPNAME  "/usr/lib/charmaps"

char *pr_mon_str();
int  pr_mon_num();

extern struct lconv *lconv_ptr;
extern char *nl_langinfo();

typedef struct entry {
  char *name;
  int type;
  int arg;             /* item arg for nl_langinfo */
} Entry;


Entry key_tab[] = {
  "LC_ALL",	       CATEGORY,    0,
  /*
  "langname",          LANG_NAM,    LC_ALL,
  "langid",            LANG_ID,     LC_ALL,
  */
  "charmap",           STRING,      CHARMAP,
  "direction",         STRING,      DIRECTION,
  "context",           CONTEXT_NUM, 0,
  "LC_CTYPE",          CATEGORY,    0,
  /*
  "langname",          LANG_NAM,    LC_CTYPE,
  "langid",            LANG_ID,     LC_CTYPE,
  */
  "bytes_char",        STRING,      BYTES_CHAR,
  "alt_punct",         STRING,      ALT_PUNCT,
  "code_scheme",       STRING,      CODE_SCHEME,
  "upper",	       CTYPE,       0,
  "lower",	       CTYPE,       0,
  "alpha",	       CTYPE,       0,
  "digit",	       CTYPE,       0,
  "space",	       CTYPE,       0,
  "cntrl",	       CTYPE,       0,
  "punct",	       CTYPE,       0,
  "graph",	       CTYPE,       0,
  "print",	       CTYPE,       0,
  "xdigit",	       CTYPE,       0,
  "blank",	       CTYPE,       0,
  "toupper",	       CTYPE,       0,
  "tolower",	       CTYPE,       0,
  "LC_COLLATE",        CATEGORY,    0,
  /*
  "langname",          LANG_NAM,    LC_COLLATE,
  "langid",            LANG_ID,     LC_COLLATE,
  */
/*  "order_start",       STRUCT_KEY,  NULL,  * forward, backward collation
					       not yet implemented */
  "LC_MONETARY",       CATEGORY,    0,
  /*
  "langname",          LANG_NAM,    LC_MONETARY,
  "langid",            LANG_ID,     LC_MONETARY,
  */
  "int_curr_symbol",   MON_STR,     0,
  "currency_symbol",   MON_STR,     0,
  "mon_decimal_point", MON_STR,     0,
  "mon_thousands_sep", MON_STR,     0,
  "mon_grouping",      GROUPING,    0,
  "positive_sign",     MON_STR,     0,
  "negative_sign",     MON_STR,     0,
  "int_frac_digits",   MON_NUM,     0,
  "frac_digits",       MON_NUM,     0,
  "p_cs_precedes",     MON_NUM,     0,
  "p_sep_by_space",    MON_NUM,     0,
  "n_cs_precedes",     MON_NUM,     0,
  "n_sep_by_space",    MON_NUM,     0,
  "p_sign_posn",       MON_NUM,     0,
  "n_sign_posn",       MON_NUM,     0,
  "crncystr",          STRING,      CRNCYSTR,
  "LC_NUMERIC",        CATEGORY,    0,
  /*
  "langname",          LANG_NAM,    LC_NUMERIC,
  "langid",            LANG_ID,     LC_NUMERIC,
  */
  "decimal_point",     STRING,      RADIXCHAR,
  "thousands_sep",     STRING,      THOUSEP,
  "grouping",          GROUPING,    0,
  "alt_digit",         STRING,      ALT_DIGIT,
  "LC_TIME",           CATEGORY,    0,
  /*
  "langname",          LANG_NAM,    LC_TIME,
  "langid",            LANG_ID,     LC_TIME,
  */
  "abday",             LIST_KEY,    ABDAY_1,
  "day",               LIST_KEY,    DAY_1,
  "abmon",             LIST_KEY,    ABMON_1,
  "mon",               LIST_KEY,    MON_1,
  "d_t_fmt",           STRING,      D_T_FMT,
  "d_fmt",             STRING,      D_FMT,
  "t_fmt",             STRING,      T_FMT,
  "am_pm",             LIST_KEY,    AM_STR,
  "t_fmt_ampm",        STRING,      T_FMT_AMPM,
  "year_unit",         STRING,      YEAR_UNIT,
  "mon_unit",          STRING,      MON_UNIT,
  "day_unit",          STRING,      DAY_UNIT,
  "hour_unit",         STRING,      HOUR_UNIT,
  "min_unit",          STRING,      MIN_UNIT,
  "sec_unit",          STRING,      SEC_UNIT,
  "era_d_fmt",         STRING,      ERA_FMT,
  "era",               ERA_KEY,     0,
  "LC_MESSAGES",       CATEGORY,    0,
  /*
  "langname",          LANG_NAM,    LC_MESSAGES,
  "langid",            LANG_ID,     LC_MESSAGES,
  */
  "yesexpr",           STRING,      YESEXPR,
  "noexpr",            STRING,      NOEXPR,
  "yesstr",            STRING,      YESSTR,
  "nostr",             STRING,      NOSTR,
   NULL,	       NULL,        NULL
};

Entry *search_table();
int ex_err = 0;
int cflag =0;
int kflag =0;
nl_catd catd;
char *category_name;
char ec;			/* escape character */
 
main(argc, argv)
int argc;
char *argv[];
{
/*
** description:
**	set up language tables
**	open message catalogs
**	parse command line
**	initialze global variables
*/
extern unsigned char __langmap[N_CATEGORY][LC_NAME_SIZE];
extern char *optarg;
extern int optind;
char optchar;
int i;
Entry *keyptr;

	if (!setlocale(LC_ALL,""))
	{
		fputs(_errlocale("locale"), stderr);
		catd = (nl_catd)-1;
	}
	else
		catd = catopen("locale", 0);


	while ((optchar = getopt(argc, argv, "amck")) != EOF) {
		switch (optchar) {
		case 'a':
			locale_print();
			break;
		case 'm':
			charmap_print();
			break;
		case 'c':
			cflag++;
			break;
		case 'k':
			kflag++;
			break;
		default:
			usage();
			break;
		}

	} /* while loop */	

	/*
	 * If there are no arguments
         */
	if (argc <= 1)
		env_print();

	/* get escape character */
	ec = *nl_langinfo(ESC_CHAR);
	if(ec == '\0') ec = '\\';		/* default */

	/* 
	 * process operands
	 */
	
	if(optind >= argc) usage();		/* no name list */

	for(i = optind; i < argc; i++) {
		keyptr = search_table(argv[i]);
		if (keyptr->type == CATEGORY) {
			category_print(argv[i], keyptr); /* print category and
							    all its keywords */
		} else {
			keyword_print(argv[i], keyptr);	/* print one keyword */
		}

	}
	exit(ex_err);
}

usage()
{
	fprintf(stderr,(catgets(catd,NL_SETN,1,
	  "usage: locale [-a | -m] or locale [-ck] name...\n")));/* catgets 1 */
	exit(1);
}

env_print()
{

char *lc_all, *lc_collate, *lang, *lc_ctype, *lc_messages, *lc_monetary,
     *lc_numeric, *lc_time;
struct locale_data *locale_info;

	locale_info = getlocale(LOCALE_STATUS);
	lang = getenv("LANG");
	lc_all = getenv("LC_ALL");
	    
	fprintf(stdout,"LANG=%s\n", lang);
	fprintf(stdout,"LC_CTYPE=");
	/* if set in the ENV and not overridden by LC_ALL setting, write
	 * the value in the ENV, else write the implied value
	 */
	if(!lc_all && (lc_ctype = getenv("LC_CTYPE")))
		fprintf(stdout,"%s\n", lc_ctype);
	else
		fprintf(stdout,"\"%s\"\n", locale_info->LC_CTYPE_D);
	fprintf(stdout,"LC_COLLATE=");
	if(!lc_all && (lc_collate = getenv("LC_COLLATE")))
		fprintf(stdout,"%s\n", lc_collate);
	else
		fprintf(stdout,"\"%s\"\n", locale_info->LC_COLLATE_D);
	fprintf(stdout,"LC_MONETARY=");
	if(!lc_all && (lc_monetary = getenv("LC_MONETARY")))
		fprintf(stdout,"%s\n", lc_monetary);
	else
		fprintf(stdout,"\"%s\"\n",locale_info->LC_MONETARY_D);
	fprintf(stdout,"LC_NUMERIC=");
	if(!lc_all && (lc_numeric = getenv("LC_NUMERIC")))
		fprintf(stdout,"%s\n", lc_numeric);
	else
		fprintf(stdout,"\"%s\"\n",locale_info->LC_NUMERIC_D);
	fprintf(stdout,"LC_TIME=");
	if(!lc_all && (lc_time = getenv("LC_TIME")))
		fprintf(stdout,"%s\n", lc_time);
	else
		fprintf(stdout,"\"%s\"\n",locale_info->LC_TIME_D);
	fprintf(stdout,"LC_MESSAGES=");
	if(!lc_all && (lc_messages = getenv("LC_MESSAGES")))
		fprintf(stdout,"%s\n", lc_messages);
	else
		fprintf(stdout,"\"%s\"\n",locale_info->LC_MESSAGES_D);
	fprintf(stdout,"LC_ALL=");
	if(lc_all)
		fprintf(stdout,"%s\n", lc_all);
	else
		fprintf(stdout,"\n");
	exit(0);
}

locale_print()
{

unsigned char fname[MAX_FNAME];		/* config path */
unsigned char buf[MAX_FNAME];		/* config file buffer */
unsigned char lcname2[MAX_FNAME];	/* locale name with terr - cs dir */
unsigned char *lcname;			/* locale name */
unsigned char *p1, *p2;			/* loop ptr */
FILE *fp;				/* file ptr of config */
struct stat   stbuf;

	/* get config filename & open it*/
	strcpy(fname, NLSDIR);
	strcat(fname, CONFIG);
	if((fp = fopen(fname, "r")) == (FILE *) NULL) {
	       fprintf(stderr,(catgets(catd,NL_SETN,2,
		      "Cannot open configuration file\n")));    /* catgets 2 */
	       exit(2);
	}
	
	/* read the config file until EOF */
	while (fgets(buf, MAX_FNAME, fp)) {
		lcname = buf;
		/* clobber newline at end */
		buf[strlen(buf)-1] = '\0';

		/* scan past langid and white space in config file */
		while(isdigit(*lcname) || isspace(*lcname)) lcname++;

		/* default locales don't have locale.inf files, but
		   still need to be reported */
		if (!strcmp(lcname, "n-computer") || !strcmp(lcname, "C") ||
			!strcmp("POSIX",lcname)) {
				fprintf(stdout,"%s\n", lcname);
				continue;
		}
		/* handle territory - codeset */
		for (p1=lcname, p2=lcname2; *p1; p1++, p2++) {
			/* if territory, codeset present make subdir
			 * else copy string
			 */
			*p2 = (*p1 == '_' || *p1 == '.') ? '/' : *p1;
		}
		*p2 = '\0';

		/* get locale.inf filename & open it*/
		strcpy(fname, NLSDIR);
		strcat(fname, lcname2);
		strcat(fname, LOCALE);
		if(stat(fname, &stbuf) == 0) {
		   /* if file is present, report it */
		   if((stbuf.st_mode & S_IFMT) == S_IFREG)
			fprintf(stdout,"%s\n", lcname);
	        }
 	}
	exit(0);
}

charmap_print()
{
	/* ALL available charmaps to be written from
	 * the directory /usr/lib/nls/charmaps
	 */
char dname[MAX_FNAME];
int i;
DIR *ddesc;
struct dirent *dentry;

	strcpy(dname,CHARMAPNAME);	/* dir where charmaps reside */
	if((ddesc = opendir(dname)) == (DIR *)NULL) {
	       fprintf(stderr,(catgets(catd,NL_SETN,3,
		      "No charmaps present\n")));    /* catgets 3 */
	       exit(0);				     /* not an error! */
	}
	/* assuming all files in this dir are charmap files */
	while((dentry = readdir(ddesc)) != NULL) {
		if(*(dentry->d_name) == '.') continue;
		fprintf(stdout, "%s\n", dentry->d_name);
	}
	closedir(ddesc);
	exit(0);
}

category_print(categ_name, keyptr)
char *categ_name;
Entry *keyptr;
{

	if (keyptr) {
		/* print the valid category */
		if (cflag)
			fprintf(stdout,"%s\n", categ_name);
		++keyptr;
		while (keyptr->type != CATEGORY && keyptr->type != NULL) {
			if (kflag) {
				fprintf(stdout, "%s=", keyptr->name);
			}
			choose_format(keyptr, keyptr->name);
			++keyptr;
		}
	} else {
		fprintf(stderr,(catgets(catd,NL_SETN,4,
			"Invalid category: %s\n")),categ_name); /* catgets 4 */
		ex_err++;
	}
}


keyword_print(keyword, keyptr)
char *keyword;
Entry *keyptr;
{

	if (keyptr)		/* successful search */
	{
		if (cflag)
			fprintf(stdout,"%s\n", category_name);
		if (kflag && keyptr->type != CATEGORY)
			fprintf(stdout,"%s=", keyword);
		choose_format( keyptr, keyword );
	}
	else
	{
		fprintf(stderr,(catgets(catd,NL_SETN,5,
			"Invalid keyword: %s\n")),keyword);	/* catgets 5 */
		ex_err++;
	}
}

choose_format(keyptr, keyword)
Entry *keyptr;
char *keyword;
{
	switch(keyptr->type)
	{
	   case MON_STR:
	      fprintf(stdout,"\"%s\"\n",
		      pr_mon_str(keyword));
	      break;
	   case MON_NUM:
	      fprintf(stdout,"%d\n",pr_mon_num(keyword));
	      break;
	   case STRING:
	      fprintf(stdout,"\"%s\"\n",
		      nl_langinfo(keyptr->arg));
	      break;
	   case LIST_KEY:
	      print_list(keyptr->arg);
	      break;
	   case ERA_KEY:
	      pr_era();
	      break;
	   case LANG_NAM:
	      fprintf(stdout,"\"%s\"\n", __langmap[keyptr->arg]);
	      break;
	   case LANG_ID:
	      fprintf(stdout,"%d\n", __nl_langid[keyptr->arg]);
	      break;
	   case GROUPING:
	      pr_grouping(keyword);
	      break;
	   case CONTEXT_NUM:
	      /* get from nl_langinfo in the future.
		 Make this a string in setlocale - FIX */
	      if (_nl_context > 0) {
	      		fprintf(stdout,"\"%d\"\n", _nl_context);
	      } else {
	      		fprintf(stdout,"\"\"\n");
	      }
	      break;
	   case CTYPE:
	      /* print out nothing */
	      if (kflag)
	     		fprintf(stdout,"\n");
	      break;
	   default:
	      fprintf(stderr,(catgets(catd,NL_SETN,5,
		      "Invalid keyword: %s\n")),keyword);	/* catgets 5 */
	      break;
	}
}


Entry*
search_table(keyword)
char *keyword;
{
register Entry *kptr = key_tab;

  while (kptr->name != NULL )
  {
    if (kptr->type == CATEGORY) category_name = kptr->name;
    if (strcmp(kptr->name,keyword) == 0)
	return(kptr);
    kptr++;
  }
  category_name = (char *)0;
  return((Entry *)0);
}


print_list(list_begin)
int list_begin;
{
int list_end,i;
char ch;

	switch(list_begin)
	{
	  case ABDAY_1:
	    list_end = ABDAY_7;
	    break;
	  case DAY_1:
	    list_end = DAY_7;
	    break;
	  case ABMON_1:
	    list_end = ABMON_12;
	    break;
	  case MON_1:
	    list_end = MON_12;
	    break;
	  case AM_STR:
	    list_end = PM_STR;
	    break;
	  default:
	    return;
	}
	for (i = list_begin; i <= list_end; i++)
	{
		ch = (i == list_end) ? '\n' : ';';
		fprintf(stdout,"\"%s\"%c",nl_langinfo(i),ch);
	}
	return;
}

struct lconv *lconv;
static int locflg = 0;

char
*pr_mon_str(key)
char *key;
{
	char *p;
	int size;

	if(!locflg)
		lconv = localeconv();
	locflg = 1;

	if (strcmp(key,"int_curr_symbol") == 0)
		return(lconv->int_curr_symbol);
	if (strcmp(key,"currency_symbol") == 0)
		return(lconv->currency_symbol);
	if (strcmp(key,"mon_decimal_point") == 0)
		return(lconv->mon_decimal_point);
	if (strcmp(key,"mon_thousands_sep") == 0)
		return(lconv->mon_thousands_sep);
	if (strcmp(key,"mon_grouping") == 0)
		return(lconv->mon_grouping);
	if (strcmp(key,"grouping") == 0)
		return(lconv->grouping);
	if (strcmp(key,"positive_sign") == 0)
		return(lconv->positive_sign);
	if (strcmp(key,"negative_sign") == 0)
		return(lconv->negative_sign);
	return((char *)0);
}

pr_mon_num(key)
char *key;
{
	if(!locflg)
		lconv = localeconv();
	locflg = 1;
	if (strcmp(key,"int_frac_digits") == 0)
		return(lconv->int_frac_digits);
	if (strcmp(key,"frac_digits") == 0)
		return(lconv->frac_digits);
	if (strcmp(key,"p_cs_precedes") == 0)
		return(lconv->p_cs_precedes);
	if (strcmp(key,"p_sep_by_space") == 0)
		return(lconv->p_sep_by_space);
	if (strcmp(key,"n_cs_precedes") == 0)
		return(lconv->n_cs_precedes);
	if (strcmp(key,"n_sep_by_space") == 0)
		return(lconv->n_sep_by_space);
	if (strcmp(key,"p_sign_posn") == 0)
		return(lconv->p_sign_posn);
	if (strcmp(key,"n_sign_posn") == 0)
		return(lconv->n_sign_posn); 
	return(0);
}

pr_era()
{
register struct _era_data	**era = _nl_era;
int list_cnt = 0;

     if(*era) {
	for (; *era; era++, list_cnt++)
	{
	   register org_year = (*era)->origin_year;

		if(list_cnt) fprintf(stdout, ";");
		fprintf(stdout,"\"");
		if(((*era)->signflag > 0 && org_year == (*era)->start_year) ||
		   ((*era)->signflag < 0 && org_year == (*era)->end_year))
			fprintf(stdout,"+:");
		else
			fprintf(stdout,"-:");
		fprintf(stdout,"%d:",(*era)->offset);

		if(org_year == (*era)->start_year)
		{
			fprintf(stdout,"%.4d/%.2d/%.2d:",(*era)->start_year,
					(*era)->start_month,
					(*era)->start_day);
                        if ((*era)->end_year == SHRT_MAX)
                                fprintf(stdout,"+*:");
                        else
                        if ((*era)->end_year == SHRT_MIN)
                                fprintf(stdout,"-*:");
                        else
                                fprintf(stdout,"%.4d/%.2d/%.2d:",
						(*era)->end_year,
						(*era)->end_month,
						(*era)->end_day);
		} else {
                        fprintf(stdout,"%.4d/%.2d/%.2d:", (*era)->end_year,
                                            (*era)->end_month,
                                            (*era)->end_day);
                        if ((*era)->start_year == SHRT_MAX)
                                fprintf(stdout,"+*:");
                        else
                        if ((*era)->start_year == SHRT_MIN)
                                fprintf(stdout,"-*:");
                        else
                                fprintf(stdout,"%.4d/%.2d/%.2d:",
						(*era)->start_year,
						(*era)->start_month,
						(*era)->start_day);
		}
		fprintf(stdout, "%s:", (*era)->name);
		fprintf(stdout, "%s\"", (*era)->format);
	}
	fprintf(stdout,"\n");
    } else
	fprintf(stdout,"\"\"\n");	/* no value present */
}


pr_grouping(keyword)
char *keyword;
{
	unsigned int size;
	char *p;

	size = strlen( p = pr_mon_str(keyword) );
	while (size--) {
		fprintf( stdout, "%d", *p++);
		if (size)
			fprintf( stdout, ";");
	}
	fprintf(stdout, "\n");
}
