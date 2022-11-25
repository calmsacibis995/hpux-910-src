/* @(#) $Revision: 70.1 $ */    
#include "o68.h"

char *program_name;
static int n_opts = 0;
static int old_n_opts = -1;

static int path_checked = 0;

/*
 * Report an optimization if the debug flag is on.
 */
/* VARARGS1 */
opt(str, a,b,c,d,e)
register char *str;
{
	change_occured = true;
	n_opts ++;

#ifdef DEBUG
	if (debug) {
		fflush(stdout);			/* Get standard out in sync */
		fprintf(stderr, "--> optimization: ");
		fprintf(stderr, str, a,b,c,d,e);
		fprintf(stderr, "\n");
	}
#endif DEBUG
}


#ifdef DEBUG
/*
 * Print out a string for debugging purposes.
 *
 * The first argument is a printf format string,
 * subsequent arguments will be passed to printf.
 *
 * If the string starts with <digit>: then only print the string
 * if the appropriate debug flag is set.
 * Otherwise, only print it if the flag 'debug' is set.
 */
/* VARARGS1 */
bugout(str, a,b,c,d,e)
register char *str;
{
	if (isdigit(str[0]) && str[1]==':') {
		switch (str[0]) {
		case '1': if (!debug1) return false; break;
		case '2': if (!debug2) return false; break;
		case '3': if (!debug3) return false; break;
		case '4': if (!debug4) return false; break;
		case '5': if (!debug5) return false; break;
		case '6': if (!debug6) return false; break;
		case '7': if (!debug7) return false; break;
		case '8': if (!debug8) return false; break;
		case '9': if (!debug9) return false; break;
		}
		str += 2;		/* skip <digit>: */

		if (str[0]==' ')	/* possible blank after the colon */
			str++;
	}
	else {				/* if no <digit>: prefix */
		if (!debug)		/* based on debug itself */
			return false;
	}

	fflush(stdout);				/* Get standard out in sync */
	fprintf(stderr, str, a,b,c,d,e);
	fprintf(stderr, "\n");
	return true;
}

/* VARARGS1 */
bugdump(str, a,b,c,d,e)
register char *str;
{
	if (bugout(str, a,b,c,d,e))
	{
		if (n_opts == old_n_opts)
			bugout("9: code is unchanged");
		else
			output();
		old_n_opts = n_opts ;
	}
}

#endif DEBUG

/* VARARGS1 */
internal_error(str, a,b,c,d,e)
char *str;
{
	fflush(stdout);				/* Get standard out in sync */

	fprintf(stderr, "\n\n");
	fprintf(stderr, "*** %s: INTERNAL OPTIMIZER ERROR ***\n", program_name);
	fprintf(stderr, str, a,b,c,d,e);
	fprintf(stderr, "\n");

	exit(10);
}


/*
 * reg = unused_dreg(where);
 *
 * Find an unused data register and return its number.
 * Return -1 if no register found.
 */
unused_dreg(where)
register node *where;
{
	register int reg;

	/*
	 * Search starting at d7 going to to d0.
	 * This way the value will tendt to hang around
	 * a long time, since d0/d1 are compiler scratch.
	 */
	for (reg=reg_d7; reg>=reg_d0; reg--)		/* for all data regs */
		if (!reg_used(where,reg))		/* if reg not used */
			return reg;			/* return it */

	return -1;					/* failure */
}




/*
 * reg_used(where, reg)
 *
 * See if a register's value is used before it's destroyed.
 */
reg_used(where, reg)
node *where;
int reg;
{
    int i;

	if (reg==reg_sp)
		return true;			/* sp is sacred! */

	/* search from here, max eight jumps */
	/* if (fort) return used(where, reg, 4); */

	/*
	 * path_checked is the value stored in the labno2
	 * field of a LABEL or DLABEL to indicate that the
	 * path has already been checked
	 */
	path_checked++;

	return(used(where, reg, 8));
}


used(where, reg, jump_count)
node *where;
register int reg;
{
	register node *p, *p2;

	for (p=where; p!=NULL; p=p->forw) {
retry:
		switch (p->op) {

		case JMP:
			/* must be a normal jump */
			if (p->mode1!=ABS_L || p->ref==NULL)
				return true;

			/* must have some leeway left */
			if (--jump_count<=0)
				return true;

			p=p->ref;
			goto retry;

		case CBR:
			/* must be a normal jump */
			if (p->mode1!=ABS_L || p->ref==NULL)
				return true;

			/* must have some leeway left */
			if (jump_count<=1)
				return true;

			/*
			 * concern: if in loop, we will always
			 * return used, even if loop does not use it
			 */
			   
			/* if the branch uses it, it is used. */
			if (used(p->ref, reg, jump_count-1))
				return true;

			/* branch didn't use it, see if normal execution does */
			break;

		case JSR:
		case BSR:
			/*
			 * We assume that subroutines require no registers
			 * but may destroy d0, d1, a0, a1
			 * unless it's like jsr (a0).
			 *
			 * In addition, structure-valued functions
			 * do require a1, and mcount (profiling) requires a0.
			 *
			 * And, of course, _exit requires *no* registers.
			 */

#ifdef DEBUG
			if (exit_exits
			&& p->mode1==ABS_L && p->type1==STRING
			&& equstr(p->string1, "_exit"))
				return false;
#endif DEBUG

			if (reg==reg_a0 || reg==reg_a1)
				return true;

			if (p->reg1==reg || p->index1==reg) /* if jsr (a3) */
				return true;		/* it's needed */

			/* function destroys temporary registers */
			if (reg==reg_d0 || reg==reg_d1)
				return false;

			/* otherwise, no decision made */
			break;

		case RTS:
			/* 
			 *  d0-d1 contain the return value
			 *  permanent regs are restored before an rts
			 */
			if (reg==reg_a0 || reg==reg_a1) return false;
			return true;

		case ADD:
		case ADDA:
		case ADDQ:
		case AND:
		case ASL:
		case ASR:
		case BCHG:
		case BCLR:
		case BSET:
		case BTST:
		case CMP:
		case DIVS:
		case DIVSL:
		case DIVU:
		case DIVUL:
		case EOR:
		case EXG:
#ifdef M68020
		case FABS:
		case FACOS:
		case FADD:
		case FASIN:
		case FATAN:
		case FCOS:
		case FCOSH:
		case FDIV:
		case FETOX:
		case FINTRZ:
		case FLOG10:
		case FLOGN:
		case FMOD:
		case FMOV:
		case FMOVM:
		case FMUL:
		case FNEG:
		case FSGLMUL:
		case FSGLDIV:
		case FSIN:
		case FSINH:
		case FSQRT:
		case FSUB:
		case FTAN:
		case FTANH:
#endif
		case LSL:
		case LSR:
		case MULS:
		case MULU:
		case OR:
		case ROL:
		case ROR:
		case SUB:
		case SUBA:
		case SUBQ:
			/* is it used in the first argument? */
			if (p->reg1==reg || p->index1==reg)
				return true;

			/* is it used in the second argument? */
			if (p->reg2==reg || p->index2==reg)
				return true;

			break;
		
		case LEA:
			/* is it used in the first argument? */
			if (p->reg1==reg || p->index1==reg)
				return true;

			/* is it used in the second argument? */
			if (p->reg2==reg)
				return false;
			break;
		
		/* This is not canonical, but reg_used is called by tidy */
		case MOVEQ:
			if (p->reg2==reg)
				return false;
			break;


		case MOVE:
			/* is it used in the first argument? */
			if (p->reg1==reg || p->index1==reg) /* if used */
				return true;			/* it's used! */

			/* is it used in the second argument? */
			if (p->reg2!=reg && p->index2!=reg) /* if ~used */
				break;				/* no effect */

			/* if not direct, register is used */
			if (p->mode2!=DDIR && p->mode2!=ADIR)
				return true;

			/* ok, it's being written to. */
			if (p->subop==LONG)			/* if writ to */
				return false;			/* data lost */

			/*
			 * Is it a word write being extended to long?
			 * If so, the register is trashed.
			 */
			if (p->mode2==DDIR && p->subop==WORD
			&& (p2=p->forw)!=NULL
			&& p2->op==EXT && p2->subop==LONG
			&& p2->reg1==reg)
				return false;

			/*
			 * Is it a byte write being extended to long?
			 * If so, the register is trashed.
			 */
			if (p->mode2==DDIR && p->subop==BYTE
			&& (p2=p->forw)!=NULL
			&& p2->op==EXT && p2->subop==WORD
			&& p2->reg1==reg
			&& (p2=p2->forw)!=NULL
			&& p2->op==EXT && p2->subop==LONG
			&& p2->reg1==reg)
				return false;

			if (p->mode2==DDIR && p->subop==BYTE
			&& (p2=p->forw)!=NULL
			&& p2->op==EXTB 
			&& p2->reg1==reg)
				return false;

			break;

		case MOVEM:
			/* are we storing into this register? */
			if (p->mode2!=IMMEDIATE || p->type2!=INTVAL)
				return true;		/* assume the worst */

			if (p->subop==LONG
			&& ((1<<reg) & p->addr2))	/* if bit is set */
				return false;		/* then it's not used */

			break;				/* else not changed */

		case NOT:
		case NEG:
		case EXT:
		case EXTB:
		case TST:
		case SWAP:
		case PEA:
		case LINK:
		case UNLK:
		case CLR:

		case SCC:
		case SCS:
		case SEQ:
		case SF:
		case SGE:
		case SGT:
		case SHI:
		case SLE:
		case SLS:
		case SLT:
		case SMI:
		case SNE:
		case SPL:
		case ST:
		case SVC:
		case SVS:

			if (p->reg1==reg || p->index1==reg)
				return true;
			break;
			
		case LABEL:
		case DLABEL:
			if (p->labno2 == path_checked) {
				return false;
			}
			else {
				p->labno2 = path_checked;
			}
			break;

		case COMMENT:
		case LALIGN:
		case TEXT:
			break;

			/* all the cases below are to get a jump table */

			/* never expect to hit these */
		case BCC:
		case BCS:
		case BEQ:
		case BGE:
		case BGT:
		case BHI:
		case BLS:
		case BLT:
		case BMI:
		case BNE:
		case BPL:
		case BRA:

			/* add to default */
		case BFCHG:
		case BFCLR:
		case BFEXTS:
		case BFEXTU:
		case BFFFO:
		case BFINS:
		case BFSET:
		case BFTST:

		case DBCC:
		case DBCS:
		case DBEQ:
		case DBF:
		case DBGE:
		case DBGT:
		case DBHI:
		case DBLE:
		case DBLS:
		case DBLT:
		case DBMI:
		case DBNE:
		case DBPL:
		case DBRA:
		case DBT:
		case DBVC:
		case DBVS:

		case ASM:
		case ASCIZ:
		case BSS:
		case COMM:
		case DATA:
		case DC:
		case DC_B:
		case DC_W:
		case DC_L:
		case DS:
		case GLOBAL:
		case JSW:
		case LCOMM:
		case SET:

#ifdef M68020
		case FCMP:
		case FBEQ:
		case FBGE:
		case FBGL:
		case FBGLE:
		case FBGT:
		case FBLE:
		case FBLT:
		case FBNEQ:
		case FBNGE:
		case FBNGL:
		case FBNGLE:
		case FBNGT:
		case FBNLE:
		case FBNLT:
#endif M68020

		default:
			return true;		/* assume used */
		}
	}

	/* ran out of nodes?  Let's assume it's used. */
	return true;
}


/*
 * reg_in_arg(arg, reg)
 *
 * Is register 'reg' used/written to in  from to in argument 'arg' ?
 */
reg_in_arg(arg, reg)
register argument *arg;
register int reg;
{

	switch (arg->mode) {

	case DDIR:
	case ADIR:
	case IND:
	case INC:
	case DEC:
	case ADISP:
	case PDISP:
#ifdef M68020
	case FREG:
#ifdef DRAGON
	case FPREG:
#endif DRAGON
#endif M68020
		return (arg->reg==reg);

	case AINDEX:
	case AINDBASE:
	case PINDEX:
	case PINDBASE:
	case DPAIR:
#ifdef PREPOST
	case MEMPRE: 
	case MEMPOST:
	case PCPRE: 
	case PCPOST:
#endif PREPOST
		return (arg->reg==reg || arg->index==reg);

	case ABS_W:
	case ABS_L:
	case IMMEDIATE:
		return false;

	case UNKNOWN:			/* unused argument */
		return false;

	default:
		internal_error("reg_in_argument: mode %d?", arg->mode);
		return false;
	}
}



/*
 * cond = negate_condition(cond);
 *
 * Reverse the sense of a condition.  For example:
 *
 *	cmp  a,b	->	cmp  b,a
 *	jlt  away	->	jgt  away
 *
 * This is not to be confused with the mapping of jlt->jge,
 * which is the macro REVERSE_CONDITION.
 *
 * Some conditions can't be negated, so return -1 for those.
 */
int
negate_condition(cond)
register int cond;
{
	switch (cond) {
	case COND_HS:	return COND_LS;		/* COND_HS==COND_CC */
	case COND_LO:	return COND_HI;		/* COND_LO==COND_CS */
	case COND_EQ:	return COND_EQ;		/* equal either direction */
	case COND_NE:	return COND_NE;		/* unequal either direction */
	case COND_GE:	return COND_LE;
	case COND_LT:	return COND_GT;
	case COND_GT:	return COND_LT;
	case COND_LE:	return COND_GE;
	case COND_HI:	return COND_LO;
	case COND_LS:	return COND_HS;
	case COND_MI:	return -1;		/* No strictly positive */
	case COND_PL:	return -1;		/* No negative or zero */
	case COND_VC:	return -1;		/* I don't understant this */
	case COND_VS:	return -1;		/* I don't understant this */
	case COND_T:	return COND_T;		/* true is always true */
	case COND_F:	return COND_F;		/* and false is always false */

	/* And just because we don't trust this god-awful program... */
	default:
		internal_error("negate_condition: impossible condition %d",
				cond);
		return -1;
	}
}


char *
true_false(b)
boolean b;
{
	switch (b) {
	case true:	return "true";
	case false:	return "false";
	default:	return "<should be true or false???>";
	}
}


/*
 * bitnum(n):
 *
 * return p if n==2**p	(if n is a power of two)
 * return -1 otherwise
 *
 * You can think of it as log2(n) defined only for powers of two.
 */
int
bitnum(n)
int n;
{
	register int bit;

	if (n==0)			/* if no bits set */
		return -1;		/* exit with a failure */

	for (bit=0; (n & 1)==0; bit++)		/* while lsb isn't set */
		(* (unsigned *) &n) >>= 1;	/* shift bits, zero fill */

	if (n==1)			/* if only one bit set */
		return bit;		/* return the number of the bit */
	else				/* if more than one bit set */
		return -1;		/* fail */
}


#ifdef M68020
/* is this worth the time involved in doing it ???
 * c1:reg allocator should handle most dead stores/loads except to fp0/fp1 
 * possible tuning: only call this for fp[01]
 */

#define reg_fp1 reg_fp0+1
/*
 * f_reg_used(where, reg)
 *
 * See if an 881 register's value is used before it's destroyed.
 */
f_reg_used(where, reg)
node *where;
int reg;
{
	/* search from here, max eight jumps */
	return f_used(where, reg, 8);
}


f_used(where, reg, jump_count)
node *where;
register int reg;
{
	register node *p, *p2;

	for (p=where; p!=NULL; p=p->forw) {
retry:
		switch (p->op) {

		case JMP:
			/* must be a normal jump */
			if (p->mode1!=ABS_L || p->ref==NULL)
				return true;

			/* must have some leeway left */
			if (--jump_count<=0)
				return true;

			p=p->ref;
			goto retry;

		case CBR:
			/* must be a normal jump */
			if (p->mode1!=ABS_L || p->ref==NULL)
				return true;

			/* must have some leeway left */
			if (jump_count<=1)
				return true;

			/*
			 * concern: if in loop, we will always
			 * return used, even if loop does not use it
			 */
			   
			/* if the branch uses it, it is used. */
			if (f_used(p->ref, reg, jump_count-1))
				return true;

			/* branch didn't use it, see if normal execution does */
			break;

		case DBF:
			/* must be a normal jump */
			if (p->mode2!=ABS_L || p->ref==NULL)
				return true;

			/* must have some leeway left */
			if (jump_count<=1)
				return true;

			/* if the branch uses it, it is used. */
			if (f_used(p->ref, reg, jump_count-1))
				return true;

			/* branch didn't use it, see if normal execution does */
			break;

		case JSR:
			/*
			 * We assume that subroutines require no registers
			 * but may destroy fp0 and fp1
			 *
			 * And, of course, _exit requires *no* registers.
			 */

#ifdef DEBUG
			if (exit_exits
			&& p->mode1==ABS_L && p->type1==STRING
			&& equstr(p->string1, "_exit"))
				return false;
#endif DEBUG

			/* function destroys temporary registers */
			if (reg==reg_fp0 || reg==reg_fp1)
				return false;

			/* otherwise, no decision made */
			break;

		case RTS:
			/* is this correct for scratch 881 regs ? */
			return true;

		case FMOV:
		case FABS:
		case FACOS:
		case FASIN:
		case FATAN:
		case FCOS:
		case FCOSH:
		case FETOX:
		case FINTRZ:
		case FLOG10:
		case FLOGN:
		case FNEG:
		case FSIN:
		case FSINH:
		case FSQRT:
		case FTAN:
		case FTANH:
			/* is it used in the first argument? */
			if (p->reg1==reg)
				return true;

			/* is it used in the second argument? */
			if (p->reg2==reg)
				return false;

			break;
		

		case FADD:
		case FCMP:
		case FDIV:
		case FMOD:
		case FMUL:
		case FSGLMUL:
		case FSGLDIV:
		case FSUB:
		case FTEST:
			/* is it used in the first argument? */
			if (p->reg1==reg)
				return true;

			/* is it used in the second argument? */
			if (p->reg2==reg)
				return true;

			break;
		
		case LABEL:
		case DLABEL:
			break;

		default:
			/* floating point branches */
			if ((p->op >= FBEQ && p->op <= FBNLT) || 
			    (p->op >= FPBEQ && p->op <= FPBNLT))
			{
			  /* must be a normal jump */
			  if (p->mode1!=ABS_L || p->ref==NULL)
				return true;

			  /* must have some leeway left */
			  if (jump_count<=1)
				return true;

			  /*
			   * concern: if in loop, we will always
			   * return used, even if loop does not use it
			   */
			   
			  /* if the branch uses it, it is used. */
			  if (f_used(p->ref, reg, jump_count-1))
				return true;

			  /* branch didn't use it, see if normal execution does */
			  break;
			}

			break; 		/* assume unused */
		}
	}

	/* ran out of nodes?  Let's assume it's used. */
	return true;
}
#endif M68020


#ifdef DRAGON
/* is this worth the time involved in doing it ???
 * c1:reg allocator should handle most dead stores/loads except to scratch regs
 * possible tuning: only call this for fpa[01]
 */

#define reg_fpa1 reg_fpa0+1
#define reg_fpa2 reg_fpa0+2
/*
 * fp_reg_used(where, reg)
 *
 * See if a fpa register's value is used before it's destroyed.
 */
fp_reg_used(where, reg)
node *where;
int reg;
{
	/* search from here, max eight jumps */
	return fp_used(where, reg, 8);
}


fp_used(where, reg, jump_count)
node *where;
register int reg;
{
	register node *p, *p2;

	for (p=where; p!=NULL; p=p->forw) {
retry:
		switch (p->op) {

		case JMP:
			/* must be a normal jump */
			if (p->mode1!=ABS_L || p->ref==NULL)
				return true;

			/* must have some leeway left */
			if (--jump_count<=0)
				return true;

			p=p->ref;
			goto retry;

		case CBR:
			/* must be a normal jump */
			if (p->mode1!=ABS_L || p->ref==NULL)
				return true;

			/* must have some leeway left */
			if (jump_count<=1)
				return true;

			/*
			 * concern: if in loop, we will always
			 * return used, even if loop does not use it
			 */
			   
			/* if the branch uses it, it is used. */
			if (fp_used(p->ref, reg, jump_count-1))
				return true;

			/* branch didn't use it, see if normal execution does */
			break;

		case DBF:
			/* must be a normal jump */
			if (p->mode2!=ABS_L || p->ref==NULL)
				return true;

			/* must have some leeway left */
			if (jump_count<=1)
				return true;

			/* if the branch uses it, it is used. */
			if (fp_used(p->ref, reg, jump_count-1))
				return true;

			/* branch didn't use it, see if normal execution does */
			break;

		case JSR:
			/*
			 * We assume that subroutines require no registers
			 * but may destroy fpa0/fpa1/fpa2
			 *
			 * And, of course, _exit requires *no* registers.
			 */

#ifdef DEBUG
			if (exit_exits
			&& p->mode1==ABS_L && p->type1==STRING
			&& equstr(p->string1, "_exit"))
				return false;
#endif DEBUG

			/* function destroys temporary registers */
			if (reg==reg_fpa0 || reg==reg_fpa1 || reg==reg_fpa2)
				return false;

			/* otherwise, no decision made */
			break;

		case RTS:
			/* is this correct for scratch fpa regs ? */
			return true;

		case FPMOV:
		case FPABS:
		case FPCVD:
		case FPCVL:
		case FPCVS:
		case FPINTRZ:
		case FPNEG:
			/* is it used in the first argument? */
			if (p->reg1==reg)
				return true;

			/* is it used in the second argument? */
			if (p->reg2==reg)
				return false;

			break;
		

		case FPADD:
		case FPCMP:
		case FPDIV:
		case FPMUL:
		case FPSUB:
		case FPTEST:
			/* is it used in the first argument? */
			if (p->reg1==reg)
				return true;

			/* is it used in the second argument? */
			if (p->reg2==reg)
				return true;

			break;
		
		case LABEL:
		case DLABEL:
			break;

		default:
			/* floating point branches */
			if ((p->op >= FBEQ && p->op <= FBNLT) || 
			    (p->op >= FPBEQ && p->op <= FPBNLT))
			{
			  /* must be a normal jump */
			  if (p->mode1!=ABS_L || p->ref==NULL)
				return true;

			  /* must have some leeway left */
			  if (jump_count<=1)
				return true;

			  /*
			   * concern: if in loop, we will always
			   * return used, even if loop does not use it
			   */
			   
			  /* if the branch uses it, it is used. */
			  if (fp_used(p->ref, reg, jump_count-1))
				return true;

			  /* branch didn't use it, see if normal execution does */
			  break;
			}

			break; 		/* assume unused */
		}
	}

	/* ran out of nodes?  Let's assume it's used. */
	return true;
}
#endif DRAGON
