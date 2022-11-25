


#include <stdio.h>
#include <nlinfo.h>

extern int _nl_errno;

short nljudge(langid, instr, length, judgeflag, err, charset)
short langid, length;
unsigned char *instr, *judgeflag;
char *charset;
unsigned short err[];
{
	char	*pcharset;
	unsigned char *einstr;
	extern struct l_info *getl_info();
	struct l_info *n;
        short  count;

	count = err[0] = err[1] = (short) 0;

	if (length <=0) {
		err [0] = E_INVALIDLENGTH;
		return (0);
        }

	/*
	 * if the user didn't supply the table:
	 *	allocate space for it,
	 *	and load it with nlinfo().
	 */
	if ((pcharset = charset) == NULL){
		_nl_errno = 0;
		n = getl_info(langid);
		if ((n==NULL) || (_nl_errno)) {
		     err [0] = E_LNOTCONFIG;
		     return (0);
                }
		pcharset = n->char_set_definition;
	}

	/*
	 * 	step through instr and define each character
	 */
	for (einstr = instr + length ; instr < einstr; ){
		switch (pcharset[(unsigned char)*instr]){
			case NLI_FIRSTOF2:
				count++;
				*judgeflag++ = '1';
				instr++;
				if (instr < einstr) *judgeflag++ = '2';
				instr++;
				break;
			default:
				*judgeflag++ = '0';
				instr++;
				break;
		}
	}
	judgeflag--;   /* reset pointer to the last character */
	if (*judgeflag == '1') {
		/* Truncation of character in instr */
		*judgeflag = '3';
		err [0] = 7;  /* Invalid character found */
        }

	return (count);
}

