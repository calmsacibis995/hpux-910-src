/* @(#) $Revision: 70.1 $ */   
char *strcat(), *strcpy();
char *convert_reg();

/*
 * parse_movem_arg:
 *
 * Convert a symbolic argument of the form
 *
 *  d0-d3/a3/a5
 *
 * into an integer bitmask of the form
 *
 *  a7 a6 a5 a4 a3 a2 a1 a0 d7 d6 d5 d4 d3 d2 d1 d0
 *
 *  or return -1 if we couldn't parse it.
 */
int
parse_movem_arg(s)
register char *s;
{
	register int mask, reg,reg2;

	mask = 0;			/* registers so far */
	
	while (*s!='\0') {
		reg = checkreg(s);	/* Try to parse a register */
		if (reg==-1)		/* If no register: */
			return (-1);	/* Retreat like the failure we are. */
		s+=3;			/* Skip the register */
		mask |= 1<<reg;		/* Or in that bit */


		if (*s=='-') {			/* If we have a range:	*/
			s++;			/* skip the dash	*/
			reg2 = checkreg(s);	/* try to get 2nd reg	*/
			if (reg2==-1)		/* If 2nd reg not found	*/
				return (-1);	/* exit defeated	*/
			s+=3;			/* Skip the register	*/
			if (reg2<reg)
				internal_error("parse_movem_arg: sequence %s-%s encountered",
					convert_reg(reg), convert_reg(reg2));
			
			for ( ; reg<=reg2; reg++)
				mask |= 1<<reg;
		}

		if (*s=='/')		/* If we have a slash seperator: */
			s++;		/* Skip the slash */
		else if (*s!='\0')	/* if not end-of-string */
			return (-1);	/* then it's not parseable */
	}

	return mask;
}


