/* @(#) $Revision: 72.1 $ */    
#include "curses.ext"

char *tparm();

struct screen *
set_term(new)
struct screen *new;
{
	register struct screen *rv = SP;

	char *tmp_attrib_str;

#ifdef DEBUG
	if(outf) fprintf(outf, "setterm: old %x, new %x\n", rv, new);
#endif

#ifndef		NONSTANDARD
	SP = new;
#endif		NONSTANDARD

	cur_term = SP->tcap;
	LINES = lines;
	COLS = columns;
	stdscr = SP->std_scr;
	curscr = SP->cur_scr;

        if (set_attributes) {
		if (make_attrstr()) {
                        tmp_attrib_str = tparm(set_attributes, 1, 0, 0, 0, 0, 0, 0, 0, 0);
                        strcpy_attrib(&(attrstr[hashattr(A_STANDOUT)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 1, 0, 0, 0, 0, 0, 0, 0);
                        strcpy_attrib(&(attrstr[hashattr(A_UNDERLINE)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 1, 0, 0, 0, 0, 0, 0);
                        strcpy_attrib(&(attrstr[hashattr(A_REVERSE)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 1, 0, 0, 0, 0, 0);
                        strcpy_attrib(&(attrstr[hashattr(A_BLINK)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 0, 1, 0, 0, 0, 0);
                        strcpy_attrib(&(attrstr[hashattr(A_DIM)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 0, 0, 1, 0, 0, 0);
                        strcpy_attrib(&(attrstr[hashattr(A_BOLD)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 0, 0, 0, 1, 0, 0);
                        strcpy_attrib(&(attrstr[hashattr(A_INVIS)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 0, 0, 0, 0, 1, 0);
                        strcpy_attrib(&(attrstr[hashattr(A_PROTECT)]), tmp_attrib_str);

                        tmp_attrib_str = tparm(set_attributes, 0, 0, 0, 0, 0, 0, 0, 0, 1);
                        strcpy_attrib(&(attrstr[hashattr(A_ALTCHARSET)]), tmp_attrib_str);

		}
        }

	return rv;
}
