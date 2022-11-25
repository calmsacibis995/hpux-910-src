/* @(#) $Revision: 64.1 $ */      
/* LINTLIBRARY */

#include <stdio.h>
#include "global.h"

/* col_init: initialize collate tables.
** Get here when you see a 'sequence' keyword.
*/
void
col_init(token)
int token;				/* keyword token */
{
	extern void rang_num();
	extern void col_exec();
	extern void null_finish();
	extern void two1_init();
	extern void two1_end();
	extern void one2_init();
	extern void one2_end();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}
	
	op = token;			/* save off the token */
	number = rang_num;		/* tell main to do a range on number */
	rang_exec = col_exec;		/* tell rang_num to exectute here */
	finish = null_finish;		/* tell main to finish here */
	l_angle = two1_init;		/* tell main to do a 2-1 on l angle */
	r_angle = two1_end;		/* tell main to do a 2-1 on r angle */
	l_square = one2_init;		/* tell main to do a 1-2 on l square */
	r_square = one2_end;		/* tell main to do a 1-2 on r square */

	left = right = AVAILABLE;	/* range pair available */
	seq_no = 0;			/* initialization */
	max_pri_no = 0;
	two1_len = 0;		
	one2_len = 0;		
	twoi = 0;		
	onei = 0;		
	lp = rp = 0;			/* set up the flags */
	lb = rb = 0;
	lab = rab = 0;
	lcb = rcb = 0;
	dash = FALSE;
}

/* col_exec: execute a collate range.
** Handle 1-1 character ranges here.
*/
void
col_exec()
{
	register int i;

	if (META) error(EXPR);			/* no meta's allowed here */
	for (i=left ; i<=right ; i++) {		/* loop thru range */
		seq_tab[i].seq_no = seq_no++;	/* 1-1 sequence number */
		seq_tab[i].type_info = 0;	/* 1-1 priority */
	}
	left = right = AVAILABLE;		/* range pair available */
}

/* col_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Can't be any meta-chars and finish any implicit ranges.
**
** Also calculate and complete the two1_tab and one2_tab.
*/
void
col_finish()
{
	extern void write_2_1();
	extern void write_1_2();
	int i;

	if (META) error(EXPR);

	if (left != AVAILABLE  &&  right == AVAILABLE)
		rang_num();
	
	/* if there are priority numbers
	**   the lowest sequence number must be greater than
	**   the highest priority number
	** else
	**   the sequence numbers range from 0 to 255
	*/
	if (max_pri_no)
		for (i = 0; i < TOT_ELMT; i++)
			seq_tab[i].seq_no += (max_pri_no + 1);

	write_2_1();
	write_1_2();
}

/* write_2_1: take info. from the sorted linked list of 2-1 characters,
** set appropriate flags and indexes in seq_tab, and
** write info. to two1_tab.
*/
void
write_2_1()
{
	int save_pri;				/* priority of the 1-1 char */
	list *lsp;				/* list search pointer */
	unsigned char type;			/* type_info of seq_ent */

	for (lsp = two1_head ; lsp != NULL ; lsp = lsp->next) {
		if (twoi >= TOT_ELMT / 2)
			Error("too many two-to-one characters");

		/* save the priority only the first time, */
		/* and then set the 2-1 flag and the index to two1_tab */ 
		if (!((type = seq_tab[lsp->first].type_info) & CHARTYPE)) {
			save_pri = type;
			seq_tab[lsp->first].type_info = TWO1 | twoi;
		}

		/* assign values to entries of two1_tab. */
		two1_tab[twoi].reserved = '\0';
		two1_tab[twoi].legal = lsp->second;
		two1_tab[twoi].seq2.seq_no = lsp->seq_no + max_pri_no + 1;
		two1_tab[twoi++].seq2.type_info = lsp->pri_no;

		/* if no more 2-1 relatin is associated with this char, */
		if (lsp->next == NULL || lsp->first != lsp->next->first) {
			two1_tab[twoi].reserved = ENDTABLE;
			two1_tab[twoi++].seq2.type_info = save_pri;
		}
	}
	two1_len = twoi * sizeof(two1);
}

/* write_1_2: take info. from one2_tab itself,
** use already established seq_tab,
** then write seq_no of the 2nd char to one2_tab.
*/
void
write_1_2()
{
	int i;				/* index into one2_tab */

	if (onei >= TOT_ELMT)
		Error("too many one-to-two characters");

	for (i = 0 ; i < onei ; i++) 
		one2_tab[i].seq_no = seq_tab[one2_tab[i].seq_no].seq_no;
	one2_len = onei * sizeof(seq_ent);
}

void
null_finish()
{
}
