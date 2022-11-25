/* @(#) $Revision: 70.3 $ */     
/* LINTLIBRARY */

#include <stdio.h>
#include <nl_ctype.h>
#include "global.h"
#include <collate.h>

#define FLAG	0300
#define OFFSET	077

/* included collate.h and removed the struct here */
extern	unsigned char	 *_seqtab;	/* dictionary sequence number table */
extern	unsigned char	 *_pritab;	/* 1to2/2to1 flag + priority table */
extern	struct col_21tab *_tab21;	/* 2-to-1 mapping table		*/
extern	struct col_12tab *_tab12;	/* 1-to-2 mapping table		*/

typedef struct ch_unit {	/* tmp storage for each collating element */
	int type;		/* CONSTANT, TWO1, ONE2 or DC */
	unsigned char pri;	/* priority number */
	unsigned char ch1;	/* 1st char for 2-to-1 or 1-to-2 pair */
	unsigned char ch2;	/* 2nd char for 2-to-1 or 1-to-2 pair */
	struct ch_unit *next;	/* pointer to the next collating element */
} ch_unit;

struct seq_unit {		/* counter & coll element list header */
	int cnt;		/* number of elements in this sequence number */
	ch_unit head;		/* dummy header of a coll element list */
} array[TOT_ELMT+1];
int sort_index=0;

/* dcoll:
** dump LC_COLLATE buildlang section
*/
dcoll()
{
	int i, cnt, flag;
	int seq, seq21;
	int index;
	unsigned char eq_char[3];

	ch_unit *ps, *ps21, *curp;
	struct col_21tab *p21;
	struct col_12tab *p12;

	/*
	** initialize the sequence array (counter & list header)
	*/
	for (i = 0; i < TOT_ELMT+1; i++) {
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
	for (i = 0; i < TOT_ELMT; i++) {
		seq = _seqtab[i];
		ps = (ch_unit *)malloc(sizeof(ch_unit));

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
				ps->ch2 = p12->seqnum;
				ps->pri = p12->priority;
				break;
			    case DC:
				ps->type = DC;
				ps->ch1 = i;
				ps->pri = 0;
				seq = TOT_ELMT;	/* store DC char separately */
				break;
		}
		array[seq].cnt++;
		putseq(seq, ps);
	}

	/*
	 * Make initial walkthrough to find two-to-one chars and
	 * fix up "collating-element" entries for them.
	 */
	cnt=0;
	for (i = 0; i < TOT_ELMT; i++) {
		if (array[i].cnt == 0)
			continue;
		if (array[i].cnt == 1) {
			cnt++;
			if(array[i].head.next->type == TWO1)
			    make_elem(array[i].head.next);
		} else {
			for (curp = array[i].head.next; curp != NULL;
			     curp = curp->next) {
				cnt++;
				if(curp->type == TWO1)
			    	    make_elem(curp);
			}
		}
	}
	printf("\n\n");

	/*
	** in increasing sequence numbers,
	** for each list of the same sequence number,
	** print each collating element
	*/
	printf("order_start\t");
	for (sort_index =0; sort_index <= COLL_WEIGHTS_MAX; sort_index++)
	  {
	    if( sort_rules[sort_index] == FORWD)
	      printf("forward; ");
	    if( sort_rules[sort_index] == BACKWD)
	      printf(" backward; ");
    	    if( sort_rules[sort_index] == POSITION)
	      printf(" position; ");
	  }
	printf("\n");
	    
	cnt = 0;
	for (i = 0; i < TOT_ELMT; i++) {
		if (array[i].cnt == 0)
			continue;
		if (array[i].cnt == 1) {
			eq_char[0] = 0;
			prtseq(array[i].head.next,eq_char);
		} else {
			/*
			 * PROTO note: this is pretty badly hacked up.
			 * It turns out that in some cases, an equivalence
			 * class lead character can be a ONE2.  The check
			 * on type takes care of that case, but may not
			 * be sufficiently general.
			 */
			eq_char[0] = array[i].head.next->ch1;
			if(array[i].head.next->type != ONE2){
			    eq_char[1] = array[i].head.next->ch2;
			    eq_char[2] = 0;
			}
			else
			    eq_char[1] = 0;
			for (curp = array[i].head.next; curp != NULL;
			     curp = curp->next) {
				prtseq(curp,eq_char);
			}
		}
	}

	cnt = 0;
	if (array[TOT_ELMT].cnt != 0) {
		for (curp = array[i].head.next; curp != NULL;
		     curp = curp->next) {
			prtseq(curp,eq_char);
		}
    	        cnt = 0;
	}
	printf("\n");
	printf("order_end\n");
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

/* prtseq:
** print out one collating element
*/
prtseq(ps,eq_char)
ch_unit *ps;
unsigned char *eq_char;
{
	unsigned char ch;
	ch_unit *tmp;

	switch (ps->type) {
		case CONSTANT:
			pcode(ps->ch1);
			printf("\t");
			if(*eq_char)
			    if(strlen(eq_char) > 1){
			        printf("<%s>;",eq_char);
			    }
			    else{
				pcode(*eq_char);
				printf(";");
			    }
			pcode(ps->ch1);
			printf("\n");
			break;
		case TWO1:
		    	printf("<%c%c>\t",ps->ch1,ps->ch2);
			if(*eq_char)
			    printf("<%s>;",eq_char);
			printf("<%c%c>\n",ps->ch1,ps->ch2);
			break;
		case ONE2:
	/*
	 * PROTO note:  I'm not sure how robust this is: it assumes that
	 * a one-to-2 character is a member of an equivalence class whose
	 * first character is a single element.  Thus the string that
	 * the ONE2 collates to can be represented by concatenating the
	 * eq_char (the equiv. class char) with the second character, found
	 * in array[ps->ch2].head.next->ch1.  
	 */ 
			pcode(ps->ch1);
			printf("\t");
			if(*eq_char){
			    pcode(*eq_char);
			    printf(";");
			}
			tmp = array[ps->ch2].head.next;
			ch = tmp->ch1;
			if ( islower(ps->ch1) )
			   printf("\"%c%c\"\n",tolower(*eq_char),tolower(ch));
			else
			   printf("\"%c%c\"\n",*eq_char,ch);
			break;
		case DC:
			pcode(ps->ch1);
			printf("\tIGNORE;");
			pcode(ps->ch1);
			printf("\n");
			break;
		default:
			break;
	}
}

/* mkstr: make/process one character string.
** Take string from buf, add double quote '"' characters around it,
** check those characters should be escaped: '\', '"', tab, newline, etc.
** return a pointer to the resulting character string.
*/
unsigned char *
mkstr(buf)
unsigned char *buf;
{
	static unsigned char result[MAX_INFO_MSGS+20];
	unsigned char *p1, *p2, c;

	p1 = buf;
	p2 = result;
	*p2++ = '"';

	while (c = *p1++) {
		if (FIRSTof2(c)) {
			if (xisprint(c) || xfirstof2(c))
				*p2++ = c;
			else {
				*p2++ = '\\';
				sprintf(p2, "%.3o", c);
				p2 += 3;
			}
			c = *p1++;
			if (xisprint(c) || xsecof2(c))
				*p2++ = c;
			else {
				*p2++ = '\\';
				sprintf(p2, "%.3o", c);
				p2 += 3;
			}
		continue;
		}
		switch (c) {
			case '"':		/* '"' */
				*p2++ = '\\';
				*p2++ = '"';
				break;
			case '\\':		/* '\' */
				*p2++ = '\\';
				*p2++ = '\\';
				break;
			case '\n':		/* '\n' */
				*p2++ = '\\';
				*p2++ = 'n';
				break;
			case '\t':		/* '\t' */
				*p2++ = '\\';
				*p2++ = 't';
				break;
			case '\b':		/* '\b' */
				*p2++ = '\\';
				*p2++ = 'b';
				break;
			case '\r':		/* '\r' */
				*p2++ = '\\';
				*p2++ = 'r';
				break;
			case '\f':		/* '\f' */
				*p2++ = '\\';
				*p2++ = 'f';
				break;
			default :		/* regular char */
				if (xisprint(c))
					*p2++ = c;
				else {
					*p2++ = '\\';
					sprintf(p2, "%.3o", c);
					p2 += 3;
				}
				break;
		}
	}
	*p2++ = '"';
	*p2 = NULL;
	return(result);
}

/*
 *  Print a "collating-element" entry for TWO1 characters
 */
int
make_elem(ps)
ch_unit *ps;
{

 	printf("collating-element\t<%c%c> from \"%c%c\"\n",
		ps->ch1,ps->ch2,ps->ch1,ps->ch2);

}
