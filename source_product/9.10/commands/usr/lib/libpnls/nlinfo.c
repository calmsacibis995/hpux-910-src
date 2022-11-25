
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <memory.h>
#include <langinfo.h>
#include "nlinfo.h"
#include <nl_types.h>
#include <nl_ctype.h>
#include <stdlib.h>      

extern char	*idtolang();
extern char	*nl_langinfo();
extern int      _nl_errno;


/*
 *	macro definitions.
 *		these were put here to make the code more readable.
 *		NL_COPY - copy b to a where the sizeof b is known.
 *		NL_ACOPY - copy b to a where the sizeof of b must be given.
 */
#define NL_COPY(a, b)		memcpy((char *)a, (char *)b, sizeof(b))
#define NL_ACOPY(a, b, c)	memcpy((char *)a, (char *)b, (int)c)

/*
 *	for the pascal library, we do not have to worry about the null byte
 *		at the end of a string.
 */
#ifdef C_LIB
#define NL_STRCOPY(a, b)	NL_COPY(a, b); *((char *)a + sizeof(b)) = '\0'
#endif 
#ifndef C_LIB
#define NL_STRCOPY(a, b)	NL_COPY(a, b)
#endif 


/*
 *	convert nulls to blanks
 *		with some actions (putting together the months of the year)
 *		the individual items going into the array may be less than
 *		the full size (i.e. Feb\0 ). since the expecting program
 *		wants to see blank padded strings, we change Feb\0 to Feb .
 */
#define CONV_NULL_TO_BL(a, b, c)	for (b = a; b < a + sizeof(a); b++) \
					if (*b == '\0') \
						*b = c;

static struct l_info	*nRoot =  NULL;  /* root node */
static struct l_info	*currN =  NULL;  /* current node */




void nlinfo(itemnumber, itemvalue, langid, error)
short		itemnumber;
int		*itemvalue;
short		*langid;
unsigned short	error[];
{
	struct l_info		*getl_info();		/* routine to retrieve
							   l_info structure
							   node */
	short			*langSupp = NULL;	/* langs supported */
	long  time();
	struct tm *localtime();

	_nl_errno = 0;
	error[0] = error[1] = (short) 0;

	/*
	 *	WARNING:
	 *		If user wants one of these three options,
	 *		this functions will check and then return.
	 */
	if (itemnumber == L_SUPPORTED  ||  itemnumber == L_LANGS){
		if (langSupp == (short *)NULL)
			getlangs(&langSupp, error);
		if (error[0] == 0){
			if (itemnumber == L_SUPPORTED)
				islangsupp(*langid, *langSupp, langSupp, 
					(short *)itemvalue);
			else
				NL_ACOPY(itemvalue, (char *)langSupp, (*langSupp + 1) * 2);
		}
		return;
	} 
	else if (itemnumber == L_LANGID){

		_nl_errno = 0;
		getlangid((char *)itemvalue, langid);
		if (_nl_errno) error [0] = E_LNOTCONFIG;
		return;
	}
	else if (itemnumber == L_CURRLANG){
		if (nRoot)
			memcpy((char *)itemvalue, (char *)&currN->langid,
				sizeof(short));
		else
			*itemvalue = 0;
		return;
	}



	/*
	 *	if this is the first language to be called,
	 *		or if this langid does not match the
	 *		one that is current,
	 *		search the list.
	 */

        if (*langid == -1)  {
            error [0] = E_LNOTCONFIG;
	    return;
        }

	if ((nRoot == NULL)  ||  
	    (currN == NULL)  || 
	    (currN->langid != *langid )) {
		if ( (currN = getl_info(*langid)) == NULL){
			error [0] = E_LNOTCONFIG;
				   /* fatal error: */
			           /* nlinfo: getl_info returned NULL */
			return;
		}
                if (_nl_errno) {
		     /* Error in the custdat.cat message catalog */
		     error [0] = E_LNOTCONFIG;
		     return;
		} 
	}

	/*
	 *	let's see what the user wants
	 *	NOTE: 
	 *		the following code makes heavy use of macros
	 *		that were defined above.
	 *		this was done to make the code more readable.
	 *
	 *		NL_COPY(a, b)     - copy memory location b to a,
	 *				    for sizeof(b) bytes
	 *		NL_ACOPY(a, b, c) - copy memory location b to a,
	 *				    for c bytes
	 *		NL_STRCOPY(a, b)  - copy b to a with NL_COPY.
	 *				    if used in the c library,
	 *				    add a null terminator.
	 */
	switch (itemnumber){
		case L_CALFORM:
			NL_STRCOPY(itemvalue, currN->cal_format);
			break;
		case L_CUSTDATE:
			NL_STRCOPY(itemvalue, currN->cust_date_format);
			break;
		case L_CLKSPEC:
			NL_STRCOPY(itemvalue, currN->clock_specification);
			break;
		case L_MNTHABBR:
			NL_STRCOPY(itemvalue, currN->month_abbrev);
			break;
		case L_MNTHFULL:
			NL_STRCOPY(itemvalue, currN->month_full);
			break;
		case L_WKDYABBR:
			NL_STRCOPY(itemvalue, currN->wk_day_abbrev);
			break;
		case L_WKDYFULL:
			NL_STRCOPY(itemvalue, currN->wk_day_full);
			break;
		case L_YESNO:
			NL_STRCOPY(itemvalue, currN->yes_no);
			break;
		case L_DECTHOU:
			NL_STRCOPY(itemvalue, currN->dec_thou_symbols);
			break;
		case L_CURRSIGNS:
			NL_STRCOPY(itemvalue, currN->curr_signs);
			break;
		case L_COLLATE:
		case L_LENCOLLATE:
			if (currN->collate == NULL)  {
				getcollate(idtolang(currN->langid),
					&currN->len_collate, 
					&currN->collate,
					error);
                        }

			if (error[0] != 0)  break;


			if (itemnumber == L_LENCOLLATE)  
			        *itemvalue = (int) currN->len_collate;
			else if (itemnumber == L_COLLATE) 
			        NL_ACOPY(itemvalue,currN->collate,
			   	         (currN->len_collate * 2));

			break;

		case L_UPSHIFT:
		case L_DOWNSHIFT:
			NL_ACOPY(itemvalue, ((itemnumber == L_UPSHIFT) 
					? currN->upshift : currN->downshift),
					SIZE_CHARSET);
			break;
		case L_TYPES:
			NL_ACOPY(itemvalue, currN->char_set_definition,
				 SIZE_CHARSET);
			break;
		case L_LANGCLASS:
			NL_ACOPY(itemvalue, &currN->class_no
                                        , sizeof(currN->class_no));
			break;
		case L_LANGNAME:
			NL_STRCOPY(itemvalue, currN->name_language);
			break;
		case L_LONGLANGNAME:
			NL_STRCOPY(itemvalue, currN->longname_language);
			break;
		case L_LONGCALFORM:
			NL_STRCOPY(itemvalue, currN->long_cal_format);
			break;
		case L_CURRENCYNAME:
			NL_STRCOPY(itemvalue, currN->curr_name);
			break;
		case L_ALTDIGITS:
			NL_STRCOPY(itemvalue, currN->alt_digits);
			break;
		case L_DIRECTION:
			NL_STRCOPY(itemvalue, currN->lang_dir);
			break;
		case L_DATELINE:
			NL_STRCOPY(itemvalue, currN->date_line);
			break;
		case L_DATAORDER:
			NL_ACOPY(itemvalue, &currN->data_order
                                        , sizeof(currN->data_order));
			break;
		case L_CHARSIZE:
			NL_ACOPY(itemvalue, &currN->char_size
                                        , sizeof(currN->char_size));
			break;
		default:
			error [0] = E_OUTOFRANGE;
			break;
	}
	return;
}



char *get_direct(langid)
short langid;
{
/*
 *	when hp-ux langinfo() calls support alternate digits
 *	the following code should be uncommented and tested.
 *
 *	NOTE:
 *		this code has not been tested properly.
	static char direct[LENDIRECT];
	char	buff[32];

	strcpy(buff, nl_langinfo(DIRECTION));
	if (buff[0] == '\0'   ||  buff[0] == '0'){	left to right
		direct[0] = '\1';
		direct[1] = '\0';
		direct[2] = ' ';
		direct[3] = '\0';
	}
	else{						right to left

		 *
		 *	NOTE
		 *		this will have to be changed for 16 bit lang
		 * 
		direct[0] = '\0';
		direct[1] = '\1';		
		strcpy(buff, nl_langinfo(ALT_DIGITS));	right to left space
		direct[2] = *(buff + 10);
		direct[3] = '\0';
	}
*/


	/*
	 *	WARNING
	 *		This is an internal table that really should be part of
	 *		UN*X langinfo().
	 *		When this program was written this info was not 
	 *		available.
	 */
	static struct diRect {
		short	langid;
		char	*directInfo;
	} dire[] = {
		51, "\000\001\240\000",
		52, "\000\001\240\000",
		71, "\000\001\240\000"
	};
	static char leftToRight[] = "\000\000 \000";
	int	i;

	for (i = 0; i < 3; i++)
		if (dire[i].langid == langid)
			return(dire[i].directInfo);

	return(leftToRight);
}





#define MAXCURRSIGNS	25

char *get_currsign(langid)
short langid;
{
	/*
	 *	WARNING
	 *		This is an internal table that really should be part of
	 *		UN*X langinfo().
	 *		When this program was written this info was not 
	 *		available.
	 */
	static struct currSigns {
		short	langid;
		char	*currsigns;
	} cSigns[] = {
		0, "\044\000\040\040\040\040",
		1, "\044\000\125\123\044\040",
		2, "\044\021\044\103\101\116",
		3, "\040\001\113\162\040\040",
		4, "\276\000\104\146\154\056",
		5, "\273\000\040\040\040\040",
		6, "\272\000\115\113\040\040",
		7, "\106\000\106\106\040\040",
		8, "\040\000\104\115\040\040",
		9, "\273\000\114\151\164\056",
		10, "\040\000\113\162\040\040",
		11, "\044\000\040\040\040\040",
		12, "\040\001\120\164\163\056",
		13, "\044\001\040\040\040\040",
		14, "\044\000\040\040\040\040",
		41, "\044\000\040\040\040\040",
		51, "\040\021\321\352\307\344",
		52, "\040\021\321\352\307\344",
		61, "\040\021\304\362\040\040",
		71, "\321\000\211\176\040\040",
		81, "\272\001\211\176\040\040",
		201, "\040\000\122\115\102\040",
		211, "\040\000\116\124\044\040",
		221, "\134\000\211\176\040\040",
		231, "\134\000\147\241\040\040",
		0, "\044\000\040\040\040\040" 
	};
	static char noCurrSign[] = "\000\000\000\000\000\000";
	int	i;

	for (i = 0; i < MAXCURRSIGNS; i++)
		if (cSigns[i].langid == langid)
			return(cSigns[i].currsigns);

	return(noCurrSign);
}






#define MAXALTDIGARRAY	1

char *get_altdigits(langid)
short langid;
{
/*
 *	when hp-ux langinfo() calls support alternate digits
 *	the following code should be uncommented and tested.
 *
 *	NOTE:
 *		this code has not been tested properly.
	static char altDig[LENALTDIGITS];
	char	buff[32];

	strcpy(buff, nl_langinfo(ALT_DIGITS));
	if (buff[0] == '\0')
		memset(altDig, '\0', LENALTDIGITS);
	else{

		 *
		 *	NOTE
		 *		this will have to be changed for 16 bit lang
		 * 
		altDig[0] = '\0';
		altDig[1] = '\1';
		altDig[2] = *buff;
		altDig[3] = *(buff + 9);
		altDig[4] = *(buff + 11);
		altDig[5] = *(buff + 12);
		altDig[6] = *(buff + 13);
		altDig[7] = *(buff + 14);
	}
*/


	/*
	 *	WARNING
	 *		This is an internal table that really should be part of
	 *		UN*X langinfo().
	 *		When this program was written this info was not 
	 *		available.
	 */
	static struct altDigits {
		short	langid;
		char	*altdig;
	} aDig[] = {
		51, "\000\001\260\271\253\255\247\060"		/* arabic */
	};
	static char noAltDig[] = "\000\000\000\000\000\000\000\000";
	int	i;

	for (i = 0; i < MAXALTDIGARRAY; i++)
		if (aDig[i].langid == langid)
			return(aDig[i].altdig);

	return(noAltDig);
}


/*
 *	get the linked list node
 */
struct l_info *getl_info(langid)
short		langid;
{
	static struct l_info *n, *m, *get_native(), *init_lnode (), *get_C();

	n = NULL;
        if (langid == -1) 
	    if (currN != 0) 
		 langid = currN->langid;
	    else return (NULL);

	/*
	 *	check if we previously loaded the language
	 *	to the linked list
	 */

	for (n = nRoot; n != NULL; n = n->next){
		if (n->langid == langid)
			return(n);
	}

	/*
	 *	if not ...
	 */
	if (langid == 0)  n = (struct l_info *) get_native();
	       else if (langid == 99)  n = (struct l_info *) get_C();
	            else  n = (struct l_info *) init_lnode(langid);

        if (n == NULL) return (NULL);

	/*
	 *	if first language called, set nRoot to the structure
         *      else add n to linked list
	 */
	if (nRoot == NULL) nRoot = n;
        else {
             for (m = nRoot; m -> next != NULL; m = m -> next);
             m -> next = n;
             }
	return(n);
}


/*
 *	initialize a node
 *		1. allocate memory
 *		2. load the language information
 * 
 *	load the information about the language
 *	see the structure for information about the data to be loaded.
 */
struct  l_info *init_lnode (langid)
short		langid;
{
        struct l_info *n;
	nl_catd	catd;
	int	i;
	char    *longlangname;
	char	*w,
		*getenv(),
		spChar;		/* space character */
	char	*get_altdigits(), *get_direct(), 
		*get_currsign();
	char	buff [256];
	char    gBuff [128];     /* message catalog file name */
	char    *slash;          /* pointer to "iso8859" string */


        _nl_errno = 0;  /* if error found: idtolang() will set _nl_errno  */
	longlangname = idtolang(langid);
	if ((longlangname == NULL) || (*longlangname == NULL))  {
		_nl_errno = E_LNOTCONFIG;
		return(NULL);
        }

        _nl_errno = 0; 
        /* if error found: nl_init can set _nl_errno  */
	if (nl_init(longlangname) == -1) {
		_nl_errno = E_LNOTCONFIG;
		return(NULL);
        }

	n = (struct l_info *) calloc(sizeof (struct l_info), 1);
	n->langid = langid ;
	n->next = (struct l_info *) NULL;   /* end of the list */
	n->collate = NULL;

	/*
	 * we need to do this one first so that we can get the right
	 *	to left space for languages that need it.
	 */
	memcpy(n->lang_dir, get_direct(n->langid), LENDIRECT);
	memcpy((char *)&i, n->lang_dir, sizeof(short));
	spChar = (i) ? n->lang_dir[2] : ' ';

        
        sprintf(gBuff, "/usr/lib/nls/%s/custdat.cat",idtolang(langid));
	if (slash = (strstr(gBuff,"iso8859")))  {
	    --slash;  
	    *slash = '/';  
        }
	if ((catd = open(gBuff, O_RDONLY)) != -1){
	
	   catgetmsg(catd, 1, L_CALFORM, buff, sizeof(n->cal_format));
	   fillbuff(buff, strlen(buff), sizeof(n->cal_format));
	   strncpy(n->cal_format, buff, sizeof(n->cal_format));

	   catgetmsg(catd, 1, L_CUSTDATE, buff, sizeof(n->cust_date_format));
	   fillbuff(buff, strlen(buff), sizeof(n->cust_date_format));
	   strncpy(n->cust_date_format, buff, sizeof(n->cust_date_format));

	   catgetmsg(catd, 1, L_CLKSPEC, buff, sizeof(n->clock_specification));
	   fillbuff(buff, strlen(buff), sizeof(n->clock_specification));
	   strncpy(n->clock_specification, buff, sizeof(n->clock_specification));

	   catgetmsg(catd, 1, L_DATELINE, buff, sizeof(n->date_line));
	   fillbuff(buff, strlen(buff), sizeof(n->date_line));
	   strncpy(n->date_line, buff, sizeof(n->date_line));

	   catgetmsg(catd, 1, L_LANGCLASS, buff, sizeof(buff));
	   n->class_no = (unsigned short)atoi(buff);

	   catgetmsg(catd, 1, L_CURRENCYNAME, buff, sizeof(n->curr_name));
	   fillbuff(buff, strlen(buff), sizeof(n->curr_name));
	   strncpy(n->curr_name, buff, sizeof(n->curr_name));
	   CONV_NULL_TO_BL(n->curr_name, w, ' ');

	   catgetmsg(catd, 1, L_LONGCALFORM, buff, sizeof(n->long_cal_format));
	   fillbuff(buff, strlen(buff), sizeof(n->long_cal_format));
	   strncpy(n->long_cal_format, buff, sizeof(n->long_cal_format));

	   close(catd);

        } else {
	      strcpy(n->cal_format, "DDD, MMM dd, yyyy ");
              strcpy(n->cust_date_format, "mm/dd/yy     ");
              strcpy(n->clock_specification, "12:AMPM ");
              strcpy(n->date_line, "DDD, MMM dd, yyyy,          ");
              n->class_no = 1;
              strcpy(n->curr_name, "$\000    ");
	      strcpy(n->long_cal_format,"WWWWWWWWWWWW, OOOOOOOOOOOO dd, yyyy ");
        }


		/* Note that if the custdat.cat message file does not exist,
		 * the n-computer formats will be used for that language 
                 */
	

	strncpy(n->name_language, idtolang(langid), sizeof(n->name_language));
	strncpy(n->longname_language, idtolang(langid), sizeof(n->longname_language));

/* The data base lab requested to have language names in lower case    
 *
 *      The following code upshifts the language name. It is commented out  
 *
 *      for (w = n->name_language; *w; w++)
 *      	*w = toupper(*w);
 *
 */
	CONV_NULL_TO_BL(n->name_language, w, ' ');
	CONV_NULL_TO_BL(n->longname_language, w, ' ');

	for (i = YESSTR; i <= NOSTR; i++)
		strncpy(n->yes_no + (SIZE_YESNOSTR * (i - YESSTR)),
			nl_langinfo(i), SIZE_YESNOSTR);
	CONV_NULL_TO_BL(n->yes_no, w, ' ');
	for (w = n->yes_no; w < n->yes_no + sizeof(n->yes_no); w++)
		*w = toupper(*w);

	for (i = ABMON_1; i <= ABMON_12; i++)
		strncpy(n->month_abbrev + (SIZE_ABMONTH * (i - ABMON_1)),
			nl_langinfo(i), SIZE_ABMONTH);
	CONV_NULL_TO_BL(n->month_abbrev, w, spChar);

	for (i = MON_1; i <= MON_12; i++)
		strncpy(n->month_full + (SIZE_MONTH * (i - MON_1)),
			nl_langinfo(i), SIZE_MONTH);
	CONV_NULL_TO_BL(n->month_full, w, spChar);


	for (i = ABDAY_1; i <= ABDAY_7; i++)
		strncpy(n->wk_day_abbrev + (SIZE_ABDAY * (i - ABDAY_1)),
			nl_langinfo(i), SIZE_ABDAY);
	CONV_NULL_TO_BL(n->wk_day_abbrev, w, spChar);

	for (i = DAY_1; i <= DAY_7; i++)
		strncpy(n->wk_day_full + (SIZE_DAY * (i - DAY_1)),
			nl_langinfo(i), SIZE_DAY);
	CONV_NULL_TO_BL(n->wk_day_full, w, spChar);

	for (i = RADIXCHAR; i <= THOUSEP; i++)
		strncpy(n->dec_thou_symbols + (sizeof(char) * (i - RADIXCHAR)),
			nl_langinfo(i), sizeof(char));
	CONV_NULL_TO_BL(n->dec_thou_symbols, w, ' ');

	n->char_size = atoi(nl_langinfo(BYTES_CHAR)) - 1;
		
	gettables(n);

	memcpy(n->curr_signs, get_currsign(n->langid), sizeof(n->curr_signs));

	memcpy(n->alt_digits, get_altdigits(n->langid), LENALTDIGITS);

	loadnumspec(n);

	/*
	**	these get allocated when they are needed.
	*/
	n->collate		= NULL;

	return (n);
}


/*
**	is the language supported
*/
islangsupp(langid, languages, langArr, itemValue)
short	langid, languages, *langArr, *itemValue;
{
	*itemValue = NL_TRUE;

	while (languages--)
		if (langid == *++langArr)
			return;

	*itemValue = NL_FALSE;
	return;
}

struct l_info *get_native()
{
	static struct l_info n = {
		(struct l_info *) NULL, /* next */
		NULL,			/* ctype */
		0,			/* language id */
		3,			/* length of collate table */
		1,			/* class number */
		"DDD, MMM dd, yyyy ",	/* calendar format */
		"mm/dd/yy     ",	/* custom date format */
		"12:AMPM ",		/* clock specification */
		"WWWWWWWWWWWW, OOOOOOOOOOOO dd, yyyy ",
					/* long calendar format */
		"DDD, MMM dd, yyyy,          ",		/* dateline format */
		"JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC ",
					/* month abbreviations */
		"JANUARY     FEBRUARY    MARCH       APRIL       MAY         JUNE        JULY        AUGUST      SEPTEMBER   OCTOBER     NOVEMBER    DECEMBER    ",
					/* full month names */
		"SUNMONTUEWEDTHUFRISAT",/* week-day abbreviations */
		"SUNDAY      MONDAY      TUESDAY     WEDNESDAY   THURSDAY    FRIDAY      SATURDAY    ",
					/* full week-day names */
		"YES   NO    ",
		".,",			/* decimal thousand symbols */
		"$\000    ",		/* currency sign */
		"\000\003\000\000\000\001",	/* collate table */
		"",			/* character types */
		"",			/* upshift */
		"",			/* downshift */
		"n-computer      ",	/* language name */
		"n-computer                                                      ",	/* long language name */
		"U.S. Dollars    ",	/* currency name */
		"\000\000\000\000\000\000\000\000",	/* alternate digits */
		"\000\000 \000",	/* language direction */
		0,			/* data order */
		0			/* character size */
	};

	gettables(&n);
	loadnumspec(&n);
	return(&n);
}



struct l_info *get_C()
{
	static struct l_info n = {
		(struct l_info *) NULL, /* next */
		NULL,			/* ctype */
		99,      		/* language id */
		3,			/* length of collate table */
		1,			/* class number */
		"DDD, MMM dd, yyyy ",	/* calendar format */
		"mm/dd/yy     ",	/* custom date format */
		"12:AMPM ",		/* clock specification */
		"WWWWWWWWWWWW, OOOOOOOOOOOO dd, yyyy ",
					/* long calendar format */
		"DDD, MMM dd, yyyy,          ",		/* dateline format */
		"JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC ",
					/* month abbreviations */
		"JANUARY     FEBRUARY    MARCH       APRIL       MAY         JUNE        JULY        AUGUST      SEPTEMBER   OCTOBER     NOVEMBER    DECEMBER    ",
					/* full month names */
		"SUNMONTUEWEDTHUFRISAT",/* week-day abbreviations */
		"SUNDAY      MONDAY      TUESDAY     WEDNESDAY   THURSDAY    FRIDAY      SATURDAY    ",
					/* full week-day names */
		"YES   NO    ",
		".,",			/* decimal thousand symbols */
		"\000     ",		/* currency sign */
		"\000\003\000\143\000\001",	/* collate table */
		"",			/* character types */
		"",			/* upshift */
		"",			/* downshift */
		"C               ",	/* language name */
		"C                                                               ",	/* long language name */
		"                ",	/* currency name */
		"\000\000\000\000\000\000\000\000",	/* alternate digits */
		"\000\000 \000",	/* language direction */
		0,			/* data order */
		0			/* character size */
	};

	gettables(&n);
	loadnumspec(&n);
	return(&n);
}



loadnumspec(n)
struct l_info *n;
{
	char	*numspec;
	char	spChar;			/* space character */
	short	shBytes;
	int	i;

	static struct cuRrency{
		short	langid;
		char	*currInfo;  /*  0     short currency sign
				        1     preceding or succeeding blanks
				        2     symbol precedes or succeeds number
				    	3 - 6 full currency symbol */
	} curr[] = {
		0, "\044\000\040\040\040\040",
		1, "\044\000\125\123\044\040",
		2, "\044\021\044\103\101\116",
		3, "\040\001\113\162\040\040",
		4, "\276\000\104\146\154\056",
		5, "\273\000\040\040\040\040",
		6, "\272\000\115\113\040\040",
		7, "\106\000\106\106\040\040",
		8, "\040\000\104\115\040\040",
		9, "\273\000\114\151\164\056",
		10, "\040\000\113\162\040\040",
		11, "\044\000\040\040\040\040",
		12, "\040\001\120\164\163\056",
		13, "\044\001\040\040\040\040",
		14, "\044\000\040\040\040\040",
		41, "\044\000\040\040\040\040",
		51, "\040\021\321\352\307\344",
		52, "\040\021\321\352\307\344",
		61, "\040\021\304\362\040\040",
		71, "\321\000\211\176\040\040",
		81, "\272\001\211\176\040\040",
		201, "\040\000\122\115\102\040",
		211, "\040\000\116\124\044\040",
		221, "\134\000\211\176\040\040",
		231, "\134\000\147\241\040\040",
		0, "\044\000\040\040\040\040" 
	};
	static char leftToRight[] = "\000\000 \000";  

	numspec = n->numspec;

	memset(numspec, '\0', SIZE_NUMSPEC);
	memcpy(numspec, (char *)&n->langid, 2);
        numspec [14] = ' ';  /* Right to left space */

	/*
	 * alternate digits
	 */
	if (n->alt_digits[1] == '\1'){
					/* digits ind */
		memcpy(numspec + NUMSP_ALTDIGITS, n->alt_digits, 2);	
					/* 0-9 range */
		memcpy(numspec + NUMSP_09RANGE, n->alt_digits + 2, 2);	
					/* +/- */
		memcpy(numspec + NUMSP_ALTPLUS, n->alt_digits + 4, 2);	
					/* dec seperator */
		*(numspec + NUMSP_ALTDEC) = *(n->alt_digits + 6);    
					/* thousands sep */
		*(numspec + NUMSP_ALTTHOU) = *(n->alt_digits + 7);   
	}


	/*
	 * language direction
	 */
	if (n->lang_dir[1] == '\1'){
				/* lang dir ind */
		memcpy(numspec + NUMSP_LANGDIR, n->lang_dir, 2);     
		memcpy((char *)&shBytes, n->lang_dir, 2);
		spChar = (shBytes) ? *(n->lang_dir + 2) : ' ';
				/* right to left space */
		*(numspec + NUMSP_RTOLSP) = *(n->lang_dir + 2);   	
	}
	else
		spChar = ' ';





	/*
	 * dec/thou separator
	 */
				/* decimal separator */
	*(numspec + NUMSP_DECSEP) = *n->dec_thou_symbols;
				/* thousands separator */
	*(numspec + NUMSP_THOUSEP) = *(n->dec_thou_symbols + 1);

	/*
	 * currency information
	 */
	for (i = 0; i < MAXCURRSIGNS; i++)
		if (curr[i].langid == n->langid)
			break;
	/*
	 * check if invalid langid
	 */
	if (i != MAXCURRSIGNS){
		/* 
	 	* preceding or succeeding blanks
	 	*/
		*(numspec+NUMSP_CURRENCYPLACE+1) = *(curr[i].currInfo + 1) >> 4;
		for_currsymb(numspec + NUMSP_CURRENCY, curr[i].currInfo, &shBytes, spChar);
		memcpy(numspec + NUMSP_BYTESCURRENCY, (char *)&shBytes, sizeof(short));
	}
}


/*
 * for the currency symbol
 */
for_currsymb(t, currI, pBytes, spChar)
char	*t, *currI;
short	*pBytes;
char	spChar;			/* space character */
{
	int	i;
	char	*wrk;
        int     flag;

	*pBytes = 0;

	/*
	 * if needed, place blank before currency symbol
	 */
	flag = *(currI +1) & 017;
	if ((flag == '\001')  ||  (flag == '\003')){
		*t++ = spChar;
		*pBytes += 1;
	}

	/*
	 * if short currency sign is present, use it
	 */
	if (*currI != ' '){
		*t++ = *currI;
		*pBytes += 1;
	}
	/*
	 * otherwise, use the long one.
	 */
	else{
		for (wrk = currI + 2 ; wrk < currI + 6 && *wrk != ' '; wrk++){
			*t++ = *wrk;
			*pBytes += 1;
		}
	}
	/*
	 * if needed, place blank after currency symbol
	 */
	if (flag == '\002'  ||  flag == '\003'){
		*t++ = spChar;
		*pBytes += 1;
	}

	/*
	 * now fill the rest of the field with blanks
	 */
	for (i = 18 - *pBytes ; i > 0 ; i--)
		*t++ = spChar;
}
