/* @(#) $Revision: 70.8 $ */     
/* LINTLIBRARY */

#include	<stdio.h>
#include	<limits.h>
#include        <ctype.h>
#include	<nl_ctype.h> 
#include	"global.h"
#include        <langinfo.h>


#define DCTYPESIZ	257
#define DSHIFTSIZ	256
#define DCTYPE2         257

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
extern  unsigned char   *__ctype2;       /* pointer to ctype2 table */
extern	unsigned char	*_1kanji;	/* pointer to 1st of 2 kanji table */
extern	unsigned char	*_2kanji;	/* pointer to 2nd of 2 kanji table */
extern	unsigned char	*_upshift;	/* pointer to up shift table	*/
extern	unsigned char	*_downshift;	/* pointer to down shift table	*/
extern	unsigned char   **__nl_info;	/* pointer to nl_langinfox msg pointers*/
extern struct _era_data	*_nl_era[]; 	/* array of era info struct pointer */
extern	int		 __nl_char_size;/* size of characters		*/
extern  int             _nl_context;    /* directionality context flag */
extern	struct lconv	*_lconv;
extern	unsigned char	__in_csize[];	/* input code size table for EUC */

int	pflag;
/*extern  unsigned int escape_char; */

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
	extern void dmesg();
	extern setlocale();


	char lcpath[LCPATHLEN];
	char name_mod[_POSIX_NAME_MAX];
	char errmsg[ERRLEN];
	char *cp, cc;
	int i, j;
	char ret_val;

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
			Error(catgets(catd,NL_SETN,4,"Usage: localedef -d[fc|fd|fo|fx] locale",4));
	}
	
	/* 
	** preserve the LC_CTYPE information from the environment
	** used to determine "isprint", "firstof2" and "secof2" characters
	*/
	 /* bsetlocale(LC_CTYPE, getenv("LC_CTYPE"));
	 */
	 ret_val = setlocale(LC_CTYPE, "");
	ctype1 = (unsigned char *)malloc(DCTYPESIZ * sizeof(unsigned char));
	ctype3 = (unsigned char *)malloc(DCTYPESIZ * sizeof(unsigned char));
	ctype4 = (unsigned char *)malloc(DCTYPESIZ * sizeof(unsigned char));
	ctype5 = (unsigned char *)malloc(DCTYPESIZ * sizeof(unsigned char));
	for (i = 0; i < DCTYPESIZ; i++) {
		ctype1[i] = __ctype[i];
		ctype3[i] = _1kanji[i];
		ctype4[i] = _2kanji[i];
		ctype5[i] = __ctype2[i];
	}

	/* 
	** open the "locale.inf" file
	*/
	(void) strcpy(lcpath, NLSDIR);
	(void) strcat(lcpath, locale);
	for (cp = lcpath; *cp; cp++)	/* replace '_' and '.' with '/' */
		if (*cp == '_' || *cp == '.')
			*cp = '/';
	(void) strcat(lcpath, "/locale.inf");

	if((fp = fopen(lcpath, "r")) == NULL) {
		(void) strcpy(errmsg, "can't open file: ");
		(void) strcat(errmsg, lcpath);
		Error(errmsg,4);
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
		Error(errmsg,4);
	}

	/*
	** print buildlang script (langname, langid and revision)
	*/
	printf("langname\t%s\n", (char *)mkstr(lctable_head.lang));
	printf("langid\t%d\n\n", lctable_head.nl_langid);

	if ( strlen(lctable_head.rev_str) ) {
	   char cp[40], *p;
	   if (lctable_head.rev_flag) {
		printf("## HP defined  %s\n\n", lctable_head.rev_str);
		printf("hp");
	   	p = strchr(lctable_head.rev_str,'$');
	   	(void) strcpy(cp,p);
	   	p = strchr((cp+1),'$');
	   	*(++p) = '\0';
	   }
	   else {
	   	p = strchr(lctable_head.rev_str,':');
	   	(void) strcpy(cp,(p+2));
	   	p = strchr(cp,'$');
	   	*(p-1) = '\0';
		printf("## User defined  %s\n\n", lctable_head.rev_str);
	   }
	   printf("revision\t\"%s\" \n\n",cp);
	}

/* 
  Posix 11.2 code
 */	

	printf("comment_char\t%c\n",lctable_head.comment_char);
        printf("escape_char\t%c   \n",lctable_head.escape_char);
/*	if ( !nl_langinfo(CHARMAP) )
	   printf("charmap\t%s\n",nl_langinfo(CHARMAP)); */ 
	cc=lctable_head.comment_char;
	/*

	** print buildlang script (LC_CTYPE section)
	*/
	if (catinfo[LC_CTYPE].size != 0) {
	printf("%c%c%c%c%c%c%c%c\n",cc,cc,cc,cc,cc,cc,cc,cc);
	printf("%c LC_CTYPE category\n\n",cc);

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
/* The following was changed to have cswidth of the format "n:n,n:n"
   since we currently do not support SCS3. This was done per Judy Chen's
   request.   AL 7/8/92
		    sprintf(buf, "%d:%d,%d:%d,%d:%d", */
		    sprintf(buf, "%d:%d,%d:%d",
			    __in_csize[1], __out_csize[1],
			    __in_csize[2]-1, __out_csize[2]);
/*			    __in_csize[3]-1, __out_csize[3]);  */
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
printf("END LC_CTYPE\n\n\n");
}
	
	/*
	** print buildlang script (LC_COLLATE section)
	*/
	if (catinfo[LC_COLLATE].size != 0) {
	printf("%c%c%c%c%c%c%c%c\n",cc,cc,cc,cc,cc,cc,cc,cc);
	printf("%c LC_COLLATE category\n\n",cc);
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
	printf("END LC_COLLATE\n\n\n");
	}


	/*
	** print buildlang script (LC_MONETARY section)
	*/
	if (catinfo[LC_MONETARY].size != 0) {
	printf("%c%c%c%c%c%c%c%c\n",cc,cc,cc,cc,cc,cc,cc,cc);
	printf("%c LC_MONETARY category\n\n",cc);

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
	printf("END LC_MONETARY\n\n\n");
	}

	/*
	** print buildlang script (LC_NUMERIC section)
	*/
	if (catinfo[LC_NUMERIC].size != 0) {
	printf("%c%c%c%c%c%c%c%c\n",cc,cc,cc,cc,cc,cc,cc,cc);
	printf("%c LC_NUMERIC category\n\n",cc);

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
	printf("END LC_NUMERIC\n\n\n");
	}

	/*
	** print buildlang script (LC_TIME section)
	*/
	if (catinfo[LC_TIME].size != 0) {
	printf("%c%c%c%c%c%c%c%c\n",cc,cc,cc,cc,cc,cc,cc,cc);
	printf("%c LC_TIME category\n\n",cc);

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
	printf("END LC_TIME\n\n\n");
	}

	/*
	** print buildlang script (LC_MESSAGES section)
	*/
	if (catinfo[LC_MESSAGES].size != 0) {
	printf("%c%c%c%c%c%c%c%c\n",cc,cc,cc,cc,cc,cc,cc,cc);
	printf("%c LC_MESSAGE category\n\n",cc);

	printf("LC_MESSAGES\n");
	if (*catinfo[LC_MESSAGES].mod_name != NULL)
		printf("modifier\t%s\n",
			(char *)mkstr(catinfo[LC_MESSAGES].mod_name));
		dmesg();
	for (i = N_CATEGORY; i < N_CATEGORY + lctable_head.mod_no; i++) {
		if (catinfo[i].catid == LC_MESSAGES) {
		    strcpy(name_mod, lctable_head.lang);
		    strcat(name_mod, "@");
		    strcat(name_mod, catinfo[i].mod_name);
		    setlocale(LC_MESSAGES, name_mod);
		    printf("\nmodifier\t%s\n",
			    (char *)mkstr(catinfo[i].mod_name));
		    dmesg();
		}
	}
	printf("END LC_MESSAGES\n\n\n");
	}

	/*
	** print buildlang script (LC_ALL section)
	*/
	if (catinfo[LC_ALL].size != 0) {
	printf("%c%c%c%c%c%c%c%c\n",cc,cc,cc,cc,cc,cc,cc,cc);
	printf("%c LC_ALL category\n\n",cc);

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
	printf("END LC_ALL\n\n\n");
	}

}

/* dall:
** dump LC_ALL buildlang section
*/
dall()
{  
       /* POSIX 11.2 addition if stmt */
/*	if (mkstr(__nl_info[YESSTR]) != NULL) 
	printf("yesstr\t%s\n", (char *)mkstr(__nl_info[YESSTR]));
        if (mkstr(__nl_info[NOSTR]) != NULL)
	printf("nostr\t%s\n", (char *)mkstr(__nl_info[NOSTR])); */
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


	cnt = 0;				/* upper */
	printf("upper\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _U) { /* was __ctype */
	      if(cnt != 0)
                printf(";");
	      pcode(i);
		cnt++;
		if ((cnt % 12) == 0) {
		    cnt = 0;
		    printf("; %c\n\t",lctable_head.escape_char);
		}
	    }
	}
	printf("\n\n");

	cnt = 0;				/* lower */
	printf("lower\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _L) { /* was __ctype */
	      if(cnt != 0)
              	printf(";");
	      pcode(i);
	      cnt++;
	      if ((cnt %12) == 0) {
		    cnt = 0;
		    printf("; %c\n\t",lctable_head.escape_char);
	      }
	    }
	}
	printf("\n\n");

	cnt = 0;				/* alpha */
	printf("alpha\t");
	for (i = 0; i < DCTYPE2; i++) {
	    if (__ctype2[i] & _A) { /* Revisit - ctype2 defn was __ctype2*/
	        if(cnt != 0)
                	printf(";");
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0) {
		    printf("; %c\n\t",lctable_head.escape_char);
		    cnt = 0;
		}
	    }
	}
	printf("\n\n");

	cnt = 0;				/* digit */
	printf("digit\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _N) {
	        if(cnt != 0)
                	printf(";");
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0) {
		    printf("; %c\n\t",lctable_head.escape_char);
		    cnt = 0;
		}
	    }
	}
	printf("\n\n");

	cnt = 0;				/* space */
	printf("space\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _S) {
	        if(cnt != 0)
                	printf(";");
	        pcode(i);
		/* printf("0x%x ", i); */
		cnt++;
		if ((cnt %12) == 0) {
		    printf("; %c\n\t",lctable_head.escape_char);
		    cnt = 0;
		}
	    }
	}
	printf("\n\n");

	cnt = 0;				/* cntrl */
	printf("cntrl\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _C) {
	        if(cnt != 0)
                	printf(";");
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0) {
		    printf("; %c\n\t",lctable_head.escape_char);
		    cnt = 0;
		}
	    }
	}
	printf("\n\n");



	cnt = 0;				/* punct */
	printf("punct\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _P) {
	        if(cnt != 0)
                	printf(";");
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0) {
		    printf("; %c\n\t",lctable_head.escape_char);
		    cnt = 0;
		}
	    }
	}
	printf("\n\n");


	cnt = 0;				/* graph */
	printf("graph\t");
	for (i = 0; i < DCTYPE2; i++) {
	    if (__ctype2[i] & _G) { /* Revisit ctype2 */
	        if(cnt != 0)
                	printf(";");
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0) {
		    printf("; %c\n\t",lctable_head.escape_char);
		    cnt = 0;
		}
	    }
	}
	printf("\n\n");


	cnt = 0;				/* print */
	printf("print\t");
	for (i = 0; i < DCTYPE2; i++) {
	    if (__ctype2[i] & _PR) { /* Revisit ctype2 */
	        if(cnt != 0)
                	printf(";");
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0) {
		    printf("; %c\n\t",lctable_head.escape_char);
		    cnt = 0;
		}
	    }
	}
	printf("\n\n");


	cnt = 0;				/* xdigit */
	printf("xdigit\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _X) {
	        if(cnt != 0)
                	printf(";");
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0) {
		    printf("; %c\n\t",lctable_head.escape_char);
		    cnt = 0;
		}
	    }
	}
	printf("\n\n");

	cnt = 0;				/* blank */
	printf("blank\t");
	for (i = 0; i < DCTYPESIZ; i++) {
	    if (__ctype[i] & _B) {
	        if(cnt != 0)
                    printf(";");
	        pcode(i);
		cnt++;
		if ((cnt %12) == 0) {
		    printf("; %c\n\t",lctable_head.escape_char);
		    cnt = 0;
		}
	    }
	}
	printf("\n\n");



	if (__nl_char_size > 1) {
		cnt = 0;			/* first */
		printf("first\t");
		for (i = 0; i < DCTYPESIZ; i++) {
/*		    if (_1kanji[i]) {  */
		    if (FIRSTof2(i)) {
	      		if(cnt != 0)
                	     printf(";");
	        	pcode(i);
			cnt++;
			if ((cnt %12) == 0) {
		    	    printf("; %c\n\t",lctable_head.escape_char);
			    cnt = 0;
			}
		    }
		}
		printf("\n\n");

		cnt = 0;			/* second */
		printf("second\t");
		for (i = 0; i < DCTYPESIZ; i++) {
/*		    if (_2kanji[i]) { /* was _2kanji[i] */  
		    if (SECof2(i)) { /* was _2kanji[i] */
	      	    	if(cnt != 0)
                    	     printf(";");
	        	pcode(i);
			cnt++;
			if ((cnt %12) == 0) {
		    	    printf("; %c\n\t",lctable_head.escape_char);
			    cnt = 0;
			}
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

	    cnt = 0;				/* toupper */
	    printf("toupper\t");
	    for (i = 0; i < DSHIFTSIZ; i++) {
	      if (_toupper(i) != i) {
		        printf("(");
		        pcode(i);
			printf(",");
		        pcode(_toupper(i)); /* was upshift */
			printf(")");
			if ( i != (DSHIFTSIZ-1) )
				printf(";");
			cnt++;
			if ((cnt %4) == 0)
			    printf(" %c\n\t",lctable_head.escape_char);
	      }
	    }
	    printf("\n\n");
	    cnt = 0;				/* tolower */
	    printf("tolower\t");
	    for (i = 0; i < DSHIFTSIZ; i++) {
	      if (_tolower(i) != i) {
		        printf("(");
		        pcode(i);
			printf(",");
		        pcode(_tolower(i));  /* was downshift */
		        printf(")");
			if( i != (DSHIFTSIZ-1) )
			  printf(";");
			cnt++;
			if ((cnt %4) == 0)
			    printf(" %c\n\t",lctable_head.escape_char);
	       }
	    }
	    printf("\n\n");

}

/* dmntry:
** dump LC_MONETARY buildlang section
*/
dmntry()
{
extern unsigned char *unstring();
	printf("int_curr_symbol\t%s\n", (char *)mkstr(_lconv->int_curr_symbol));
	printf("currency_symbol\t%s\n", (char *)mkstr(_lconv->currency_symbol));
	printf("mon_decimal_point\t%s\n",
		(char *)mkstr(_lconv->mon_decimal_point));
	printf("mon_thousands_sep\t%s\n",
		(char *)mkstr(_lconv->mon_thousands_sep));
	/* Posix 11.2
	 * take the grouping string of form "\00\03" and turn it to 0;3
	 * by using unstring function
	 */
	printf("mon_grouping\t%s\n", unstring(_lconv->mon_grouping));
	printf("positive_sign\t%s\n", (char *)mkstr(_lconv->positive_sign));
	printf("negative_sign\t%s\n", (char *)mkstr(_lconv->negative_sign));

	if (_lconv->int_frac_digits == CHAR_MAX)
		printf("int_frac_digits\t-1\n");
	else
		printf("int_frac_digits\t%d\n", _lconv->int_frac_digits);
	if (_lconv->frac_digits == CHAR_MAX)
		printf("frac_digits\t-1\n");
	else
		printf("frac_digits\t%d\n", _lconv->frac_digits);
	if (_lconv->p_cs_precedes == CHAR_MAX)
		printf("p_cs_precedes\t-1\n");
	else
		printf("p_cs_precedes\t%d\n", _lconv->p_cs_precedes);
	if (_lconv->p_sep_by_space == CHAR_MAX)
		printf("p_sep_by_space\t-1\n");
	else
		printf("p_sep_by_space\t%d\n", _lconv->p_sep_by_space);
	if (_lconv->n_cs_precedes == CHAR_MAX)
		printf("n_cs_precedes\t-1\n");
	else
		printf("n_cs_precedes\t%d\n", _lconv->n_cs_precedes);
	if (_lconv->n_sep_by_space == CHAR_MAX)
		printf("n_sep_by_space\t-1\n");
	else
		printf("n_sep_by_space\t%d\n", _lconv->n_sep_by_space);
	if (_lconv->p_sign_posn == CHAR_MAX)
		printf("p_sign_posn\t-1\n");
	else
		printf("p_sign_posn\t%d\n", _lconv->p_sign_posn);
	if (_lconv->n_sign_posn == CHAR_MAX)
		printf("n_sign_posn\t-1\n");
	else
		printf("n_sign_posn\t%d\n", _lconv->n_sign_posn);

	printf("crncystr\t%s\n", (char *)mkstr(__nl_info[CRNCYSTR]));
}


/* dnmrc:
** dump LC_NUMERIC buildlang section
*/
dnmrc()
{
	printf("decimal_point\t%s\n", (char *)mkstr(_lconv->decimal_point));
	printf("thousands_sep\t%s\n", (char *)mkstr(_lconv->thousands_sep));
	printf("grouping\t%s\n", unstring(_lconv->grouping)); /*Posix 11.2*/

	printf("alt_digit\t%s\n", (char *)mkstr(__nl_info[ALT_DIGIT]));
}

/* dtime:
** dump LC_TIME buildlang section
*/
dtime()
{
	int i;
	unsigned char buf[MAX_INFO_MSGS+20];
	int day_loop;
	int mon_loop;

	printf("d_t_fmt\t%s\n", (char *)mkstr(__nl_info[D_T_FMT]));
	printf("d_fmt\t%s\n", (char *)mkstr(__nl_info[D_FMT]));
	printf("t_fmt\t%s\n", (char *)mkstr(__nl_info[T_FMT]));

	/* Added loop in accordance with Posix 11.2 design */
	printf("day");
	for (day_loop=DAY_1; day_loop <= DAY_7 ; day_loop++)
	       {
		 if (day_loop != DAY_7)
		     printf("\t%s; %c\n",(char *)mkstr(__nl_info[day_loop]),lctable_head.escape_char);
		 else
		     printf("\t%s \n",(char *)mkstr(__nl_info[day_loop]));

	       }
	printf("abday");
	for (day_loop=ABDAY_1; day_loop <= ABDAY_7 ; day_loop++)
	       {
		 if (day_loop != ABDAY_7)
		    printf("\t%s; %c\n", (char *)mkstr(__nl_info[day_loop]),lctable_head.escape_char);
		 else
		   printf("\t%s \n",(char *)mkstr(__nl_info[day_loop]));
	       }
	printf("mon");
	for (mon_loop=MON_1; mon_loop <= MON_12 ; mon_loop++)
	       {
		 if (mon_loop != MON_12)
		   printf("\t%s; %c\n", (char *)mkstr(__nl_info[mon_loop]),lctable_head.escape_char);
		 else
		   printf("\t%s \n", (char *)mkstr(__nl_info[mon_loop]));
	       }
	printf("abmon");
	for (mon_loop=ABMON_1; mon_loop <= ABMON_12 ; mon_loop++)
	       {
		 if (mon_loop != ABMON_12)
		   printf("\t%s; %c\n", (char *)mkstr(__nl_info[mon_loop]),lctable_head.escape_char);
		 else
		   printf("\t%s \n",(char *)mkstr(__nl_info[mon_loop]));
		}
	       /* Revisit here: am pm may have to change like the above
		  code
		*/
	printf("am_pm\t%s;", (char *)mkstr(__nl_info[AM_STR]));
	printf("%s\n", (char *)mkstr(__nl_info[PM_STR]));

	printf("year_unit\t%s\n", (char *)mkstr(__nl_info[YEAR_UNIT]));
	printf("mon_unit\t%s\n", (char *)mkstr(__nl_info[MON_UNIT]));
	printf("day_unit\t%s\n", (char *)mkstr(__nl_info[DAY_UNIT]));
	printf("hour_unit\t%s\n", (char *)mkstr(__nl_info[HOUR_UNIT]));
	printf("min_unit\t%s\n", (char *)mkstr(__nl_info[MIN_UNIT]));
	printf("sec_unit\t%s\n", (char *)mkstr(__nl_info[SEC_UNIT]));

	printf("era_d_fmt\t%s\n", (char *)mkstr(__nl_info[ERA_FMT]));
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
	/* Posix 11.2 */
	 printf("t_fmt_ampm\t%s\n",(char *)mkstr(__nl_info[T_FMT_AMPM])); 

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
		printf("'%c'", i);
	else {
		switch (pflag) {
			case CFLAG:
				if (i == 0 || i == '\n') /* null or newline */
					printf("0x%x", i);
				else
					printf("'%c'", i);
				break;
			case DFLAG:
				printf("%cd%d",lctable_head.escape_char, i);
				break;
			case OFLAG:
				printf("%c%o",lctable_head.escape_char, i);
				break;
			case XFLAG:
				printf("%cx%x",lctable_head.escape_char, i);
				break;
		}
	}
}


/* unstring function new in Posix 11.2*/
unsigned char *
unstring (buf)
unsigned char *buf;
{
	static unsigned char result[MAX_INFO_MSGS+20];
	int i;

	for (i=0;i<strlen(buf); i++) 
            if ( *(mntry_tab[MON_GROUPING]+i) != CHAR_MAX )
              (void) sprintf(result, "%d;", buf[i]);
            else
              (void) sprintf(result, "-1;");

	result[strlen(result)-1]='\0';
	return(result);
}		


void
dmesg()
  {
    printf("yesexpr\t%s\n", (char *)mkstr(__nl_info[YESEXPR]));
    printf("noexpr\t%s\n", (char *)mkstr(__nl_info[NOEXPR]));
    printf("yesstr\t%s\n", (char *)mkstr(__nl_info[YESSTR]));
    printf("nostr\t%s\n", (char *)mkstr(__nl_info[NOSTR]));
    
  }
