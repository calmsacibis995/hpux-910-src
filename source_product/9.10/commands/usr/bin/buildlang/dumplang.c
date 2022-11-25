/* @(#) $Revision: 66.3 $ */     
/* LINTLIBRARY */

#include	<stdio.h>
#include	<limits.h>
#include	<nl_ctype.h>
#include	"global.h"

#define DCTYPESIZ	257
#define DSHIFTSIZ	256

#define CFLAG	1
#define DFLAG	2
#define OFLAG	3
#define XFLAG	4

#define LCPATHLEN	80
#define ERRLEN		90

#ifdef _NAMESPACE_CLEAN
#   define _1kanji    __1kanji
#   define _2kanji    __2kanji
#   define _upshift   __upshift
#   define _downshift __downshift
#endif /* _NAMESPACE_CLEAN */

extern	unsigned char	*__ctype;	/* pointer to ctype table 	*/
extern	unsigned char	*_1kanji;	/* pointer to 1st of 2 kanji table */
extern	unsigned char	*_2kanji;	/* pointer to 2nd of 2 kanji table */
extern	unsigned char	*_upshift;	/* pointer to up shift table	*/
extern	unsigned char	*_downshift;	/* pointer to down shift table	*/
extern	unsigned char   **__nl_info;	/* pointer to nl_langinfo msg pointers*/
extern struct _era_data	*_nl_era[]; 	/* array of era info struct pointer */
extern	int		 __nl_char_size;/* size of characters		*/
extern  int             _nl_context;    /* directionality context flag */
extern	struct lconv	*_lconv;
extern	unsigned char	__in_csize[];	/* input code size table for EUC */

int	pflag;

/* dumplang:
** take the named 'locale' and produce its buildlang script
*/
dumplang(locale, option)
char *locale;
char *option;
{
	FILE *fp, *fopen();

	extern char *strcpy(),
		    *strcat();
	extern unsigned char *mkstr();
	extern setlocale();

	char lcpath[LCPATHLEN];
	char name_mod[_POSIX_NAME_MAX];
	char errmsg[ERRLEN];
	char *cp;
	int i, j;

	switch (*option) {
		case 'c':
			pflag = CFLAG;
			break;
		case 'd':
			pflag = DFLAG;
			break;
		case 'o':
			pflag = OFLAG;
			break;
		case 'x':
			pflag = XFLAG;
			break;
		default:
			Error("Usage: buildlang -d[fc|fd|fo|fx] locale");
	}

	/* 
	** preserve the LC_CTYPE information from the environment
	** used to determine "isprint", "firstof2" and "secof2" characters
	*/
	setlocale(LC_CTYPE, getenv("LC_CTYPE"));

	ctype1 = (unsigned char *)malloc(DCTYPESIZ * sizeof(unsigned char));
	ctype3 = (unsigned char *)malloc(DCTYPESIZ * sizeof(unsigned char));
	ctype4 = (unsigned char *)malloc(DCTYPESIZ * sizeof(unsigned char));

	for (i = 0; i < DCTYPESIZ; i++) {
		ctype1[i] = __ctype[i];
		ctype3[i] = _1kanji[i];
		ctype4[i] = _2kanji[i];
	}

	/* 
	** open the "locale.def" file
	*/
	(void) strcpy(lcpath, NLSDIR);
	(void) strcat(lcpath, locale);
	for (cp = lcpath; *cp; cp++)	/* replace '_' and '.' with '/' */
		if (*cp == '_' || *cp == '.')
			*cp = '/';
	(void) strcat(lcpath, "/locale.def");

	if((fp = fopen(lcpath, "r")) == NULL) {
		(void) strcpy(errmsg, "can't open file: ");
		(void) strcat(errmsg, lcpath);
		Error(errmsg);
	}

	/*
	** read the locale.def table header section
	*/
	cp = (char *)&lctable_head;
	for (i = 0; i < sizeof(lctable_head); i++)
		cp[i] = getc(fp);

	/*
	** read the category/modifier section
	*/
	for (i = 0; i < N_CATEGORY + lctable_head.mod_no; i++) {
		cp = (char *)&catinfo[i];
		for (j = 0; j < sizeof(struct catinfotype); j++)
			cp[j] = getc(fp);
	}

	/*
	** call setlocale(LC_ALL, locale)
	** if modifiers exist for some categories,
	** call setlocale() again with appropriate category and modifier names.
	** then take appropriate pointers to print out information.
	*/
	if (!setlocale(LC_ALL, lctable_head.lang)) {
		(void) strcpy(errmsg, "locale \"");
		(void) strcat(errmsg, locale);
		(void) strcat(errmsg, "\" cannot be accessed successfully");
		Error(errmsg);
	}

	/*
	** print buildlang script (langname, langid and revision)
	*/
	if (lctable_head.rev_flag)
		printf("## HP defined  %s\n\n", lctable_head.rev_str);
	else
		printf("## User defined  %s\n\n", lctable_head.rev_str);

	printf("langname\t%s\n", (char *)mkstr(lctable_head.lang));
	printf("langid\t%d\n\n", lctable_head.nl_langid);

	printf("revision\t\"nnn.nn\"\t# fill in revision number here\n\n");
	
	/*
	** print buildlang script (LC_ALL section)
	*/
	if (catinfo[LC_ALL].size != 0) {
	printf("##################################################\n");
	printf("# LC_ALL category\n\n");

	printf("LC_ALL\n");
	if (*catinfo[LC_ALL].mod_name != NULL)
		printf("modifier\t%s\n",
			(char *)mkstr(catinfo[LC_ALL].mod_name));
		dall();
	for (i = N_CATEGORY; i < N_CATEGORY + lctable_head.mod_no; i++) {
		if (catinfo[i].catid == LC_ALL) {
		    strcpy(name_mod, lctable_head.lang);
		    strcat(name_mod, "@");
		    strcat(name_mod, catinfo[i].mod_name);
		    setlocale(LC_ALL, name_mod);
		    printf("\nmodifier\t%s\n",
			    (char *)mkstr(catinfo[i].mod_name));
		    dall();
		}
	}
	printf("END_LC\n\n\n");
	}

	/*
	** print buildlang script (LC_COLLATE section)
	*/
	if (catinfo[LC_COLLATE].size != 0) {
	printf("##################################################\n");
	printf("# LC_COLLATE category\n\n");

	printf("LC_COLLATE\n");
	if (*catinfo[LC_COLLATE].mod_name != NULL)
		printf("modifier\t%s\n",
			(char *)mkstr(catinfo[LC_COLLATE].mod_name));
		dcoll();
	for (i = N_CATEGORY; i < N_CATEGORY + lctable_head.mod_no; i++) {
		if (catinfo[i].catid == LC_COLLATE) {
		    strcpy(name_mod, lctable_head.lang);
		    strcat(name_mod, "@");
		    strcat(name_mod, catinfo[i].mod_name);
		    setlocale(LC_COLLATE, name_mod);
		    printf("\nmodifier\t%s\n",
			    (char *)mkstr(catinfo[i].mod_name));
		    dcoll();
		}
	}
	printf("END_LC\n\n\n");
	}

	/*
	** print buildlang script (LC_CTYPE section)
	*/
	if (catinfo[LC_CTYPE].size != 0) {
	printf("##################################################\n");
	printf("# LC_CTYPE category\n\n");

	printf("LC_CTYPE\n");
	if (*catinfo[LC_CTYPE].mod_name != NULL)
		printf("modifier\t%s\n",
			(char *)mkstr(catinfo[LC_CTYPE].mod_name));
		dctype();
		dshift();
		printf("\nbytes_char\t%s\n",
			(char *)mkstr(__nl_info[BYTES_CHAR]));
		printf("alt_punct\t%s\n", (char *)mkstr(__nl_info[ALT_PUNCT]));
#ifdef EUC
		printf("code_scheme\t");
		if (__cs_HP15)
		    printf(mkstr("HP15"));
		else if (__cs_EUC)
		    printf(mkstr("EUC"));
		else
		    printf(mkstr(""));
		printf("\n");
		if (__cs_EUC) {	/* cswidth is needed only for EUC */
		    char buf[100];
		    sprintf(buf, "%d:%d,%d:%d,%d:%d",
			    __in_csize[1], __out_csize[1],
			    __in_csize[2]-1, __out_csize[2],
			    __in_csize[3]-1, __out_csize[3]);
		    printf("cswidth\t%s\n", mkstr(buf));
		}
#endif /* EUC */
	for (i = N_CATEGORY; i < N_CATEGORY + lctable_head.mod_no; i++) {
		if (catinfo[i].catid == LC_CTYPE) {
		    strcpy(name_mod, lctable_head.lang);
		    strcat(name_mod, "@");
		    strcat(name_mod, catinfo[i].mod_name);
		    setlocale(LC_CTYPE, name_mod);
		    printf("\nmodifier\t%s\n",
			    (char *)mkstr(catinfo[i].mod_name));
		    dctype();
		    dshift();
		    printf("\nbytes_char\t%s\n",
			    (char *)mkstr(__nl_info[BYTES_CHAR]));
		    printf("alt_punct\t%s\n",
			    (char *)mkstr(__nl_info[ALT_PUNCT]));
#ifdef EUC
		    printf("code_scheme\t");
		    if (__cs_HP15)
			printf(mkstr("HP15"));
		    else if (__cs_EUC)
			printf(mkstr("EUC"));
		    else
			printf(mkstr(""));
		    printf("\n");
		    if (__cs_EUC) {	/* cswidth is needed only for EUC */
			char buf[100];
			sprintf(buf, "%d:%d,%d:%d,%d:%d",
				__in_csize[1], __out_csize[1],
				__in_csize[2]-1, __out_csize[2],
				__in_csize[3]-1, __out_csize[3]);
			printf("cswidth\t%s\n", mkstr(buf));
		    }
#endif /* EUC */
		}
	}
	printf("END_LC\n\n\n");
	}

	/*
	** print buildlang script (LC_MONETARY section)
	*/
	if (catinfo[LC_MONETARY].size != 0) {
	printf("##################################################\n");
	printf("# LC_MONETARY category\n\n");

	printf("LC_MONETARY\n");
	if (*catinfo[LC_MONETARY].mod_name != NULL)
		printf("modifier\t%s\n",
			(char *)mkstr(catinfo[LC_MONETARY].mod_name));
		dmntry();
	for (i = N_CATEGORY; i < N_CATEGORY + lctable_head.mod_no; i++) {
		if (catinfo[i].catid == LC_MONETARY) {
		    strcpy(name_mod, lctable_head.lang);
		    strcat(name_mod, "@");
		    strcat(name_mod, catinfo[i].mod_name);
		    setlocale(LC_MONETARY, name_mod);
		    printf("\nmodifier\t%s\n",
			    (char *)mkstr(catinfo[i].mod_name));
		    dmntry();
		}
	}
	printf("END_LC\n\n\n");
	}

	/*
	** print buildlang script (LC_NUMERIC section)
	*/
	if (catinfo[LC_NUMERIC].size != 0) {
	printf("##################################################\n");
	printf("# LC_NUMERIC category\n\n");

	printf("LC_NUMERIC\n");
	if (*catinfo[LC_NUMERIC].mod_name != NULL)
		printf("modifier\t%s\n",
			(char *)mkstr(catinfo[LC_NUMERIC].mod_name));
		dnmrc();
	for (i = N_CATEGORY; i < N_CATEGORY + lctable_head.mod_no; i++) {
		if (catinfo[i].catid == LC_NUMERIC) {
		    strcpy(name_mod, lctable_head.lang);
		    strcat(name_mod, "@");
		    strcat(name_mod, catinfo[i].mod_name);
		    setlocale(LC_NUMERIC, name_mod);
		    printf("\nmodifier\t%s\n",
			    (char *)mkstr(catinfo[i].mod_name));
		    dnmrc();
		}
	}
	printf("END_LC\n\n\n");
	}

	/*
	** print buildlang script (LC_TIME section)
	*/
	if (catinfo[LC_TIME].size != 0) {
	printf("##################################################\n");
	printf("# LC_TIME category\n\n");

	printf("LC_TIME\n");
	if (*catinfo[LC_TIME].mod_name != NULL)
		printf("modifier\t%s\n",
			(char *)mkstr(catinfo[LC_TIME].mod_name));
		dtime();
	for (i = N_CATEGORY; i < N_CATEGORY + lctable_head.mod_no; i++) {
		if (catinfo[i].catid == LC_TIME) {
		    strcpy(name_mod, lctable_head.lang);
		    strcat(name_mod, "@");
		    strcat(name_mod, catinfo[i].mod_name);
		    setlocale(LC_TIME, name_mod);
		    printf("\nmodifier\t%s\n",
			    (char *)mkstr(catinfo[i].mod_name));
		    dtime();
		}
	}
	printf("END_LC\n\n\n");
	}

}

/* dall:
** dump LC_ALL buildlang section
*/
dall()
{
	printf("yesstr\t%s\n", (char *)mkstr(__nl_info[YESSTR]));
	printf("nostr\t%s\n", (char *)mkstr(__nl_info[NOSTR]));
	printf("direction\t%s\n", (char *)mkstr(__nl_info[DIRECTION]));
	if (_nl_context)
		printf("context\t\"%d\"\n", _nl_context);
	else
		printf("context\t\"\"\n");
}

/* dctype:
** dump LC_CTYPE (ctype) buildlang section
*/
dctype()
{
	int i, cnt;

	cnt = 0;				/* isupper */
	printf("isupper\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _U) {
	        pcode(i);
		cnt++;
		if ((cnt % 12) == 0)
		    printf("\n\t");
	    }
	}
	printf("\n\n");

	cnt = 0;				/* islower */
	printf("islower\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _L) {
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0)
		    printf("\n\t");
	    }
	}
	printf("\n\n");

	cnt = 0;				/* isdigit */
	printf("isdigit\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _N) {
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0)
		    printf("\n\t");
	    }
	}
	printf("\n\n");

	cnt = 0;				/* isspace */
	printf("isspace\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _S) {
	        pcode(i);
		/* printf("0x%x ", i); */
		cnt++;
		if ((cnt %12) == 0)
		    printf("\n\t");
	    }
	}
	printf("\n\n");

	cnt = 0;				/* ispunct */
	printf("ispunct\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _P) {
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0)
		    printf("\n\t");
	    }
	}
	printf("\n\n");

	cnt = 0;				/* iscntrl */
	printf("iscntrl\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _C) {
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0)
		    printf("\n\t");
	    }
	}
	printf("\n\n");

	cnt = 0;				/* isblank */
	printf("isblank\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _B) {
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0)
		    printf("\n\t");
	    }
	}
	printf("\n\n");

	cnt = 0;				/* isxdigit */
	printf("isxdigit\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _X) {
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0)
		    printf("\n\t");
	    }
	}
	printf("\n\n");

	if (__nl_char_size > 1) {
		cnt = 0;			/* isfirst */
		printf("isfirst\t");
		for (i = 0; i < DCTYPESIZ; i++) {
		    if (_1kanji[i]) {
	        	pcode(i);
			cnt++;
			if ((cnt %12) == 0)
			    printf("\n\t");
		    }
		}
		printf("\n\n");

		cnt = 0;			/* issecond */
		printf("issecond\t");
		for (i = 0; i < DCTYPESIZ; i++) {
		    if (_2kanji[i]) {
	        	pcode(i);
			cnt++;
			if ((cnt %12) == 0)
			    printf("\n\t");
		    }
		}
		printf("\n\n");
	}
}

/* dshift:
** dump LC_CTYPE (conv) buildlang section
*/
dshift()
{
	int i, cnt, flag = 0;

	cnt = 0;				/* ul */
	printf("ul\t");
	for (i = 0; i < DSHIFTSIZ; i++) {
	    if (_upshift[i] != i) {
	        if (_downshift[_upshift[i]] == i) {
		    printf("< ");
		    pcode(_upshift[i]);
		    pcode(i);
		    printf("> ");
		    cnt++;
		    if ((cnt % 4) == 0)
		        printf("\n\t");
		} else
		    flag++;
	    }
	}
	printf("\n\n");

	if (flag) {
	    cnt = 0;				/* toupper */
	    printf("toupper\t");
	    for (i = 0; i < DSHIFTSIZ; i++) {
	        if (_upshift[i] != i) {
	            if (_downshift[_upshift[i]] != i) {
		        printf("< ");
		        pcode(i);
		        pcode(_upshift[i]);
		        printf("> ");
			cnt++;
			if ((cnt %4) == 0)
			    printf("\n\t");
		    }
		}
	    }
	    printf("\n\n");

	    cnt = 0;				/* tolower */
	    printf("tolower\t");
	    for (i = 0; i < DSHIFTSIZ; i++) {
	        if (_downshift[i] != i) {
	            if (_upshift[_downshift[i]] != i) {
		        printf("< ");
		        pcode(i);
		        pcode(_downshift[i]);
		        printf("> ");
			cnt++;
			if ((cnt %4) == 0)
			    printf("\n\t");
		    }
		}
	    }
	    printf("\n\n");
	}
}

/* dmntry:
** dump LC_MONETARY buildlang section
*/
dmntry()
{
	printf("int_curr_symbol\t%s\n", (char *)mkstr(_lconv->int_curr_symbol));
	printf("currency_symbol\t%s\n", (char *)mkstr(_lconv->currency_symbol));
	printf("mon_decimal_point\t%s\n",
		(char *)mkstr(_lconv->mon_decimal_point));
	printf("mon_thousands_sep\t%s\n",
		(char *)mkstr(_lconv->mon_thousands_sep));
	printf("mon_grouping\t%s\n", (char *)mkstr(_lconv->mon_grouping));
	printf("positive_sign\t%s\n", (char *)mkstr(_lconv->positive_sign));
	printf("negative_sign\t%s\n", (char *)mkstr(_lconv->negative_sign));

	if (_lconv->int_frac_digits == CHAR_MAX)
		printf("int_frac_digits\t\"\"\n");
	else
		printf("int_frac_digits\t\"%c\"\n", _lconv->int_frac_digits);
	if (_lconv->frac_digits == CHAR_MAX)
		printf("frac_digits\t\"\"\n");
	else
		printf("frac_digits\t\"%c\"\n", _lconv->frac_digits);
	if (_lconv->p_cs_precedes == CHAR_MAX)
		printf("p_cs_precedes\t\"\"\n");
	else
		printf("p_cs_precedes\t\"%c\"\n", _lconv->p_cs_precedes);
	if (_lconv->p_sep_by_space == CHAR_MAX)
		printf("p_sep_by_space\t\"\"\n");
	else
		printf("p_sep_by_space\t\"%c\"\n", _lconv->p_sep_by_space);
	if (_lconv->n_cs_precedes == CHAR_MAX)
		printf("n_cs_precedes\t\"\"\n");
	else
		printf("n_cs_precedes\t\"%c\"\n", _lconv->n_cs_precedes);
	if (_lconv->n_sep_by_space == CHAR_MAX)
		printf("n_sep_by_space\t\"\"\n");
	else
		printf("n_sep_by_space\t\"%c\"\n", _lconv->n_sep_by_space);
	if (_lconv->p_sign_posn == CHAR_MAX)
		printf("p_sign_posn\t\"\"\n");
	else
		printf("p_sign_posn\t\"%c\"\n", _lconv->p_sign_posn);
	if (_lconv->n_sign_posn == CHAR_MAX)
		printf("n_sign_posn\t\"\"\n");
	else
		printf("n_sign_posn\t\"%c\"\n", _lconv->n_sign_posn);

	printf("crncystr\t%s\n", (char *)mkstr(__nl_info[CRNCYSTR]));
}

/* dnmrc:
** dump LC_NUMERIC buildlang section
*/
dnmrc()
{
	printf("decimal_point\t%s\n", (char *)mkstr(_lconv->decimal_point));
	printf("thousands_sep\t%s\n", (char *)mkstr(_lconv->thousands_sep));
	printf("grouping\t%s\n", (char *)mkstr(_lconv->grouping));

	printf("alt_digit\t%s\n", (char *)mkstr(__nl_info[ALT_DIGIT]));
}

/* dtime:
** dump LC_TIME buildlang section
*/
dtime()
{
	int i;
	unsigned char buf[MAX_INFO_MSGS+20];

	printf("d_t_fmt\t%s\n", (char *)mkstr(__nl_info[D_T_FMT]));
	printf("d_fmt\t%s\n", (char *)mkstr(__nl_info[D_FMT]));
	printf("t_fmt\t%s\n", (char *)mkstr(__nl_info[T_FMT]));

	printf("day_1\t%s\n", (char *)mkstr(__nl_info[DAY_1]));
	printf("day_2\t%s\n", (char *)mkstr(__nl_info[DAY_2]));
	printf("day_3\t%s\n", (char *)mkstr(__nl_info[DAY_3]));
	printf("day_4\t%s\n", (char *)mkstr(__nl_info[DAY_4]));
	printf("day_5\t%s\n", (char *)mkstr(__nl_info[DAY_5]));
	printf("day_6\t%s\n", (char *)mkstr(__nl_info[DAY_6]));
	printf("day_7\t%s\n", (char *)mkstr(__nl_info[DAY_7]));

	printf("abday_1\t%s\n", (char *)mkstr(__nl_info[ABDAY_1]));
	printf("abday_2\t%s\n", (char *)mkstr(__nl_info[ABDAY_2]));
	printf("abday_3\t%s\n", (char *)mkstr(__nl_info[ABDAY_3]));
	printf("abday_4\t%s\n", (char *)mkstr(__nl_info[ABDAY_4]));
	printf("abday_5\t%s\n", (char *)mkstr(__nl_info[ABDAY_5]));
	printf("abday_6\t%s\n", (char *)mkstr(__nl_info[ABDAY_6]));
	printf("abday_7\t%s\n", (char *)mkstr(__nl_info[ABDAY_7]));

	printf("mon_1\t%s\n", (char *)mkstr(__nl_info[MON_1]));
	printf("mon_2\t%s\n", (char *)mkstr(__nl_info[MON_2]));
	printf("mon_3\t%s\n", (char *)mkstr(__nl_info[MON_3]));
	printf("mon_4\t%s\n", (char *)mkstr(__nl_info[MON_4]));
	printf("mon_5\t%s\n", (char *)mkstr(__nl_info[MON_5]));
	printf("mon_6\t%s\n", (char *)mkstr(__nl_info[MON_6]));
	printf("mon_7\t%s\n", (char *)mkstr(__nl_info[MON_7]));
	printf("mon_8\t%s\n", (char *)mkstr(__nl_info[MON_8]));
	printf("mon_9\t%s\n", (char *)mkstr(__nl_info[MON_9]));
	printf("mon_10\t%s\n", (char *)mkstr(__nl_info[MON_10]));
	printf("mon_11\t%s\n", (char *)mkstr(__nl_info[MON_11]));
	printf("mon_12\t%s\n", (char *)mkstr(__nl_info[MON_12]));

	printf("abmon_1\t%s\n", (char *)mkstr(__nl_info[ABMON_1]));
	printf("abmon_2\t%s\n", (char *)mkstr(__nl_info[ABMON_2]));
	printf("abmon_3\t%s\n", (char *)mkstr(__nl_info[ABMON_3]));
	printf("abmon_4\t%s\n", (char *)mkstr(__nl_info[ABMON_4]));
	printf("abmon_5\t%s\n", (char *)mkstr(__nl_info[ABMON_5]));
	printf("abmon_6\t%s\n", (char *)mkstr(__nl_info[ABMON_6]));
	printf("abmon_7\t%s\n", (char *)mkstr(__nl_info[ABMON_7]));
	printf("abmon_8\t%s\n", (char *)mkstr(__nl_info[ABMON_8]));
	printf("abmon_9\t%s\n", (char *)mkstr(__nl_info[ABMON_9]));
	printf("abmon_10\t%s\n", (char *)mkstr(__nl_info[ABMON_10]));
	printf("abmon_11\t%s\n", (char *)mkstr(__nl_info[ABMON_11]));
	printf("abmon_12\t%s\n", (char *)mkstr(__nl_info[ABMON_12]));

	printf("am_str\t%s\n", (char *)mkstr(__nl_info[AM_STR]));
	printf("pm_str\t%s\n", (char *)mkstr(__nl_info[PM_STR]));

	printf("year_unit\t%s\n", (char *)mkstr(__nl_info[YEAR_UNIT]));
	printf("mon_unit\t%s\n", (char *)mkstr(__nl_info[MON_UNIT]));
	printf("day_unit\t%s\n", (char *)mkstr(__nl_info[DAY_UNIT]));
	printf("hour_unit\t%s\n", (char *)mkstr(__nl_info[HOUR_UNIT]));
	printf("min_unit\t%s\n", (char *)mkstr(__nl_info[MIN_UNIT]));
	printf("sec_unit\t%s\n", (char *)mkstr(__nl_info[SEC_UNIT]));

	printf("era_fmt\t%s\n", (char *)mkstr(__nl_info[ERA_FMT]));
	for (i = 0; i < MAX_ERA_FMTS && _nl_era[i] != (struct _era_data *)NULL;
	     i++) {
		if (i == 0)
			printf("era");
		if ((_nl_era[i]->signflag > 0 && _nl_era[i]->origin_year == _nl_era[i]->start_year)
		|| (_nl_era[i]->signflag < 0 && _nl_era[i]->origin_year == _nl_era[i]->end_year))
			printf("\t\"+");
		else
			printf("\t\"-");
		printf(":%d:", _nl_era[i]->offset);

		if (_nl_era[i]->origin_year == _nl_era[i]->start_year) {
			printf("%.4d/%.2d/%.2d:", _nl_era[i]->start_year,
					    _nl_era[i]->start_month,
					    _nl_era[i]->start_day);
			if (_nl_era[i]->end_year == SHRT_MAX)
				printf("+*:");
			else
			if (_nl_era[i]->end_year == SHRT_MIN)
				printf("-*:");
			else
				printf("%.4d/%.2d/%.2d:", _nl_era[i]->end_year,
						    _nl_era[i]->end_month,
						    _nl_era[i]->end_day);
		} else {
			printf("%.4d/%.2d/%.2d:", _nl_era[i]->end_year,
					    _nl_era[i]->end_month,
					    _nl_era[i]->end_day);
			if (_nl_era[i]->start_year == SHRT_MAX)
				printf("+*:");
			else
			if (_nl_era[i]->start_year == SHRT_MIN)
				printf("-*:");
			else
				printf("%.4d/%.2d/%.2d:", _nl_era[i]->start_year,
						    _nl_era[i]->start_month,
						    _nl_era[i]->start_day);
		}

		(void) strcpy((char *)buf, (char *)(mkstr(_nl_era[i]->name)+1));

		if (*_nl_era[i]->format == '\0')	/* no format string */
			printf("%s\n", (char *)buf);
		else {
			buf[strlen(buf)-1] = '\0';
			printf("%s", (char *)buf);
			printf(":%s\n", (char *)(mkstr(_nl_era[i]->format)+1));
		}
	}
}

/* pcode:
** if a character code is not printable,
** print the character code in "character", "decimal", "octal" or
** "hexadecimal" constant according to the "-f" option.
*/
pcode(i)
int i;
{
	if (xisprint(i))
		printf("'%c' ", i);
	else {
		switch (pflag) {
			case CFLAG:
				if (i == 0 || i == '\n') /* null or newline */
					printf("0x%x ", i);
				else
					printf("'%c' ", i);
				break;
			case DFLAG:
				printf("%d ", i);
				break;
			case OFLAG:
				printf("0%o ", i);
				break;
			case XFLAG:
				printf("0x%x ", i);
				break;
		}
	}
}
