/* @(#) $Revision: 66.4 $ */     
/* LINTLIBRARY */

/*
** SYNOPSIS:
**	buildlang [-n] infile
**	buildlang -d[fc|fd|fo|fx] locale
*/

#include <stdio.h>
#include <stdlib.h>
#include <nl_ctype.h>
#include "global.h"

/* main:
** command line parsing.  set up flags.
** if no "-d" option, call buildlang() function.
** if "-d" option, call dumplang() function.
*/
main(argc, argv)
int argc;
char **argv;
{
	extern char *optarg;
	extern int optind;
	int c;
	int dump = 0, form = 0;

	/*
	** read command line options
	*/
	while ((c = getopt(argc, argv, "ndf:")) != EOF) {
		switch (c) {
			case 'n':
				no_install++;
				break;
			case 'd':
				dump++;
				break;
			case 'f':
				form++;
				break;
			case '?':
				usage();
		}
	}

	if (no_install && dump)		/* -n and -d are mutually exclusive */
		usage();
	if (argc - optind != 1)		/* "infile" and "locale" are required */
		usage();

	/*
	** invoke buildlang() or dumplang()
	*/
	if (dump) {			/* with "-d" option, dumplang() */
		if (!form)		/* no "-f", default to "-fx" */
			dumplang(argv[optind], "x");
		else
			dumplang(argv[optind], optarg);	
	}
	else				/* without "-d" option, buildlang() */
		buildlang(argv[optind]);

	return(0);
}

/* usage:
** print usage message and exit.
*/
usage()
{
	extern int fprintf();
	extern void exit();
	
	fprintf(stderr, "Usage:\tbuildlang [-n] infile\n");
	fprintf(stderr, "or\tbuildlang -d[fc|fd|fo|fx] locale\n");
	exit(1);
}

/* buildlang:
** build and install a "locale.def" file
*/
#define	CMDLEN	64
#define	ERRLEN	80
buildlang(infile)
char *infile;
{
	extern FILE *yyin;
	FILE *fp, *fopen();
	extern void *calloc();

	extern void name_init();
	extern void id_init();
	extern void rev_init();
	extern void mod_init();
	extern void all_init();
	/* extern void ctrang_init(); */
	extern void cty_init();
	/* extern void shrang_init(); */
	extern void con_init();
#ifdef EUC
	extern void cscheme_init();
	extern void cswidth_init();
#endif /* EUC */
	extern void col_init();
	extern void col_finish();
	extern void pri_init();
	extern void dc_init();
	extern void info_init();
	extern void era_init();
	extern void mntry_init();
	extern void numeric_init();
	extern void pri_end();
	extern void dc_end();

	extern void fill_lc_header();
	extern void flc_all();
	extern void flc_coll();
	extern void flc_ctype();
	extern void flc_mntry();
	extern void flc_nmrc();
	extern void flc_time();
	extern void write_locale();
	extern void debug_locale();

	extern char *strcpy();
	extern char *strcat();
	extern int fprintf(), sprintf();
	extern int system();

	extern char lname[], territory[], codeset[];
	int token;
	int i;
	char cmd[CMDLEN], errmsg[ERRLEN];

	/*
	** initialization
	 */
#ifdef EUC
	lctable_head.codeset = CODE1BYTE;

#endif /* EUC */
	lcctype_head.sh_low = DFL_LOWCHAR + 1;
	lcctype_head.sh_high = DFL_HICHAR;
	c_offset = ZERO - DFL_LOWCHAR;
	ctype1 = (unsigned char *)calloc(CTYPESIZ, sizeof(unsigned char));
	ctype2 = (unsigned char *)calloc(CTYPESIZ, sizeof(unsigned char));
	ctype3 = (unsigned char *)calloc(CTYPESIZ, sizeof(unsigned char));
	ctype4 = (unsigned char *)calloc(CTYPESIZ, sizeof(unsigned char));

	s_offset = ZERO;
	upper = (unsigned char *)calloc(SHIFTSIZ, sizeof(unsigned char));
	lower = (unsigned char *)calloc(SHIFTSIZ, sizeof(unsigned char));
	for (i = s_offset; i <= DFL_HICHAR; i++)
		upper[i] = i;
	for (i = s_offset; i <= DFL_HICHAR; i++)
		lower[i] = i;

#ifdef EUC
	e_cset = (unsigned char *)malloc(SHIFTSIZ * sizeof(unsigned char));
	for (i = 0; i < SHIFTSIZ/2; i++)
		e_cset[i] = 0;
	for (i = SHIFTSIZ/2; i < SHIFTSIZ; i++)
		e_cset[i] = 1;

	ein_csize = (unsigned char *)malloc(SHIFTSIZ * sizeof(unsigned char));
	eout_csize = (unsigned char *)malloc(SHIFTSIZ * sizeof(unsigned char));
	for (i = 0; i < SHIFTSIZ; i++)
		ein_csize[i] = 1;
	for (i = 0; i < SHIFTSIZ; i++)
		eout_csize[i] = 1;

#endif /* EUC */
	for (i = 0; i < TOT_ELMT; i++) {
		seq_tab[i].seq_no = 0;
		seq_tab[i].type_info = DC;
	}

#ifdef	DBG
	/*
	** open "debug" output file
	*/
	if((dfp = fopen("debug", "w")) == NULL)
		Error("can't open 'debug' file");
#endif

	/*
	** open input file
	*/
	if((fp = fopen(infile, "r")) == NULL) {
		(void) strcpy(errmsg, "can't open input file: ");
		(void) strcat(errmsg, infile);
		Error(errmsg);
	}
	yyin = fp;

	/*
	** read & process input file (buildlang script)
	*/
	while (token = yylex()) {
	    switch (token) {
		case LANGNAME:
		    name_init(token);
		    break;
		case LANGID:
		    id_init(token);
		    break;
		case HPREVISION:
		case REVISION:
		    rev_init(token);
		    break;
		case STRING:
		    (*string)();
		    break;
		case NUMBER:
		    if (number == NULL) error(STATE);
		    (*number)(); 
		    break;
		case TLC_ALL:
		    cur_cat = LC_ALL;
		    cur_mod = 0;
		    cmlog[LC_ALL].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
			switch (token) {
			    case MODIFIER:
				mod_init(token);
				break;
			    case ALL:
				switch (num_value) {
			 	    case YESSTR:
				    case NOSTR:
				    case DIRECTION:
				    case CONTEXT:
					info_init(token);
					break;
				    default:
					error(STATE);
					break;
				}
				gotinfo = TRUE;
				break;
			    case STRING:
				(*string)();
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
			    default:
				error(STATE);
				break;
			}
		    }
		    flc_all();
		    break;
		case TLC_COLLATE:
		    cur_cat = LC_COLLATE;
		    cur_mod = 0;
		    cmlog[LC_COLLATE].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
			switch (token) {
			    case MODIFIER:
				mod_init(token);
				break;
			    case SEQUENCE:
				collate++;
				col_init(token);
				gotinfo = TRUE;
				break;
			    case STRING:
				(*string)();
				break;
			    case NUMBER:
				if (number == NULL) error(STATE);
				(*number)(); 
				break;
			    case DASH:
				dash = TRUE;
				break;
			    case LCB:
				lcb += 1;
				dc_init(); 
				break;
			    case RCB:
				rcb += 1;
				dc_end(); 
				break;
			    case LP:
				lp += 1;
				pri_init(); 
				break;
			    case RP:
				rp += 1;
				pri_end(); 
				break;
			    case LAB:
				lab += 1;
				if (l_angle == NULL) error(STATE);
				(*l_angle)();
				break;
			    case RAB:
				rab += 1;
				if (r_angle == NULL) error(STATE);
				(*r_angle)();
				break;
			    case LB:
				lb += 1;
				if (l_square == NULL) error(STATE);
				(*l_square)(); 
				break;
			    case RB:
				rb += 1;
				if (r_square == NULL) error(STATE);
				(*r_square)(); 
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
			    default:
				error(STATE);
				break;
			}
		    }
		    col_finish();
		    flc_coll();
		    break;
		case TLC_CTYPE:
		    cur_cat = LC_CTYPE;
		    cur_mod = 0;
		    cmlog[LC_CTYPE].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
			switch (token) {
			    case MODIFIER:
				mod_init(token);
				break;
			    /* case CTYPERANGE:
				ctrang_init(token);
				break; */
			    case ISUPPER:
				cty_init(token,_U,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISLOWER:
				cty_init(token,_L,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISDIGIT:
				cty_init(token,_N,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISSPACE:
				cty_init(token,_S,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISPUNCT:
				cty_init(token,_P,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISCNTRL:
				cty_init(token,_C,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISBLANK:
				cty_init(token,_B,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISXDIGIT:
				cty_init(token,_X,ctype1+c_offset);
				gotinfo = TRUE;
				break;
			    case ISFIRST:
				is1st++;
				cty_init(token,_K1,ctype2+c_offset);
				gotinfo = TRUE;
				break;
			    case ISSECOND:
				cty_init(token,_K2,ctype2+c_offset);
				gotinfo = TRUE;
				break;
			    /* case SHIFTRANGE:
				shrang_init(token);
				break; */
			    case UL:
				con_init(token);
				gotinfo = TRUE;
				break;
			    case TOUPPER:
				con_init(token);
				gotinfo = TRUE;
				break;
			    case TOLOWER:
				con_init(token);
				gotinfo = TRUE;
				break;
#ifdef EUC
			    case CODE_SCHEME:
				cscheme_init(token);
				gotinfo = TRUE;
				break;
			    case CSWIDTH:
				cswidth_init(token);
				gotinfo = TRUE;
				break;
#endif /* EUC */
			    case CTYPE:
				switch (num_value) {
			 	    case BYTES_CHAR:
				    case ALT_PUNCT:
					info_init(token);
					break;
				    default:
					error(STATE);
					break;
				}
				gotinfo = TRUE;
				break;
			    case STRING:
				(*string)();
				break;
			    case NUMBER:
				if (number == NULL) error(STATE);
				(*number)(); 
				break;
			    case DASH:
				dash = TRUE;
				break;
			    case LAB:
				lab += 1;
				if (l_angle == NULL) error(STATE);
				(*l_angle)();
				break;
			    case RAB:
				rab += 1;
				if (r_angle == NULL) error(STATE);
				(*r_angle)();
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
			    default:
				error(STATE);
				break;
			}
		    }
		    flc_ctype();
		    break;
		case TLC_MONETARY:
		    cur_cat = LC_MONETARY;
		    cur_mod = 0;
		    cmlog[LC_MONETARY].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
			switch (token) {
			    case MODIFIER:
				mod_init(token);
				break;
			    case MONEY:
				switch (num_value) {
			 	    case INT_CURR:
			 	    case CURRENCY_LC:
			 	    case CURRENCY_LI:
			 	    case MON_DECIMAL:
			 	    case MON_THOUSANDS:
			 	    case MON_GROUPING:
			 	    case POSITIVE_SIGN:
			 	    case NEGATIVE_SIGN:
			 	    case INT_FRAC:
			 	    case FRAC_DIGITS:
			 	    case P_CS:
			 	    case P_SEP:
			 	    case N_CS:
			 	    case N_SEP:
			 	    case P_SIGN:
			 	    case N_SIGN:
					mntry_init(token);
					break;
				    default:
					error(STATE);
					break;
				}
				gotinfo = TRUE;
				break;
			    case STRING:
				(*string)();
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
			    default:
				error(STATE);
				break;
			}
		    }
		    flc_mntry();
		    break;
		case TLC_NUMERIC:
		    cur_cat = LC_NUMERIC;
		    cur_mod = 0;
		    cmlog[LC_NUMERIC].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
			switch (token) {
			    case MODIFIER:
				mod_init(token);
				break;
			    case NUMERIC:
				switch (num_value) {
			 	    case GROUPING:
			 	    case DECIMAL_P:
			 	    case THOUSANDS_S:
					numeric_init(token);
					break;
			 	    case ALT_DIGIT:
					info_init(token);
					break;
				    default:
					error(STATE);
					break;
				}
				gotinfo = TRUE;
				break;
			    case STRING:
				(*string)();
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
			    default:
				error(STATE);
				break;
			}
		    }
		    flc_nmrc();
		    break;
		case TLC_TIME:
		    cur_cat = LC_TIME;
		    cur_mod = 0;
		    cmlog[LC_TIME].n_mod++;
		    gotinfo = FALSE;
		    while ((token = yylex()) && (token != END_LC)) {
			switch (token) {
			    case MODIFIER:
				mod_init(token);
				break;
			    case TIME:
				switch (num_value) {
			 	    case D_T_FMT:
			 	    case D_FMT:
			 	    case T_FMT:
			 	    case DAY_1:
			 	    case DAY_2:
			 	    case DAY_3:
			 	    case DAY_4:
			 	    case DAY_5:
			 	    case DAY_6:
			 	    case DAY_7:
			 	    case ABDAY_1:
			 	    case ABDAY_2:
			 	    case ABDAY_3:
			 	    case ABDAY_4:
			 	    case ABDAY_5:
			 	    case ABDAY_6:
			 	    case ABDAY_7:
			 	    case MON_1:
			 	    case MON_2:
			 	    case MON_3:
			 	    case MON_4:
			 	    case MON_5:
			 	    case MON_6:
			 	    case MON_7:
			 	    case MON_8:
			 	    case MON_9:
			 	    case MON_10:
			 	    case MON_11:
			 	    case MON_12:
			 	    case ABMON_1:
			 	    case ABMON_2:
			 	    case ABMON_3:
			 	    case ABMON_4:
			 	    case ABMON_5:
			 	    case ABMON_6:
			 	    case ABMON_7:
			 	    case ABMON_8:
			 	    case ABMON_9:
			 	    case ABMON_10:
			 	    case ABMON_11:
			 	    case ABMON_12:
			 	    case AM_STR:
			 	    case PM_STR:
			 	    case YEAR_UNIT:
			 	    case MON_UNIT:
			 	    case DAY_UNIT:
			 	    case HOUR_UNIT:
			 	    case MIN_UNIT:
			 	    case SEC_UNIT:
			 	    case ERA_FMT:
					info_init(token);
					break;
				    default:
					error(STATE);
					break;
				}
				gotinfo = TRUE;
				break;
			    case ERA:
				era_init(token);
				break;
			    case STRING:
				(*string)();
				break;
		 	    case BAD_LEXEME:
		 		error(CHAR);
		 		break;
			    case BAD_KEYWORD:
				error(KEY);
				break;
			    default:
				error(STATE);
				break;
			}
		    }
		    flc_time();
		    break;
		case BAD_LEXEME:
		    error(CHAR);
		    break;
		case BAD_KEYWORD:
		    error(KEY);
		    break;
		}
	}
	if (finish == NULL) error(STATE);
	(*finish)(); 

	fclose(fp);

	/*
	** check the completeness & consistency of the buildlang script
	*/
	if (*lctable_head.lang == '\0')
		Error("no 'langname' specification");

	if (lctable_head.nl_langid == 0)
		Error("no 'langid' specification");

	if (newlang && !newid || newid && !newlang)
		Error("specification conflict in 'langname' and 'langid'");

	if (is1st && !LANG2BYTE || LANG2BYTE && !is1st)
		Error("specification conflict in 'isfirst' and 'byte_chars'");

	if (LANG2BYTE && collate)
		fprintf(stderr,
			"WARNING: sequence specification will be ignored\n");

	/* make sure there is no conflict between yes and no strings */
	if (*info_tab[YESSTR] != NULL && *info_tab[NOSTR] != NULL) {
		if (LANG2BYTE) {
			if (!strncmp((char *)info_tab[YESSTR],
		    	    (char *)info_tab[NOSTR], 2)) 
				error(SAME_YESNO);
			if (!strncmp((char *) info_tab[YESSTR],
			    (char *)info_tab[NOSTR], 1) &&
			    !(ctype2[*info_tab[YESSTR]+c_offset]&_K1))
				error(SAME_YESNO);
		}
		else {
			if (!strncmp((char *) info_tab[YESSTR],
			    (char *)info_tab[NOSTR], 1))
				error(SAME_YESNO);
		}
	}

	/*
	** fill in all locale.def header information
	*/
	fill_lc_header();

	/*
	** write header & data to the "locale.def" file
	*/
	write_locale();

	/*
	** for debugging purpose only (output to stdout)
	*/
	debug_locale();
#ifdef	DBG
	fclose(dfp);
#endif

	/*
	** if the "-n" option is not specified
	** install the "locale.def" file and update the "config" file
	*/
	if (!no_install) {			/* install "locale.def" file */
		if (newlang || newid)		/* update "config" file */
#ifndef DBG
			sprintf(cmd, "/usr/bin/instlang y %d %s %s %s %s",
				lctable_head.nl_langid, lctable_head.lang,
				lname, territory, codeset);
#else DBG
			sprintf(cmd, "./instlang y %d %s %s %s %s",
				lctable_head.nl_langid, lctable_head.lang,
				lname, territory, codeset);
#endif DBG
		else				/* don't update "config" file */
#ifndef DBG
			sprintf(cmd, "/usr/bin/instlang n %s %s %s",
				lname, territory, codeset);
#else DBG
			sprintf(cmd, "./instlang n %s %s %s",
				lname, territory, codeset);
#endif DBG
		system(cmd);
	}
}


/* fill_lc_header:
** fill in each locale.def section header
** fill in locale.def file header 
** fill in each catinfo structure
*/
#define	CATINFO_SIZE	(sizeof(struct catinfotype))
void
fill_lc_header()
{
	int mod_addr = sizeof(lctable_head) + (N_CATEGORY * CATINFO_SIZE);
	int m = N_CATEGORY;
	int cat_total = 0;
	int i;

	/*
	** fill in locale.def FILE HEADER
	*/
	if (LANG2BYTE && collate) {
		cmlog[LC_COLLATE].n_mod = 0;
		cmlog[LC_COLLATE].cm[0].size = 0;
		(void) strcpy(cmlog[LC_COLLATE].cm[0].mod_name, "");
		cmlog[LC_COLLATE].cm[0].tmp = NULL;
	}

	for (i = 0; i < N_CATEGORY; i++) {
		if (cmlog[i].n_mod > 0) {
			cat_total += cmlog[i].cm[0].size;
			lctable_head.cat_no++;
			lctable_head.mod_no += cmlog[i].n_mod;
		}
	}
	lctable_head.mod_no -= lctable_head.cat_no;
	lctable_head.cat_no = N_CATEGORY;
	lctable_head.size = sizeof(lctable_head) +
			   (N_CATEGORY + lctable_head.mod_no) * CATINFO_SIZE;

#ifdef EUC
	if (LANG2BYTE && lctable_head.codeset == CODE1BYTE)
		lctable_head.codeset = CODE_HP15;
#else /* !EUC */
	if (LANG2BYTE)
		lctable_head.codeset = CODE2BYTE;
	else
		lctable_head.codeset = CODE1BYTE;
#endif /* !EUC */

	/*
	** fill in LC_ALL category and/or modifier catinfo
	*/
	catinfo[LC_ALL].size = cmlog[LC_ALL].cm[0].size;
	catinfo[LC_ALL].addr = 0;
	catinfo[LC_ALL].catid = LC_ALL;
	(void) strcpy(catinfo[LC_ALL].mod_name, cmlog[LC_ALL].cm[0].mod_name);
	if (cmlog[LC_ALL].n_mod <= 1)
		catinfo[LC_ALL].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_ALL].n_mod; i++) {
			if (i == 1)
				catinfo[LC_ALL].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_ALL].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_ALL;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_ALL].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}

	/*
	** fill in LC_COLLATE category and/or modifier catinfo
	*/
	catinfo[LC_COLLATE].size = cmlog[LC_COLLATE].cm[0].size;
	catinfo[LC_COLLATE].addr = catinfo[LC_ALL].addr +
				    catinfo[LC_ALL].size;
	catinfo[LC_COLLATE].catid = LC_COLLATE;
	(void) strcpy(catinfo[LC_COLLATE].mod_name,
		      cmlog[LC_COLLATE].cm[0].mod_name);
	if (cmlog[LC_COLLATE].n_mod <= 1)
		catinfo[LC_COLLATE].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_COLLATE].n_mod; i++) {
			if (i == 1)
				catinfo[LC_COLLATE].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_COLLATE].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_COLLATE;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_COLLATE].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}

	/*
	** fill in LC_CTYPE catinfo
	*/
	catinfo[LC_CTYPE].size = cmlog[LC_CTYPE].cm[0].size;
	catinfo[LC_CTYPE].addr = catinfo[LC_COLLATE].addr +
				  catinfo[LC_COLLATE].size;
	catinfo[LC_CTYPE].catid = LC_CTYPE;
	(void) strcpy(catinfo[LC_CTYPE].mod_name,
		      cmlog[LC_CTYPE].cm[0].mod_name);
	if (cmlog[LC_CTYPE].n_mod <= 1)
		catinfo[LC_CTYPE].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_CTYPE].n_mod; i++) {
			if (i == 1)
				catinfo[LC_CTYPE].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_CTYPE].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_CTYPE;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_CTYPE].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}

	/*
	** fill in LC_MONETARY catinfo
	*/
	catinfo[LC_MONETARY].size = cmlog[LC_MONETARY].cm[0].size;
	catinfo[LC_MONETARY].addr = catinfo[LC_CTYPE].addr +
				     catinfo[LC_CTYPE].size;
	catinfo[LC_MONETARY].catid = LC_MONETARY;
	strcpy(catinfo[LC_MONETARY].mod_name,
		      cmlog[LC_MONETARY].cm[0].mod_name);
	if (cmlog[LC_MONETARY].n_mod <= 1)
		catinfo[LC_MONETARY].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_MONETARY].n_mod; i++) {
			if (i == 1)
				catinfo[LC_MONETARY].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_MONETARY].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_MONETARY;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_MONETARY].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}

	/*
	** fill in LC_NUMERIC catinfo
	*/
	catinfo[LC_NUMERIC].size = cmlog[LC_NUMERIC].cm[0].size;
	catinfo[LC_NUMERIC].addr = catinfo[LC_MONETARY].addr +
				    catinfo[LC_MONETARY].size;
	catinfo[LC_NUMERIC].catid = LC_NUMERIC;
	strcpy(catinfo[LC_NUMERIC].mod_name,
		      cmlog[LC_NUMERIC].cm[0].mod_name);
	if (cmlog[LC_NUMERIC].n_mod <= 1)
		catinfo[LC_NUMERIC].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_NUMERIC].n_mod; i++) {
			if (i == 1)
				catinfo[LC_NUMERIC].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_NUMERIC].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_NUMERIC;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_NUMERIC].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}

	/*
	** fill in LC_TIME catinfo
	*/
	catinfo[LC_TIME].size = cmlog[LC_TIME].cm[0].size;
	catinfo[LC_TIME].addr = catinfo[LC_NUMERIC].addr +
				 catinfo[LC_NUMERIC].size;
	catinfo[LC_TIME].catid = LC_TIME;
	strcpy(catinfo[LC_TIME].mod_name,
		      cmlog[LC_TIME].cm[0].mod_name);
	if (cmlog[LC_TIME].n_mod <= 1)
		catinfo[LC_TIME].mod_addr = NULL;
	else {
		for (i = 1; i < cmlog[LC_TIME].n_mod; i++) {
			if (i == 1)
				catinfo[LC_TIME].mod_addr = mod_addr;
			else
				catinfo[m-1].mod_addr = mod_addr;
			catinfo[m].size = cmlog[LC_TIME].cm[i].size;
			catinfo[m].addr = cat_total;
			catinfo[m].catid = LC_TIME;
			(void) strcpy(catinfo[m].mod_name,
				      cmlog[LC_TIME].cm[i].mod_name);

			cat_total += catinfo[m].size;
			mod_addr += CATINFO_SIZE;
			m++;
		}
	}
}

/* flc_all:
** fill in LC_ALL section header & category size
*/
void
flc_all()
{
	lcall_head.yes_addr = sizeof(lcall_head);
	lcall_head.no_addr = lcall_head.yes_addr +
				strlen(info_tab[YESSTR]) + 1;
	lcall_head.direct_addr = lcall_head.no_addr +
				strlen(info_tab[NOSTR]) + 1;
	lcall_head.context_addr = lcall_head.direct_addr +
				strlen(info_tab[DIRECTION]) + 1;

	cmlog[LC_ALL].cm[cur_mod].size = lcall_head.context_addr +
					  strlen(info_tab[CONTEXT]) + 1;
	cmlog[LC_ALL].cm[cur_mod].pad =
			(4 - (cmlog[LC_ALL].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_ALL].cm[cur_mod].size += cmlog[LC_ALL].cm[cur_mod].pad;
	
	if (cmlog[LC_ALL].cm[cur_mod].size > _LC_ALL_SIZE)
		Error("LC_ALL category exceeds the maximum size allowed");
}


/* flc_coll:
** fill in LC_COLLATE section header & catinfo size
*/
void
flc_coll()
{
	if (two1_len)
		lccol_head.nl_map21 = TRUE;
	else
		lccol_head.nl_map21 = FALSE;

	if (max_pri_no)
		lccol_head.nl_onlyseq = FALSE;
	else
		lccol_head.nl_onlyseq = TRUE;

	lccol_head.seqtab_addr = sizeof(lccol_head);
	lccol_head.pritab_addr = lccol_head.seqtab_addr + TOT_ELMT;
	lccol_head.two1tab_addr = lccol_head.pritab_addr + TOT_ELMT;
	lccol_head.one2tab_addr = lccol_head.two1tab_addr + two1_len;

	cmlog[LC_COLLATE].cm[cur_mod].size = lccol_head.one2tab_addr + one2_len;
	cmlog[LC_COLLATE].cm[cur_mod].pad =
			(4 - (cmlog[LC_COLLATE].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_COLLATE].cm[cur_mod].size += cmlog[LC_COLLATE].cm[cur_mod].pad;

	if (cmlog[LC_COLLATE].cm[cur_mod].size > _LC_COLLATE_SIZE)
		Error("LC_COLLATE category exceeds the maximum size allowed");
}

/* flc_ctype:
** fill in LC_CTYPE section header & catinfo size
*/
void
flc_ctype()
{
	lcctype_head.ctype_addr = sizeof(lcctype_head);
	lcctype_head.kanji1_addr = lcctype_head.ctype_addr + CTYPESIZ;
	lcctype_head.kanji2_addr = lcctype_head.kanji1_addr + CTYPESIZ;
	lcctype_head.upshift_addr = lcctype_head.kanji2_addr + CTYPESIZ;
	lcctype_head.downshift_addr = lcctype_head.upshift_addr + SHIFTSIZ;
	lcctype_head.byte_char_addr = lcctype_head.downshift_addr + SHIFTSIZ;
	lcctype_head.alt_punct_addr = lcctype_head.byte_char_addr +
				strlen(info_tab[BYTES_CHAR]) + 1;
#ifdef EUC
	lcctype_head.io_csize_addr = lcctype_head.alt_punct_addr +
				strlen(info_tab[ALT_PUNCT]) + 1;
	lcctype_head.e_cset_addr = lcctype_head.io_csize_addr +
				(2 * MAX_CODESET);
	lcctype_head.ein_csize_addr = lcctype_head.e_cset_addr + SHIFTSIZ;
	lcctype_head.eout_csize_addr = lcctype_head.ein_csize_addr + SHIFTSIZ;
#endif /* EUC */

#ifdef EUC
	cmlog[LC_CTYPE].cm[cur_mod].size = lcctype_head.eout_csize_addr +
					SHIFTSIZ;
#else /* EUC */
	cmlog[LC_CTYPE].cm[cur_mod].size = lcctype_head.alt_punct_addr +
					    strlen(info_tab[ALT_PUNCT]) + 1;
#endif /* EUC */
	cmlog[LC_CTYPE].cm[cur_mod].pad =
			(4 - (cmlog[LC_CTYPE].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_CTYPE].cm[cur_mod].size += cmlog[LC_CTYPE].cm[cur_mod].pad;

	if (cmlog[LC_CTYPE].cm[cur_mod].size > _LC_CTYPE_SIZE)
		Error("LC_CTYPE category exceeds the maximum size allowed");
}

/* flc_mntry:
** fill in LC_MONETARY section header & catinfo size
*/
void
flc_mntry()
{
	lcmonetary_head.int_frac_digits = sizeof(lcmonetary_head);
	lcmonetary_head.frac_digits = lcmonetary_head.int_frac_digits +
				      strlen(mntry_tab[INT_FRAC]) + 1;
	lcmonetary_head.p_cs_precedes = lcmonetary_head.frac_digits +
				        strlen(mntry_tab[FRAC_DIGITS]) + 1;
	lcmonetary_head.p_sep_by_space = lcmonetary_head.p_cs_precedes +
				         strlen(mntry_tab[P_CS]) + 1;
	lcmonetary_head.n_cs_precedes = lcmonetary_head.p_sep_by_space +
				        strlen(mntry_tab[P_SEP]) + 1;
	lcmonetary_head.n_sep_by_space = lcmonetary_head.n_cs_precedes +
				         strlen(mntry_tab[N_CS]) + 1;
	lcmonetary_head.p_sign_posn = lcmonetary_head.n_sep_by_space +
				      strlen(mntry_tab[N_SEP]) + 1;
	lcmonetary_head.n_sign_posn = lcmonetary_head.p_sign_posn +
				      strlen(mntry_tab[P_SIGN]) + 1;
	lcmonetary_head.currency_symbol_lc = lcmonetary_head.n_sign_posn +
				          strlen(mntry_tab[N_SIGN]) + 1;
	lcmonetary_head.currency_symbol_li = lcmonetary_head.currency_symbol_lc
					   + strlen(mntry_tab[CURRENCY_LC]) + 1;
	lcmonetary_head.mon_decimal_point = lcmonetary_head.currency_symbol_li +
				          strlen(mntry_tab[CURRENCY_LI]) + 1;
	lcmonetary_head.int_curr_symbol = lcmonetary_head.mon_decimal_point +
				          strlen(mntry_tab[MON_DECIMAL]) + 1;
	lcmonetary_head.mon_thousands_sep = lcmonetary_head.int_curr_symbol +
				            strlen(mntry_tab[INT_CURR]) + 1;
	lcmonetary_head.mon_grouping = lcmonetary_head.mon_thousands_sep +
				       strlen(mntry_tab[MON_THOUSANDS]) + 1;
	lcmonetary_head.positive_sign = lcmonetary_head.mon_grouping +
				        strlen(mntry_tab[MON_GROUPING]) + 1;
	lcmonetary_head.negative_sign = lcmonetary_head.positive_sign +
				        strlen(mntry_tab[POSITIVE_SIGN]) + 1;

	cmlog[LC_MONETARY].cm[cur_mod].size = lcmonetary_head.negative_sign +
					strlen(mntry_tab[NEGATIVE_SIGN]) + 1;
	cmlog[LC_MONETARY].cm[cur_mod].pad =
			(4 - (cmlog[LC_MONETARY].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_MONETARY].cm[cur_mod].size +=
					cmlog[LC_MONETARY].cm[cur_mod].pad;

	if (cmlog[LC_MONETARY].cm[cur_mod].size > _LC_MONETARY_SIZE)
		Error("LC_MONETARY category exceeds the maximum size allowed");
}

/* flc_nmrc:
** fill in LC_NUMERIC section header & catinfo size
*/
void
flc_nmrc()
{
	lcnumeric_head.grouping = sizeof(lcnumeric_head);
	lcnumeric_head.decimal_point = lcnumeric_head.grouping +
				       strlen(nmrc_tab[GROUPING]) + 1;
	lcnumeric_head.thousands_sep = lcnumeric_head.decimal_point +
				       strlen(nmrc_tab[DECIMAL_P]) + 1;
	lcnumeric_head.alt_digit_addr = lcnumeric_head.thousands_sep +
				        strlen(nmrc_tab[THOUSANDS_S]) + 1;

	cmlog[LC_NUMERIC].cm[cur_mod].size = lcnumeric_head.alt_digit_addr +
			 		      strlen(info_tab[ALT_DIGIT]) + 1;
	cmlog[LC_NUMERIC].cm[cur_mod].pad =
			(4 - (cmlog[LC_NUMERIC].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_NUMERIC].cm[cur_mod].size += cmlog[LC_NUMERIC].cm[cur_mod].pad;

	if (cmlog[LC_NUMERIC].cm[cur_mod].size > _LC_NUMERIC_SIZE)
		Error("LC_NUMERIC category exceeds the maximum size allowed");
}

/* flc_time:
** fill in LC_TIME section header & catinfo size
*/
void
flc_time()
{
	pad_byte = 0;

	lctime_head.d_t_fmt = sizeof(lctime_head);
	lctime_head.d_fmt = lctime_head.d_t_fmt + strlen(info_tab[D_T_FMT]) + 1;
	lctime_head.t_fmt = lctime_head.d_fmt + strlen(info_tab[D_FMT]) + 1;

	lctime_head.day_1 = lctime_head.t_fmt + strlen(info_tab[T_FMT]) + 1;
	lctime_head.day_2 = lctime_head.day_1 + strlen(info_tab[DAY_1]) + 1;
	lctime_head.day_3 = lctime_head.day_2 + strlen(info_tab[DAY_2]) + 1;
	lctime_head.day_4 = lctime_head.day_3 + strlen(info_tab[DAY_3]) + 1;
	lctime_head.day_5 = lctime_head.day_4 + strlen(info_tab[DAY_4]) + 1;
	lctime_head.day_6 = lctime_head.day_5 + strlen(info_tab[DAY_5]) + 1;
	lctime_head.day_7 = lctime_head.day_6 + strlen(info_tab[DAY_6]) + 1;

	lctime_head.abday_1 = lctime_head.day_7 +
				strlen(info_tab[DAY_7]) + 1;
	lctime_head.abday_2 = lctime_head.abday_1 +
				strlen(info_tab[ABDAY_1]) + 1;
	lctime_head.abday_3 = lctime_head.abday_2 +
				strlen(info_tab[ABDAY_2]) + 1;
	lctime_head.abday_4 = lctime_head.abday_3 +
				strlen(info_tab[ABDAY_3]) + 1;
	lctime_head.abday_5 = lctime_head.abday_4 +
				strlen(info_tab[ABDAY_4]) + 1;
	lctime_head.abday_6 = lctime_head.abday_5 +
				strlen(info_tab[ABDAY_5]) + 1;
	lctime_head.abday_7 = lctime_head.abday_6 +
				strlen(info_tab[ABDAY_6]) + 1;

	lctime_head.mon_1 = lctime_head.abday_7 + strlen(info_tab[ABDAY_7]) + 1;
	lctime_head.mon_2 = lctime_head.mon_1 + strlen(info_tab[MON_1]) + 1;
	lctime_head.mon_3 = lctime_head.mon_2 + strlen(info_tab[MON_2]) + 1;
	lctime_head.mon_4 = lctime_head.mon_3 + strlen(info_tab[MON_3]) + 1;
	lctime_head.mon_5 = lctime_head.mon_4 + strlen(info_tab[MON_4]) + 1;
	lctime_head.mon_6 = lctime_head.mon_5 + strlen(info_tab[MON_5]) + 1;
	lctime_head.mon_7 = lctime_head.mon_6 + strlen(info_tab[MON_6]) + 1;
	lctime_head.mon_8 = lctime_head.mon_7 + strlen(info_tab[MON_7]) + 1;
	lctime_head.mon_9 = lctime_head.mon_8 + strlen(info_tab[MON_8]) + 1;
	lctime_head.mon_10 = lctime_head.mon_9 + strlen(info_tab[MON_9]) + 1;
	lctime_head.mon_11 = lctime_head.mon_10 + strlen(info_tab[MON_10]) + 1;
	lctime_head.mon_12 = lctime_head.mon_11 + strlen(info_tab[MON_11]) + 1;

	lctime_head.abmon_1 = lctime_head.mon_12 +
				strlen(info_tab[MON_12]) + 1;
	lctime_head.abmon_2 = lctime_head.abmon_1 +
				strlen(info_tab[ABMON_1]) + 1;
	lctime_head.abmon_3 = lctime_head.abmon_2 +
				strlen(info_tab[ABMON_2]) + 1;
	lctime_head.abmon_4 = lctime_head.abmon_3 +
				strlen(info_tab[ABMON_3]) + 1;
	lctime_head.abmon_5 = lctime_head.abmon_4 +
				strlen(info_tab[ABMON_4]) + 1;
	lctime_head.abmon_6 = lctime_head.abmon_5 +
				strlen(info_tab[ABMON_5]) + 1;
	lctime_head.abmon_7 = lctime_head.abmon_6 +
				strlen(info_tab[ABMON_6]) + 1;
	lctime_head.abmon_8 = lctime_head.abmon_7 +
				strlen(info_tab[ABMON_7]) + 1;
	lctime_head.abmon_9 = lctime_head.abmon_8 +
				strlen(info_tab[ABMON_8]) + 1;
	lctime_head.abmon_10 = lctime_head.abmon_9 +
				strlen(info_tab[ABMON_9]) + 1;
	lctime_head.abmon_11 = lctime_head.abmon_10 +
				strlen(info_tab[ABMON_10]) + 1;
	lctime_head.abmon_12 = lctime_head.abmon_11 +
				strlen(info_tab[ABMON_11]) + 1;

	lctime_head.am_str = lctime_head.abmon_12 +
				strlen(info_tab[ABMON_12]) + 1;
	lctime_head.pm_str = lctime_head.am_str +
				strlen(info_tab[AM_STR]) + 1;

	lctime_head.year_unit = lctime_head.pm_str +
				strlen(info_tab[PM_STR]) + 1;
	lctime_head.mon_unit = lctime_head.year_unit +
				strlen(info_tab[YEAR_UNIT]) + 1;
	lctime_head.day_unit = lctime_head.mon_unit +
				strlen(info_tab[MON_UNIT]) + 1;
	lctime_head.hour_unit = lctime_head.day_unit +
				strlen(info_tab[DAY_UNIT]) + 1;
	lctime_head.min_unit = lctime_head.hour_unit +
				strlen(info_tab[HOUR_UNIT]) + 1;
	lctime_head.sec_unit = lctime_head.min_unit +
				strlen(info_tab[MIN_UNIT]) + 1;

	lctime_head.era_fmt = lctime_head.sec_unit +
				strlen(info_tab[SEC_UNIT]) + 1;
	lctime_head.era_count = era_num;
	lctime_head.era_names = lctime_head.era_fmt +
				strlen(info_tab[ERA_FMT]) + 1;
	lctime_head.era_addr = lctime_head.era_names + era_len2;

	pad_byte = (4 - (lctime_head.era_addr % 4)) % 4;/* make sure era_addr starts */ 
	lctime_head.era_addr += pad_byte;		/* at word boundary. */

	cmlog[LC_TIME].cm[cur_mod].size = lctime_head.era_addr + era_len;
	cmlog[LC_TIME].cm[cur_mod].pad =
			(4 - (cmlog[LC_TIME].cm[cur_mod].size % 4)) % 4;
	cmlog[LC_TIME].cm[cur_mod].size += cmlog[LC_TIME].cm[cur_mod].pad;

	if (cmlog[LC_TIME].cm[cur_mod].size > _LC_TIME_SIZE)
		Error("LC_TIME category exceeds the maximum size allowed");
}

/* write_locale:
** write the "locale.def" file
*/
void
write_locale()
{
	extern void plc_all();
	extern void plc_coll();
	extern void plc_ctype();
	extern void plc_mntry();
	extern void plc_nmrc();
	extern void plc_time();

	FILE *fp, *tfp, *fopen();
	int i, j, k;
	char ch;

  	/*
  	** create the "locale.def" file
  	*/ 
  	if ((fp = fopen(FILENAME, "w")) == NULL)
		Error("can't create 'locale.def' file");
  
  	/*
  	** write out the LOCALE file header section
  	*/
	for (i = 0; i < sizeof(lctable_head); i++)
		(void) fprintf(fp, "%c", ((char *)&lctable_head)[i]);

  	/*
  	** write out the CATEGORY/MODIFIER catinfo structure section
  	*/
	for (i = 0; i < N_CATEGORY; i++)
		for (j = 0; j < sizeof(struct catinfotype); j++)
			(void) fprintf(fp, "%c", ((char *)&catinfo[i])[j]);
	for (i = N_CATEGORY; i < (N_CATEGORY+lctable_head.mod_no); i++)
		for (j = 0; j < sizeof(struct catinfotype); j++)
			(void) fprintf(fp, "%c", ((char *)&catinfo[i])[j]);

	/*
	** write out the LC_ALL section (header & data)
	*/
	tfp = cmlog[LC_ALL].cm[0].tmp;
	if (cmlog[LC_ALL].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_all(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_ALL].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the LC_COLLATE section (header & data)
	*/
	tfp = cmlog[LC_COLLATE].cm[0].tmp;
	if (cmlog[LC_COLLATE].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_coll(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_COLLATE].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the LC_CTYPE section (header & data)
	*/
	tfp = cmlog[LC_CTYPE].cm[0].tmp;
	if (cmlog[LC_CTYPE].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_ctype(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_CTYPE].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the LC_MONETARY section (header & data)
	*/
	tfp = cmlog[LC_MONETARY].cm[0].tmp;
	if (cmlog[LC_MONETARY].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_mntry(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_MONETARY].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the LC_NUMERIC section (header & data)
	*/
	tfp = cmlog[LC_NUMERIC].cm[0].tmp;
	if (cmlog[LC_NUMERIC].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_nmrc(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_NUMERIC].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the LC_TIME section (header & data)
	*/
	tfp = cmlog[LC_TIME].cm[0].tmp;
	if (cmlog[LC_TIME].n_mod > 0) {
		if (tfp == NULL)
			(void) plc_time(fp, 0);
		else {
			for (i = 0; i < cmlog[LC_TIME].cm[0].size; i++) {
				ch = getc(tfp);
				(void) fprintf(fp, "%c", ch);
			}
		}
	}

	/*
	** write out the MODIFIER section (header & data)
	*/
	for (i = 0; i < N_CATEGORY; i++) {
		for (j = 1; j < cmlog[i].n_mod; j++) {
			tfp = cmlog[i].cm[j].tmp;
			if (tfp == NULL) {
				switch (i) {
				    case LC_ALL:
					(void) plc_all(fp, j);
					break;
				    case LC_COLLATE:
					(void) plc_coll(fp, j);
					break;
				    case LC_CTYPE:
					(void) plc_ctype(fp, j);
					break;
				    case LC_MONETARY:
					(void) plc_mntry(fp, j);
					break;
				    case LC_NUMERIC:
					(void) plc_nmrc(fp, j);
					break;
				    case LC_TIME:
					(void) plc_time(fp, j);
					break;
				}
			}
			else {
				for (k = 0; k < cmlog[i].cm[j].size; k++) {
				    ch = getc(tfp);
				    (void) fprintf(fp, "%c", ch);
				}
			}
		}
	}

	fclose(fp);
}
