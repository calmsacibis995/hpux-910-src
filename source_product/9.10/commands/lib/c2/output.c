/* @(#) $Revision: 70.1 $ */      
#include "o68.h"
#define quote '"'

int output_map[] = {
	(COND_CC<<16)+CBR,	BCC,
	(COND_CS<<16)+CBR,	BCS,
	(COND_EQ<<16)+CBR,	BEQ,
	(COND_GE<<16)+CBR,	BGE,
	(COND_GT<<16)+CBR,	BGT,
	(COND_HI<<16)+CBR,	BHI,
	(COND_LE<<16)+CBR,	BLE,
	(COND_LS<<16)+CBR,	BLS,
	(COND_LT<<16)+CBR,	BLT,
	(COND_MI<<16)+CBR,	BMI,
	(COND_NE<<16)+CBR,	BNE,
	(COND_PL<<16)+CBR,	BPL,
	(COND_VC<<16)+CBR,	BVC,
	(COND_VS<<16)+CBR,	BVS,
	0
	};

char *out_op[] = { 
0	/* 0 */,
0	/* 1 */,
"add"	/* 2 */,
"adda"	/* 3 */,
"addq"	/* 4 */,
"and"	/* 5 */,
"asl"	/* 6 */,
"asr"	/* 7 */,
"bcc"	/* 8 */,
"bchg"	/* 9 */,
"bclr"	/* 10 */,
"bcs"	/* 11 */,
"beq"	/* 12 */,
"bfchg"	/* 13 */,
"bfclr"	/* 14 */,
"bfexts"	/* 15 */,
"bfextu"	/* 16 */,
"bfffo"	/* 17 */,
"bfins"	/* 18 */,
"bfset"	/* 19 */,
"bftst"	/* 20 */,
"bge"	/* 21 */,
"bgt"	/* 22 */,
"bhi"	/* 23 */,
"ble"	/* 24 */,
"bls"	/* 25 */,
"blt"	/* 26 */,
"bmi"	/* 27 */,
"bne"	/* 28 */,
"bpl"	/* 29 */,
"bra"	/* 30 */,
"bset"	/* 31 */,
"bsr"	/* 32 */,
"btst"	/* 33 */,
"bvc"	/* 34 */,
"bvs"	/* 35 */,
"clr"	/* 36 */,
"cmp"	/* 37 */,
"dbcc"	/* 38 */,
"dbcs"	/* 39 */,
"dbeq"	/* 40 */,
"dbf"	/* 41 */,
"dbge"	/* 42 */,
"dbgt"	/* 43 */,
"dbhi"	/* 44 */,
"dble"	/* 45 */,
"dbls"	/* 46 */,
"dblt"	/* 47 */,
"dbmi"	/* 48 */,
"dbne"	/* 49 */,
"dbpl"	/* 50 */,
"dbra"	/* 51 */,
"dbt"	/* 52 */,
"dbvc"	/* 53 */,
"dbvs"	/* 54 */,
"divs"	/* 55 */,
"divsl"	/* 56 */,
"divu"	/* 57 */,
"divul"	/* 58 */,
"eor"	/* 59 */,
"exg"	/* 60 */,
"ext"	/* 61 */,
"extb"	/* 62 */,
"jmp"	/* 63 */,
"jsr"	/* 64 */,
"lea"	/* 65 */,
"link"	/* 66 */,
"lsl"	/* 67 */,
"lsr"	/* 68 */,
"mov"	/* 69 */,
"movm"	/* 70 */,
"movq"	/* 71 */,
"muls"	/* 72 */,
"mulu"	/* 73 */,
"neg"	/* 74 */,
"not"	/* 75 */,
"or"	/* 76 */,
"pea"	/* 77 */,
"rol"	/* 78 */,
"ror"	/* 79 */,
"rts"	/* 80 */,
"scc"	/* 81 */,
"scs"	/* 82 */,
"seq"	/* 83 */,
"sf"	/* 84 */,
"sge"	/* 85 */,
"sgt"	/* 86 */,
"shi"	/* 87 */,
"sle"	/* 88 */,
"sls"	/* 89 */,
"slt"	/* 90 */,
"smi"	/* 91 */,
"sne"	/* 92 */,
"spl"	/* 93 */,
"st"	/* 94 */,
"sub"	/* 95 */,
"suba"	/* 96 */,
"subq"	/* 97 */,
"svc"	/* 98 */,
"svs"	/* 99 */,
"swap"	/* 100 */,
"trap"	/* 101 */,
"tst"	/* 102 */,
"unlk"	/* 103 */,
0	/* 104 */,
"asciz"	/* 105 */,
"bss"	/* 106 */,
0	/* 107 */,
"comm"	/* 108 */,
0	/* 109 */,
"data"	/* 110 */,
0	/* 111 */,
"byte"	/* 112 */,
"short"	/* 113 */,
"long"	/* 114 */,
0	/* 115 */,
"space"	/* 116 */,
0	/* 117 */,
"global"	/* 118 */,
0	/* 119 */,
0	/* 120 */,
"lalign"	/* 121 */,
"lcomm"	/* 122 */,
"set"	/* 123 */,
"text"	/* 124 */,
"version"	/* 125 */,
"sglobal"	/* 126 */,
"shlib_version"	/* 127 */,
0	/* 128 */,
0	/* 129 */,
#ifdef M68020
"fabs"	/* 130 */,
"facos"	/* 131 */,
"fadd"	/* 132 */,
"fasin"	/* 133 */,
"fatan"	/* 134 */,
"fbeq"	/* 135 */,
"fbge"	/* 136 */,
"fbgl"	/* 137 */,
"fbgle"	/* 138 */,
"fbgt"	/* 139 */,
"fble"	/* 140 */,
"fblt"	/* 141 */,
"fbneq"	/* 142 */,
"fbnge"	/* 143 */,
"fbngl"	/* 144 */,
"fbngle"	/* 145 */,
"fbngt"	/* 146 */,
"fbnle"	/* 147 */,
"fbnlt"	/* 148 */,
"fcmp"	/* 149 */,
"fcos"	/* 150 */,
"fcosh"	/* 151 */,
"fdiv"	/* 152 */,
"fetox"	/* 153 */,
"fintrz"	/* 154 */,
"flog10"	/* 155 */,
"flogn"	/* 156 */,
"fmod"	/* 157 */,
"fmov"	/* 158 */,
"fmovm"	/* 159 */,
"fmul"	/* 160 */,
"fneg"	/* 161 */,
"fsglmul"	/* 162 */,
"fsgldiv"	/* 163 */,
"fsin"	/* 164 */,
"fsinh"	/* 165 */,
"fsqrt"	/* 166 */,
"fsub"	/* 167 */,
"ftan"	/* 168 */,
"ftanh"	/* 169 */,
"ftest"	/* 170 */,
#endif M68020
#ifdef DRAGON
"fpabs"	/* 171 */,
"fpadd"	/* 172 */,
"fpbeq"	/* 173 */,
"fpbge"	/* 174 */,
"fpbgl"	/* 175 */,
"fpbgle"	/* 176 */,
"fpbgt"	/* 177 */,
"fpble"	/* 178 */,
"fpblt"	/* 179 */,
"fpbne"	/* 180 */,
"fpbnge"	/* 181 */,
"fpbngl"	/* 182 */,
"fpbngle"	/* 183 */,
"fpbngt"	/* 184 */,
"fpbnle"	/* 185 */,
"fpbnlt"	/* 186 */,
"fpcmp"	/* 187 */,
"fpcvd"	/* 188 */,
"fpcvl"	/* 189 */,
"fpcvs"	/* 190 */,
"fpdiv"	/* 191 */,
"fpintrz"	/* 192 */,
"fpmov"	/* 193 */,
"fpmul"	/* 194 */,
"fpneg"	/* 195 */,
"fpsub"	/* 196 */,
"fptest"	/* 197 */,
"fpmabs"	/* 198 */,
"fpmadd"	/* 199 */,
"fpmcmp"	/* 200 */,
"fpmcvd"	/* 201 */,
"fpmcvl"	/* 202 */,
"fpmcvs"	/* 203 */,
"fpmdiv"	/* 204 */,
"fpmintrz"	/* 205 */,
"fpmmov"	/* 206 */,
"fpmmul"	/* 207 */,
"fpmneg"	/* 208 */,
"fpmsub"	/* 209 */,
"fpmtest"	/* 210 */,
"fpm2add"	/* 211 */,
"fpm2cmp"	/* 212 */,
"fpm2div"	/* 213 */,
"fpm2mul"	/* 214 */,
"fpm2sub"	/* 215 */,
#endif DRAGON
0 }; 


char *strcat(), *strcpy(), *true_false();

boolean secret_hex_flag = false;

output()
{
	register node *t;
	register char *opptr;
	register int op, subop;
	register int *outp;
	register char **r_out_op; /* register to hold out_op */

#ifdef DEBUG
	bugout("change_occured=%s", true_false(change_occured));
#endif DEBUG

	r_out_op = out_op;

	for (t=first.forw; t!=NULL; t=t->forw) {

#ifdef DEBUG
	if (debug1)
		printf("[%x b:%x f:%x r:%x]\t",
			t, t->back, t->forw, t->ref);
#endif DEBUG

	op = t->op;
	subop = t->subop;

	switch (op) {

	case COMMENT:
		puts(t->string1);
		break;

	case END:
		bugout("1:\t<end>");
		return;

	case LABEL:
		printf("L%d:\n", t->labno1);
		break;

	case DLABEL:
		printf("%s:\n", t->string1);
		break;

	case JSW:
		printf("\tlong\t");
		print_arg(&t->op1);		/* first Lxxx */
		printf("-");			/* separating minus */
		print_arg(&t->op2);		/* second Lxxx */
		printf("\n");
		break;

	case AND:
	case OR:
	case EOR:
		secret_hex_flag = true;		/* print operand as hex */
		goto ordinary;

ordinary:
	default:
		/* If it's a conditional branch or DC, combine op and subop */
		
		if (op==CBR)
		{
			op += subop<<16;	/* get a combined op */
			subop = (int) t->ref;
			/* Translate it */
			for (outp=output_map; *outp!=0; outp+=2)
				if (op==*outp) {
					op = *(outp+1);
					break;
				}
		}

		else if (op==DC)
		{
			op += subop;	/* get a combined op */
			subop = UNSIZED;
		}

		/*
		 * If we have a JMP or JSR that's not going
		 * to a normal label, leave it as a JMP/JSR.
		 * This is just to make it prettier, so we
		 * don't get a jbsr (a0).
		 */
		else if ((op==JMP || op==JSR) && t->mode1!=ABS_L)
		{
			subop = UNSIZED;
			goto skipit;
		}
		else if (op==JMP) 
		{
			subop = (int) t->ref;
			/* Translate it */
			op = BRA;
		}
#ifdef DRAGON
		else if (op >= FPBEQ && op <= FPBNLT)
		{
			subop = (int) t->ref;
		}
#endif DRAGON

skipit:
		/* Look for the op in the out_op table */

#ifdef PIC
                /*
                 * for pic, if we have a "jsr <label>"
		 * convert it to "bsr label"
		 */

                if ((pic || oforty) && (op == JSR) && (t->mode1 == ABS_L)) {
		   op = BSR;
		   subop = LONG;
		}
#endif

		opptr = r_out_op[op];

		if (opptr==NULL)
			internal_error("output: can't find opcode %d.%d",
					t->op, t->subop);

		/* Print the opcode */
		printf("\t%s", opptr);
		
		print_subop(subop);		/* Print the size */

		if (t->mode1!=UNKNOWN)		/* do we have arguments? */
			printf("\t");		/* only print tab if we do */
		printargs(t);			/* print some arguments */
		secret_hex_flag = false;
		printf("\n");

		break;

	} /* end of switch */
	} /* end of for loop thru all nodes */
}




print_subop(subop)
register int subop;
{
	switch (subop) {

	case BYTE:
		printf(".b");
		break;
	case WORD:
		printf(".w");
		break;
	case LONG:
		printf(".l");
		break;
#ifdef M68020
	case SINGLE:
		printf(".s");
		break;
	case DOUBLE:
		printf(".d");
		break;
	case EXTENDED:
		printf(".x");
		break;
#endif
	}
}
	
/*
 * Given a pointer to a node, print its arguments with no newline.
 */
printargs(p)
register node *p;
{
	if (p->mode1!=UNKNOWN) {		/* if 1st argument exists */
		print_arg(&p->op1);		/* print 1st argument */
#ifdef M68020
		if (p->op >= BFCHG && p->op <= BFTST && p->op != BFINS)
			print_bit_fld(p);
#endif
		if (p->mode2!=UNKNOWN) {	/* if 2nd argument exists */
			printf(",");		/* separate arguments */
			print_arg(&p->op2);	/* print 2nd argument */
#ifdef M68020
			if (p->op == BFINS)
				print_bit_fld(p);
#endif
		}
	}
}


/*
 * Given a pointer to an argument, print it with no newline.
 */
print_arg(arg)
register argument *arg;
{
	char *convert_reg();

#ifdef DEBUG
	if (debug1)
		printf("[mode %d]", arg->mode);
#endif DEBUG

	switch (arg->mode) {

	case OTHER:
		print_value(arg);
		break;

	case DDIR:
		if (!isD(arg->reg))
			internal_error("print_arg: mode=%d, reg=%d",
					arg->mode, arg->reg);
		printf("%s", convert_reg(arg->reg));
		break;

#ifdef M68020
	case DPAIR:
		if (!isD(arg->reg) || !isD(arg->index))
			internal_error("print_arg: mode=%d, reg1=%d, reg2=%d",
					arg->mode, arg->reg, arg->index);
		printf("%s", convert_reg(arg->reg));
		printf(":%s", convert_reg(arg->index));
		break;
#endif

	case ADIR:
		if (!isA(arg->reg))
			internal_error("print_arg: mode=%d, reg=%d",
					arg->mode, arg->reg);
		printf("%s", convert_reg(arg->reg));
		break;

#ifdef M68020
	case FREG:
		if (!isF(arg->reg))
			internal_error("print_arg: mode=%d, reg=%d",
					arg->mode, arg->reg);
		printf("%%fp%d", (int) arg->reg - reg_fp0);
		break;

#ifdef DRAGON
	case FPREG:
		if (!isFP(arg->reg))
			internal_error("print_arg: mode=%d, reg=%d",
					arg->mode, arg->reg);
		printf("%%fpa%d", (int) arg->reg - reg_fpa0);
		break;

	case FPREGS:
		printf("%%fpa%d,%%fpa%d", (int) arg->reg - reg_fpa0,
			(int) arg->index - reg_fpa0);
		break;
#endif DRAGON
#endif M68020

	case IND:
		if (!isA(arg->reg))
			internal_error("print_arg: mode=%d, reg=%d",
					arg->mode, arg->reg);
		printf("(%s)", convert_reg(arg->reg));
		break;

	case INC:
		if (!isA(arg->reg))
			internal_error("print_arg: mode=%d, reg=%d",
					arg->mode, arg->reg);
		printf("(%s)+", convert_reg(arg->reg));
		break;

	case DEC:
		if (!isA(arg->reg))
			internal_error("print_arg: mode=%d, reg=%d",
					arg->mode, arg->reg);
		printf("-(%s)", convert_reg(arg->reg));
		break;

	case IMMEDIATE:
		printf("&");
		print_imm_value(arg);
		break;

	case ABS_W:
		print_value(arg);
		/* printf(".w"); */
		break;

	case ABS_L:
		print_value(arg);
		break;

	case ADISP:
	case PDISP:
		print_value(arg);
		printf("(%s)", convert_reg(arg->reg));
		break;

	case AINDEX:
	case PINDEX:
		print_value(arg);
#ifdef M68020
		printf("(%s,%s.%c*%c)",
#else
		printf("(%s,%s.%c)",
#endif
			convert_reg(arg->reg),		/* address register */
			convert_reg(arg->index),	/* index register */
			arg->word_index ? 'w' : 'l' 	/* length of index */
#ifdef M68020
			,arg->scale);
#else
			);
#endif
		break;

#ifdef M68020
	case AINDBASE: 	/* (bd,%an,%rn.size*scale)	*/
	case PINDBASE: 	/* (bd,%pc,%rn.size*scale)	*/
		printf("(");
		if (arg->type != UNKNOWN) { print_value(arg); printf(",");}
		printf("%s", convert_reg(arg->reg));
		if (arg->index != -1)
			printf(",%s.%c*%c",
			   convert_reg(arg->index),	/* index register */
			   arg->word_index ? 'w' : 'l',	/* length of index */
			   arg->scale);
		printf(")");
		break;
		
#ifdef PREPOST
	case MEMPRE:		/* ([bd,%an,%rn.size*scale],od) */
	case MEMPOST:		/* ([bd,%an],%rn.size*scale,od) */
	case PCPRE:		/* ([bd,%pc,%rn.size*scale],od) */
	case PCPOST:		/* ([bd,%pc],%rn.size*scale,od) */
		printf("([");
		if (arg->type != UNKNOWN) { print_value(arg); printf(",");}
		printf("%s", convert_reg(arg->reg));
		if (arg->mode == MEMPOST || arg->mode == PCPOST)
			printf("]");
		if (arg->index != -1)
			printf(",%s.%c*%c",
			   convert_reg(arg->index),	/* index register */
			   arg->word_index ? 'w' : 'l',	/* length of index */
			   arg->scale);
		if (arg->mode == MEMPRE || arg->mode == PCPRE)
			printf("]");
		if (arg->od_type != UNKNOWN) { printf(","); print_od(arg); }
		printf(")");
		break;
#endif PREPOST
#endif

	default:
		internal_error("print_arg: strange mode %d", arg->mode); 
		break;

	} /* end switch */
}



/*
 * Convert the internal representation in r
 * to a two-character external representation in buf
 */
char *convert_reg(r)
register int r;
{
	static char *regs[16] = {
		"%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7",
		"%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%a6", "%sp"
		};

	if (isAD(r))				/* is it a normal register? */
		return regs[r];

	else if (r==reg_pc)			/* is it the program counter? */
		return "%pc";

	else if (r==reg_za0)			/* is it a supp. register ? */
		return "%za0";

	else {					/* it's an error! */
		internal_error("convert_reg: impossible register %d", r);
		return "<impossible register>";
	}

}


print_value(arg)
register argument *arg;
{
#ifdef DEBUG
	if (debug1)
		printf("[type %d]", arg->type);
#endif DEBUG
	switch (arg->type) {

	case STRING:
#ifdef DEBUG
		if (debug1)
			printf("[%x]", arg->string);
#endif DEBUG
		printf("%s", arg->string);
		break;

	case INTVAL:
		printf("%d", arg->addr);
		break;

	case INTLAB:
		printf("L%d", arg->labno);
		break;

	default:
		internal_error("<unknown type %d>", arg->type);
		break;
	}
}

print_imm_value(arg)
register argument *arg;
{
	register char *fmt;

	switch (arg->type) {

	case STRING:
		printf("%s", arg->string);
		break;

	case INTVAL:
		/* Print funny stuff as hex, otherwise decimal */
		if (secret_hex_flag)			/* they want hex */
			fmt="0x%x";
		else
			fmt="%d";

		printf(fmt, arg->addr);
		secret_hex_flag = false;
		break;

	case INTLAB:
		printf("L%d", arg->labno);
		break;

	default:
		internal_error("<unknown type %d>", arg->type);
		break;
	}
}




#ifdef PREPOST
print_od(arg)
register argument *arg;
{
	register char *fmt;

	if (debug1)
		printf("[od_type %d]", arg->type);
	switch (arg->od_type) {

	case STRING:
		if (debug1)
			printf("[%x]", arg->odstring);
		printf("%s", arg->odstring);
		break;

	case INTVAL:
		/* Print funny stuff as hex, otherwise decimal */
		if (abs(arg->odaddr)>1024			/* if too big */
		|| secret_hex_flag			/* or they want hex */
		   && (arg->odaddr>9 || arg->odaddr<0))	/* and not one digit */
			fmt="0x%x";
		else
			fmt="%d";

		printf(fmt, arg->odaddr);
		secret_hex_flag = false;
		break;

	case INTLAB:
		printf("L%d", arg->odlabno);
		break;

	default:
		internal_error("<unknown type %d>", arg->od_type);
		break;
	}
}
#endif PREPOST

#ifdef M68020
print_bit_fld(p)
register node *p;
{
	printf("{&%d:&%d}", (int) p->offset, (int) p->width );
}
#endif
