/* @(#) $Revision: 70.1 $ */   
#include "o68.h"

/*
 *	parse(string, node_ptr): parse arguments into the node
 */

parse(str, nodep)
register char *str;
register node *nodep;
{
	char *copy(), *get_arg(), *parse_bit_fld();
	char buf[256];

	/* Skip whitespace before arguments */
	while (*str==' ' || *str=='\t')
		str++;
	
	/*
	 * Comments can be anywhere after the operands.
	 */
	if (*str=='#' || *str==';')
		return;

	/* For unknown opcodes, just copy the string */
	if (nodep->op == UNKNOWN) {
		nodep->type1 = STRING;		/* strange operand */
		nodep->string1 = copy(str);  /* the operands */
		return;
	}

	/* parse the operands */
	str=get_arg(str, buf);		/* get first string into buf */
	if (*buf!='\0') {			/* if first arg available */
		parse_arg(buf, &nodep->op1);	/* parse first arg */
#ifdef M68020
		if (*str=='{')	/* bit fld operand */
			str=parse_bit_fld(str, nodep);
#endif
		if (*str==',')		/* comma between operands? */
			++str;		/* skip the comma */
		str=get_arg(str, buf);	/* get 2nd string into buf */
		if (*buf!='\0')			/* if second arg available */
			parse_arg(buf, &nodep->op2);  /* parse second arg */
#ifdef M68020
		if (*str=='{')	/* bit fld operand */
			str=parse_bit_fld(str, nodep);
#endif
	}
}


/*
 * Get an argument from arg into buf.
 * Update arg to point past the argument
 */
char *get_arg(arg, buf)
register char *arg;
register char *buf;
{
	register paren_count=0;			/* count of nested parens */
	register char ch;

	/* get the argument */
	/* blanks, tabs, newlines, and comments '#' delimit arguments */
	/* so does a comma if the parenthesis are balanced! */
	while ((ch= *arg)!='\n' && ch!='{' && ch!=' ' 
		&& ch!='\t' && ch!='#' && ch!=0) {
		if (ch==',' && paren_count==0)
			break;
		else if (ch=='(')
			paren_count++;
		else if (ch==')')
			paren_count--;
		*buf++ = ch;
		arg++;
	}
	if (paren_count!=0)
		internal_error("unbananced parenthesis in %s", input_line);
	*buf = '\0';				/* terminate string */
	return(arg);
}


parse_arg(p, arg)
register char *p;
register argument *arg;
{
	register char *pbuf;
	register char r;
	register int immed_flag, word_flag, negative;
	char buf[256];

	/* set up defaults */
	arg->mode = UNKNOWN;
	arg->reg = -1;
	arg->index = -1;
	arg->type = UNKNOWN;
	arg->labno = 0;
	arg->string = NULL;


	/* check for simple register */
	r = checkreg(p);
	if (r >= 0)
	{
	if (isAD(r) && p[3]=='\0') {
		arg->reg = r;
		arg->mode = ((r<reg_a0) ? DDIR:ADIR);	/* either a/d reg */
		return;
	}

#ifdef M68020
	/* check for floating point register (68881) */
	if (isF(r) && p[4]=='\0') {
		arg->reg = r;
		arg->mode = FREG;
		return;
	}

#ifdef DRAGON
	/* check for floating point register (dragon) */
	if (isFP(r)) {
		arg->reg = r;
		arg->mode = FPREG;
		return;
	}
#endif DRAGON

	/* check for register pair %dn:%dm  */
	if (isD(r) && p[3]==':') {
		arg->reg = r;
		arg->mode = DPAIR;
		r = checkreg(p+4);
		if (!isD(r)) goto noparse;
		arg->index = r;
		return;
	}
#endif M68020
	}

	/* check for (%a0) or (%a0)+ */
	if (p[0]=='(' && p[4]==')' )
	{
	   r=checkreg(p+1);
	   if (isA(r)) 
	   {
		/* check for (%a0) */
	   	if (p[5]=='\0') {
		   arg->reg = r;
		   arg->mode = IND;		/* register indirect mode */
		   return;
	      	}

	   	/* check for (%a0)+ */
	   	/* r is still set from above */
		if (p[5]=='+' && p[6]=='\0') {
		   arg->reg = r;
		   arg->mode = INC;	/* register postincrement */
		   return;
		}

	  }
	}

	/* check for -(%a0) */
	if (p[0]=='-' && p[1]=='(' && p[5]==')' && p[6]=='\0') {
		r=checkreg(p+2);
		if (isA(r))
		{
		  arg->reg = r;
		  arg->mode = DEC;			/* predecrement mode */
		  return;
		}
	}

#ifdef M68020
	/* is it a 68020 addressing mode */
	if (*p=='(') {
		parse_new_modes(p, arg);
		return;
	}
#endif
		
	/* is it an immediate operand? */
	if (*p=='&') {
		p++;					/* skip # */
		immed_flag = 1;
	}
	else
		immed_flag = 0;


	/* get displacement/abs_w/abs_l/immediate into buf */
	for (pbuf=buf; *p!='\0' && *p!='(' && *p!='.'; )
		*pbuf++ = *p++;
	*pbuf = '\0';
	pbuf = buf;				/* get a pointer into buf */

	/* handle leading sign */
	if (*pbuf=='+')
		++pbuf;

	negative=false;
	if (*pbuf=='-') {
		++pbuf;
		negative=true;
	}

	/* put buf into value somehow */

#ifdef DEFOUT
	/* Assume : code gen will not have char or octal constants */

	/* is it a character? */
	if (pbuf[0]=='\'' && pbuf[1]!='\0' && pbuf[2]=='\'') {
		arg->addr = pbuf[1];
		if (negative)
			arg->addr *= -1;
	}

	/* is it an octal number? */
	else if (*pbuf=='0' && only(pbuf+1, "01234567")) {
		sscanf(pbuf, "%o", &arg->addr);
		if (negative)
			arg->addr *= -1;
		arg->type = INTVAL;
	}
#endif DEFOUT

	/* is it a hexadecimal number? */
	if (*pbuf=='0' && (pbuf[1]=='x' || pbuf[1]=='X')
	&& only(pbuf+2, "0123456789abcdefABCDEF")) {
		sscanf(pbuf+2, "%x", &arg->addr);
		if (negative)
			arg->addr = -arg->addr;
		arg->type = INTVAL;
	}

	/* is it a compiler-generated label (L2345) ? */
	else if (*pbuf=='L' && only(pbuf+1, "1234567890")) {
		arg->labno = atoi(pbuf+1);
		arg->type = INTLAB;
	}

	/* is it a decimal number? */
	else if (isdigit(*pbuf) && only(pbuf, "0123456789")) {
		sscanf(pbuf, "%d", &arg->addr);
		if (negative)
			arg->addr =  -arg->addr;
		arg->type = INTVAL;
	}

	/* it's a string */
	else {
		arg->type = STRING;
		arg->string = copy(buf);
	}

	/* Is it sized? */
	if (p[0]=='.') 
	{
	   if (p[1]=='w') {		/* followed by .w ? */
		p+=2;
		word_flag=true;
	   } else if (p[1]=='l') {	/* followed by .l ? */
		p+=2;
		word_flag=false;
	   } 
	}
	else					/* Not followed by anything */
		word_flag=false;

	/* what followed the thing? */
	if (*p=='\0') {				/* nothing follows */
		if (immed_flag)			/* of the form #<thing> */
			arg->mode = IMMEDIATE;
		else
			arg->mode = word_flag ? ABS_W : ABS_L;
		return;
	}

	/*
	 * what's left is:
	 *
	 *	34(a0)			ADISP
	 *	34(a0,d3) or 34(a0,a2)	AINDEX
	 *	34(pc)			PDISP
	 *	34(pc,d3) or 34(pc,a2)	PINDEX
	 */

	if (*p++ != '(')
		goto noparse;

	/* see if an a-register or the pc follows */
	r=checkreg(p);
	if ( !isA(r) && r!=reg_pc)		/* must be address reg or pc */
		goto noparse;

	arg->reg = r;
	p+=3;				/* skip past register/pc */


	/* is it simple PC/A-reg displacement? */
	if (p[0]==')' && p[1]=='\0') {	/* 34(a0) or 34(pc) */
		arg->mode = r==reg_pc ? PDISP : ADISP;
		return;
	}



	/* we're either PC indexed or A-register indexed */
	arg->mode = isPC(r) ? PINDEX : AINDEX;

	if (*p++ != ',')
		goto noparse;

	/* what follows is an index register */
	r=checkreg(p);
	if (!isAD(r))			/* if not dn or an */
		goto noparse;


	arg->index = r;
	p+=3;				/* skip past reg */

	if (p[0]=='.' && p[1]=='w') {
		p+=2;			/* skip .w */
		arg->word_index=true;
	}
	else if (p[0]=='.' && p[1]=='l') {
		p+=2;
		arg->word_index=false;
	}
	else
		arg->word_index=true;	/* default is word */

#ifdef M68020
	arg->scale = '1';
	if (p[0]=='*')
	{
		arg->scale = p[1];
		p+=2;
	}
#endif
	if (p[0]==')' && p[1]=='\0')
		return;			/* success */

noparse:
	/* can't parse it */
	internal_error("Parsing fails at '%s' \n input_line='%s'",
				p, input_line);
}

#ifdef M68020
/* While removing the code for mempre, mempost, pcpre and pcpost
   it was getting too cumbersome to have this routine with ifdef
   PREPOST. Hence, I am changing it to remove the pre/post code.
   If we need the other code, it will always be in sccs.
 */
parse_new_modes(p, arg)
register char *p;
register argument *arg;
{
	register char *pbuf;
	register int r, immed_flag, word_flag, negative;
	char buf[256];

	if (*++p != '%') /* base disp is present */
	{
	/* STORE BASE DISP */
	/* get displacement into buf */
	for (pbuf=buf; *p!=','&& *p!='\0'; )
		*pbuf++ = *p++;
	*pbuf = '\0';
	pbuf = buf;				/* get a pointer into buf */

	/* handle leading sign */
	if (*pbuf=='+')
		++pbuf;

	negative=false;
	if (*pbuf=='-') {
		++pbuf;
		negative=true;
	}

	/* put buf into value somehow */

#ifdef DEFOUT
	/* Assume : code gen will not have octal constants */

	/* is it an octal number? */
	if (*pbuf=='0' && only(pbuf+1, "01234567")) {
		sscanf(pbuf, "%o", &arg->addr);
		if (negative)
			arg->addr *= -1;
		arg->type = INTVAL;
	}
#endif DEFOUT

	/* is it a hexadecimal number? */
	if (*pbuf=='0' && (pbuf[1]=='x' || pbuf[1]=='X')
	&& only(pbuf+2, "0123456789abcdefABCDEF")) {
		sscanf(pbuf+2, "%x", &arg->addr);
		if (negative)
			arg->addr *= -1;
		arg->type = INTVAL;
	}

	/* is it a compiler-generated label (L2345) ? */
	else if (*pbuf=='L' && only(pbuf+1, "1234567890")) {
		arg->labno = atoi(pbuf+1);
		arg->type = INTLAB;
	}

	/* is it a decimal number? */
	else if (isdigit(*pbuf) && only(pbuf, "0123456789")) {
		sscanf(pbuf, "%d", &arg->addr);
		if (negative)
			arg->addr *= -1;
		arg->type = INTVAL;
	}

	/* it's a string */
	else {
		arg->type = STRING;
		arg->string = copy(buf);
	}
	if (*p++ != ',')
		goto noparse;

	/* STORE BASE DISP */
	}

	/* see if an a-register or the pc follows */
	/* assumption : cannot leave out base reg */
	r=checkreg(p);
	if ( !isA(r) && r!=reg_pc && r!=reg_za0)/* must be address reg or pc */
		goto noparse;

	arg->reg = r;
	p+=3;				/* skip past register/pc */
	if (r == reg_za0) p++;

	arg->mode = (r == reg_pc) ? PINDBASE : AINDBASE;
	if (*p == ')') return;

	if (*p++ != ',')
		goto noparse;

	/* what follows is an index register */
	r=checkreg(p);
	if (!isAD(r) && (r != reg_za0))		/* if not dn or an */
		goto noparse;


	arg->index = r;
	p+=3;				/* skip past reg */
	if (r == reg_za0) p++;

	if (p[0]=='.' && p[1]=='w') {
		p+=2;			/* skip .w */
		arg->word_index=true;
	}
	else if (p[0]=='.' && p[1]=='l') {
		p+=2;
		arg->word_index=false;
	}
	else
		arg->word_index=true;	/* default is word */

	arg->scale = '1';
	if (p[0]=='*')
	{
		arg->scale = p[1];
		p+=2;
	}

	if (p[0]==')' && p[1]=='\0')
		return;			/* success */

noparse:
	/* can't parse it */
	internal_error("Parsing fails at '%s' \n input_line='%s'",
				p, input_line);
}
#endif


/*
 * if p points to a register, return:
 *		0-7 for d0-d7		checked by isD(r)
 *		8-15 for a0-a7		checked by isA(r)
 *		20-27 for fp0-fp7	checked by isF(r)
 *		(remember, sp is a7)
 *		15 for sp		checked by isSP(r)
 *		-2 for pc		checked by isPC(r)
 *		-3 for za0
 *		-1 for anything else
 */
checkreg(s)
register char *s;
{
	static struct reg_info {
		char name[2];
		int number;
	} reg_info[] = {
		"d0", reg_d0,	"a0", reg_a0,	"sp",reg_sp,
		"d1", reg_d1,	"a1", reg_a1,	"pc",reg_pc,
		"d2", reg_d2,	"a2", reg_a2,	"za",reg_za0,
		"d3", reg_d3,	"a3", reg_a3,
		"d4", reg_d4,	"a4", reg_a4,
		"d5", reg_d5,	"a5", reg_a5,
		"d6", reg_d6,	"a6", reg_a6,
		"d7", reg_d7,	"a7", reg_a7,

		"xx", 0
	};
	
	register struct reg_info *p;

	if (s[0] != '%') return (-1);
	for (p=reg_info; p->name[0]!='x'; p++)
		if (p->name[0]==s[1] && p->name[1]==s[2])
			return p->number;

#ifdef M68020
	if (s[1]=='f' && s[2]=='p')
#ifdef DRAGON
		if (s[3]=='a')		/* dragon reg */
			return ( reg_fpa0 + atoi(&s[4]));
		else			/* 68881 reg */
#endif DRAGON
			return ( reg_fp0 + s[3] - '0' );
#endif M68020

	return(-1);				/* failure */
}


/*
 * Does 'target' consist of only characters from 'set' ?
 */
only(target, set)
register char *target;
char *set;
{
	char *strchr();

	for (; *target!='\0'; ++target)		/* check target one-by-one */
		if (strchr(set, *target)==NULL)	/* if it's not in the set */
			return false;		/* then return failure */

	return true;				/* all targets were in set */
}

#ifdef M68020
char *parse_bit_fld(str, nodep)
register char *str;
register node *nodep;
{
	char 	buf[10], *pbuf;
	int	off, wid;

	if (nodep->op < BFCHG || nodep->op > BFTST)
		internal_error("illegal bit field op in %s", input_line);
	str++;
	for (pbuf=buf; *str != '}'; ) *pbuf++ = *str++;
	*pbuf = '\0' ;
	pbuf = buf;
	str++;
	sscanf(pbuf, "&%d:&%d", &off, &wid);
	nodep->offset = (char) off;
	nodep->width  = (char) wid;
	return (str);
}
#endif
