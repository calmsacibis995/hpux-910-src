/* @(#) $Revision: 70.7 $ */      
/* LINTLIBRARY */

#include <stdio.h>
#include <collate.h>
#include "global.h"

static last_value = 0;
extern char *getelem();
extern void q_init();
void col_exec();

/*
 * initialize counters
 */
void 
order_init()
{
        left = right = AVAILABLE;	/* range pair available */
	seq_no = 0;			/* initialization */
	max_pri_no = 0;
	two1_len = 0;		
	one2_len = 0;		
	twoi = 0;		
	onei = 0;		
	q_init();
	ellipsis = FALSE;
}

void sort_rule_enter(token,pos)
int token;
int pos;
{

	switch(token){
	case FORWARD:
		sort_rules[pos] |= FORWD; 
		break;
	case BACKWARD:
		sort_rules[pos] |= BACKWD;
		break;
	case POSITION:
		sort_rules[pos] |= POSITION;
		break;
	}
}

void get_entry(token)
int token;
{
unsigned char buf[MAX_COLL_ELEMENT];
int pos =0;

	entry[1].token = 0;
	entry[2].token = 0;
	entry[0].token = token;
	switch(token) {
		case NUMBER:
			entry[pos].value.number = num_value;
			break;
		case COLL_SYM:
			getstr(buf);
			strcpy(entry[pos].value.string, getelem(buf));
			break;
		case ELLIPSIS:
		case UNDEFINED:
			entry[pos].value.number = 0;
			break;
		case EOL:
			break;
		default:
			error(INV_COL_STMT);
			break;
	}

	while ((token = yylex()) !=EOL)
	{
		if (token == SEMI) continue;
		pos++;
		if(pos > COLL_WEIGHTS_MAX) error(WEIGHT_ELLIPSIS);
		entry[pos].token = token;

		switch(token) {
			case NUMBER:
				entry[pos].value.number = num_value;
				break;
			case ELLIPSIS:
				entry[pos].value.number = 0;
				entry[0].token = ELLIPSIS;
			case IGNORE:
				entry[pos].value.number = 0;
				break;
			case COLL_SYM:
				getstr(buf);
				strcpy(entry[pos].value.string,getelem(buf));
				break;
			case STRING:
				getstr(buf);
				strcpy(entry[pos].value.string, buf);
				break;
			default:
				error(INV_COL_STMT);
		}
	}
}

/*
 * The type is passed as an arguement to this function that stores the 
 * information in the structure and takes other actions based on the type of
 * entry
 */
void process_entry(token)
int token;
{
extern void pri_range();
unsigned int value=0;

	switch(entry[0].token)
	{
		case NUMBER:
			value = entry[0].value.number;
			if(ellipsis) {
				right = value-1;
				col_exec();
				ellipsis = FALSE;
			}
			else last_value = value;

			/* if collating-element is defined, it should appear
			 * in the first column.
			 */
			if((entry[1].token == COLL_SYM) ||
			   (entry[2].token == COLL_SYM))
				error(SYM_NOT_FOUND); 
			if(entry[1].token == STRING)
				error(INV_COL_STMT);
			if (entry[1].token==IGNORE || entry[2].token==IGNORE)
			{
				/* process as  DC character : NOTE:
				 * if the following was defined:
				 * 'A'   'A';'A'
				 * 'Z'   IGNORE;'Z'
				 * 'ZZ'  IGNORE;"ZZ"
				 * 'B'   'B';'B'
				 * then AZB = AB, AZZB = AB, but AZB != AZZB 
				 * this makes no sense, so we dont recognize
				 * class equivalence in IGNORE chars.
				 */
				if (left != AVAILABLE && right == AVAILABLE)
					rang_num();
				left = right = AVAILABLE;
				seq_tab[value].seq_no = 0;/* for IGNORE chars */
				seq_tab[value].type_info = DC;
			}
			else {
				if ((entry[1].value.number) != last_class) {
					if(pri_seq) {
						rang_num();
						pri_exec();
						seq_no++;
					}
					/* handle it as standalone */
					rang_exec = col_exec;
					rang_num();
				}
				else /* same class */
				{
					rang_exec = pri_range;
					rang_num();
				}  
			}
			if(entry[2].token == STRING)
			{
				left = value;
				right = entry[2].value.string[1];
						/* assign sec char to right */
				p_1_2exec();
			}
			break;

		case COLL_SYM:
			if(ellipsis) error(INV_COL_STMT);
			if(entry[1].value.number != last_class) {
				rang_exec = pri_range;
				rang_num();
				pri_exec();
				seq_no++;
			}
			if(left !=AVAILABLE && right == AVAILABLE)
				rang_num();
			left = entry[2].value.string[0];
			right = entry[2].value.string[1];
			p_2_1exec();

			if (entry[2].token==IGNORE || entry[1].token==IGNORE) {
				/* process as  DC character on second pass */
				if (left != AVAILABLE && right == AVAILABLE) {
					rang_num();
					left = right = AVAILABLE;
				}
				seq_tab[value].seq_no = 0;/* for IGNORE chars */
				seq_tab[value].type_info = DC;
			}
			break;

		case UNDEFINED:
			if(ellipsis) error(INV_COL_STMT);
			if((entry[1].token != 0) && (entry[2].token != 0))
				error(INV_COL_STMT);
			if(left !=AVAILABLE && right == AVAILABLE)
				rang_num();
			undef_seq = seq_no++;
			break;

		case ELLIPSIS:
			rang_num();
			if(pri_seq) {
				pri_exec();
				seq_no++;
			}
			ellipsis = TRUE;
			left = last_value+1;
			break;

		default:
			error(SYM_NOT_FOUND);
			break;

	} /* end of switch */
	last_class = entry[1].value.number;

}

/* col_exec: execute a collate range.
** Handle 1-1 character ranges here.
*/
void
col_exec()
{
	register int i;


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

	if (left != AVAILABLE  &&  right == AVAILABLE)
		rang_num();

	/* if there are priority numbers
	**   the lowest sequence number must be greater than
	**   the highest priority number
	** else
	**   the sequence numbers range from 0 to 255
	*  God knows why this should happen- i see no reason
	*  to be thrown away later
	*/
	if (max_pri_no) {
		for (i = 0; i < TOT_ELMT; i++)
			/* this if condition added to take care of UNDEFINED chars */
			if(seq_tab[i].seq_no != UNDEF)
				seq_tab[i].seq_no += (max_pri_no + 1);
	/* for UNDEFINED handling... */
	seq_no += max_pri_no + 1;
	}

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
			Error("too many two-to-one characters",2);

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
		Error("too many one-to-two characters",2);

	for (i = 0 ; i < onei ; i++) 
		one2_tab[i].seq_no = seq_tab[one2_tab[i].seq_no].seq_no;
	one2_len = onei * sizeof(seq_ent);
}

