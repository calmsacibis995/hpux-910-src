/* @(#) $Revision: 70.1 $ */      

# include <stdio.h>

# ifdef SEQ68K
# include "seq.a.out.h"
# include "exthdr.h"
# else
# include <a.out.h>
# endif

# include "symbols.h"
# include "adrmode.h"
# include "align.h"
# include "icode.h"
# include "fixup.h"
# include "sdopt.h"

extern char *file;
extern char *filenames[];

extern symbol * dot;
extern short  nerrors;

#ifdef SEQ68K
extern struct seqhdr  exec_header;
extern struct seqhdr *filhdr; 	/*  &exec_header; */
#else
extern struct exec exec_header;
#endif

extern struct header_extension	debug_ext_header;

extern FILE *fdtext, *fddata, *fdmod;
extern FILE *fdtextfixup, *fddatafixup;
extern FILE *fdgntt, *fdlntt, *fdslt, *fdvt, *fdxt;
extern int gfiles_open;
extern short  ofile_open;

#ifdef PIC
extern short shlib_flag;
#endif

FILE	* fdout,
	* fdrel;

long	uptxt,
	updat,
	upbss;

long	txtsiz = 0;
long	datsiz = 0;
long	bsssiz = 0;
long	lstcnt = 0;
long	lstsiz = 0;
long	modsiz = 0;
long	gnttsiz = 0;
long	lnttsiz = 0;
long	sltsiz = 0;
long	vtsiz = 0;
long	xtsiz = 0;


extern long dottxt, dotdat, dotbss, dotgntt, dotlntt, dotslt, dotvt, dotxt;
/*extern*/ long text_tdelta;
	
long	relent = 0;
long	datrel = 0;
long	txtrel = 0;

long	codestart;
long	relstart;
aspass2()
{ 
  char errbuff[200];

  txtsiz=dottxt;
  if (uptxt = txtsiz % ALSECTION) {
  	uptxt = ALSECTION - uptxt;
  	txtsiz += uptxt;
	}
  datsiz=dotdat;
  if (updat = datsiz % ALSECTION) {
  	updat = ALSECTION - updat;
  	datsiz += updat;
	}
  bsssiz = dotbss;
  if (upbss = bsssiz % ALSECTION) {
  	upbss = ALSECTION - upbss;
  	bsssiz += upbss;
	}

  if (gfiles_open) {
	gnttsiz = dotgntt;
	lnttsiz = dotlntt;
	sltsiz  = dotslt;
	
        /* Vt is either built:						  */
        /*  1) by compilers & passed via a temp file, in which case vtsiz */
	/*     is set in the "dump_value_table" routine.                  */
	/*     At this point it's zero.                                   */
	/*  2) Via "vtbytes" pseudo ops (from a ".s" file).  In this case */
	/*     "dotvt" should have the correct size in it.                */
	/* -------------------------------------------------------------- */
	vtsiz   =  dotvt;   

	xtsiz   =  dotxt;
    }

  hash_predef_symbols();

  fixsyms();
  
  if((fdout = fopen(filenames[1],"w+"))==NULL) {
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	sprintf(errbuff, "Cannot Open Output File %s", filenames[1]);
	aerror(errbuff);
  }
  ofile_open = 1;
  setvbuf(fdout, NULL, _IOFBF, 8192);

  header(0);

#ifndef SEQ68K
  fix_version();	/* set the a_stamp field in the a.out header */
#endif

#ifdef SEQ68K
  codestart =  TEXT_PPOS;
  relstart =   RTEXT_PPOS;
#else
  codestart =  TEXT_OFFSET(exec_header);
  relstart =   RTEXT_OFFSET(exec_header);
#endif
  relent = 0;

  fseek(fdout,codestart,0);	/* position pointer past area for headers */

  /* open file decriptor for relocation records.  The mode is "w+" instead
   * of "w", because if we do a listing, we'll resuse this file descriptor
   * to read back the .o bits (data section), saving an extra file open.
   */
  if((fdrel = fopen(filenames[1],"w+"))==NULL) {
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	sprintf(errbuff, "Cannot Open Output File %s", filenames[1]);
	aerror(errbuff);
  }
  setvbuf(fdrel, NULL, _IOFBF, 4096);
  fseek(fdrel,relstart,0);

  pass2_fixup(fdtext,fdtextfixup,STEXT,0,txtsiz-uptxt-text_tdelta);

  if (uptxt>0) {
	/* at end of text, pad with 'nop's 
	 * If the section ended with data decls., the uptxt value 
	 * could be odd.  If so, put out a zero byte before generating
	 * nop's.
	 */
	if (uptxt & 0x01) {
	   gen_fill_bytes(fdout, 1, TXTFILL, SZBYTE);
	   uptxt--;
	   }

	if (uptxt>0)
	   gen_fill_bytes(fdout, uptxt>>1, TXTPAD, SZWORD);
	}

  txtrel = relent;
# ifdef DEBUG
  fflush(fdrel);
  fflush(fdout);
# endif

  /* set up to process data section */
  relent = 0;

  pass2_fixup(fddata,fddatafixup,SDATA,txtsiz,datsiz-updat);
  if (updat>0) gen_fill_bytes(fdout, updat, FILL, SZBYTE);

  datrel = relent;
  fflush(fdrel);	/* flush the buffer */
  fflush(fdout);
  if (ferror(fdrel))
#ifdef  BBA
#pragma BBA_IGNORE
#endif
  	aerror("trouble writing output file");
  /*fclose(fdrel);	/* used to close here, now left open so can
			 * reuse descriptor for lister.
			 */

  dump_module_info();

  dump_linker_symtab();

#ifdef PIC
  if (shlib_flag) {
     dump_symbol_sizes();
  }
#endif

  if (gfiles_open) 
  {
      header(0);
      fill_debug_ext_header(0);

      if (fseek(fdout, debug_ext_header.e_spec.debug_header.gntt_offset, 0) != 0)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	  aerror("seek error seeking to debugger GNTT area.");
      
      dump_debug_name_table(fdgntt, gnttsiz, SGNTT);
      dump_debug_name_table(fdlntt, lnttsiz, SLNTT);
      dump_source_line_table(fdslt, sltsiz);
      dump_value_table(fdvt, &vtsiz);		/* if via tempfile, don't know vtsize yet */
      dump_cross_reference_table(fdxt, xtsiz);

      fill_debug_ext_header(1);
      write_XDB_header();
  }

  header(1);


  fflush(fdout);
  /*fclose(fdout);	/* used to close here, now left open so can
			 * reuse descriptor for lister.
			 */


}


/* First hack at the pass 2 fixup routine.
 * We need to read back the preliminary code and the fixup records.
 *	- where no change was required echo the code bits to the .o file
 *	- do the appropriate fixups, put out final code and relocation
 *	  records.
 *	- it looks like the hardest fixups will be the branches, especially
 *	  in the face of span-dependent optimization.
 */
struct fixup_info fixrec;
extern struct r_info relbuf[];
extern int relindx;
extern long line;
extern long newdot;

/*extern long text_tdelta;*/

pass2_fixup(ifd, ifixfd, segtype, segstart, segsize)
  FILE * ifd, * ifixfd;
  unsigned int segtype;
  long segstart, segsize;
{ long segoffset = 0;
/*# if SDOPT*/
   long locadjust = 0;
/*# endif*/

  (void) rewind(ifd);
  (void) rewind(ifixfd);

  dot->stype = segtype;
  dot->svalue = newdot = segstart;

  relindx = 0;

  /* Read the fixup records */
  while (fread(&fixrec, sizeof(struct fixup_info), 1, ifixfd)==1) {
	/* reset line number in case any error messages need to 
	 * be given.
	 */
	line = fixrec.f_lineno;
	dot->svalue = fixrec.f_dotval + segstart;
# if SDOPT
	dot->svalue += locadjust;
# endif

	/* have to remove the following check if ever want two
	 * relocation records for same word.
	 */
	if (fixrec.f_location < segoffset)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("location in fixup record too small");

	/* upper bound check.  Note that for the F_ALIGN case only, the
	 * location could be equal to the segsize if the "align" was the
	 * last statement in the section.
	 */
	if ((fixrec.f_type != F_ALIGN && fixrec.f_location >= segsize /*- text_tdelta*/)
		|| fixrec.f_location > segsize)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("location in fixup record too large");

	/* echo code up to the next fixup record */
	while(segoffset < fixrec.f_location) {
	   int x;
	   putc(x=getc(ifd), fdout);
	   /*printf("echo byte = %02x\n", x);*/
	   segoffset++;
	   }

	fixrec.f_location += locadjust;

	/* read in the code bits and do the fixup */
	switch(fixrec.f_type) {
	   default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("illegal fixrec type");
		break;

	   case F_EXPR:
		/* Do the fixup to the code bits.
		 * If the symbol is now absolute, that is sufficient.
		 * Otherwise replace the fixup record with a relocation
		 * record.
		 */
		/* note - shouldn't add a value part if the symbol is
		 * undefined.  ?? won't the sym->svalue still be zero
		 * anyway.  SinceI've got a bug apparenetly not.
		 * *** comm symbols have their size in the value field.
		 * only add in symbol value for "normal defined" stype's.
		 */
	      {	 expvalue expval;
		 int nbytes;
		 short cbuf[2];
		 switch(fixrec.f_size) {
			default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			   aerror("illegal fixup size");
			   break;
			case SZBYTE:
			   nbytes = 1;
			   break;
			case SZWORD:
			   nbytes = 2;
			   break;
			case SZLONG:
			   nbytes = 4;
			   break;
			}
		fread(cbuf, 1, nbytes, ifd);
		segoffset += nbytes;
		fix_expr(&fixrec, cbuf);
		fwrite(cbuf, 1, nbytes, fdout);
		/* kludge for now so we can use the old "gen_reloc" */
		break;
	      }

	   case F_BCCDISP:
		{ short cbuf[4];
		  int nbytes;
# if SDOPT
		  if (sdopt_flag && fixrec.f_size==SZNULL) {
		  	nbytes = sdi_length[fixrec.f_sdiinfo->sdi_optype][SDUNKN];
			/*printf("nbytes in = %d\n", nbytes);*/
			locadjust += fixrec.f_sdiinfo->sdi_tdelta;
			}
		  else
# endif
			nbytes = fixrec.f_size==SZBYTE ? 2 : 
				(fixrec.f_size==SZWORD? 4:6);
				/*printf("nbytes in = %d\n", nbytes);*/
		  fread(cbuf, 1, nbytes, ifd);
		  segoffset += nbytes;
		  nbytes = fix_bcc_branch_disp(&fixrec, cbuf);
		  /*printf("nbytes out = %d\n",nbytes);*/
		  fwrite(cbuf, 1, nbytes, fdout);
		  break;
		}

# if (defined(M68881) || defined(M68851)) && defined(SDOPT)
	   /* NOTE: these fixup records are only generated for
	    * span-dependent instructions, so we can assume that
	    * is the case, and don't need the special checks that
	    * you see in the F_BCCDISP case.
	    */
	   case F_CPBCCDISP:
		{ int nbytes;
		  short cbuf[3];
		  nbytes = 6;	/* sdi_length[SDCPBCC][SDUNKN] */
		  locadjust += fixrec.f_sdiinfo->sdi_tdelta;
		  fread(cbuf, 1, nbytes, ifd);
		  segoffset += nbytes;
		  if (fixrec.f_sdiinfo->sdi_size==SZLONG)
			cbuf[0] |= 0x0040;
		  nbytes = 2 + fix_branch_disp(&fixrec, &cbuf[1]);
		  fwrite(cbuf, 1, nbytes, fdout);
		  break;
		}
# endif

	   case F_PCDISP:
		{ short cbuf[2];
		  int nbytes;
# if SDOPT
		  if (sdopt_flag && fixrec.f_size==SZNULL) {
		  	nbytes = sdi_length[fixrec.f_sdiinfo->sdi_optype][SDUNKN];
			locadjust += fixrec.f_sdiinfo->sdi_tdelta;
			}
		  else
# endif
			nbytes = fixrec.f_size==SZBYTE ? 1 : 
				(fixrec.f_size==SZWORD? 2:4);

		  fread(cbuf, 1, nbytes, ifd);
		  segoffset += nbytes;
		  fix_branch_disp(&fixrec, cbuf);
		  fwrite(cbuf, 1, nbytes, fdout);
		  break;
		}

	   case F_ALIGN:
		{
		  gen_reloc(fixrec.f_location, fixrec.f_symptr, SZALIGN, 0);
		  break;
		}

# ifdef SDOPT
	   case F_LALIGN:
		/* Fix up an lalignment after span-dependent optimization.
		 * There is a corresponding span-dependent record that
		 * contains all the needed info (*fixrec.f_sdiinfo).
		 * The relavent fields in the sdi struct are:
		 *	sdi_span :	the lalignment value.
		 *	sdi_offset:	final number of filler bytes
		 *	sdi_tdelta:	the change in number of filler
		 *			bytes, from the initial worst
		 *			case assignment of (lalignval-2).
		 *	sdi_location0:	original location of lalign.
		 *	sdi_location:	final location after span-dependent.
		 */
		{ 
		  struct sdi * psdi;
		  int nskip;

		  psdi = fixrec.f_sdiinfo;
# ifdef SDDEBUG
		  {
		  /* while debugging, recalculate the lalignment based on
		   * current location info and verify that this agrees
		   * with the calculations done during the span-dependent
		   * pass.
		   * lalmod = fixrec.f_offset contains the alignment.
		   * During pass1, (lalmod-2) "placeholder" bytes were
		   * emitted.
		   * f_location has already been updated by "locadjust".
		   */

		  int lalmod = fixrec.f_offset;
		  int  ngap;

		  /* calculate the size filler needed now to generate the
		   * correct lalignment.
		   */
		  ngap = fixrec.f_location % lalmod;
		  if (ngap != 0)
			ngap = lalmod - ngap;

		  /* excess filler from pass1.*/
		  nskip = (lalmod - 2) - ngap;

		  if (ngap != psdi->sdi_offset ||
		      nskip != -psdi->sdi_tdelta ||
		      fixrec.f_location != psdi->sdi_location ||
		      lalmod != psdi->sdi_span)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			aerror("lalign fixup integrity check failed");

		  printf("lalmod = %d f_location = %d ngap = %d nskip = %d\n",
		    lalmod, fixrec.f_location, ngap, nskip);
		  }
# endif  /* SDDEBUG */

		  /* skip over the excess fill bytes.
		   * Update "segoffset" to reflect that "nskip" bytes
		   * will have been read from the intermediate file;
		   * update "locadjust" to relect "nskip" bytes that
		   * are discarded.
		   */
		  nskip = -psdi->sdi_tdelta;
		  segoffset += nskip;
		  locadjust -= nskip;
		  while(nskip-- > 0) getc(ifd);

		  /* now we just continue, and the remaining "ngap"
		   * filler bytes will get naturally echoed out.
		   */
		  break;
		}
# endif
	   }	/* switch f_type */

	if (relindx>0) {
#ifdef SEQ68K
	   fwrite(relbuf, sizeof(struct reloc), relindx, fdrel);
#else
	   fwrite(relbuf, sizeof(struct r_info), relindx, fdrel);
#endif
	   relindx = 0;
	   }
	
	}	/* while fread next fixup record */

  /* echo the remaining code bits after the last fixup */
  while(segoffset++ < segsize /*- text_tdelta*/) {
	putc(getc(ifd), fdout);
	}
}


/*
 * dump the module info into the .o file
 *
 */

dump_module_info()
{ 
    register char c;

    if (modsiz) {
       rewind(fdmod);
    
       while((c=getc(fdmod)) != EOF)  {
	  putc(c, fdout);
       }
    
       fclose(fdmod);
    }
}
