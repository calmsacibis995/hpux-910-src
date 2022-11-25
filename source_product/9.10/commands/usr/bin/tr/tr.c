static char *HPUX_ID = "@(#) $Revision: 72.1 $";

/*
 * NOTE:  The tr(1) command has a very long history; it was never
 *        intended to do translations for languages other than
 *        English/ASCII.  Consequently, running this command
 *        with LANG set to other languages will do some kind
 *        of translation, but may not be very meaningful.
 */

#include <stdio.h>
#include <nl_ctype.h>
#include <locale.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <nl_types.h>
#include <collate.h>
#include <errno.h>

/*
 * POSIX.2 requires handling NULLs just like any other character.
 * We will accomplish this by using MYZERO below to indicate
 * end-of-string and actually treating the NULLs exactly like
 * other characters.
 *
 * Be sure to use signed variables to manipulate this value!!!
 */
#define MYZERO		-1

#define FIRSTin21	0x0000ff00
#define SECin21		0x000000ff
#define MARK21		0x00ff0000
#define TWO1		0100
#define MAXINPUT  33024
#define MAXSIZE8    255
#define MAXSIZE16 65536
#define TABSIZE16 32768 
#define CTABSIZE  (TABSIZE16 + MAXSIZE8)
#define ERRORSIZE    256
#define MAPSIZE16  4096 
#define NO_NLS	(_nl_onlyseq && !_nl_mb_collate && !_nl_map21 && !_nl_collate_on)
#define NL_8 	(!_nl_mb_collate && (_nl_map21 || _nl_collate_on))
#define NL_16 	(_nl_mb_collate)

extern	u_char	 *_seqtab;	/* dictionary sequence number table */
extern	u_char	 *_pritab;	/* 1to2/2to1 flag + priority table */
extern  u_char   *__ctype;
extern	struct col_21tab *_tab21;	/* 2-to-1 mapping table		*/
extern	struct col_12tab *_tab12;	/* 1-to-2 mapping table		*/

#define	catgets(i, j, k, s)	(s)

/* tr - transliterate data stream */
int dflag = 0;
int sflag = 0;
int cflag = 0;
int save = 0;
int old = 0;

int code[CTABSIZE];	/* table of substitutions */
int vect[MAXINPUT];	/* table of complements */
int *compl;		/* pointer to vect */

union  {
	unsigned short unit;
	struct {
		unsigned 	bit0 : 1;
		unsigned	bit1 : 1;
		unsigned	bit2 : 1;
		unsigned	bit3 : 1;
		unsigned	bit4 : 1;
		unsigned	bit5 : 1;
		unsigned	bit6 : 1;
		unsigned	bit7 : 1;
		} bf;
} squeez[MAPSIZE16];

struct string { int last, max, rep; u_char *p; u_char exp[256]; } string1, string2;
FILE *input;
int no_string2 = 0;

extern int _nl_map21;		/* setlocale: true if two-to-one mapping */
extern int _nl_collate_on;	/* setlocale: true if collation table loaded */
extern int _nl_mb_collate;	/* setlocale: true if collation is multibyte */
extern int _nl_onlyseq;		/* setlocale: true if mach coll. only */
extern int optind;		/* for getopt() */
extern char *optarg;		/* for getopt() */

struct two1_unit {
	int from[2];
	int to[2];
} two1_list[16];
int two1_cnt = 0;
int lookahead = 0;

char usage[] = "tr: usage: tr [-cds] string1 string2\n";

#ifdef DBG
int dbgflag = 0;
#endif

u_char cmatch();
int    seq_check();

nl_catd catd;				/* to store id of error msg file */
extern int errno;

/*
 * The following variables are introduced to handle the POSIX.2
 * requirement for character class upper and lower: when string1 
 * is "lower" and string2 is "upper" then toupper() has to be used 
 * to obtain the substitution character and when string1 is "upper"
 * and string2 is "lower" then tolower() has to be used to obtain
 * the substitution character.
 */
int	str_is_upper=0;		/* The passed string is upper */
int	str_is_lower=0;		/* The passed string is lower */
int	str1_is_upper=0;	/* string1 of the tr argument is upper */
int	str1_is_lower=0;	/* string1 of the tr argument is lower */
int	str2_is_upper=0;	/* string2 of the tr argument is upper */
int	str2_is_lower=0;	/* string2 of the tr argument is lower */

main(argc,argv)
int argc;
u_char **argv;
{
	register i, j, k;
	register c, d = 0, last_d = 0;
	char ch;
	int errs = 0;
	int offset, index;
	int find21;

	/* initialize to current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("tr"), stderr);
		putenv("LANG=");
		catd = (nl_catd)-1;
	} 

	catd = catopen("tr", 0);

	string1.last = string2.last = 0;
	string1.max = string2.max = 0;
	string1.rep = string2.rep = 0;
	string1.p = string2.p = (u_char *)"";

	while((ch = getopt(argc,argv,"cds")) != EOF)
	    switch(ch){
		case 'c':
			cflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 's':
			sflag++;
			break;
		default:
			errs++;
	    }


	if(argc - optind > 0) 
	    string1.p = argv[optind];
	else
	    errs++;
	if(argc - optind > 1) 
	    string2.p = argv[optind+1];
	else
	    no_string2++;    

	if(errs) {
		fprintf(stderr, catgets(catd, 1, 1, "%s"), usage);
		exit(1);
	}
#ifdef DBG
printf("String1 = %s\tString2 = %s\n",string1.p,string2.p);
#endif

	if(NO_NLS){
	    setup();
	    proc_input();
	}
	else
	    if(NL_8){
		setup8();
		proc_input8();
	    }
	    else{
		setup16();
		proc_input16();
	    }

	if(catd != -1)
		catclose (catd);
	exit(0);
}

/*******************
 *  The next two functions do the processing in the easiest case, where
 *  there is no special collating, 2-1 chars, or multibyte chars.
 *******************/
int
setup()
{
	register i, j, k;
	register c, d = 0, last_d = 0;
	int maxinput = MAXSIZE8;

	/* Use MYZERO to "zero" out code and vect tables.
         * In this way, we can use NULL as a regular/valid character.
         */
	for(i = 0; i < 256; i++)
	    vect[i] = code[i] = MYZERO;
	    
	if(cflag) {
		while ((c = next (&string1)) != MYZERO) 
			vect [c&0377] = 1;

		for (i=j=0; i< 256; i++)
			if (vect[i] == MYZERO) vect[j++] = i;
                vect[j] = MYZERO;
		compl   = vect;
	}

	for(i=0; i<256; i++)
		squeez[i].unit = 0;

	for(i = 0; i < maxinput; i++ ){
		/*
		 * The following code contains stuff to handle the POSIX.2
		 * requirement for character class upper and lower: when 
		 * string1 is "lower" and string2 is "upper" then toupper() 
		 * has to be used to obtain the substitution character and 
		 * when string1 is "upper" and string2 is "lower" then 
		 * tolower() has to be used to obtain
		 * the substitution character.
		 */
		if(cflag) c = (compl[i]);
		else{
		    c = next(&string1);

		    /*
		     * The next() sets the variables str_is_upper
		     * or str_is_lower if "upper" or "lower" is
		     * passed as the initial string.
		     */

		    if(str_is_upper){
			str1_is_upper = 1;
			str_is_upper=0;
		    }
		    else if(str_is_lower){
			str1_is_lower = 1;
			str_is_lower = 0;
		    }
		}
		if(c==MYZERO) break;
		/*  This saves all that next() processing if no string2
 		 *  was given. */
		if(no_string2) d = c;
	    	else {
		    /*
		     * Once it is known that string1 is "upper"
		     * and string2 is "lower" or vice versa,
		     * then next(&string2) can be totally avoided.
		     * As per POSIX.2 requirement it is just 
		     * sufficient to use "tolower" or "toupper"
		     * functions to get the substitution character.
		     * The next() has to be executed only once
		     * to determine whether the abovementioned
		     * conditions are met.
		     */
		    if(str1_is_upper && str2_is_lower)
			d = _tolower(c);
		    else if(str1_is_lower && str2_is_upper)
			d = _toupper(c);
		    else{
    		        d = next(&string2);
    		        /*  If d returns MYZERO, turn it into c  */
    		        if(d==MYZERO) d = c;
		        if(str_is_upper){
			    str2_is_upper = 1;
			    str_is_upper=0;
		        }
		        else if(str_is_lower){
			    str2_is_lower = 1;
			    str_is_lower = 0;
		        }
			/*
			 * This is the first time the substitution
			 * character is obtained in the following
			 * way.  I won't be coming here again
			 * if the conditions (i.e. the translation
			 * is upper to lower or vice versa) is met.
			 */
		        if(str1_is_upper && str2_is_lower)
			    d = _tolower(c);
		        else if(str1_is_lower && str2_is_upper)
			    d = _toupper(c);
		    }
		}

#ifdef DBG
printf("setup: c = %c (%d)\td = %c (%d)\n",c,c,d,d);
#endif

		code[c&0377] = d;
		squeez[d&0377].unit = 1;
	}

	if(string2.rep == 0)
		string2.rep--;

	while ((d = next(&string2)) != MYZERO)
		squeez[d&0377].unit = 1;

	for(i=0;i<256;i++) {
		if(code[i]==MYZERO)
			code[i] = i;
		else 
		    if(dflag) 
			code[i] = MYZERO; /* MYZERO means skip/delete */
	}

}

int
proc_input()
{
	int c, save;

	input = stdin;
	while((c=getc(input)) != EOF ) {
                c =  code[c&0377];
                if (c != MYZERO) 
			if(!sflag || c!=save || !squeez[c].unit) {
                                save = c;
				putchar(c);
                        } /* else, squeeze it out */
                /* else, skip/delete it */
	}
}


/***********************
 *  The next two functions, setup8() and proc_input8, are used whenever
 *  there are 2-1 collating elements or table collation (as opposed to
 *  machine collation) is used, but all chars are in the 8-bit range.
 ***********************/
int
setup8()
{
	register i, j, k;
	register c, d = 0, last_d = 0;
	int maxinput = MAXSIZE8;

	/* Zero out code and vect tables */
	for(i = 0; i < 256; i++)
	    vect[i] = code[i] = MYZERO;

	coll_sort();  /* set up sequencing and priority table */
	    
	if(cflag) {
		while ((c = next (&string1)) != MYZERO) {
			if (c & MARK21)		/* ignore 2-to-1 char */
				continue;
			else
			    vect[c] = 1;
		}

		for (i=j=0; i< 256; i++)
			if (vect[i] == MYZERO) vect[j++] = i;
		vect[j] = MYZERO;
		compl   = vect;
	}

	for(i=0; i<256; i++)
		squeez[i].unit = 0;

	for(i = 0; i < maxinput; i++ ){
		/*
		 * The following code contains stuff to handle the POSIX.2
		 * requirement for character class upper and lower: when 
		 * string1 is "lower" and string2 is "upper" then toupper() 
		 * has to be used to obtain the substitution character and 
		 * when string1 is "upper" and string2 is "lower" then 
		 * tolower() has to be used to obtain
		 * the substitution character.
		 */
		if(cflag) c = (compl[i]);
		else{
		    c = next(&string1);
		    if(str_is_upper){
			str1_is_upper = 1;
			str_is_upper=0;
		    }
		    else if(str_is_lower){
			str1_is_lower = 1;
			str_is_lower = 0;
		    }
		}
		if(c==MYZERO) break;
		/*  This saves all that next() processing if no string2
 		 *  was given. */
		if(no_string2) d = c;
	    	else {
		    /*
		     * Once it is known that string1 is "upper"
		     * and string2 is "lower" or vice versa,
		     * then next(&string2) can be totally avoided.
		     * As per POSIX.2 requirement it is just 
		     * sufficient to use "tolower" or "toupper"
		     * functions to get the substitution character.
		     * The next() has to be executed only once
		     * to determine whether the abovementioned
		     * conditions are met.
		     */
		    if(str1_is_upper && str2_is_lower)
			d = _tolower(c);
		    else if(str1_is_lower && str2_is_upper)
			d = _toupper(c);
		    else{
    		        d = next(&string2);
    		        /*  If d returns MYZERO, turn it into c  */
    		        if(d==MYZERO) d = c;
		        if(str_is_upper){
			    str2_is_upper = 1;
			    str_is_upper=0;
		        }
		        else if(str_is_lower){
			    str2_is_lower = 1;
			    str_is_lower = 0;
		        }
			/*
			 * This is the first time the substitution
			 * character is obtained in the following
			 * way.  I won't be coming here again
			 * if the conditions (i.e. the translation
			 * is upper to lower or vice versa) is met.
			 */
		        if(str1_is_upper && str2_is_lower)
			    d = _tolower(c);
		        else if(str1_is_lower && str2_is_upper)
			    d = _toupper(c);
		    }
		}

#ifdef DBG
printf("setup8: c = %c (%d)\td = %c (%d)\n",c,c,d,d);
#endif

		if(c & MARK21 || d & MARK21) {
                        two1_list[two1_cnt].from[0] = (c & FIRSTin21)>>8; 
                        if (two1_list[two1_cnt].from[0] == NULL)
                           two1_list[two1_cnt].from[0] = MYZERO;

			two1_list[two1_cnt].from[1] = (c & SECin21); 

			two1_list[two1_cnt].to[0] = (d & FIRSTin21)>>8; 
                        if (two1_list[two1_cnt].to[0] == NULL)
                           two1_list[two1_cnt].to[0] = MYZERO;

			two1_list[two1_cnt].to[1] = (d & SECin21); 
			two1_cnt++;
		}
		else {
		    code[c&0377] = d;
		    squeez[d&0377].unit = 1;
	        }
	}


	if(string2.rep == 0)
		string2.rep--;

	while ((d = next(&string2)) != MYZERO)
		squeez[d&0377].unit = 1;

	for(i=0;i<256;i++)
		if(code[i]==MYZERO) code[i] = i;
		else if(dflag) code[i] = MYZERO;
}

int
proc_input8()
{
	int c, c2; 
	int i, save;
	int find21,squeezep;

	input = stdin;
	while((c=getc(input)) != EOF ) {
		find21 = 0;
		/*  locale has 2-1 and this could be one, search table */
		if (_nl_map21) {	
		    for (i = 0; i < two1_cnt; i++) {
		        if (c == two1_list[i].from[0]) {
			    lookahead = getc(input);
			    if (lookahead == two1_list[i].from[1]) {
			        /* find a 2-to-1 input char */
			        if (dflag) c = c2 = MYZERO;
				/* if tr to another 2-1, create two
				 * output chars */
			        else{
				    c = two1_list[i].to[0];
			    	    c2 = two1_list[i].to[1];
				}
				find21++;
				squeezep = 1;
				lookahead = 0;
				break;
			    }
			    /* Prevent skipping chars by putting c back */
			    else
				ungetc(lookahead,input);
			    continue;	/* for loop */
			} else if ((two1_list[i].from[0] == MYZERO)
			    && (c == two1_list[i].from[1])) {
			    /* find a char tr to 2-to-1 output char */
			    if (dflag) c = c2 = MYZERO;
			    else{
			        c = two1_list[i].to[0];
				c2 = two1_list[i].to[1];
			    }
			    find21++;
			    squeezep = 1;
			    break;
			}
		    }			/* for loop */
		    if (!find21)
		        c = code[c];
		} else		/* locale without 2-to-1 mapping */
		    c = code[c];

		   if (!find21) {
                      if (c != MYZERO) {
		         squeezep = squeez[c&0377].unit;
		         if(!sflag || c!=save || !squeezep)
			    putchar(save = c);
                      }
		   }
		   /* If translated to a 2-1, output both chars */
		   else if ((c != MYZERO) || (c2 != MYZERO)) {
		      if(!sflag || ((c<<8)|c2) != save || !squeezep){
			 if (c != MYZERO) putchar(c);
                         putchar(c2);
		 	 save = c<<8 | c2;
		      }
		   }
	}	/*  while getc() */
}

/***********************
 *  The next two functions, setup16() and proc_input16, are used when the
 *  language contains multi-byte characters.  It is presently assumed
 *  that the possibilty of 2-1 collating elements is implied in NLS16.
 ***********************/
int
setup16()
{
	register i, j, k;
	register c, d = 0, last_d = 0;
	char ch;
	int errs = 0;
	int offset, index;
	int find21;

	/* Zero out code and vect tables */
	memset((u_char*)vect,MYZERO,sizeof(vect));
	memset((u_char*)code,MYZERO,sizeof(code));
	
	coll_sort();  /* set up sequencing and priority table */

	if(cflag) {
		while ((c = next (&string1)) != MYZERO) {
			if (c & MARK21)		/* ignore 2-to-1 char */
				continue;
			if (c > MAXSIZE8) {
				offset = c - TABSIZE16;
				index = MAXSIZE8 + 1 + offset;
				vect[index] = 1;
			}
			else
				vect[c] = 1;
		}

		for (i= j = k = 0; i < MAXINPUT; i++, k++) {
			if (vect[i] == MYZERO)
				vect[j++] = k;
			if (k == MAXSIZE8)
				k = TABSIZE16 - 1;
		}

		vect[j] = MYZERO;
		compl     = vect; 
	}

	memset(squeez,0,sizeof(squeez));

	for(i = 0; i < MAXINPUT; i++ ){
		/*
		 * The following code contains stuff to handle the POSIX.2
		 * requirement for character class upper and lower: when 
		 * string1 is "lower" and string2 is "upper" then toupper() 
		 * has to be used to obtain the substitution character and 
		 * when string1 is "upper" and string2 is "lower" then 
		 * tolower() has to be used to obtain
		 * the substitution character.
		 */
		if(cflag) c = (compl[i]);
		else{
		    c = next(&string1);
		    if(str_is_upper){
			str1_is_upper = 1;
			str_is_upper=0;
		    }
		    else if(str_is_lower){
			str1_is_lower = 1;
			str_is_lower = 0;
		    }
		}
		if(c==MYZERO) break;
		/*  This saves all that next() processing if no string2
 		 *  was given. */
		if(no_string2) d = c;
	    	else {
		    /*
		     * Once it is known that string1 is "upper"
		     * and string2 is "lower" or vice versa,
		     * then next(&string2) can be totally avoided.
		     * As per POSIX.2 requirement it is just 
		     * sufficient to use "tolower" or "toupper"
		     * functions to get the substitution character.
		     * The next() has to be executed only once
		     * to determine whether the abovementioned
		     * conditions are met.
		     */
		    if(str1_is_upper && str2_is_lower)
			d = _tolower(c);
		    else if(str1_is_lower && str2_is_upper)
			d = _toupper(c);
		    else{
    		        d = next(&string2);
    		        /*  If d returns MYZERO, turn it into c  */
    		        if(d==MYZERO) d = c;
		        if(str_is_upper){
			    str2_is_upper = 1;
			    str_is_upper=0;
		        }
		        else if(str_is_lower){
			    str2_is_lower = 1;
			    str_is_lower = 0;
		        }
			/*
			 * This is the first time the substitution
			 * character is obtained in the following
			 * way.  I won't be coming here again
			 * if the conditions (i.e. the translation
			 * is upper to lower or vice versa) is met.
			 */
		        if(str1_is_upper && str2_is_lower)
			    d = _tolower(c);
		        else if(str1_is_lower && str2_is_upper)
			    d = _toupper(c);
		    }
		}

#ifdef DBG
printf("setup16: c = %c (%d)\td = %c (%d)\n",c,c,d,d);
#endif

		if(c & MARK21 || d & MARK21) {
			two1_list[two1_cnt].from[0] = (c & FIRSTin21)>>8; 
			if (two1_list[two1_cnt].from[0] == NULL)
                           two1_list[two1_cnt].from[0] = MYZERO;

			two1_list[two1_cnt].from[1] = (c & SECin21); 

			two1_list[two1_cnt].to[0] = (d & FIRSTin21)>>8; 
			if (two1_list[two1_cnt].to[0] == NULL)
                           two1_list[two1_cnt].to[0] = MYZERO;

			two1_list[two1_cnt].to[1] = (d & SECin21); 
			two1_cnt++;
		}
		else {
		    if ( c > MAXSIZE8 ) 
			code[(c - TABSIZE16)+MAXSIZE8] = d;
		    else 
			code[c&0377] = d;

		    if ( d > MAXSIZE8)
			squeezeOn (d);
		    else
			squeez[d&0377].unit = 1;
		}
	}

	if(string2.rep == 0)
		string2.rep--;

	while ((d = next(&string2)) != MYZERO)
		if (d > MAXSIZE8)
			squeezeOn(d);
		else
			squeez[d].unit = 1;

	/* the part of the table in the single-byte range gets
	 * the character values plugged in for any that aren't translated */
	for(i=0;i<MAXSIZE8;i++) {
	    if(code[i]==MYZERO) code[i] = i;
	    else if(dflag) code[i] = MYZERO;

	}
	/*  the multibyte part of the table is offset downward to save
	 *  space.
	 *  table index 256 corresponds to character value 32769, and
	 *  so on up the line.*/
	for(i = MAXSIZE8 + 1; i < CTABSIZE; i++){
	    if(code[i] == MYZERO) 
		code[i] = i + TABSIZE16 - MAXSIZE8;
	    else if(dflag) code[i] = MYZERO;
	}
}

int
proc_input16()
{
	int c,c2, i, save;
	int find21;
	int squeezep;

	input = stdin;
	while((c=getC(input)) != EOF ) {
		find21 = 0;
		if (c > MAXSIZE8)  
			c = code[(c - TABSIZE16)+MAXSIZE8];
		else {
		    if (_nl_map21) {	
		        for (i = 0; i < two1_cnt; i++) {
		            if (c == two1_list[i].from[0]) {
			        lookahead = getc(input);
			        if (lookahead == two1_list[i].from[1]) {
			        /*     find a 2-to-1 input char */
			            if (dflag) c = c2 = MYZERO;
				/*     if tr to another 2-1, create two
				 *     output chars */
			            else{
				        c = two1_list[i].to[0];
			    	        c2 = two1_list[i].to[1];
				    }
				    find21++;
				    squeezep = 1;
				    lookahead = 0;
				    break;
			        }
			    /* P    revent skipping chars by putting c back */
			        else
				    ungetc(lookahead,input);
			        continue;	/* for loop */
			    } else if ((two1_list[i].from[0] == MYZERO)
			    &&     (c == two1_list[i].from[1])) {
			    /*     find a char tr to 2-to-1 output char */
			        if (dflag) c = c2 = MYZERO;
			        else{
			            c = two1_list[i].to[0];
				    c2 = two1_list[i].to[1];
			        }
			        find21++;
			        squeezep = 1;
			        break;
			    }
		        }			/* for loop */
		        if (!find21)
		            c = code[c];
		    } else		/* locale without 2-to-1 mapping */
			c = code[c];
		}

		   if (!find21) {
                      if (c != MYZERO) {
		         if (c > MAXSIZE8) squeezep = squeeze(c);
		         else squeezep = squeez[c&0377].unit;
		          /* If all conditions are false, suppress output */
		          if(!sflag || c!=save || !squeezep) {
	    	             if ((save = c) > 0377) putchar ((c>>8) & 0377);
	    	             putchar (c & 0377);
		          }
                      }
		   }
		   else{   /* found a 2-1 */
                      if (c != MYZERO || c2 != MYZERO) {
		         if (c > MAXSIZE8) squeezep = squeeze(c);
		         else squeezep = squeez[c&0377].unit;
		         if(!sflag || c!=save || !squeezep) {
			   if (c != MYZERO) putchar(c);
			   putchar(c2);
			   save = c<<8 | c2;
		         }
                      }
		  }
	}	/* while getC() */
}



next(s)
struct string *s;
{
	int a, b, c, n, k, i=0;
	int base;
	char ctyclass[8], *j;

	/* this handles the case of string2 being explicitly NULL,
	 * which is quite different from string2 given. 
	 * (see POSIX.2)
	 */
	if((*(s->p) == NULL) && (s->rep == 0))
 	    return(MYZERO);

	if(--s->rep > 0) 
	    return(s->last);

	if(NO_NLS){
	    if(s->last < s->max) 
		return(++s->last);
	}
	else {		/* NL_8 || NL_16 */
	    /* 16-bit chars on "jump" the gap from last legal 8-bit char (Oxff) 
	     * to first legal 16-bit char (0x8000) */
	    if (((k = xfrmcmp(s->last, s->max)) < 0) && (s->last == MAXSIZE8)) {
	    	s->last = TABSIZE16;	
	    	return (s->last);
	    }
	    else if (k < 0) {
	    	s->last = nextcoll(s->last);
	    	return(s->last);
	    }
	}

	if(*s->p=='[') {
		nextc(s);
		if (*s->p == '[' && (j=strchr(s->p, ']')) && *(++j) == ']') {
			nextc(s);
			old++;
		}
		a=nextc(s);
		if(((a == ':') && strchr(s->p, ':')) || 
			((a == '=') && strchr(s->p, '=')) || 
			((a == '.') && strchr(s->p, '.'))) {
			switch(a) {
				case ':':
					while((c=nextc(s)) && c!=':' && i < 9)
						ctyclass[i++] = c;
					if (c=='\0' || *s->p++!=']' || (old && *s->p++!=']'))
							goto error;
					ctyclass[i] = 0;
					if(strcmp(ctyclass,"lower")==0)
						str_is_lower=1;
					else if(strcmp(ctyclass,"upper")==0)
						str_is_upper=1;
					exp_ctype(ctyclass, s);
					return(nextc(s));
				case '=':
					b = nextc(s);
					if (b=='\0' || *s->p++!='=' ||
						*s->p++!=']' || (old && *s->p++!=']'))
							goto error;
					exp_eq(b, s);
					return(nextc(s));
				case '.':
					b = nextc(s);
					c = nextc(s);
					if (b=='\0' || c=='\0' || *s->p++!='.' ||
						*s->p++!=']' || (old && *s->p++!=']'))
							goto error;
					return((b<<8) | c | MARK21);
			}
		} else{
			s->last = a;
			s->max = 0;
                        switch(nextc(s)) {
			case '-':
				b = nextc(s);
				if(xfrmcmp(b,a) < 0 || *s->p++!=']')
					goto error;
				s->max = b;
				return(a);
			case '*':
				base = (*s->p=='0')?8:10;
				n = 0;
				while((c = *s->p)>='0' && c<'0'+base) {
					n = base*n + c - '0';
					s->p++;
				}
				if(*s->p++!=']') goto error;
				if(n==0) n = MAXINPUT;
				s->rep = n;
				return(a);
			default:
			error:
				fprintf(stderr, catgets(catd, 1, 2, "Bad string\n"));
				exit(1);
			}
		}
	}
	else{
			a = nextc(s);
			s->last = a;
			s->max = 0;
			if (*s->p == '-' && *((s->p)+1) != '\0') {
				b = nextc(s);
				b = nextc(s);
				if(xfrmcmp(b,a) < 0)
					goto error;
				s->max = b;
			}
		}
	return(a);  /* could be MYZERO to indicate end-of-string  */
}

/*
 * nextc() must differentiate between an honest NULL in the string
 * and the end-of-string.
 *
 * An honest NULL is returned as 0; the end-of-string is returned as MYZERO.
 *
 * This approach allows us to use 0 as a valid character to index into
 * tables, etc. very nicely.
 *
 */
nextc(s)
struct string *s;
{
	int validnull = 0;
	register c, i, n;

	if(NO_NLS || NL_8){   /*  do it this way unless mb chars */
	    c = *s->p++;
	    if(c=='\\') {
		i = n = 0;
		while(i<3 && (c = *s->p)>='0' && c<='7') {
			n = n*8 + c - '0';
			i++;
			s->p++;
		}
		if(i>0) {
		    c = n;
                    if (c == 0) validnull++;  /* a NULL, not end-of-string */
#ifdef DBG
	 	    if (c == 0) printf("NUL character in operand, %s\n",s);
#endif
		}
		else c = *s->p++;
	    }

	    if(c==0) {
		*--s->p = 0;
                if (!validnull) c = MYZERO;	/* indicate end-of-string */
            }
	    return(c);
	}      /*  if NO_NLS  */
	else {			/*  NL_16 */
	    c = CHARADV(s->p);
	    if(c=='\\') {
		i = n = 0;
		while(i<3 && (c = *s->p)>='0' && c<='7') {
			n = n*8 + c - '0';
			i++;
			s->p++;
		}
		if(i>0) {
		    c = n;
                    if (c == 0) validnull++;  /* a NULL, not end-of-string */
#ifdef DBG
	 	    if (c == 0) printf("NUL character in operand, %s\n",s);
#endif
		}
		else c = CHARADV(s->p);
	    }

	    if(c==0) { 
		*--s->p = 0;
                if (!validnull) c = MYZERO;	/* indicate end-of-string */

            }
	    return(c);
	}   /*  else NL_8 or NL_16 */
}


getC(f) FILE *f;	/* Beginning of new routines added for 16 bit */
{
	int c;
	static	peekc = 0;

	if (lookahead) {
		c = lookahead;
		lookahead = 0;
		return(c);
	}
	c = peekc;
	peekc = 0;
	if (!c)
		c = getc(f);
	if (c != EOF && FIRSTof2(c) && SECof2(peekc=getc(f))){
		c = (c<<8 | peekc);
		peekc = 0;
	} 
	return c;
}


squeezeOn(in)
{
	int	bit, index; 
	int	value = in - TABSIZE16;

	index = value / 8;
	bit   = value % 8;	

	switch (bit) {
	case 0 :
		squeez[index].bf.bit0 = 1;
		break;
	case 1 :
		squeez[index].bf.bit1 = 1;
		break;
	case 2 :
		squeez[index].bf.bit2 = 1;
		break;
	case 3 :
		squeez[index].bf.bit3 = 1;
		break;
	case 4 :
		squeez[index].bf.bit4 = 1;
		break;
	case 5 :
		squeez[index].bf.bit5 = 1;
		break;
	case 6 :
		squeez[index].bf.bit6 = 1;
		break;
	case 7 :
		squeez[index].bf.bit7 = 1;
		break;
	default:
		fprintf(stdout, 
			catgets(catd, 1, 3, "error in squeezeOn \n"));
		exit (1);
	}
}

squeeze(value)
{
	int index, out;
	int bit;
	int in = value - TABSIZE16;

	index = in / 8;
	bit   = in % 8; 

	switch (bit) {
	case 0 :
		out = squeez[index].bf.bit0;
		break;
	case 1 :
		out = squeez[index].bf.bit1;
		break;
	case 2 :
		out = squeez[index].bf.bit2;
		break;
	case 3 :
		out = squeez[index].bf.bit3;
		break;
	case 4 :
		out = squeez[index].bf.bit4;
		break;
	case 5 :
		out = squeez[index].bf.bit5;
		break;
	case 6 :
		out = squeez[index].bf.bit6;
		break;
	case 7 :
		out = squeez[index].bf.bit7;
		break;
	default:
		fprintf(stdout, 
			catgets(catd, 1, 4, "error in squeeze map \n"));
		exit (1);
	}

	return (out);
}


#define TOT_ELMT	256		/* total elements in sequence table */
#define CONSTANT	0
#define ONE2		0200
#define DC		0300
#define FLAG	0300
#define OFFSET	077


struct c_u {		/* tmp storage for each collating element */
	int type;		/* CONSTANT, TWO1, ONE2 or DC */
	u_char pri;	/* priority number */
	u_char ch1;	/* 1st char for 2-to-1 or 1-to-2 pair */
	u_char ch2;	/* 2nd char for 2-to-1 or 1-to-2 pair */
	struct c_u *next;	/* pointer to the next collating element */
};

typedef struct c_u ch_unit;

struct seq_unit {		/* counter & coll element list header */
	int cnt;		/* number of elements in this sequence number */
	ch_unit head;		/* dummy header of a coll element list */
} array[TOT_ELMT];

/* coll_sort:
** put all collating element in order
*/
coll_sort()
{
	int i, cnt, flag;
	int seq, seq21;
	int index;

	ch_unit *ps, *ps21, *curp;
	struct col_21tab *p21;
	struct col_12tab *p12;

	/*
	** initialize the sequence array (counter & list header)
	*/
	for (i = 0; i < TOT_ELMT; i++) {
		array[i].cnt = 0;
		array[i].head.type = CONSTANT;
		array[i].head.pri = 0;
		array[i].head.ch1 = 0;
		array[i].head.ch2 = 0;
		array[i].head.next = NULL;
	}

	/*
	** for each character code, get its sequence number and
	** put it in the list of coll elements with the same sequence number
	** keep the list in the order of increasing priority numbers
	*/
	if (!_nl_collate_on || _nl_mb_collate) { 	/* machine collation */
		for (i = 0; i < TOT_ELMT; i++) {
			seq = i;
			ps = (ch_unit *)malloc(sizeof(ch_unit));
			ps->type = CONSTANT;
			ps->pri = 0;
			ps->ch1 = i;
			array[seq].cnt++;
			putseq(seq, ps);
		}
	} else {
	for (i = 0; i < TOT_ELMT; i++) {
		seq = _seqtab[i];
		ps = (ch_unit *)malloc(sizeof(ch_unit));

#ifdef DBG
              if (dbgflag)
                 printf("coll_sort: char = %c (%d)\tseq = %d (0x%x)\n",
                          i, i, seq, seq);
#endif

		flag = _pritab[i] & FLAG;
		index = _pritab[i] & OFFSET;
		switch (flag) {
			case CONSTANT:
				ps->type = CONSTANT;
				ps->pri = index;
			    	ps->ch1 = i;
				break;
			case TWO1:
				ps->type = CONSTANT;
				ps->ch1 = i;
				for (p21 = _tab21 + index;
				     p21->ch1 != ENDTABLE; p21++) {
				    ps21 = (ch_unit *)malloc(sizeof(ch_unit));
				    ps21->type = TWO1;
				    ps21->pri = p21->priority;
				    ps21->ch1 = i;
			 	    ps21->ch2 = p21->ch2;
				    seq21 = p21->seqnum;
				    array[seq21].cnt++;
				    putseq(seq21, ps21);
				}
				ps->pri = p21->priority;
				break;
			case ONE2:
				ps->type = ONE2;
				ps->ch1 = i;
				p12 = _tab12 + index;
				ps->pri = p12->priority;
				break;
			case DC:
				ps->type = DC;
				ps->ch1 = i;
				ps->pri = 0;
				break;
		}
		array[seq].cnt++;
		putseq(seq, ps);
	}
	}
}

/* putseq:
** put one collating element in its corresponding sequence list
** in the order of increasing priority numbers
*/
putseq(seq, ps)
int seq;
ch_unit *ps;
{
	ch_unit *prep, *curp;

	prep = &array[seq].head;
	for (curp = array[seq].head.next; curp != NULL;) {
		if (curp->pri > ps->pri)
			break;
		prep = curp;
		curp = curp->next;
	}
	prep->next = ps;
	ps->next = curp;
}

/* nextcoll:
** return the next collating element in the sequence
*/
nextcoll(c)
int c;
{
	ch_unit *curp;
	int c2 = 0, seq;
	struct col_21tab *p21;

	if (!_nl_collate_on || _nl_mb_collate)	/* 8 or 16 bit machine coll */
	    return (++c);
	
	if (c & MARK21) {		/* 2-to-1 collating element */
	    c2 = c & SECin21;
	    c = (c & FIRSTin21)>>8;
	    for (p21 = _tab21 + (_pritab[c] & OFFSET);
		 p21->ch1 != ENDTABLE; p21++) {
		if ((p21->ch2 == c2)) {		/* found it */
		    seq = p21->seqnum;
		    break;
		}
	    }
	    if (p21->ch1 == ENDTABLE) {
	    		fprintf(stderr, 
				catgets(catd, 1, 5, "Internal collating table error\n"));
	    		exit(1);
	    }
	} else 
	    seq = _seqtab[c];

	if (array[seq].cnt == 0) {
	    	fprintf(stderr, 
			catgets(catd, 1, 6, "Internal collating table error\n"));
	    exit(1);
	}

	for (curp = array[seq].head.next; curp != NULL;
	     curp = curp->next) {
	    if ((!c2 && (curp->ch1 == c)) ||
		(c2 && (curp->ch1 == c) && (curp->ch2 == c2))) { /* found it */
		    if ((curp = curp->next) != NULL) {
			if (curp->type == TWO1)
			    return((curp->ch1<<8) | curp->ch2 | MARK21);
			else
			    return(curp->ch1);
		    } else {
			curp = array[seq+1].head.next;
			if (curp->type == TWO1)
			    return((curp->ch1<<8) | curp->ch2 | MARK21);
			else
			    return(curp->ch1);
		    }
	    }
	}  /* for loop */
}

exp_ctype(class, s)
char *class;
struct string *s;
{
	int i,c, j = 0;
	u_char flag;     /* ctype flag returned by cmatch */
	ch_unit *curp;

	flag = cmatch(class);

	/*  If we're not in a multi-byte or table collated locale,
	 *  we didn't call coll_sort() to set up a sequence list,
	 *  but we don't really need it because everything is in
	 *  machine collating order
	 */
	if(NO_NLS){
	    for(i = 1; i < TOT_ELMT; i++){
		if(isclass(i,flag)){
		    switch(i){
        		case '[':
        		    s->exp[j++] = '\\';
        		    s->exp[j++] = '1';
        		    s->exp[j++] = '3';
        		    s->exp[j++] = '3';
        		    break;
        		case ']':
        		    s->exp[j++] = '\\';
        		    s->exp[j++] = '1';
        		    s->exp[j++] = '3';
        		    s->exp[j++] = '5';
        		    break;
        		case '-':
        		    s->exp[j++] = '\\';
        		    s->exp[j++] = '0';
        		    s->exp[j++] = '5';
        		    s->exp[j++] = '5';
        		    break;
        		case '*':
        		    s->exp[j++] = '\\';
        		    s->exp[j++] = '0';
        		    s->exp[j++] = '5';
        		    s->exp[j++] = '2';
        		    break;
        		case '\\':
        		    s->exp[j++] = '\\';
        		    s->exp[j++] = '\\';
        		    break;
        		default:
        	    	    s->exp[j++] = i;
        		    break;
		    }    /* switch */
		}	/* if(isclass) */
	    }		/* for(i) */
	}		/* if(NO_NLS) */
	
	/*  If using table collation, all chars are in "array" in 
	 *  correct collating order, we go through and pull out the
	 *  ones with needed class membership
	 */
 	else{
    	    for (i = 0; i < TOT_ELMT; i++) {
    	        if (array[i].cnt == 0)
    	            continue;
    	        else {
    	            for (curp = array[i].head.next; 
			 curp != NULL;curp = curp->next) {
    	                if (isclass(curp->ch1, flag) && curp->type != TWO1) {
    	    	            switch (curp->ch1) {
    	    		        case '[':
    	    		            s->exp[j++] = '\\';
    	    		            s->exp[j++] = '1';
    	    		            s->exp[j++] = '3';
    	    		            s->exp[j++] = '3';
    	    		            break;
    	    		        case ']':
    	    		            s->exp[j++] = '\\';
    	    		                s->exp[j++] = '1';
    	    		            s->exp[j++] = '3';
    	    		            s->exp[j++] = '5';
    	    		            break;
    	    		        case '-':
    	    		            s->exp[j++] = '\\';
    	    		            s->exp[j++] = '0';
    	    		            s->exp[j++] = '5';
    	    		            s->exp[j++] = '5';
    	    		            break;
    	    		        case '*':
    	    		            s->exp[j++] = '\\';
    	    		            s->exp[j++] = '0';
    	    		            s->exp[j++] = '5';
    	    		            s->exp[j++] = '2';
    	    		            break;
    	    		        case '\\':
    	    		            s->exp[j++] = '\\';
    	    		            s->exp[j++] = '\\';
    	    		            break;
    	    		        default:
    	    	    	            s->exp[j++] = curp->ch1;
    	    		            break;
    	    	            }	/* switch */
    	    	        }	/* if(isclass) */
    	            }		/* for(curp)  */
    	        }		/* else(array.cnt != 0) */
    	    }			/* for(i) */
	}			/* else (not NO_NLS) */
	strcpy(&(s->exp[j]), s->p);
	s->p = s->exp;

}

/* function called by qsort in exp_ctype */
int
seq_check(c1,c2)
u_char *c1,*c2;
{
	int i;

	if((i = (_seqtab[*c1] - _seqtab[*c2])) != 0)
	    return(i);
	else
	    return(_pritab[*c1] - _pritab[*c2]);
}

exp_eq(ch, s)
int ch;
struct string *s;
{
	int i = 0;
	ch_unit *curp;

	if (!_nl_collate_on || _nl_mb_collate)
		s->exp[i++] = ch;
	else {
		/* Note: ch cannot be a 2-to-1 char for now*/
		if (array[_seqtab[ch]].cnt == 0) {
	    		fprintf(stderr, 
				catgets(catd, 1, 7, "Internal collating table error\n"));
			exit(1);
		}
		for (curp = array[_seqtab[ch]].head.next; curp != NULL;
		     curp = curp->next) {
			if (curp->type != TWO1) {
			    switch (curp->ch1) {
				case '[':
				    s->exp[i++] = '\\';
				    s->exp[i++] = '1';
				    s->exp[i++] = '3';
				    s->exp[i++] = '3';
				    break;
				case ']':
				    s->exp[i++] = '\\';
				    s->exp[i++] = '1';
				    s->exp[i++] = '3';
				    s->exp[i++] = '5';
				    break;
				case '-':
				    s->exp[i++] = '\\';
				    s->exp[i++] = '0';
				    s->exp[i++] = '5';
				    s->exp[i++] = '5';
				    break;
				case '*':
				    s->exp[i++] = '\\';
				    s->exp[i++] = '0';
				    s->exp[i++] = '5';
				    s->exp[i++] = '2';
				    break;
				case '\\':
				    s->exp[i++] = '\\';
				    s->exp[i++] = '\\';
				    break;
				default:
			    	    s->exp[i++] = curp->ch1;
				    break;
			    }
			}
		}
	}
	strcpy(&(s->exp[i]), s->p);
	s->p = s->exp;
}

xfrmcmp(a, b)
int a, b;
{
	u_char tmp[4];
	u_char stra[4], strb[4];

	if (!_nl_collate_on || _nl_mb_collate)
		return(a - b);

	if (a < 256) {
		tmp[0] = a;
		tmp[1] = '\0';
	} else {
		tmp[0] = a>>8;
		tmp[1] = a&0377;
		tmp[2] = '\0';
	}
	strxfrm(stra, tmp, 4);

	if (b < 256) {
		tmp[0] = b;
		tmp[1] = '\0';
	} else {
		tmp[0] = b>>8;
		tmp[1] = b&0377;
		tmp[2] = '\0';
	}
	strxfrm(strb, tmp, 4);

	return(strcmp(stra, strb));
}

char *ctype_class[] = {
	"alpha",
	"upper",
	"lower",
	"digit",
	"xdigit",
	"alnum",
	"space",
	"print",
	"punct",
	"graph",
	"cntrl",
	"ascii",
	"blank",
	""
};

/*  Returns a flag whose value is the necessary bit-wise OR required to
 *  identify the ctype class in __ctype, as defined in ctype.h
 *  These are the same flags used by the is<class> macros, e.g.
 *  isalpha() is defined as (__ctype[__c]&(_U|_L)).
 */
u_char
cmatch(class)
u_char *class;
{
	int i = 0;

	while (*ctype_class[i]) {
		if (strcmp(ctype_class[i], class) == 0) {
			switch (i) {
			    case 0:		/* alpha */
				return(_U | _L);
			    case 1:		/* upper */
				return(_U);
			    case 2:		/* lower */
				return(_L);
			    case 3:		/* digit */
				return(_N);
			    case 4:		/* xdigit */
				return(_X);
			    case 5:		/* alnum */
				return(_U|_L|_N);
			    case 6:		/* space */
				return(_S);
			    case 7:		/* print */
				return(_P|_U|_L|_N|_B);
			    case 8:		/* punct */
				return(_P);
			    case 9:		/* graph */
				return(_P|_U|_L|_N);
			    case 10:		/* cntrl */
				return(_C);
			    case 11:		/* ascii */
				return(0);
			    case 12:		/* blank */
				return(_B);
			}
		}
		i++;
	}
	fprintf(stderr, 
		catgets(catd, 1, 8, "Invalid character class\n"));
	exit(1);
}

/* Isclass() uses the flag found in cmatch to determine a character's
 * membership in the class identified by flag.  The ascii class is
 * treated as a separate case because it is identified by a flag
 * value of 0, which is not useful to AND with characters 
 */
int
isclass(ch,flag)
u_char ch;
u_char flag;
{
	
	if(flag == 0){	/* ascii */
	    if(ch < 128)
		return(1);
	    else
		return(0);
	}
	else
	    return(__ctype[ch] & flag);
}
