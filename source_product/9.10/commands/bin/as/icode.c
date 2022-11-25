/* @(#) $Revision: 70.1 $ */      

# include <stdio.h>
# include "symbols.h"
# include "adrmode.h"
# include "ivalues.h"
# include "asgram.h"
# include "icode.h"

# ifdef SEQ68K
# include "seq.a.out.h"
# else
# include <a.out.h>
# endif

# include  "fixup.h"

/*******************************************************************************
 *** Intermediate code generation routines ***
 */


extern FILE *fdtext, *fddata;
extern FILE *fdtextfixup, *fddatafixup;
extern FILE *fdcsect;
extern FILE *fdcsectfixup;
extern symbol * dot;
extern unsigned long line;
extern long newdot;

long fixindx;
long fixent;
struct fixup_info  fixbuf[FIXBUFSZ];

extern int codeindx;
extern char * codebuf;

extern short nerrors;
extern short nerrors_this_stmt;
extern struct sdi * makesdi_lalignnode();

#ifdef PIC
extern short data_flag;
#endif

/* VARARGS */
generate_icode(icode, value_ptr, count)
		unsigned int icode;
		char * value_ptr;
		unsigned int count;
{
	if (dot->stype & (STEXT|SDATA))
		goto stype_ok;
	if (dot->stype & SBSS)
		if (count != 0) {
			uerror("Attempt to initialize bss");
			return;
			}
		/* otherwise, can write on any file */
		else goto stype_ok;

	if (dot->stype & (SGNTT|SLNTT|SSLT|SVT|SXT))
		uerror("illegal section (gntt, lntt, slt, vt, or xt)");
		return;

	stype_ok:
	/*if (nerrors) return;*/
	if (nerrors_this_stmt) return;
	switch(icode) {

		default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			aerror("unrecognized icode value");
			break;

		case I_FILL:
			{ 
			  /* Future enhancement: if the fill size exceeds
			   * some base size, should build a fixup
			   * record instead.
			   */

			  gen_fill_bytes(fdcsect, count, (int)value_ptr, SZBYTE );
			  newdot += count;
			  break;
			}

		/* NOTE: "count" parameter not passed in any of the
		 * 	 INSTRUCTION cases.
		 */
		case I_INSTRUCTION:
			build_instruction_bits((struct instruction *) value_ptr);
			break;

# ifdef M68881
		case I_FPINSTRUCTION0:
		case I_FPINSTRUCTION1:
		case I_FPINSTRUCTION2:
		case I_FPINSTRUCTION4:
		case I_FPINSTRUCTION5:
		     {  int cptype;
			cptype = icode-I_FPINSTRUCTION0;
			if (cptype==0) build_fpinstruction_bits((struct instruction *) value_ptr, cptype);
			else build_cpinstruction_bits((struct instruction *) value_ptr, cptype);
			break;
		     }

# endif

# ifdef M68851
		case I_PMUINSTRUCTION0:
		case I_PMUINSTRUCTION1:
		case I_PMUINSTRUCTION2:
		case I_PMUINSTRUCTION4:
		case I_PMUINSTRUCTION5:
		     {  int cptype;
			cptype = icode-I_PMUINSTRUCTION0;
			if (cptype == 0) build_pmuinstruction_bits((struct instruction *) value_ptr, cptype);
			else build_cpinstruction_bits((struct instruction *) value_ptr, cptype);
			break;
		     }
# endif

			{ int nbytes;
			  int expsize;
			  rexpr * exp;
			  /* expsize should really be set in asgram.y */
		case I_BYTE:
			  nbytes = 1;
			  expsize = SZBYTE;
			  goto do_data;
		case I_SHORT:
			  nbytes = 2;
			  expsize = SZWORD;
			  goto do_data;
		case I_LONG:
			  nbytes = 4;
			  expsize = SZLONG;
			  goto do_data;
		case I_FLOAT:
			  nbytes = 4;
			  expsize = SZSINGLE;
			  goto do_data;
		case I_DOUBLE:
			  nbytes = 8;
			  expsize = SZDOUBLE;
			  goto do_data;
		case I_PACKED:
			  nbytes = 12;
			  expsize = SZPACKED;
			  goto do_data;
		case I_EXTEND:
			  nbytes = 12;
			  expsize = SZEXTEND;
			  goto do_data;
			do_data:
			  exp = (rexpr *) value_ptr;
			  /* generate expression from exp, nbytes */
# ifdef PIC
			  data_flag = 1;
# endif
			  build_expr(codebuf+codeindx, exp, expsize);
# ifdef PIC
			  data_flag = 0;
# endif
			  codeindx += nbytes;
			  break;
			}
		case I_STRING:
			{ int  i;
			  for(i=0; i<count; i++,value_ptr++)
				fputc(*value_ptr,fdcsect);
			  newdot += count;
			  break;
			}
		case I_ALIGN:
			{ /* generate a relocation record for the alignment
			   * symbol.
			   */
			  /*symbol * sym;
			  /*sym = (symbol *) getw(ifd);
			  /* validity checks */
			  /*if ((sym->stype&STYPE)!=segtype)
				/*aerror("alignment symbol type mismatch");
			  /*if (sym->svalue != newdot)
				/*aerror("alignment symbol value/dot mismatch");
			  /* ****NOTE:
			     ****need some test for mixing of alignment/local
			   * symbols in the same segment. */
			  /* **** Need to set some king of flag here so won't
			   *      try to adjust relocation address below.
			   */
			  /*gen_reloc(newdot-segstart, sym, SZALIGN);
			  /*break;
			  */

			 /* generate a fixup record */
   			register struct fixup_info * fix;

   			if (fixindx >= FIXBUFSZ)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
				aerror("fixbuf overflow");
   
   			fixent++;
   			fix = &fixbuf[fixindx++];

   			fix->f_type = F_ALIGN;
   			fix->f_size = 0;
   			fix->f_location = newdot;
   			fix->f_dotval = dot->svalue;
   			fix->f_pcoffs = 0;	/* not used */
   			fix->f_lineno = line;
   			fix->f_symptr = * (symbol **) value_ptr;
   			fix->f_offset = 0;
   			fix->f_exptype = 0;
			break;

			}

# ifdef SDOPT
			/* these are never needed accept when doing
			 * span-dependent optimization.
			 */
		case I_LALIGN:
			{ /* generate a fixup record */
   			register struct fixup_info * fix;

   			if (fixindx >= FIXBUFSZ)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
				aerror("fixbuf overflow");
   
   			fixent++;
   			fix = &fixbuf[fixindx++];

   			fix->f_type = F_LALIGN;
   			fix->f_size = 0;	/* not used */
   			fix->f_location = newdot;
   			fix->f_dotval = dot->svalue;
   			fix->f_pcoffs = 0;	/* not used */
   			fix->f_lineno = line;
   			fix->f_symptr = 0;	/* not used */
   			fix->f_offset = (int) value_ptr;  /* the lalmod */
   			fix->f_exptype = 0;
			fix->f_sdiinfo = makesdi_lalignnode(value_ptr);
			break;
			}
# endif

		}  /* end switch */


	/* flush code and reloc buffers */
	if (codeindx>0) {
	   fwrite(codebuf, codeindx, 1, fdcsect);
#ifdef SHOW_BITS
        print_bits (codebuf, codeindx);
#endif
	   newdot += codeindx;
	   codeindx = 0;
	}
	if (fixindx>0) {
	   long fixadjust;
	   int i;
	   int ftype;
	   fixadjust = dot->svalue - (long) codebuf;
	   for (i=0; i< fixindx; i++) {
		if ((ftype=fixbuf[i].f_type) != F_ALIGN && 
			ftype != F_LALIGN)
		   fixbuf[i].f_location += fixadjust;
		}
	   fwrite(fixbuf, sizeof(struct fixup_info), fixindx, fdcsectfixup);
	   fixindx = 0;
	   }
# ifdef DEBUG
	/* while debugging: */
	fflush(fdcsect);
	fflush(fdcsectfixup);
# endif

	/* update value of dot */
	/* Update the value of dot at the end of each icode "transaction".
	 * This should keep the new pass1 bit generation in synch with
	 * the way newdot used  to be updated in pass2 as bits were
	 * generated.  The primary consideration here is that an uptodate
	 * dotval be available for expressions like ".-L1" which will
	 * get calculated during pass1 (except when -O option is on).
	 */
	dot->svalue = newdot;

}

#ifdef SHOW_BITS
print_bits (buffer, num_bytes)
char	*buffer;
int		num_bytes;
{
	int	i;
	while (num_bytes-- > 0)	{
		for (i = 0; i < 8; i++)
			if ( (*buffer << i) & 0x80)
				printf ("1");
			else
				printf ("0");
		printf (" ");
		buffer++;
	}
	printf ("\n");
}
#endif


/*  generate count of "fillval" to the file fd.
 *  The size of each value is determined by size.
 *  This routine is used both in generating the intermediate code
 *  file, and in some cases, in pass2, to generate fill bytes in the
 *  final file.
 *  ??? modify to add a data size parameter, then could use in conjunction
 *  with a replication factor in data declarations.
 */

gen_fill_bytes(fd, count, fillval, size)
  FILE *fd;
  int count;
  int fillval;
{
  if (size==SZBYTE)
	while(count--) putc(fillval, fd);
  else if (size==SZWORD) {
	int hibyte, lobyte;
	hibyte = fillval>>8;
	lobyte = fillval & 0xff;
	while(count--) { 
	   putc(hibyte, fd);
	   putc(lobyte, fd);
	   }
	}
  else if (size==SZLONG)
	while(count--) putw(fillval, fd);
  else 
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("gen_fill called with illegal size");
}

