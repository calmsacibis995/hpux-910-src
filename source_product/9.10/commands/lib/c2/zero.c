/* @(#) $Revision: 70.1 $ */    
#include "o68.h"

#define IS_DIRECTIVE(op) ((op)==TEXT || (op)==DATA || (op)==BSS)

node *insert();
node *release();

/*
 *  Move zero data to bss segment.
 */
movezero()
{
	register node *p, *start, *dc_begin, *the_end;
	register int op, size;
	register int segment_type;

	if (first.forw == NULL)			/* is there any code at all? */
		return;				/* if no code, give up now */

	if (!data_to_bss)			/* are we allowed to do this? */
		return;				/* exit if shouldn't do this */

	segment_type = UNKNOWN;

	for (p = first.forw; p!=NULL; p = p->forw) {
		op = p->op;
		if (IS_DIRECTIVE(op))
			segment_type=op;
		else 
			continue;
		if (segment_type!=DATA)
			continue;

		p = p->forw;
		if (p == NULL) return;
		op = p->op;
		if (!(op==LABEL || op==DLABEL))
			continue;

		/*
		 * If we have just labels and dc.l 0's following us
		 * until the next label or segment directive,
		 * put us into the bss segment.
		 */
		
		start = p;

		/* skip labels */
		while (p->op == LABEL || p->op == DLABEL && p->forw!=NULL)
			p=p->forw;

		/* skip dc's */
		dc_begin = p;
		size=0;
		while (p->op == DC && p->mode1==ABS_L && p->type1==STRING 
		&& (equstr("0", p->string1)))
		{
			switch(p->subop) {
			case BYTE: size+=1; break;
			case WORD: size+=2; break;
			case LONG: size+=4; break;
			default: internal_error("movezero: subop %d", p->subop);
			}
			p=p->forw;
			if (p == NULL) break;
		}

		if (size==0)			/* nothing to do? */
			continue;

		if (p!=NULL) 
		{
			if (!(IS_DIRECTIVE(p->op) || p->op==END
			|| p->op==LABEL || p->op==DLABEL))
				continue;
			the_end = p;
		}
		
		/* ok, let's do it. */
		insert(start, BSS);		/* Put BSS before the label */
		if (p!=NULL) 			/* Back into DATA-land */
			insert(the_end, DATA);

		/* put in the statement */
		p = insert(dc_begin, DS);
		p->mode1 = ABS_L;
		p->type1 = INTVAL;
		p->addr1 = size;

		/* remove the dc's */
		while(dc_begin->op==DC) {
			p = dc_begin;
			dc_begin = dc_begin->forw;
			release(p);
		}
	}
}
