/* @(#) $Revision: 70.7 $ */     
#include <stdio.h>
#include "sed.h"

#ifndef hpe
#include <regexp.h>
#else
#include "regexp.h"
#endif

extern	int eargc;
extern	int optind, opterr;

union reptr     *abuf[ABUFSIZE];
union reptr **aptr;
char    ibuf[512];
char    *cbp;
char    *ebp;
char    genbuf[LBSIZE];
char    *lbend;
char	*lcomend;
int     dolflag;
int     sflag;
int     jflag;
int     delflag;
long    lnum;
char    holdsp[LBSIZE+1];
char    *spend;
char    *hspend;
int     nflag;
long    tlno[NLINES];
int     f;
int	numpass;
union reptr     *pending;
char	*trans[040]  = {
	"\\001",
	"\\002",
	"\\003",
	"\\004",
	"\\005",
	"\\006",
/*	"\\007", bel */
	"\\a",
/*	"-<",	 backspace */
	"\\b",
/*	"->",	 tab */
	"\\t",
	"\\n",
/*	"\013",	 vertical tab */
	"\\v",
/*	"\014",	 form-feed */
	"\\f",
/*	"\015",	 carriage-return */
	"\\r",
	"\\016",
	"\\017",
	"\\020",
	"\\021",
	"\\022",
	"\\023",
	"\\024",
	"\\025",
	"\\026",
	"\\027",
	"\\030",
	"\\031",
	"\\032",
	"\\033",
	"\\034",
	"\\035",
	"\\036",
	"\\037"
};
/*	The above table has been changed according to POSIX.2/D11.2 doc. */
char	bksl[] = "\\\\";	/* Added for backslash, POSIX.2/D11.2 */
char	rub[] = "\\177";

execute(file, hend)
char *file;
char *hend;
{
	register char *p1, *p2;
	register union reptr	*ipc;
	int	c;
	char	*execp;

	if (file) {
#ifndef hpe
		if ((f = open(file, 0)) < 0) {
#else
		/* Open with trim of trailing blanks */
		if ((f = open(file, 0 | 020, 0, "T")) < 0) { /* } */
#endif hpe
			fprintf(stderr, "Can't open %s\n", file);
		}
	} else
		f = 0;

	ebp = ibuf;
	cbp = ibuf;

	if(pending) {
		ipc = pending;
		pending = 0;
		goto yes;
	}

	/**********
	* Loops thru each line in the input file.  After one line in 
	* the input file is read, all commands are then performed on that
	* line; then the next input line is read.
	**********/ 
	for(;;) {
		if((execp = gline(linebuf)) == badp) {
			close(f);
			return;
		}
		spend = execp;
		sflag = 0;      /* clear subst. flag when new line read */

		/********** 
		* While there are still commands to perform on this
		* line....
		**********/ 
		for(ipc = ptrspace; ipc->r1.command; ) {

			p1 = ipc->r1.ad1;
			p2 = ipc->r1.ad2;

			if(p1) {   /* If there is an address 1... */

				if(ipc->r1.inar) { 
				/**********
				* We are within a range of ad1 - ad2
				**********/ 

					if(*p2 == CEND) {
					/********** 
					* ad2 is equal to end of file 
					**********/ 
						p1 = 0;
					} else if(*p2 == CLNUM) {
						/********** 
						* ad2 is a line number 
						**********/ 
						c = p2[1];
						if(lnum > tlno[c]) {
						/**********
						* Current line is greater
						* than line number of ad2
						**********/ 
							ipc->r1.inar = 0;
							if(ipc->r1.negfl)
								goto yes;
							ipc++;
							continue;
						}
						if(lnum == tlno[c]) {
							ipc->r1.inar = 0;
						}

					/**********
					* else: ad2 is a regular expression
					**********/ 
					} else if(match(p2, 0)) {
						ipc->r1.inar = 0;
					}

				/**********
				* else:  we are outside the range of ad1 - ad2,
				* i.e., ipc->r1.inar == 0
				**********/ 
				} else if(*p1 == CEND) {
					/********** 
					* ad1 is equal to end of file 
					**********/ 
					if(!dolflag) {
						if(ipc->r1.negfl)
							goto yes;
						ipc++;
						continue;
					}

				} else if(*p1 == CLNUM) {
					/**********
					* ad1 is a line number
					**********/
					c = p1[1];
					if(lnum != tlno[c]) {
						if(ipc->r1.negfl)
							goto yes;
						ipc++;
						continue;
					}
					if(p2)
						ipc->r1.inar = 1;

				/**********
				* ad1 is a regular expression
				**********/ 
				} else if(match(p1, 0)) {
					if(p2) {
					    /**********
					    * If ad2 is a line number, and the
					    * current line is greater than or
					    * equal to ad2, then we are NOT in
					    * a range; otherwise we are.
					    **********/ 
					    if ((*p2 == CLNUM) && (lnum >= tlno[p2[1]]))
						/********** 
						* This is really a no-op; just
						* wanted to be explicit.
						**********/ 
						ipc->r1.inar = 0;
					    else
						ipc->r1.inar = 1;
					}
				/**********
				* else: the reg expr (ad1) does not match
				* the current line
				**********/ 
				} else {
					if(ipc->r1.negfl)
						goto yes;
					ipc++;
					continue;
				}
			}

			/**********
			* If we are to process opposite/negative ranges
			**********/ 
			if(ipc->r1.negfl) {
				ipc++;
				continue;
			}
	yes:
			command(ipc, hend);

			if(delflag)
				break;

			if(jflag) {
				jflag = 0;
				if((ipc = ipc->r2.lb1) == 0) {
					ipc = ptrspace;
					break;
				}
			} else
				ipc++;

		}
		if(!nflag && !delflag) {
			for(p1 = linebuf; p1 < spend; p1++)
				putc(*p1, stdout);
			putc('\n', stdout);
		}

		if(aptr > abuf) {
			arout();
		}

		delflag = 0;

	}
} /* execute */

match(expbuf, gf)
char	*expbuf;
{
	register char   *p1, *p2;

	/* FSDlj09573, FSDlj09462, FSDlj07393, FSDlj09501: */
	/* Added checking for null expbuf, eg: s//foo/, to */
	/* avoid passing 0'ed expbuf to step(), which then */
	/* cause core dump. 				   */
	if (!expbuf) return(0);

	if(gf) {
		if(*expbuf)	return(0);
/*		p1 = linebuf;
		p2 = genbuf;
		while(*p1++ = *p2++);	*/
		locs = p1 = loc2;
	} else {
		p1 = linebuf;
		locs = 0;
	}

	circf = *expbuf++;
	return(step(p1, expbuf));
}

substitute(ipc)
union reptr	*ipc;
{
	if(match(ipc->r1.re1, 0) == 0)	return(0);

	numpass = 0;
	sflag = 0;		/* Flags if any substitution was made */
	dosub(ipc->r1.rhs, ipc->r1.gfl);

	if(ipc->r1.gfl) {
		while(*loc2) {
			if(match(ipc->r1.re1, 1) == 0) break;
			dosub(ipc->r1.rhs, ipc->r1.gfl);
		}
	}
	return(sflag);
}

#ifdef NLS16
#define MRK 0177 	/* mark substitution points and end of
			** translation buffer */

dosub(rhsbuf,n)		/* avoid '&' as 2nd byte of kanji */
char	*rhsbuf;
int	n;
{
	register char *lp, *sp, *rp;
	int c;

/*	if(n > 0 && n < 999)	*/
	/* POSIX.2/D11.2 wants the limit to be LINE_MAX */
	if(n > 0 && n < GOCCUR)	
		{numpass++;
		if(n != numpass) return;
		}
	sflag = 1;
	lp = linebuf;
	sp = genbuf;
	rp = rhsbuf;
	while (lp < loc1)
		*sp++ = *lp++;
	while(c = CHARADV(rp)) {
		if (c == '&') {
			sp = place(sp, loc1, loc2);
			continue;
		} else if (c == MRK && (c = CHARADV(rp)) >= '1' && c < NBRA+'1'){
			sp = place(sp, braslist[c-'1'], braelist[c-'1']);
			continue;
		}
		PCHARADV(c, sp);
		if (sp >= &genbuf[LBSIZE])
			fprintf(stderr, "output line too long.\n");
	}
	lp = loc2;
	loc2 = sp - genbuf + linebuf;
	while (*sp++ = *lp++)
		if (sp >= &genbuf[LBSIZE]) {
			fprintf(stderr, "Output line too long.\n");
		}
	lp = linebuf;
	sp = genbuf;
	while (*lp++ = *sp++);
	spend = lp-1;
}
#else /* 8-bit algorithm */
dosub(rhsbuf,n)
char	*rhsbuf;
int	n;
{
	register char *lp, *sp, *rp;
	int c;

/*	if(n > 0 && n < 999)	*/
	/* POSIX.2/D11.2 wants the limit to be LINE_MAX */
	if(n > 0 && n <= LINE_MAX)	
		{numpass++;
		if(n != numpass) return;
		}
	sflag = 1;
	lp = linebuf;
	sp = genbuf;
	rp = rhsbuf;
	while (lp < loc1)
		*sp++ = *lp++;
	while(c = (*rp++ & 0377)) {
		if (c == '&') {
			sp = place(sp, loc1, loc2);
			continue;

		/* modified for 8 bit handling */
		} else if (c&0200 && (c - 0200) >= 1 && (c - 0200) < NBRA+1) {
			c = c + '0' - 0200;
			sp = place(sp, braslist[c-'1'], braelist[c-'1']);
			continue;
		}
		*sp++ = c;		/* modified for 8 bit */
		if (sp >= &genbuf[LBSIZE])
			fprintf(stderr, "output line too long.\n");
	}
	lp = loc2;
	loc2 = sp - genbuf + linebuf;
	while (*sp++ = *lp++)
		if (sp >= &genbuf[LBSIZE]) {
			fprintf(stderr, "Output line too long.\n");
		}
	lp = linebuf;
	sp = genbuf;
	while (*lp++ = *sp++);
	spend = lp-1;
}
#endif NLS16

char	*place(asp, al1, al2)
char	*asp, *al1, *al2;
{
	register char *sp, *l1, *l2;

	sp = asp;
	l1 = al1;
	l2 = al2;
	while (l1 < l2) {
		*sp++ = *l1++;
		if (sp >= &genbuf[LBSIZE])
			fprintf(stderr, "Output line too long.\n");
	}
	return(sp);
}

command(ipc, hend)
union reptr	*ipc;
char *hend;
{
	register int	i;
	register char   *p1, *p2, *p3;
	char	*execp;


	switch(ipc->r1.command) {

		case ACOM:
			*aptr++ = ipc;
			if(aptr >= &abuf[ABUFSIZE]) {
				fprintf(stderr, "Too many appends after line %ld\n",
					lnum);
			}
			*aptr = 0;
			break;

		case CCOM:
			delflag = 1;
			if(!ipc->r1.inar || dolflag) {
				for(p1 = ipc->r1.re1; *p1; )
					putc(*p1++, stdout);
				putc('\n', stdout);
			}
			break;
		case DCOM:
			delflag++;
			break;
		case CDCOM:
			p1 = p2 = linebuf;

			while(*p1 != '\n') {
				if(*p1++ == 0) {
					delflag++;
					return;
				}
			}

			p1++;
			while(*p2++ = *p1++);
			spend = p2-1;
			jflag++;
			break;

		case EQCOM:
			fprintf(stdout, "%ld\n", lnum);
			break;

		case GCOM:
			p1 = linebuf;
			p2 = holdsp;
			while(*p1++ = *p2++);
			spend = p1-1;
			break;

		case CGCOM:
			*spend++ = '\n';
			p1 = spend;
			p2 = holdsp;
			while(*p1++ = *p2++);
			spend = p1-1;
			break;

		case HCOM:
			p1 = holdsp;
			p2 = linebuf;
			while((p1 <= hend) && (*p1++ = *p2++));
			if (p1 > hend) {
			    fprintf(stderr, "Holding area overflowed.\n");
			    exit(2);
			}
			hspend = p1-1;
			break;

		case CHCOM:
			*hspend++ = '\n';
			p1 = hspend;
			p2 = linebuf;
			while((p1 <= hend) && (*p1++ = *p2++));
			if (p1 > hend) {
			    fprintf(stderr, "Holding area overflowed.\n");
			    exit(2);
			}
			hspend = p1-1;
			break;

		case ICOM:
			for(p1 = ipc->r1.re1; *p1; )
				putc(*p1++, stdout);
			putc('\n', stdout);
			break;

		case BCOM:
			jflag = 1;
			break;


		case LCOM:
			p1 = linebuf;
			p2 = genbuf;
			genbuf[72] = 0;
			while(*p1)
				if(*p1 >= 040 || *p1 & 0200) {
#ifdef NLS16	/* ensure multibyte chars are not broken across lines */
				    if (FIRSTof2(*p1) && SECof2(*(p1+1))) {
					    if (p2+1 >= lcomend) {
						    *p2++ = '\\';
						    *p2 = '\0';
						    fprintf(stdout, "%s\n", genbuf);
						    p2 = genbuf;
					    } else {
						    *p2++ = *p1++;
						    if(p2 >= lcomend) {
							    *p2 = '\\';
							    fprintf(stdout, "%s\n", genbuf);
							    p2 = genbuf;
						    }
					    }
				    } else {
#endif NLS16
					if(*p1 == 0177) {
						p3 = rub;
						while(*p2++ = *p3++)
							if(p2 >= lcomend) {
								*p2 = '\\';
								fprintf(stdout, "%s\n", genbuf);
								p2 = genbuf;
							}
						p2--;
						p1++;
						continue;
					}
	/* POSIX.2/D11.2 wants backslash as \\, so here it is!!! */
					if(*p1 == 0134) {
						p3 = bksl;
						while(*p2++ = *p3++)
							if(p2 >= lcomend) {
								*p2 = '\\';
								fprintf(stdout, "%s\n", genbuf);
								p2 = genbuf;
							}
						p2--;
						p1++;
						continue;
					}
					*p2++ = *p1++;
					if(p2 >= lcomend) {
						*p2 = '\\';
						fprintf(stdout, "%s\n", genbuf);
						p2 = genbuf;
					}
#ifdef NLS16	/* close off the 'else {' we started up above */
				    }
#endif NLS16
				} else {
					p3 = trans[*p1-1];
					while(*p2++ = *p3++)
						if(p2 >= lcomend) {
							*p2 = '\\';
							fprintf(stdout, "%s\n", genbuf);
							p2 = genbuf;
						}
					p2--;
					p1++;
				}
			/*
			 * POSIX.2 (P1003.2/D11.2 section 4.55.7.3)
			 * The end of each line shall be marked with $.
			 */
			*p2++ = '$';
			*p2 = 0;
			fprintf(stdout, "%s\n", genbuf);
			break;

		case NCOM:
			if(!nflag) {
				for(p1 = linebuf; p1 < spend; p1++)
					putc(*p1, stdout);
				putc('\n', stdout);
			}

			if(aptr > abuf)
				arout();
			if((execp = gline(linebuf)) == badp) {
				pending = ipc;
				delflag = 1;
				break;
			}
			spend = execp;

			break;
		case CNCOM:
			if(aptr > abuf)
				arout();
			*spend++ = '\n';
			if((execp = gline(spend)) == badp) {
				pending = ipc;
				delflag = 1;
				break;
			}
			spend = execp;
			break;

		case PCOM:
			for(p1 = linebuf; p1 < spend; p1++)
				putc(*p1, stdout);
			putc('\n', stdout);
			break;
		case CPCOM:
	cpcom:
			for(p1 = linebuf; *p1 != '\n' && *p1 != '\0'; )
				putc(*p1++, stdout);
			putc('\n', stdout);
			break;

		case QCOM:
			if(!nflag) {
				for(p1 = linebuf; p1 < spend; p1++)
					putc(*p1, stdout);
				putc('\n', stdout);
			}
			if(aptr > abuf) arout();
			fclose(stdout);
			exit(0);
		case RCOM:

			*aptr++ = ipc;
			if(aptr >= &abuf[ABUFSIZE])
				fprintf(stderr, "Too many reads after line%ld\n",
					lnum);

			*aptr = 0;

			break;

		case SCOM:
			i = substitute(ipc);
			if(ipc->r1.pfl && nflag && i)
				if(ipc->r1.pfl == 1) {
					for(p1 = linebuf; p1 < spend; p1++)
						putc(*p1, stdout);
					putc('\n', stdout);
				}
				else
					goto cpcom;
			if(i && ipc->r1.fcode)
				goto wcom;
			break;

		case TCOM:
			if(sflag == 0)  break;
			sflag = 0;
			jflag = 1;
			break;

		wcom:
		case WCOM:
			fprintf(ipc->r1.fcode, "%s\n", linebuf);
			break;
		case XCOM:
			p1 = linebuf;
			p2 = genbuf;
			while(*p2++ = *p1++);
			p1 = holdsp;
			p2 = linebuf;
			while(*p2++ = *p1++);
			spend = p2 - 1;
			p1 = genbuf;
			p2 = holdsp;
			while(*p2++ = *p1++);
			hspend = p2 - 1;
			break;

		case YCOM:
#ifdef NLS16		/* varying character sizes, can no longer */
			/* translate in place */
			{
			char tmpbuf[LBSIZE+1];
		        int  c;
		        char * tp = tmpbuf;

			p1 = linebuf;
			p2 = ipc->r1.re1;
			while(c = CHARADV(p1)){
			        c = translate (c, p2);
				PCHARADV(c, tp);
			}
			*tp++ = '\0';
			spend = linebuf + strlen(tmpbuf);
			strcpy(linebuf, tmpbuf);
			break;
			}
#else /* 8-bit algorithm */

			p1 = linebuf;
			p2 = ipc->r1.re1;
			while(*p1 = p2[*(unsigned char *)p1])	p1++;	/* modified for 8 bit data */
			break;
#endif NLS16
	}

}

char	*gline(addr)
char	*addr;
{
	register char   *p1, *p2;
	register	c, i;
	p1 = addr;
	p2 = cbp;
	for (;;) {
		if (p2 >= ebp) {
			if ((c = read(f, ibuf, 512)) <= 0) {
				return(badp);
			}
			p2 = ibuf;
			ebp = ibuf+c;
		}
		if ((c = *p2++) == '\n') {
			if(p2 >=  ebp) {
				if((c = read(f, ibuf, 512)) <= 0) {
					close(f);
					if(optind == eargc) /* last file? */
							dolflag = 1;
				}

				p2 = ibuf;
				ebp = ibuf + c;
			}
			break;
		}
		if(c) {
		        if(p1 < lbend) *p1++ = c;
			}
	}
	lnum++;
	*p1 = 0;
	cbp = p2;

	return(p1);
} /* gline */



/*************
* sed_compile replaces compile in regexp.h for sed.
* This routine was created so that sed will ignore delimiters
* in bracket expressions.
**************/


#if defined (__STDC__)
char *sed_compile(register char *ep, char *endbuf, int seof)
#else
char *sed_compile(ep, endbuf, seof)
register char *ep;
char          *endbuf;
int           seof;
#endif

{

	INIT           /* Dependent declarations and initializations */

        register                c;
        register                eof = seof & 0377;      /* 8 bit     */
        unsigned char           re_buf[RE_BUF_SIZE+2];  /* place to assemble 
							 * incoming regular 
						         * expression  
							 */
        register unsigned char  *re = re_buf;
	unsigned char		*re_bracket_ptr;
	char			*string_bracket_ptr;
	int			found_match;
        int                     err_num;
	int			closing_token;


        if ( ((c = _GETC()) == eof) || (c == '\n') ) {
                if(c == '\n') 
                        UNGETC(c);                  /* don't eat the newline */
                if (*ep == 0) 
                        RETURN(ep);
        }

        circf = nbra = 0;

        if (c == '^')                    /* anchor at the beginning of line? */
                circf++;
        else
                UNGETC(c);

       /***********
        * Copy remainder of RE input to RE buffer (for passing to regcomp).
	* This is where compile [in regexp.h] and this procedure really differ. 
	* The goal here is to determine the end of the regular expression
	* and pass it to regcomp later.  Regcomp can determine the validity
	* of the regular expression.
 	***********/

        while ( ((c = _GETC()) != eof) && (c != '\n') )   /* begin outer while loop */
        {
            *re++ = c;

	    switch (c) 
	    {
            case '\\' :   /* handle quoted characters */

                    c = _GETC();

                    if (c == 'n')         /* '\' 'n' is converted to '\n' */
                        *(re-1) = '\n';
                    else if (c == '\n') {      /* '\' '\n' is not allowed */
                        ERROR(REG_ENEWLINE);
                    } else
                        *re++ = c;

		    break;


	    case '[' :   /* handles bracket expression */

		    if (_PEEKC() == ']')   /* Ignore ']' in first position. */
			*re++ = _GETC();
		    else if (_PEEKC() == '^') 
		    {
			*re++ = _GETC();
			if (_PEEKC() == ']')
			    *re++ = _GETC();
		    }
			

		    /*************
		    * We are in a bracket expression.
		    * Stay in this loop until we hit the end of
		    * a bracket expression.
		    *************/
		    while ((c = _GETC()) != '\n')
		    {
			*re++ = c;

			if (c == ']') 
			    break;   /* No longer in bracket expression */

			else if (c == '[') 
			{
			    switch (_PEEKC()) 
			    {
			    case '.' :    
			    case '=' :   
			    case ':' :   
				    /*********
				     * Looking for '.]' or '=]' or ':]' in
				     * the input buffer. If I find one, I'll 
				     * assume I have a collating symbol [.x.],
				     * an equiv class [=x=] or a char class 
				     * [:xxxx:], and regcomp can check it's 
				     * validity.  If I don't find a '.]' or
				     * '=]' or ':]', I'll back up the buffer
				     * pointers to the first char after '[', 
				     * and assume '[' was not a 
				     * special character to begin with. 
				     *********/

				    re_bracket_ptr = re;
				    string_bracket_ptr = sp;
				    c = _GETC();
				    *re++ = c;
				    closing_token = c;
			 	    found_match = 0;		
				
				    while (((c = _GETC()) != '\n') && (!found_match) && (c) ) 
				    {
					*re++ = c;

            			        if (re > &re_buf[RE_BUF_SIZE]) 
                    			    ERROR(REG_ESPACE);              
				        if (FIRSTof2(c) && SECof2(_PEEKC()))
					    *re++ = _GETC();

					if ((c == closing_token) && (_PEEKC() == ']')) {
					    /* Found .], =], or :] 
					     * Eat ']'.  
					     */
					    *re++ = _GETC();
					    found_match = 1;
					    break;
					}
				    }  /* end while */						
			
				    if (!found_match) {	
				       /* A [.x.], [=x=], or [:xxxx:] was not
					* found.  Assume '[' was not special.
					* Back buffer pointers up to '['.
					*/
				        re = re_bracket_ptr;
				        sp = string_bracket_ptr;
				    } 

				    continue;  
				    
			    default:
				    ;    /* do nothing */
			    }            /* end switch (_PEEKC()) */
			}                /* end else if */


		        /* traditional to handle no delimiter as */
		        /* out of space error. */

            	        if (!c || re > &re_buf[RE_BUF_SIZE]) 
			    /* Never found closing ']' */
                    	    ERROR(REG_ESPACE);              

            	        if (FIRSTof2(c) && SECof2(_PEEKC()))
                    	    *re++ = _GETC();

		    }        /* end while "in bracket expression" */

		    break;   /* end case '[' */

	    default:
		    ;        /* do nothing */
	    }                /* end switch (c) */



	    /* traditional to handle no delimiter as */
	    /* out of space error		     */

            if (!c || re > &re_buf[RE_BUF_SIZE]) { 
                    ERROR(REG_ESPACE);            
	    }					     

            if (FIRSTof2(c) && SECof2(_PEEKC()))
                    *re++ = _GETC();

        }   /* end outer while loop */


        /******
	* At the end of the regular expression.
	*******/

        *re = '\0';


        if(c == '\n') {
            UNGETC(c);                /* don't eat the newline */
	    		/* sed doesn't like RE's that end with newline */
            ERROR(REG_ENEWLINE);	
        }

#ifdef __hp9000s800
        __preg = (regex_t *)((int)(ep+7)&~3);
#else /* __hp9000s800 */
        __preg = (regex_t *)(ep+4);
#endif /* __hp9000s800 */

        __preg->__c_re    = (unsigned char *)__preg+sizeof(regex_t);
        __preg->__c_buf_end = (unsigned char *)endbuf;

/*
 * DTS: DSDe406288
 * Take out the flag REG_NEWLINE in regcomp(), so that a newline character 
 * is treated as an ordinary character by step().
 * See also DTS UCSqm00653 and UCSqm00593
 */
        if ( err_num = regcomp(__preg, (const char *)re_buf, _REG_NOALLOC) ) 
                ERROR(err_num);

        nbra = __preg->__re_nsub;                    /* count of subexpressions */
        *ep=1;    /* so that we don't get an ERROR when compiling null string */

        RETURN((char *)__preg->__c_re_end);          /* return ptr to end of 
						    * compiled RE + 1 
						    */
}


char *comple(x1, ep, x3, x4)
char *x1, *x3;
char x4;
register char *ep;
{
        register char *p;

        p = sed_compile(ep + 1, x3, x4);
        if(p == ep + 1)
                return(ep);
        *ep = circf;
        return(p);
}


arout()
{
	register char   *p1;
	FILE	*fi;
	char	c;
	int	t;

	aptr = abuf - 1;
	while(*++aptr) {
		if((*aptr)->r1.command == ACOM) {
			for(p1 = (*aptr)->r1.re1; *p1; )
				putc(*p1++, stdout);
			putc('\n', stdout);
		} else {
#ifndef hpe
			if((fi = fopen((*aptr)->r1.re1, "r")) == NULL)
#else
			/* Open with trim of trailing blanks */
			if((fi = fopen((*aptr)->r1.re1, "r T")) == NULL)
#endif hpe
				continue;
			while((t = getc(fi)) != EOF) {
				c = t;
				putc(c, stdout);
			}
			fclose(fi);
		}
	}
	aptr = abuf;
	*aptr = 0;
}

#ifdef NLS16
translate (c, pp) char *pp;	/* scan variable length tranlsation buffer */
{
        unsigned char *p = (unsigned char *) pp;
	int b1, b2;

	b1 = (c>>8) & 0377;
	b2 = c & 0377;
	while (*p != MRK){
	        if (*p == b1 && *(p+1) == b2)
		        return (*(p+2)<<8) | *(p+3);
		p += 4;
	}
	return c;
}
#endif NLS16
