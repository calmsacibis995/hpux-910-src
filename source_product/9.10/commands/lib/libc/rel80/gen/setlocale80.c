/* @(#) $Revision: 70.1 $ */
/* LINTLIBRARY */
/*
 *      setlocale()
 */

#ifdef _NAMESPACE_CLEAN
#define atoi    _atoi
#define memcpy  _memcpy
#define setlocale _setlocale
#define _1kanji __1kanji
#define _2kanji __2kanji
#define getenv  _getenv
#define close   _close
#define lseek   _lseek
#define open    _open
#define read    _read
#define strcpy  _strcpy
#define strncpy _strncpy
#define strchr  _strchr
#define strcmp  _strcmp
#define strcat  _strcat
#define strlen  _strlen
#define _ctype __C_ctype
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "setlocale.h"
#include "locale.h"
#include <nl_ctype.h>
#include "langinfo.h"



#define set(var,value)		var |= (1<<value)	/* set a bit*/
#define set1(var,value)		var |= value		/* set a value */
#define is_set(var,value) 	(var & (1<<value)) 	/* check a bit */

#define HDR_SIZE   sizeof(struct table_header) 
#define ERROR 		-1
#define SUCCESS		1
#define FAILED		0
#define CHECKSUM 	15   /* this should be N_CATEGORY+N_CATEGORY-1+...+0 */
		      	     /* used for checking category value of save/restore
			 	calls	           			      */
#define MAXLOPTLEN 	16

#define _TBL_NAME	"/locale.def"

static unsigned char *LANG_name;		/* locale name associated with $LANG */
static unsigned char *msgindex[_NL_MAX_MSG+1];	/* array to hold pointers to nl_langinfo strings */

/* flags */
static short int loaded;     /* true if category is loaded */
static short int badlang;    /* true if a badlang was found */
static short int toload;     /* contains which categories are to be loaded */
static int mb_lang;	     /* true if multibyte language active */

char *getenv();
char *strchr();
char *memcpy();

/* 
 * langmap holds the language name associated with each category. 
 * It is loaded only after the setlocale has been determined to 
 * be successful 
 */
   
unsigned char __langmap[N_CATEGORY][LC_NAME_SIZE]=
						{
						 "C",
						 "C",
						 "C",
						 "C",
 						 "C",
						 "C"
						 };
/*
 * tmp_langmap holds the language name value for each category temporarily -
 * during the execution of setlocale.  If the call is eventually succesfull
 * these values will be copied into langmap
 */
static unsigned char tmp_langmap[N_CATEGORY][LC_NAME_SIZE];

/* 
 * modify value and temporary modifier value arrays.  These follow the
 * same principal as langmap and tmp_langmap.
 */
unsigned char __modmap[N_CATEGORY][MOD_NAME_SIZE+1];
static unsigned char tmp_modmap[N_CATEGORY][MOD_NAME_SIZE+1];

/*
 * error values will be stored in __error_store.  Used by getlocale().
 *
 */
unsigned char __error_store[N_CATEGORY][SL_NAME_SIZE+1];

/* 
 * table is where locale.def header data is read.  There is room for
 * up to 15 category and modifiers. 
 */
static unsigned char table[512];		/* allows for up to 15 category and modifiers */
static unsigned char data_ptr0[_LC_ALL_SIZE];
static unsigned char data_ptr1[_LC_COLLATE_SIZE];
static unsigned char data_ptr2[_LC_CTYPE_SIZE];
static unsigned char data_ptr3[_LC_MONETARY_SIZE];
static unsigned char data_ptr4[_LC_NUMERIC_SIZE];
static unsigned char data_ptr5[_LC_TIME_SIZE];
unsigned char *__errptr=data_ptr2;	/* used by errlocale only */

static unsigned char *tableptr=table;
static int fd;
static unsigned int current_lang;
static int header_size;

static void check_category();
static void load();
static int openlang();
static void default_env();
static int category_load();
static int restore();
static unsigned char *query();
static void set_error_struct();


unsigned char *__category_name[] = {
	(unsigned char *)"LANG",				/* no $LC_ALL */
	(unsigned char *)"LC_COLLATE",
	(unsigned char *)"LC_CTYPE",
	(unsigned char *)"LC_MONETARY",
	(unsigned char *)"LC_NUMERIC",
	(unsigned char *)"LC_TIME",
};



extern unsigned char _ctype[];
extern unsigned char __C_upshift[];
extern unsigned char __C_downshift[];
#ifdef EUC
extern unsigned char __C_e_cset[];
extern unsigned char __C_ein_csize[];
extern unsigned char __C_eout_csize[];
#endif /* EUC */

#ifdef _NAMESPACE_CLEAN
#undef setlocale
#pragma _HP_SECONDARY_DEF _setlocale setlocale
#define setlocale _setlocale
#endif /* _NAMESPACE_CLEAN */

char 
*setlocale(category,locale)
unsigned int category;
const unsigned char *locale;
{
	register unsigned int i;

	toload=0;				
	mb_lang=0;
	badlang=0;

	/* initialize to no errors */

	for (i=0; i<N_CATEGORY; i++) {
		__error_store[i][0]='\0';
		tmp_modmap[i][0]='\0';
	}

	if (category >= N_CATEGORY)		/* check for invalid category number */
		return(NULL);
	if (locale==0)				/* query operation */
		return((char *)query(category));
	if (*locale=='/')  {			/* if locale begins with a slash it is a restore operation */
		if (!(restore(locale+1,category))) {
			set_error_struct();     /* this is questionable */
			return(NULL) ;
		}
	}
	else {
		if (*locale=='\0') {		/* query users environment */

						/* establish the LANG name */
			if ((LANG_name=(unsigned char *)getenv("LANG")) == NULL)
				LANG_name = (unsigned char *)"C";
			if (*LANG_name=='\0') 
				LANG_name = (unsigned char *)"C";

			if (category==LC_ALL) {
				check_category(LC_ALL,LANG_name);
				for (i=1; i<N_CATEGORY; i++) 
					check_category(i,(unsigned char *)getenv(__category_name[i]));
			}
			else  
				check_category(category,(unsigned char *)getenv(__category_name[category]));	
		} 
		else {				/* hard coded locale name */
			if (category==LC_ALL) 
				for (i=0; i<N_CATEGORY; i++)
					check_category(i,locale);
			else
				check_category(category,locale);
		}
	}
	if ((toload ))				/* this is true when any category is different then what is already set */
		load();
	if (!(badlang)) {			/* if no errors */
		for (i=0; i<N_CATEGORY;i++)
	        	if (is_set(loaded,i)) {	/* set up langmap */
			   (void)strcpy(__langmap[i],tmp_langmap[i]);  /* successful load */
						/* set up modifier map */
			   (void)strcpy(__modmap[i],tmp_modmap[i]);
		    	}
		return((char *)query(category));	/* return with a query string */
	}
	else {					/* an error occurred */
		set_error_struct();

		for (i=0; i<N_CATEGORY;i++) {	/* load good values into tmp_langmap */
		     (void)strcpy(tmp_langmap[i],__langmap[i]);
		     (void)strcpy(tmp_modmap[i],__modmap[i]);
		}  

		/* only restore categories which have been loaded */

		if (loaded) {
		   toload=loaded;
		   load();
		}
		return(NULL);
	}
}
	   
		
/* 
 * determine the value for each category and if that value is different
 * than what it is already set to.  If it is different then set the toload
 * flag 
 */
static void
check_category(category,locale)
unsigned int category;
unsigned char *locale;
{	 
	register unsigned char *token;
	register unsigned int modflag=0;

	if ((locale == NULL)  || (*locale== '\0'))
		locale=LANG_name;
	else 
		if ((token=(unsigned char *)strchr(locale,'@')) != NULL) {
			modflag++;
			*token++='\0';
			(void)strncpy(tmp_modmap[category],token,MOD_NAME_SIZE+1);
		}
	(void)strncpy(tmp_langmap[category],locale,LC_NAME_SIZE-1);
	tmp_langmap[category][LC_NAME_SIZE-1] = '\0';
	if (modflag)
		*--token='@';

	/* if it different than what is already set, mark toload flag */

	if ((strcmp(__langmap[category],tmp_langmap[category])) ||
		   (strcmp(tmp_modmap[category],__modmap[category])))
			set(toload,category);
}


/* 
 * load will go through each category that needs to be loaded and
 * group them such that all categories for a particular locale 
 * are loaded with one open call.
 * 
 * Load will call category load, which actually does the loading.
 */
static void
load()
{

	register int i,j;
	register short lang[N_CATEGORY];	  
	static first_load = 1;

	/*
	 * Prior to the first load, __nl_info (which is what nl_langinfo()
	 * uses) points to the array of default strings __C_langinfo.  Now
	 * that we're going to do a load and potentially have some langinfo
	 * strings remain the C defaults while others are changed to a new
	 * locale, we need to copy all the default pointers to the msgindex
	 * array and reference only it from now on.  Except that __C_langinfo
	 * is still used to re-initialized msgindex elements as desired to
	 * the C locale defaults.
	 */
	if (first_load) {
		(void)memcpy(msgindex, __C_langinfo, (_NL_MAX_MSG+1) * sizeof(char *));
		__nl_info = msgindex;
		first_load--;
	}

	badlang=0;
	loaded=0;

/* lang[i] will be set if that language associated with the category should
   be loaded */

	for (i=0; i < N_CATEGORY; i++)
	{
	   lang[i]=0;

	   /* if the category is to be loaded */

	   if (is_set(toload,i) && (!(is_set(loaded,i)))) {

		/* look for other categories to be loaded with same language 
		   and mark them in the lang array */

		/* fix to only look for to load categories */

		for (j=i; j<N_CATEGORY; j++)
			if (strcmp(tmp_langmap[i],tmp_langmap[j]) == 0)
				set(lang[i],j);

		if (!(strcmp(tmp_langmap[i],"C"))) {       /* check for C */
			default_env(lang[i],99);
			set1(loaded,lang[i]);
			continue;
		}

		if (!(strcmp(tmp_langmap[i],"n-computer"))) {  /* check for n-computer */
			default_env(lang[i],0);
			set1(loaded,lang[i]);
			continue;
		}
		else {					/* otherwise load lang */

		   if (openlang(i) == ERROR) 		/* see if language is available */
			set1(badlang,lang[i]);	/* if it is, mark it as bad */
		   else {
 		        for (j=i ; j<N_CATEGORY ;j++)  
		             if (is_set(lang[i],j)) {
			         if (category_load(j) != SUCCESS)  {
					set(badlang,j);
					break;
				 }
				 else
					set(loaded,j);
			    }
		   }
		   (void)close(fd);
		}
	    }
	}
}


static int
openlang(catid)
int catid;
{
		register unsigned char fnbuf[LC_NAME_SIZE + 25];
		register unsigned char *token;  
		register int total_cats;
		register int total_cats_size;
		struct table_header *table_hdr;


		(void) strcpy(fnbuf,NLSDIR);
		(void) strcat(fnbuf,tmp_langmap[catid]);
		if ((token=(unsigned char *)strchr(fnbuf,'_')) != NULL) 
			*token='/';
		if ((token=(unsigned char *)strchr(fnbuf,'.')) != NULL)
			*token='/';
		(void) strcat(fnbuf,_TBL_NAME);
		if ((fd=open(fnbuf,O_RDONLY)) < 0   
			|| read(fd, (unsigned char *)tableptr,HDR_SIZE) < HDR_SIZE) 
			return(ERROR);
		table_hdr=(struct table_header *)tableptr;
		current_lang=table_hdr->nl_langid;
		total_cats=table_hdr->cat_no + table_hdr->mod_no;
		header_size=table_hdr->size;
		mb_lang=table_hdr->codeset;
		total_cats_size=total_cats*sizeof(struct catinfotype);
		if (read(fd, (unsigned char *)tableptr,total_cats_size) < total_cats_size)
			return(ERROR);
		return(0);		/* successful load */
}
				
/* 
 * This routine sets up the default "C" and "n-computer" environments.
 * The langtype parameter determine which environment.
 */
static void
default_env(cats,langtype)
int cats;
unsigned int langtype;		/* 99 = "C"; 0 = "n-computer" */
{
	register int i;

	for (i=0; i<N_CATEGORY; i++) {
	   if (is_set(cats,i)) {
		switch(i) {
	case LC_ALL:
		__nl_langid[LC_ALL]=	langtype;
		msgindex[YESSTR]=	__C_langinfo[YESSTR];
		msgindex[NOSTR]=	__C_langinfo[NOSTR];
		msgindex[DIRECTION]=	__C_langinfo[DIRECTION];
		_nl_direct= 		NL_LTR;
		_nl_context=		0;
		break;

	case LC_COLLATE:
		__nl_langid[LC_COLLATE]= langtype;
		_seqtab=	0;
		_pritab=	0;
		_nl_map21=	0;
		_nl_onlyseq=	1;
		_nl_collate_on= mb_lang;
		_nl_mb_collate= mb_lang;
		_tab12=		0;
		_tab21=		0;
		break;

	case LC_CTYPE:
		__nl_langid[LC_CTYPE]=langtype;
		__ctype= 	&_ctype[1];
		_1kanji=	&_ctype[1+128+1];
		_2kanji=	&_ctype[1+128+1];
		_upshift=	__C_upshift;
		_downshift=	__C_downshift;
#ifdef EUC
		__e_cset=	__C_e_cset;
		__ein_csize=	__C_ein_csize;
		__eout_csize=	__C_ein_csize;
#endif /* EUC */
		_sh_low=	0;
		_sh_high=	0377;
		msgindex[ALT_PUNCT]=	_nl_punct_alt=	__C_langinfo[ALT_PUNCT];
		_nl_space_alt=	32;
		msgindex[BYTES_CHAR]=	__C_langinfo[BYTES_CHAR];
		__nl_char_size=	1;
#ifdef EUC
		__nl_code_scheme=	mb_lang;
		__cs_SBYTE=	1;
		__cs_HP15=	0;
		__cs_EUC=	0;
		__in_csize[0]=	1;
		__in_csize[1]=	0;
		__in_csize[2]=	0;
		__in_csize[3]=	0;
		__out_csize[0]=	1;
		__out_csize[1]=	0;
		__out_csize[2]=	0;
		__out_csize[3]=	0;
#endif /* EUC */
		break;

	case LC_MONETARY:
		__nl_langid[LC_MONETARY]=langtype;
		msgindex[CRNCYSTR]=(langtype==0 ? (unsigned char *)"-$" : __C_langinfo[CRNCYSTR]); /* defined only if n-computer */
		_lconv->int_curr_symbol="";
	 	_lconv->currency_symbol="";
	 	_lconv->mon_decimal_point="";
		_lconv->mon_thousands_sep="";
	 	_lconv->mon_grouping=""; 
	 	_lconv->positive_sign="";
	 	_lconv->negative_sign="";
	 	_lconv->int_frac_digits=CHAR_MAX;
	 	_lconv->frac_digits=CHAR_MAX;
	 	_lconv->p_cs_precedes=CHAR_MAX;
	 	_lconv->p_sep_by_space=CHAR_MAX;
	 	_lconv->n_cs_precedes=CHAR_MAX;
	 	_lconv->n_sep_by_space=CHAR_MAX;
	 	_lconv->p_sign_posn=CHAR_MAX;
	 	_lconv->n_sign_posn=CHAR_MAX;
		break;

	case LC_NUMERIC:
		__nl_langid[LC_NUMERIC]=langtype;
		_lconv->decimal_point=".";
		_lconv->thousands_sep="";
		_lconv->grouping="";
		msgindex[RADIXCHAR]=	__C_langinfo[RADIXCHAR];
		msgindex[THOUSEP]=(langtype==0 ? (unsigned char *)"," : __C_langinfo[THOUSEP]);  /* defined only in "n-computer" */
		msgindex[ALT_DIGIT]=	__C_langinfo[ALT_DIGIT];
		_nl_radix=	'.';
		_nl_dgt_alt= (unsigned char *)"";
		_nl_mode = NL_NONLATIN;
		_nl_order = NL_KEY;
		_nl_outdigit = NL_ASCII;
		break;

	case LC_TIME:
		__nl_langid[LC_TIME]=langtype;

		/* depends upon item numbers 0 thru ABMON_12 all belonging to the LC_TIME category */
		(void)memcpy(msgindex, __C_langinfo, (ABMON_12+1) * sizeof(char *));

		/* depends upon item numbers AM_STR thru ERA_FMT all belonging to the LC_TIME category */
		(void)memcpy(&msgindex[AM_STR], &__C_langinfo[AM_STR], (ERA_FMT-AM_STR+1) * sizeof(char *));

		_nl_era[0]=0;
		break;
	   }
	}
    }
}
				
/*
 * This routine actually reads the data from the locale.def file and
 * initializes the setlocale variables.
 */

static int
category_load(catid)
int catid;
{
	struct col_header *col_hdr;
	struct ctype_header *ctype_hdr;
	struct numeric_header *numeric_hdr;
	struct time_header *time_hdr;
	struct monetary_header *monetary_hdr;
	/* struct msg_header *msg_hdr; */
	struct all_header *all_hdr;
	register unsigned char loptbuf[MAXLOPTLEN+1];
	register unsigned char *token;
	register unsigned char *data_ptr;
	struct catinfotype *cat_hdr_ptr;
	register int n;
	register int era_count;
	struct _era_data *era_ptr;

	cat_hdr_ptr = (struct catinfotype *)(tableptr + catid*sizeof(struct catinfotype));

	/* if the category is not defined for the language then default to 
		"C" */

	if (cat_hdr_ptr->size== 0) {
		default_env(1<<catid,current_lang);
		return(SUCCESS);
	}

	/* step through category headers looking for the appropriate modifier */

	if (tmp_modmap[catid][0] != NULL) {
	    while (cat_hdr_ptr != NULL) {
	       if (!(n=strcmp(tmp_modmap[catid],cat_hdr_ptr->mod_name)))  
		    break;
	       if (cat_hdr_ptr->mod_addr != 0)
	       	   cat_hdr_ptr = (struct catinfotype *)(table+cat_hdr_ptr->mod_addr-sizeof(struct table_header));
	       else
		   return(ERROR);
            }	 
	}


	(void)lseek(fd,cat_hdr_ptr->address+header_size,0);
	
	/* set the langid for the category */

	__nl_langid[catid]=current_lang;
		
	switch(catid) {
	case LC_ALL:
		if (read(fd, data_ptr0, cat_hdr_ptr->size) < cat_hdr_ptr->size)
			return(ERROR);
		data_ptr=data_ptr0;
		all_hdr=(struct all_header *)data_ptr;

		/* all data in LC_ALL category is miscellaneous data that
		   does not fit in any other defined category */

		msgindex[YESSTR]=(unsigned char *)data_ptr+all_hdr->yes_addr;
		msgindex[NOSTR]=(unsigned char *)data_ptr+all_hdr->no_addr;
		msgindex[DIRECTION]=(unsigned char *)data_ptr+all_hdr->direct_addr;
		_nl_direct=atoi(msgindex[DIRECTION]) ? NL_RTL : NL_LTR;
		_nl_context=atoi((char *)data_ptr+all_hdr->context_addr);

		/* Process LANGOPTS environment variable */

		(void)strncpy(loptbuf,getenv("LANGOPTS"),MAXLOPTLEN);
		if (*loptbuf == 'l')
			_nl_mode = NL_LATIN;
		if ((token=(unsigned char *)strchr(loptbuf,'_')) != NULL)
			if (*++token == 's')
				_nl_order = NL_SCREEN;
		if ((token=(unsigned char *)strchr(loptbuf,'.')) != NULL)
			if (*++token == 'l')
				_nl_outdigit = NL_ALT; 
		break;

	/* load in collation data */

	case LC_COLLATE:
		if (read(fd, data_ptr1, cat_hdr_ptr->size) < cat_hdr_ptr->size)
			return(ERROR);
		data_ptr=data_ptr1;
		col_hdr=(struct col_header *)data_ptr;
		_seqtab=(unsigned char *)data_ptr+col_hdr->seqtab_addr;
		_pritab=(unsigned char *)data_ptr+col_hdr->pritab_addr;
		_nl_map21=col_hdr->nl_map21;
	        _nl_onlyseq=col_hdr->nl_onlyseq;
		_tab12=(struct col_12tab *)(data_ptr+col_hdr->tab12_addr);
		_tab21=(struct col_21tab *)(data_ptr+col_hdr->tab21_addr);
		_nl_collate_on=1;
		_nl_mb_collate=0;
		break;

	/* load in ctype data */

	case LC_CTYPE:
		if (read(fd, data_ptr2, cat_hdr_ptr->size) < cat_hdr_ptr->size)
			return(ERROR);
		data_ptr=data_ptr2;
		ctype_hdr=(struct ctype_header *)data_ptr;
	 	__ctype=(unsigned char *)data_ptr+ctype_hdr->_ctype_addr+1;
	 	_1kanji=(unsigned char *)data_ptr+ctype_hdr->kanji1_addr+1;
	 	_2kanji=(unsigned char *)data_ptr+ctype_hdr->kanji2_addr+1;
	 	_upshift=(unsigned char *)data_ptr+ctype_hdr->upshift_addr;
	 	_downshift=(unsigned char *)data_ptr+ctype_hdr->downshift_addr;
		_sh_low=ctype_hdr->_sh_low;
		_sh_high=ctype_hdr->_sh_high;
		msgindex[ALT_PUNCT]=(unsigned char *)data_ptr+ctype_hdr->alt_punct_addr;
		_nl_punct_alt=msgindex[ALT_PUNCT];
	 	msgindex[BYTES_CHAR]=(unsigned char *)data_ptr+ctype_hdr->byte_char_addr;
		__nl_char_size=atoi(msgindex[BYTES_CHAR]);
#ifdef EUC
		__nl_code_scheme=	mb_lang;
		if (mb_lang == 1) {
			__cs_HP15=	1;
			__cs_SBYTE=	0;
			__cs_EUC=	0;
		}
		else if (mb_lang == 2) {
			__cs_EUC=	1;
			__cs_SBYTE=	0;
			__cs_HP15=	0;
		}
		else {
			__cs_HP15=	0;
			__cs_EUC=	0;
			__cs_SBYTE=	1;
		}
		_nl_space_alt= (*_nl_punct_alt) ? CHARAT(_nl_punct_alt) : 32;
		__in_csize[0]=(unsigned char)(*(data_ptr+ctype_hdr->io_csize_addr));
		__in_csize[1]=(unsigned char)(*(data_ptr+ctype_hdr->io_csize_addr+1));
		__in_csize[2]=(unsigned char)(*(data_ptr+ctype_hdr->io_csize_addr+2));
		__in_csize[3]=(unsigned char)(*(data_ptr+ctype_hdr->io_csize_addr+3));
		__out_csize[0]=(unsigned char)(*(data_ptr+ctype_hdr->io_csize_addr+4));
		__out_csize[1]=(unsigned char)(*(data_ptr+ctype_hdr->io_csize_addr+5));
		__out_csize[2]=(unsigned char)(*(data_ptr+ctype_hdr->io_csize_addr+6));
		__out_csize[3]=(unsigned char)(*(data_ptr+ctype_hdr->io_csize_addr+7));
	 	__e_cset=(unsigned char *)data_ptr+ctype_hdr->e_cset_addr;
	 	__ein_csize=(unsigned char *)data_ptr+ctype_hdr->ein_csize_addr;
	 	__eout_csize=(unsigned char *)data_ptr+ctype_hdr->eout_csize_addr;
#endif /* EUC */
		break;

	/* load in monetary data */

	case LC_MONETARY:
		if (read(fd, data_ptr3, cat_hdr_ptr->size) < cat_hdr_ptr->size)
			return(ERROR);
		data_ptr=data_ptr3;
		monetary_hdr=(struct monetary_header *)data_ptr;
		_lconv->int_curr_symbol=(char *)(data_ptr+monetary_hdr->int_curr_symbol);
	 	_lconv->currency_symbol=(char *)(data_ptr+monetary_hdr->curr_symbol_lconv);
	 	_lconv->mon_decimal_point=(char *)(data_ptr+monetary_hdr->mon_decimal_point);
		_lconv->mon_thousands_sep=(char *)(data_ptr+monetary_hdr->mon_thousands_sep);
	 	_lconv->mon_grouping=(char *)(data_ptr+monetary_hdr->mon_grouping);
	 	_lconv->positive_sign=(char *)(data_ptr+monetary_hdr->positive_sign);
	 	_lconv->negative_sign=(char *)(data_ptr+monetary_hdr->negative_sign);
	 	_lconv->int_frac_digits=(unsigned char)(*(data_ptr+monetary_hdr->int_frac_digits));
	 	_lconv->frac_digits=(unsigned char)(*(data_ptr+monetary_hdr->frac_digits));
	 	_lconv->p_cs_precedes=(unsigned char)(*(data_ptr+monetary_hdr->p_cs_precedes));
	 	_lconv->p_sep_by_space=(unsigned char)(*(data_ptr+monetary_hdr->p_sep_by_space));
	 	_lconv->n_cs_precedes=(unsigned char)(*(data_ptr+monetary_hdr->n_cs_precedes));
	 	_lconv->n_sep_by_space=(unsigned char)(*(data_ptr+monetary_hdr->n_sep_by_space));
	 	_lconv->p_sign_posn=(unsigned char)(*(data_ptr+monetary_hdr->p_sign_posn));
	 	_lconv->n_sign_posn=(unsigned char)(*(data_ptr+monetary_hdr->n_sign_posn));
		msgindex[CRNCYSTR]=(unsigned char *)data_ptr+monetary_hdr->curr_symbol_li;
		break;

	/* load in numeric data */

	case LC_NUMERIC:
		if (read(fd, data_ptr4, cat_hdr_ptr->size) < cat_hdr_ptr->size)
			return(ERROR);
		data_ptr=data_ptr4;
		numeric_hdr=(struct numeric_header *)data_ptr;
	 	_lconv->decimal_point=(char *)(data_ptr+numeric_hdr->decimal_point);
	 	_lconv->thousands_sep=(char *)(data_ptr+numeric_hdr->thousands_sep);
	 	_lconv->grouping=(char *)(data_ptr+numeric_hdr->grouping);
	 	msgindex[RADIXCHAR]=(unsigned char *)_lconv->decimal_point;
		_nl_radix=CHARAT(msgindex[RADIXCHAR]);
		if (_nl_radix == 0)
			_nl_radix = '.';

	 	msgindex[THOUSEP]=(unsigned char *)_lconv->thousands_sep;
	 	msgindex[ALT_DIGIT]=(unsigned char *)data_ptr+numeric_hdr->alt_digit_addr;
		_nl_dgt_alt=msgindex[ALT_DIGIT];
		break;

	/* load in time data */

	case LC_TIME: 
		if (read(fd, data_ptr5, cat_hdr_ptr->size) < cat_hdr_ptr->size)
			return(ERROR);
		data_ptr=data_ptr5;
		time_hdr=(struct time_header *)data_ptr;
	 	msgindex[D_T_FMT]=(unsigned char *)data_ptr+time_hdr->d_t_fmt;
		msgindex[D_FMT]=(unsigned char *)data_ptr+time_hdr->d_fmt;
		msgindex[T_FMT]=(unsigned char *)data_ptr+time_hdr->t_fmt;   
	 	msgindex[AM_STR]=(unsigned char *)data_ptr+time_hdr->am_str;
	 	msgindex[PM_STR]=(unsigned char *)data_ptr+time_hdr->pm_str;
	 	msgindex[DAY_1]=(unsigned char *)data_ptr+time_hdr->day_1;
	 	msgindex[DAY_2]=(unsigned char *)data_ptr+time_hdr->day_2;
	 	msgindex[DAY_3]=(unsigned char *)data_ptr+time_hdr->day_3;
	 	msgindex[DAY_4]=(unsigned char *)data_ptr+time_hdr->day_4;
	 	msgindex[DAY_5]=(unsigned char *)data_ptr+time_hdr->day_5;
	 	msgindex[DAY_6]=(unsigned char *)data_ptr+time_hdr->day_6;
	 	msgindex[DAY_7]=(unsigned char *)data_ptr+time_hdr->day_7;
	 	msgindex[ABDAY_1]=(unsigned char *)data_ptr+time_hdr->abday_1;
	 	msgindex[ABDAY_2]=(unsigned char *)data_ptr+time_hdr->abday_2;
	 	msgindex[ABDAY_3]=(unsigned char *)data_ptr+time_hdr->abday_3;
	 	msgindex[ABDAY_4]=(unsigned char *)data_ptr+time_hdr->abday_4;
	 	msgindex[ABDAY_5]=(unsigned char *)data_ptr+time_hdr->abday_5;
	 	msgindex[ABDAY_6]=(unsigned char *)data_ptr+time_hdr->abday_6;
	 	msgindex[ABDAY_7]=(unsigned char *)data_ptr+time_hdr->abday_7;
	 	msgindex[MON_1]=(unsigned char *)data_ptr+time_hdr->mon_1;
	 	msgindex[MON_2]=(unsigned char *)data_ptr+time_hdr->mon_2;
	 	msgindex[MON_3]=(unsigned char *)data_ptr+time_hdr->mon_3;
	 	msgindex[MON_4]=(unsigned char *)data_ptr+time_hdr->mon_4;
	 	msgindex[MON_5]=(unsigned char *)data_ptr+time_hdr->mon_5;
	 	msgindex[MON_6]=(unsigned char *)data_ptr+time_hdr->mon_6;
	 	msgindex[MON_7]=(unsigned char *)data_ptr+time_hdr->mon_7;
	 	msgindex[MON_8]=(unsigned char *)data_ptr+time_hdr->mon_8;
	 	msgindex[MON_9]=(unsigned char *)data_ptr+time_hdr->mon_9;
	 	msgindex[MON_10]=(unsigned char *)data_ptr+time_hdr->mon_10;
	 	msgindex[MON_11]=(unsigned char *)data_ptr+time_hdr->mon_11;
	 	msgindex[MON_12]=(unsigned char *)data_ptr+time_hdr->mon_12;
	 	msgindex[ABMON_1]=(unsigned char *)data_ptr+time_hdr->abmon_1;
	 	msgindex[ABMON_2]=(unsigned char *)data_ptr+time_hdr->abmon_2;
	 	msgindex[ABMON_3]=(unsigned char *)data_ptr+time_hdr->abmon_3;
	 	msgindex[ABMON_4]=(unsigned char *)data_ptr+time_hdr->abmon_4;
	 	msgindex[ABMON_5]=(unsigned char *)data_ptr+time_hdr->abmon_5;
	 	msgindex[ABMON_6]=(unsigned char *)data_ptr+time_hdr->abmon_6;
	 	msgindex[ABMON_7]=(unsigned char *)data_ptr+time_hdr->abmon_7;
	 	msgindex[ABMON_8]=(unsigned char *)data_ptr+time_hdr->abmon_8;
	 	msgindex[ABMON_9]=(unsigned char *)data_ptr+time_hdr->abmon_9;
	 	msgindex[ABMON_10]=(unsigned char *)data_ptr+time_hdr->abmon_10;
	 	msgindex[ABMON_11]=(unsigned char *)data_ptr+time_hdr->abmon_11;
	 	msgindex[ABMON_12]=(unsigned char *)data_ptr+time_hdr->abmon_12;
		msgindex[YEAR_UNIT]=(unsigned char *)data_ptr+time_hdr->year_unit;
		msgindex[MON_UNIT]=(unsigned char *)data_ptr+time_hdr->mon_unit;
		msgindex[DAY_UNIT]=(unsigned char *)data_ptr+time_hdr->day_unit;
		msgindex[HOUR_UNIT]=(unsigned char *)data_ptr+time_hdr->hour_unit;
		msgindex[MIN_UNIT]=(unsigned char *)data_ptr+time_hdr->min_unit;
		msgindex[SEC_UNIT]=(unsigned char *)data_ptr+time_hdr->sec_unit;
		msgindex[ERA_FMT]=(unsigned char *)data_ptr+time_hdr->era_fmt;
		era_count=time_hdr->era_count;
		if (era_count > 0) {
			era_ptr=(struct _era_data *)(data_ptr+time_hdr->era_addr);	
			for (n=0; n<era_count; n++) {
				_nl_era[n] = era_ptr++;
				_nl_era[n]->name = (unsigned char *)data_ptr+time_hdr->era_names+(unsigned int)_nl_era[n]->name;
				_nl_era[n]->format = (unsigned char *)data_ptr+time_hdr->era_names+(unsigned int)_nl_era[n]->format;
			}
		}
		_nl_era[era_count] = 0;
		break;
	}
	return(SUCCESS);
}
	




static unsigned char localesave[LC_BUFSIZ];


/* 
 *	query/restore string looks like :
 *
 *	"/catid1:langname;catid2:langname;...;catidn:langname;/"
 *
 *	If restore category does not match category that was saved
 *	then error 
 *
 *	Note that LC_BUFSIZE = 2 + (3+SL_NAME_SIZE)*N_CATEGORY + 1
 *			       ^    ^              		 ^
 *			       |    |              		 |
 *           beginning and end /'s  |				null terminator
 *				  catid+':'+';'
 *
 */

static unsigned char  
*query(category)
unsigned int category;
{
	
	register unsigned char *l=localesave;
	register unsigned int end_category;
	register unsigned int i=0;
	
	end_category=(category == LC_ALL) ? N_CATEGORY-1 : category;
		
	*l++='/';
	for (i=category; i<= end_category; i++) {
		*l++=i+1;
		*l++=':';
		(void)strcpy(l,__langmap[i]);
		while (*l)
			l++;
		if (__modmap[i][0] != NULL) {
			*l++='@';
			(void)strcpy(l,__modmap[i]);
			while (*l)
				l++;
		}
		*l++=';';
	}
	*l++='/';
	*l=0;
	return(localesave);
}
			
	




static
restore(locale,category)
unsigned char *locale;
unsigned int category;
{

/*
 * Must check that the category is equivalent to that indicated by
 * the return string.  The checksum is used for this purpose  */


	unsigned char *l;
	unsigned short checksum=(category==LC_ALL ? CHECKSUM : category);
	unsigned int category_id;
	unsigned short check=0;

	while (*locale && *locale != '/')  {
		category_id = (*locale++)-1;
		if (*locale++ != ':')
			return(FAILED);
		else {
			l=locale;
			while (*locale && *locale != ';')
				locale++;
			if (*locale) {
			   *locale++=0;
			   check_category(category_id,l);
			   check+=category_id;
			}
			else
			   return(FAILED);
		}
	}
	if (*locale != '/')
	   return(FAILED);

	/* No error if categories match */

	return(check==checksum);
}


static void
set_error_struct()
{
	register int i;

	for (i=0; i<N_CATEGORY; i++) 
		if (is_set(badlang,i)) {
		            (void)strcpy(__error_store[i],tmp_langmap[i]);
		            if (tmp_modmap[i][0] != NULL) {
		   	        (void)strcat(__error_store[i],"@");
		   	        (void)strcat(__error_store[i],tmp_modmap[i]);
		            }
		}
}
