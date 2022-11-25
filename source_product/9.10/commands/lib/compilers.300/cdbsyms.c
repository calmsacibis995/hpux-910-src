/* file cdbsyms.c */
/*    SCCS    REV(64.5);       DATE(92/05/01        14:20:26) */
/* KLEENIX_ID @(#)cdbsyms.c	64.5 92/04/30 */
/* This file contains routines to generate symbol table information for
 * HP's implementation of cdb.
 */
# include <string.h>
# include <malloc.h>
# include  "mfile1"
# ifdef SA
# include "sa.h"
# endif

# ifdef HPCDB	/*  encloses the rest of this file */

# include "dnttsizes.h"
# include "vtlib.h"

/* # define DEBUG */

#if ( defined DEBUG || !defined IRIF )
#     define DEBUGornotIRIF
#endif /* defined DEBUG || !defined IRIF */

unsigned long *atype_dntt;

#ifndef IRIF
        extern FILE * outfile;	/* defined in code.c */
        extern FILE *dtmpfile;  /* gntt table output, defined in code.c */
        extern FILE *ltmpfile;  /* lntt table output, defined in code.c */
#else /* IRIF */
       int dntt_type;
# ifdef DEBUG
       static FILE *outfile = stderr;
# endif /* DEBUG */
#endif /* IRIF */

extern int lineno;	/* Compiler's current source line  counter */

flag  cdbflag = 0;	/* Will be set to 1 in main() (scan.c) if ccom is
			 * invoked with -g  */

LOCAL int sltindex = 0;  /* index of next slt to be emitted  */
LOCAL int save_ft = 0;   /* save file table index */

#ifndef IRIF
        LOCAL int gnttindex = 0; /* index of next gntt to be emitted */
        LOCAL int lnttindex = 0; /* index of next lntt to be emitted */
#else /* IRIF */
        static int symbolic_id = 0x80000000;  /* use instead of dntt indexes */
#endif /* IRIF */

LOCAL unsigned long build_name_dntt();
LOCAL unsigned long build_struct_dntt();
LOCAL unsigned long build_enum_dntt();

#ifdef DEBUGornotIRIF

/* GLAB_FMT gives the format of a label in gntt space.                      */
/* LLAB_FMT gives the format of a label in lntt space.                      */

# define GLAB_FMT_DEF "GD%d: "
# define GLAB_FMT_REF "GD%d"
# define LLAB_FMT_DEF "LD%d: "
# define LLAB_FMT_REF "LD%d"

#endif /* DEBUGornotIRIF */

#ifndef IRIF
#       define EXNAME(sym)  exname(sym->sname)
#else /* IRIF */
#       define EXNAME(sym)  NULL
#endif /* IRIF */

# define CCOM_OFF 5   /* offset from sizoff of CCOM_DNTTPTR for str/union */

/*********************************************************************/
/* initialize cdb by establishing the vt error handling routine.     */

void vt_error_handler(msg) char *msg;
{
cerror(msg);
}

void cdb_init() {
     register_vt_error_callback (vt_error_handler);
}


/**************************************************************************/
/* Following are the routines to emit an sltexit, sltnormal & sltspecial  */
/* They each emit the current lineno and assume that the current location */
/* is the 'text' segment (i.e. PROG).  They each return the sltindex of   */
/* the entry emitted.                                                     */

#ifndef IRIF 

int sltexit()
  {
#ifdef DEBUG
  fprntf(outfile,"(%x)", sltindex);
#endif
  fprntf(outfile,"\tsltexit %d\n",lineno);
  return(sltindex++);
  }

int sltnormal()
  {
#ifdef DEBUG
  fprntf(outfile,"(%x)", sltindex);
#endif
  fprntf(outfile,"\tsltnormal %d\n",lineno);
  return(sltindex++);
  }

LOCAL int sltspecial(slttype, sltlineno, dnttptr) 
  SLTTYPE slttype;
  int sltlineno;
  unsigned int dnttptr;		/* a ccom_dntpt */
  {
#ifdef DEBUG
  fprntf(outfile,"(%x)", sltindex);
#endif
  fprntf(outfile,"\tsltspecial %d,%d,",slttype,sltlineno);
  putdnttpointer(dnttptr);
  (void)fputc('\n',outfile);
  return(sltindex++);
  }

#else  /*IRIF */

int sltexit() {
     static struct SLT_NORM
	  SLT_NORM = { SLT_EXIT, 0, 0 };
     
#ifdef DEBUG
     fprntf(outfile,"(%x)", sltindex);
     fprntf(outfile,"\tsltexit %d\n",lineno);
#endif
     ir_loc( lineno );
     SLT_NORM.line = lineno;
     SLT_NORM.address = lineno;
     ir_slt( SLT_EXIT, &SLT_NORM );

     return(sltindex++);
}

int sltnormal() {
     static struct SLT_NORM
	  SLT_NORM = { SLT_NORMAL, 0, 0 };
     
#ifdef DEBUG
     fprntf(outfile,"(%x)", sltindex);
     fprntf(outfile,"\tsltnormal %d\n",lineno);
#endif
     ir_loc( lineno );
     SLT_NORM.line = lineno;
     SLT_NORM.address = lineno;
     ir_slt( SLT_NORMAL, &SLT_NORM );

     return(sltindex++);
}

LOCAL int sltspecial(slttype, sltlineno, dnttptr) 
     SLTTYPE slttype;
     int sltlineno;
     unsigned int dnttptr;		/* a ccom_dntpt */ 
{
     struct SLT_SPEC SLT_SPEC;

#ifdef DEBUG
     fprntf(outfile,"(%x)", sltindex);
     fprntf(outfile,"\tsltspecial %d,%d,",slttype,sltlineno);
     putdnttpointer(dnttptr);
     (void)fputc('\n',outfile);
#endif
     SLT_SPEC.sltdesc = slttype;
     SLT_SPEC.line = sltlineno;
     SLT_SPEC.backptr.word = dnttptr;

     ir_slt( slttype, &SLT_SPEC );

     return(sltindex++);
}
#endif /* IRIF */

/************************************************************/
/* emit a dnttpointer -- given the compilers internal form  */

#ifdef DEBUGornotIRIF

LOCAL putdnttpointer(ccom_dp) CCOM_DNTTPTR  ccom_dp;
  {
  switch(ccom_dp.dnttptr_symbolic.dptrkind)
    {
    case DP_LNTT_SYMBOLIC:
      fprntf(outfile,LLAB_FMT_REF,ccom_dp.dnttptr_symbolic.labnum);
      break;
    case DP_GNTT_SYMBOLIC:
      fprntf(outfile,GLAB_FMT_REF,ccom_dp.dnttptr_symbolic.labnum);
      break;
    case DP_LNTT_INDEXED :
    case DP_GNTT_INDEXED :
    case DP_IMMED :
    case DP_NIL :
      fprntf(outfile,"0x%x",ccom_dp.dnttptr_ccom);
      break;
    default:
      cerror("illegal type in internal dnttptr: %x",ccom_dp.dnttptr_ccom);
      break;
    };
  }

LOCAL int lntt_labnum = 1;  /* next lntt label-number to be allocated */
LOCAL int gntt_labnum = 1;  /* next gntt label-number to be allocated */

LOCAL int get_lntt_labnum()
  { return(lntt_labnum++); }

LOCAL int get_gntt_labnum()
  { return(gntt_labnum++); }

LOCAL def_lntt_labnum(labnum) int labnum; {
     fprntf(outfile,LLAB_FMT_DEF,labnum);
}

LOCAL def_gntt_labnum(labnum) int labnum; {
     fprntf(outfile,GLAB_FMT_DEF,labnum);
}
#endif /* DEBUGornotIRIF */

/**************************************************************************/
/* The following routine is called to actually print out a dnttentry.  It
 * returns the index (lntt or gntt) of the dnttentry that is output.
 * By now, all the real work is done -- this routine just knows the print
 * format required by each type of entry.  */

char * dnt_opname[] = {
	"dnt_srcfile",	"dnt_module",	"dnt_function",
	"dnt_entry",	"dnt_begin",	"dnt_end",
	"dnt_import",	"dnt_label",	"dnt_fparam",
	"dnt_svar",	"dnt_dvar",		   0,
	"dnt_const",	"dnt_typedef",	"dnt_tagdef",
	"dnt_pointer",	"dnt_enum",	"dnt_memenum",
	"dnt_set",	"dnt_subrange",	"dnt_array",
	"dnt_struct",	"dnt_union",	"dnt_field",
	"dnt_variant",	"dnt_file",	"dnt_functype",
        	0,               0,                 0,
        "dnt_xref",	"dnt_sa"};

# define PUTCOMMA  (void)fputc(',',outfile)

/*VARARGS1*/
LOCAL unsigned int
dnttentry(kind, arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11)
  KINDTYPE kind;
  int arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10,arg11;

  {
  register int * arg = (int *) &kind;
  unsigned int odnttindex;
  long vtindex, vtindex1;

#ifdef IRIF
  union dnttentry
       DNTTentry;
  int sp = 0;
#endif /* IRIF */

#ifdef DEBUG
#ifndef IRIF
  fprntf(outfile,"(%x)",outfile==dtmpfile ? gnttindex : lnttindex);
#else /* IRIF */
  fprntf(outfile,"(%x)", symbolic_id );
#endif /* IRIF */
#endif /* DEBUG */

#ifdef DEBUGornotIRIF
  fprntf(outfile,"\t%s\t",dnt_opname[kind]);
#endif /* DEBUGornotIRIF */

  switch(kind)
    {

    case K_MODULE:
	/* in:  sltindex                         */ 
	/* out: modname(nil),alias(nil),sltindex */

#ifdef DEBUGornotIRIF
	fprntf(outfile,"%d,%d,%d",VTNIL,VTNIL,arg1);
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	DNTTentry.dmodule.unused = 0;
	DNTTentry.dmodule.name = VTNIL;
	DNTTentry.dmodule.alias = VTNIL;
	DNTTentry.dmodule.dummy.word = DNTTNIL;
	DNTTentry.dmodule.address = arg1;
#endif /* IRIF */

	break;

    case K_SRCFILE:
	/* in:  cdbfile_vtindex,sltindex        */
	/* out: LANG_C,filename vt index,sltindex */


#ifdef DEBUGornotIRIF
	fprntf(outfile,"%d,%d,%d",LANG_C,arg1,arg2);
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	DNTTentry.dsfile.language = LANG_C;
	DNTTentry.dsfile.unused = 0;
	DNTTentry.dsfile.name = arg1;
	DNTTentry.dsfile.address = arg2;
#endif /* IRIF */

	break;

    case K_FUNCTION:
	/* in:  public,optimize,varargs,compatablilty-info,symtab,alias,  */
	/*	firstparam,sltindex,entryaddr,retval,highaddr		  */
	/* out: public,LANG_C,level,optimize,varargs,compatibility-info,  */
	/*	name,alias,firstparam,sltindex,entryaddr,retval,	  */
	/*	lowaddr,highaddr					  */

	vtindex = add_to_vt( ((struct symtab *)arg5)->sname,TRUE,TRUE);
	if (arg6) {
	     /* non-null alias name */
	     vtindex1 = add_to_vt(arg6,TRUE,TRUE);
	}
	else vtindex1 = VTNIL;

#ifdef DEBUGornotIRIF
	fprntf(outfile,"%d,%d,0,%d,%d,%d,0,%d,",
		arg1,LANG_C,arg2,arg3,arg4,vtindex);
	fprntf(outfile,"%d,",vtindex1);

	putdnttpointer(arg7);
	fprntf(outfile,",%d,%s,",arg8,arg9);
	putdnttpointer(arg10);
	fprntf(outfile,",%s,%s",arg9,arg11);
#endif /* DEBUGornotIRIF */ 

#ifdef IRIF
	DNTTentry.dfunc.public = arg1;
	DNTTentry.dfunc.language = LANG_C;
	DNTTentry.dfunc.level = 0;
	DNTTentry.dfunc.optimize = arg2;
	DNTTentry.dfunc.varargs = arg3;
	DNTTentry.dfunc.info = arg4;
	DNTTentry.dfunc.unused = 0;
	DNTTentry.dfunc.name = vtindex;
	DNTTentry.dfunc.alias = vtindex1;
	DNTTentry.dfunc.firstparam.word = arg7;
	DNTTentry.dfunc.address = arg8;
	DNTTentry.dfunc.entryaddr = vtindex;
	DNTTentry.dfunc.retval.word = arg10;
	DNTTentry.dfunc.lowaddr = /* vtindex */ 0;
	DNTTentry.dfunc.hiaddr = /* arg11 */ 0;
	sp = arg5;
#endif /* IRIF */

	break;

    case K_BEGIN:
	/* in:  sltindex */

#ifdef DEBUGornotIRIF
	fprntf(outfile,"0,%d",arg1);
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	DNTTentry.dbegin.unused = 0;
	DNTTentry.dbegin.address = arg1;
#endif /* IRIF */

	break;

    case K_END:
	/* in:  endkind,sltindex,beginscope */
	/* out: endkind,sltindex,beginscope */

#ifdef DEBUGornotIRIF
	fprntf(outfile,"%d,0,%d,",arg1,arg2);
	putdnttpointer(arg3);
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	DNTTentry.dend.endkind = arg1;
	DNTTentry.dend.unused = 0;
	DNTTentry.dend.address = arg2;
	DNTTentry.dend.beginscope.word = arg3;
#endif /* IRIF */	

	break;

    case K_LABEL:
	/* in:  symtab,slt-index */
	/* out: name,slt-index */

	vtindex = add_to_vt( ((struct symtab *)arg1)->sname, TRUE,TRUE);

#ifdef DEBUGornotIRIF
	fprntf(outfile,"%d,%d",vtindex,arg2);
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	DNTTentry.dlabel.unused = 0;
	DNTTentry.dlabel.name = vtindex;
	DNTTentry.dlabel.address = arg2;
	sp = arg1;
#endif /* IRIF */

	break;

    case K_FPARAM:
	/* in:  regparam,indirect,symtab,offset,type,next-param      */
	/* out: regparam,indirect,longaddr=0,copyparam=0, */
	/*	name,offset,type,next-param,misc=0	    */

	vtindex = (arg3==VTNIL) ? VTNIL : 
	     add_to_vt( ((struct symtab *)arg3)->sname, TRUE, TRUE );

#ifdef DEBUGornotIRIF
	fprntf(outfile,"%d,%d,0,0,0,%d,%d,",arg1,arg2,vtindex,arg4);
	putdnttpointer(arg5);
	PUTCOMMA;
	putdnttpointer(arg6);
	fprntf(outfile,",0");
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	DNTTentry.dfparam.regparam = arg1;
	DNTTentry.dfparam.indirect = arg2;
	DNTTentry.dfparam.longaddr = 0;
	DNTTentry.dfparam.copyparam = 0;
	DNTTentry.dfparam.unused = 0;
	DNTTentry.dfparam.name = vtindex;
	DNTTentry.dfparam.location = 0;    /* IRIF fills this in */
	DNTTentry.dfparam.type.word = arg5;
	DNTTentry.dfparam.nextparam.word = arg6;
	DNTTentry.dfparam.misc = 0;
	sp = arg3;
#endif /* IRIF */

	break;

    case K_SVAR:
	/* in:  public,indirect,symtab,label,type	  */
	/* out: public,indirect,longaddr=0,name,label, */
	/*	type,offset=0,displacement=0		  */

	vtindex = add_to_vt( ((struct symtab *)arg3)->sname ,TRUE, TRUE );

#ifdef DEBUGornotIRIF
	fprntf(outfile,"%d,%d,0,0,0,%d,%s,",arg1,arg2,vtindex,arg4);
	putdnttpointer(arg5);
	fprntf(outfile,",0,0");
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	
	DNTTentry.dsvar.public = arg1;
	DNTTentry.dsvar.indirect = arg2;
	DNTTentry.dsvar.longaddr = 0;
	DNTTentry.dsvar.unused = 0;
	DNTTentry.dsvar.name = vtindex;
	DNTTentry.dsvar.location = 0;
	DNTTentry.dsvar.type.word = arg5;
	DNTTentry.dsvar.offset = 0;
	DNTTentry.dsvar.displacement = 0; /* IRIF figures this out */
	sp = arg3;
#endif /* IRIF */

	break;

    case K_DVAR:
	/* in:  public,indirect,regvar,symtab,stackoffset/reg#,type          */
	/* out: public,indirect,regvar,name,stackoffset/reg#,type,offset=0 */
	/* map +bfpa flt regnums to generic range for cdb 	           */

#ifndef IRIF
	if (arg3 && (fpaflag > 0) && (arg5 >= REG_FP0)) 
	  arg5 = arg5 + (REG_FGEN0 - REG_FPA0);
#endif /* not IRIF */
	vtindex = add_to_vt( ((struct symtab *)arg4)->sname ,TRUE, TRUE );
 
#ifdef DEBUGornotIRIF
	fprntf(outfile,"%d,%d,%d,0,%d,%d,",arg1,arg2,arg3,vtindex,arg5);
	putdnttpointer(arg6);
	fprntf(outfile,",0");
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	DNTTentry.ddvar.public = arg1;
	DNTTentry.ddvar.indirect = arg2;
	DNTTentry.ddvar.regvar = arg3;
	DNTTentry.ddvar.unused = 0;
	DNTTentry.ddvar.name = vtindex;
	DNTTentry.ddvar.location = 0;     /* IRIF figures this out */
	DNTTentry.ddvar.type.word = arg6;
	DNTTentry.ddvar.offset = 0;
	sp = arg4;
#endif /* IRIF */

	break;

# ifdef SA
    case K_XREF:
       /* in:  (K_XREF,xt_index,xt_index_is_symbolic) */
       /* out: dnt_xref        LANG_C,xt_index    */

#ifdef DEBUGornotIRIF
       if (arg2)
	  fprntf(outfile, "%d,%s", LANG_C, arg1 );
       else    
	  fprntf(outfile, "%d,%d", LANG_C, arg1 );
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	DNTTentry.dxref.language = LANG_C;
	DNTTentry.dxref.unused = 0;

	/* UCODE/SLLIC determines XREFPOINTER 'xreflist' */
	DNTTentry.dxref.xreflist = 0;

	DNTTentry.dxref.extra = 0;
#endif /* IRIF */

	break;
# endif /* SA */ 

#if( defined SA && defined IRIF ) 
      case K_SA:
	/* in:	(K_SA,base_kind,symtab)
	 * out:	dnt_sa	base_kind,name_vt_index
	 */

	vtindex = add_to_vt( ((struct symtab *)arg2)->sname, TRUE, TRUE );
	 
#ifdef DEBUG
	fprntf( outfile, "%d,%d", arg1, vtindex );
#endif /* DEBUG */

	DNTTentry.dsa.base_kind = arg1;
	DNTTentry.dsa.unused = 0;
	DNTTentry.dsa.name = vtindex;
	DNTTentry.dsa.extra = 0;
	sp = arg2;
	break;

      case K_MACRO:
	/* in:	(K_MACRO,name)
	 * out:	dnt_macro name_vt_index
	 */
	DNTTentry.dsa.base_kind = 0;
	DNTTentry.dsa.unused = 0;
	DNTTentry.dsa.name = add_to_vt( arg1, TRUE, TRUE );
	DNTTentry.dsa.extra = 0;
	break;
#endif /* defined SA && defined IRIF */
	  
    case K_TYPEDEF:
	/* in:  public,symtab,type */
	/* out: public,name,type */

	vtindex = add_to_vt( ((struct symtab *)arg2)->sname, TRUE, TRUE);

#ifdef DEBUGornotIRIF
	fprntf(outfile,"%d,%d,",arg1,vtindex);
	putdnttpointer(arg3);
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	DNTTentry.dtype.public = arg1;
	DNTTentry.dtype.typeinfo = 0;
	DNTTentry.dtype.unused = 0;
	DNTTentry.dtype.name = vtindex;
	DNTTentry.dtype.type.word = arg3;
	sp = arg2;
#endif /* IRIF */

	break;

    case K_TAGDEF:
	/* in:  typeinfo,symtab,type */
	/* out: public-bit=0,typeinfo,name,type */

	vtindex = arg2 == NULL ? 
		   VTNIL :
		   add_to_vt( ((struct symtab *)arg2)->sname, TRUE, TRUE );

#ifdef DEBUGornotIRIF
	fprntf(outfile,"0,%d,%d,",arg1,vtindex);
	putdnttpointer(arg3);
#endif /* DEBUGornotIRIF */

#ifdef IRIF
	DNTTentry.dtype.public = 0;
	DNTTentry.dtype.typeinfo = arg1;
	DNTTentry.dtype.unused = 0;
	DNTTentry.dtype.name = vtindex;
	DNTTentry.dtype.type.word = arg3;
	sp = arg2;
#endif /* IRIF */

	break;

    case K_POINTER:
	/* in:  points-to,ptr-size */
	/* out: points-to,ptr-size */

#ifdef DEBUGornotIRIF
	putdnttpointer(arg1);
	fprntf(outfile,",%d",arg2);
#endif /* DEBUGornotIRIF*/

#ifdef IRIF
	DNTTentry.dptr.unused = 0;
	DNTTentry.dptr.pointsto.word = arg1;
	DNTTentry.dptr.bitlength = arg2;
#endif /* IRIF */

	break;

    case K_ENUM:
	/* in:  first-member,bitlength */
	/* out: first-member,bitlength */

#ifdef DEBUGornotIRIF
	putdnttpointer(arg1);
	fprntf(outfile,",%d",arg2);
#endif /* DEBUGornotIRIF*/

#ifdef IRIF
	DNTTentry.denum.unused = 0;
	DNTTentry.denum.firstmem.word = arg1;
	DNTTentry.denum.bitlength = arg2;
#endif /* IRIF */

	break;

    case K_MEMENUM:
	/* in:  symtab,value,next-member */
	/* out: name,value,next-member */

	vtindex = add_to_vt( ((struct symtab *)arg1)->sname,TRUE,TRUE);

#ifdef DEBUGornotIRIF
	fprntf(outfile,"0,%d,%d,",vtindex,arg2);
	putdnttpointer(arg3);
#endif /* DEBUGornotIRIF*/

#ifdef IRIF
	DNTTentry.dmember.unused = 0;
	DNTTentry.dmember.name = vtindex;
	DNTTentry.dmember.value = arg2;
	DNTTentry.dmember.nextmem.word = arg3;
	sp = arg1;
#endif /* IRIF */

	break;

    case K_SUBRANGE:
	/* in:  lowbound,highbound,subtype,bitlength			  */
	/* out: dyn_low=0,dyn_high=0,lowbound,highbound,subtype,bitlength */

#ifdef DEBUGornotIRIF
	fprntf(outfile,"0,0,%d,%d,",arg1,arg2);
	putdnttpointer(arg3);
	fprntf(outfile,",%d",arg4);
#endif /* DEBUGornotIRIF*/

#ifdef IRIF
	DNTTentry.dsubr.dyn_low = 0;
	DNTTentry.dsubr.dyn_high = 0;
	DNTTentry.dsubr.unused = 0;
	DNTTentry.dsubr.lowbound = arg1;
	DNTTentry.dsubr.highbound = arg2;
	DNTTentry.dsubr.subtype.word = arg3;
	DNTTentry.dsubr.bitlength = arg4;
#endif /* IRIF */

	break;

    case K_ARRAY:
	/* in:  arraylength,indextype,elemtype,elemlength	    */
	/* out: declaration=0,dyn_low=0,dyn_high=0,arrayisbytes=0,  */
	/*	elemisbytes=0,elemorder=0,justified=0,arraylength,  */
	/*	indextype,elemtype,elemlength			    */

#ifdef DEBUGornotIRIF
	fprntf(outfile,"0,0,0,0,0,0,0,%d,",arg1);
	putdnttpointer(arg2);
	PUTCOMMA;
	putdnttpointer(arg3);
	fprntf(outfile,",%d",arg4);
#endif /* DEBUGornotIRIF*/

#ifdef IRIF
	DNTTentry.darray.declaration = 0;
	DNTTentry.darray.dyn_low = 0;
	DNTTentry.darray.dyn_high = 0;
	DNTTentry.darray.arrayisbytes = 0;
	DNTTentry.darray.elemisbytes = 0;
	DNTTentry.darray.elemorder = 0;
	DNTTentry.darray.justified = 0;
	DNTTentry.darray.unused = 0;
	DNTTentry.darray.arraylength = arg1;
	DNTTentry.darray.indextype.word = arg2;
	DNTTentry.darray.elemtype.word = arg3;
	DNTTentry.darray.elemlength = arg4;
#endif /* IRIF */

	break;

    case K_STRUCT:
	/* in:  firstfield,bitlength 			      */
	/* out: declaration=0,firstfield,vartagfield=DNTTNIL, */
	/*	varlist=DNTTNIL,bitlength		      */

#ifdef DEBUGornotIRIF
	fprntf(outfile,"0,");
	putdnttpointer(arg1);
	PUTCOMMA;
	putdnttpointer(DNTTNIL);
	PUTCOMMA;
	putdnttpointer(DNTTNIL);
	fprntf(outfile,",%d",arg2);
#endif /* DEBUGornotIRIF*/

#ifdef IRIF
	DNTTentry.dstruct.declaration = 0;
	DNTTentry.dstruct.unused = 0;
	DNTTentry.dstruct.firstfield.word = arg1;
	DNTTentry.dstruct.vartagfield.word = DNTTNIL;
	DNTTentry.dstruct.varlist.word = DNTTNIL;
	DNTTentry.dstruct.bitlength = arg2;
#endif /* IRIF */

	break;

    case K_UNION:
	/* in:  first-field,bitlength */
	/* out: first-field,bitlength */

#ifdef DEBUGornotIRIF
	putdnttpointer(arg1);
	fprntf(outfile,",%d",arg2);
#endif /* DEBUGornotIRIF*/

#ifdef IRIF
	DNTTentry.dunion.unused = 0;
	DNTTentry.dunion.firstfield.word = arg1;
	DNTTentry.dunion.bitlength = arg2;
#endif /* IRIF */

	break;

    case K_FIELD:
	/* in:  symtab,bitoffset,type,bitlength,next-field */
	/* out: name,bitoffset,type,bitlength,next-field */

	vtindex = add_to_vt( ((struct symtab *)arg1)->sname, TRUE, TRUE);

#ifdef DEBUGornotIRIF
	fprntf(outfile,"0,0,%d,%d,",vtindex,arg2);
	putdnttpointer(arg3);
	fprntf(outfile,",%d,",arg4);
	putdnttpointer(arg5);
#endif /* DEBUGornotIRIF*/

#ifdef IRIF
	DNTTentry.dfield.unused = 0;
	DNTTentry.dfield.name = vtindex;
	DNTTentry.dfield.bitoffset = arg2;
	DNTTentry.dfield.type.word = arg3;
	DNTTentry.dfield.bitlength = arg4;
	DNTTentry.dfield.nextfield.word = arg5;
	sp = arg1;
#endif /* IRIF */

	break;

    case K_FUNCTYPE:
	/* in:  varargs,info,bitlength,first-param,return-type */
	/* out: varargs,info,bitlength,first-param,return-type */

#ifdef DEBUGornotIRIF
	fprntf(outfile,"%d,%d,%d,",arg1,arg2,arg3);
	putdnttpointer(arg4);
	PUTCOMMA;
	putdnttpointer(arg5);
#endif /* DEBUGornotIRIF*/

#ifdef IRIF
	DNTTentry.dfunctype.varargs = arg1;
	DNTTentry.dfunctype.info = arg2;
	DNTTentry.dfunctype.unused = 0;
	DNTTentry.dfunctype.bitlength = arg3;
	DNTTentry.dfunctype.firstparam.word = arg4;
	DNTTentry.dfunctype.retval.word = arg5;
#endif /* IRIF */

	break;

    default:
	cerror("invalid kind of dnttentry: %d",kind);
    }

#ifdef DEBUGornotIRIF
  fputc('\n',outfile);
#endif /* DEBUGornotIRIF */

#ifndef IRIF
  if( outfile == dtmpfile ) {
       odnttindex = INDEXED_GNTT_DP( gnttindex );
       gnttindex += DNTTSIZE[kind];
  }
  else {
       odnttindex = INDEXED_LNTT_DP( lnttindex );
       lnttindex += DNTTSIZE[kind];
  }

#else /* IRIF */

  odnttindex = symbolic_id++;
  DNTTentry.dblock.extension = 0;
  DNTTentry.dblock.kind = kind;

  if( sp ) {
       if( ((struct symtab *)sp)->symbolic_id == 0 ) 
	    ((struct symtab *)sp)->symbolic_id = odnttindex;
       else odnttindex = ((struct symtab *)sp)->symbolic_id;
  }

  ir_dntt( kind, &DNTTentry, odnttindex, dntt_type == GNTT, sp );

#endif /* IRIF */

  return(odnttindex);
  }
/***************************************************************************/
/*  Handle source-file switches and generate a K_SRCFILE/SLT_SRCFILE pair.
 *  This is called after a cpp 
 *		# nnn  "filename"
 *  directive is seen.  If the filename has changed, output a K_SRCFILE
 *  entry (this check avoids alot of uneccessary K_SRCFILE's since cpp puts
 *  out alot of # nnn directives that don't correspond to file switches.
 *  Also, strip the leading "./" added by cpp for local files.
 */
extern int cdbfile_vtindex;

cdb_srcfile()
  {
  int slt;
  int l;

#ifndef IRIF
  /* Put out a "lalign 1" just in case.
   * lalign 1 is a nop, but is needed if a label has just been placed
   * in data (otherwise the assembler complains).  */
  prntf("\tlalign\t1\n");
  l = locctr(PROG);
  slt = sltspecial(SLT_SRCFILE,lineno,(unsigned)INDEXED_LNTT_DP(lnttindex));
  (void)locctr(LNTT);
  (void)dnttentry(K_SRCFILE,cdbfile_vtindex,slt);
  (void)locctr(l);

#else /* IRIF */

  l = dntt_type;
  slt = sltspecial( SLT_SRCFILE, lineno, symbolic_id );
  dntt_type = LNTT;
  (void)dnttentry(K_SRCFILE,cdbfile_vtindex,slt);
  dntt_type = l;

#endif /* IRIF */
  }


/*****************************************************************************/
/* The following routine is called to produce all the dntt-type info that 
 * will be required for symbol (* sym).
 * Return a ccom_dnttptr (as unsigned long) telling where the type info
 * went.  This pointer could be SYMOLIC, INDEXED, or IMMED.
 * In addition if the 'sizeof_type' pointer is non-zero, return the size of the
 * type indirect through this pointer.  This is currently only used when
 * building structure field names -- these are the only "name type" dntt's
 * that require a size.
 *
 * If the (* sym) is of basetype STRTY UNIONTY or ENUMTY, this routine is
 * free to alter the associated dnttptr word in the dimtab info, and on
 * return will have set this field to a CCOM_DNTTPTR for the type (this
 * could be INDEXED or SYMBOLIC).
 * This allows multiple references to a STRTY,... basetype to share the
 * same dntt-definitions, and allows handling of recursive and forward
 * references.
 *
 * The type of (* sym) is defined by the field
 *		TWORD  stype;
 * which defines a basetype and up to 14 modifiers (ARY, PTR, FTN).
 * Build_type_dntt first builds the type information required for the
 * base-type (the C base-type), and then builds the modifier information
 * on top of this.
 */
# define NMODIFIERS 14
# define MAXPOSINT 0x7fffffff

LOCAL unsigned long
build_type_dntt(sym,sizeof_type)
  register struct symtab * sym;
   int * sizeof_type;
{
   register TWORD type,td_type;
   register int nmods;		/* number of type modifiers */
   register int td_nmods;	/* number of type modifiers from type def */
   register int size;		/* size of the type in bits */
   register int i;
   unsigned long basetype;	/* the C base-type of *sym */
   unsigned long modifier[NMODIFIERS];	/* the ARY, PTR, FTN modifiers */
					/* from sym->stype */
   unsigned long td_modifier[NMODIFIERS]; /* the ARY, PTR, FTN modifiers */
					  /* from the typedef */
   unsigned long basedntt;	/* ccom-dntt-ptr for the C base-type of *sym */
   unsigned long typedntt;	/* the ccom-dntt-ptr for the C type of sym --
				   starting from the base-type and building
				   modifiers above it.  */
   unsigned int dtindex;        /* dnttentry returns "real" dntt * index. */
   int have_typedef = FALSE;
   int dimoff;			/* index of dimension information in 'dimtab'
				   for ARY modifiers.  */

   /* Check that the type looks legitimate.  Empty symbol table entries have
    * the stype field set to TNULL.  */
   if (sym->stype == TNULL)
	cerror("TNULL type for '%s'",sym->sname);

   basetype = BTYPE(sym->stype);
   type = (sym->stype)&(~BTMASK); /* packed word of modifiers--each 2-bits */
   td_type = 0; /* initialize the typedef type to 0 */
   dimoff = sym->dimoff;  /* first (left-most) dimension if any ARY involved */

   /* if a typedef name was used to define sym and no other type was used
    * (i.e. the basetypes are the same), then remove any type modifiers
    * attributable to the typedef name.
    */
   if (sym->typedefsym && basetype == BTYPE(sym->typedefsym->stype))
     {
     TWORD all_type, mask;

     have_typedef = TRUE;
     mask = BTMASK;
     td_type = (sym->typedefsym->stype)&(~BTMASK);
     all_type = type;
     /* Remove modifiers that the typedef and sym share */
     while (td_type)
       {
       td_type = DECREF(td_type);
       all_type = DECREF(all_type);
       }
     /* For each modifier that sym has alone, shift it into the mask */
     while (all_type)
       {
       all_type = DECREF(all_type);
       mask = (mask << TSHIFT) | 03;
       }
     /* Mask out the modifiers that the typedef and sym share */
     type = type & mask;
     /* Restore the type word from the typedef */
     td_type = (sym->typedefsym->stype)&(~BTMASK);
     }

   /* If this is an array that was converted to a pointer; then make it
    * look like an array for debug info.  Furthermore, in the coversion
    * to a pointer, the dimtab offset was incremented to skip over the
    * the first  dimension.  This decrementing of dimoff is done to
    * adjust it correctly to pick up the actual array dimensions.
    */
   if(ISPTR(type) && WAS_ARRAY(sym->sattrib))
     {
     type += (ARY-PTR);
     --dimoff;
     }

   /* Pull out the modifiers related to the typedef so we can more easily
      access them "bottom up".  When this loop finishes, the typedef modifers
      are in td_modifier[0],...,td_modifier[td_nmods-1].  We will access these
      in reverse order as we calculate the size of the typedef. Also, increment
      'dimoff' whenever we see ARY, so that we can work bottom-up through
      the dimensions of a multi-dimension array.
    */
   td_nmods = 0;
   while (td_type) {
        if (td_nmods==NMODIFIERS)
           cerror("type of '%s' too complex for dntt-type",sym->sname);
        td_modifier[td_nmods] = td_type & TMASK;
	if(td_modifier[td_nmods]==ARY || td_modifier[td_nmods]==FTN) 
		dimoff++;
        td_type = DECREF(td_type);
        td_nmods++;
   }

   /* Now do the same for the modifiers that apply to the sym alone.
    * When this loop finishes, the modifers are in modifier[0],...
    * modifier[nmods-1].  We will access these in reverse order as we build
    * a type and calculate the size bottom-up from the basic type.
    * Also, increment 'dimoff' whenever we see ARY, so that we can work
    * bottom-up through the dimensions of a multi-dimension array.
    */
   nmods = 0;
   while (type) {
	if (nmods==NMODIFIERS)
	   cerror("type of '%s' too complex for dntt-type",sym->sname);
	modifier[nmods] = type & TMASK;
	if(modifier[nmods]==ARY || modifier[nmods]==FTN) 
		dimoff++;
	type = DECREF(type);
	nmods++;
   }

   /* build the dntt for the basetype.  The hard ones will be STRTY,
    * UNIONTY, ENUMTY.
    *
    * First, special cases:  multibyte character & enumeration type
    */
   if (have_typedef)
   {
      /* Haven't done anything for the typedef's dntt, force it out now */
      if (sym->typedefsym->cdb_info.dntt.dnttptr_null.dptrkind==DP_NULL)
	 build_name_dntt(sym->typedefsym);
      basedntt = sym->typedefsym->cdb_info.dntt.dnttptr_ccom;
      goto gotbase;
   }
#ifdef ANSI
   if (ISWIDE( basetype, sym->sattrib ))
     {
     basedntt = IMMED_DP(T_WIDE_CHAR, SZWIDE);
     goto gotbase;
     }
#endif
   if( ISENUM( basetype, sym->sattrib ))
             basetype = ENUMTY;

   switch(basetype)
     {
     default:	
	cerror("invalid dntt basetype [%d] for '%s'", basetype,sym->sname);
	break;

     case VOID:
	/* legally this should only occur with VOID FTN (return
	   type or PTR FTN VOID).
	   Any other use is an error, but should be caught 
	   elsewhere by the compiler as a syntax error.
	*/
	/* There is no proper base type for void, int
	   will be used until symtab.h is sorted out
	*/
	basedntt = IMMED_DP(T_INT, SZINT);
	break;

     case STRTY:
     case UNIONTY:
        {
	int force;
	/* If the *sym is the STNAME (UNAME) or the structure
	 * (union) type was declared without a STNAME, then
	 * force the structure type to be built now even if
	 * the member list is empty -- don't use a foward
	 * reference.
	 */
	/* Need to force if sym has same scope as the tag.  This insures */
	/* that if no members are defined, it still gets output.         */
	force = ((sym->sclass==STNAME) || (sym->sclass==UNAME)
		|| (dimtab[sym->sizoff+3] <= 0)
		 || (sym->slevel==((struct symtab *)
				   dimtab[sym->sizoff+3])->slevel));
	basedntt = build_struct_dntt(sym,force);
	break;
	}

     case ENUMTY:
	{
	int force;
	force = ((sym->sclass==ENAME) || (dimtab[sym->sizoff+3] <= 0)
		 || (sym->slevel==((struct symtab *)
				    dimtab[sym->sizoff+3])->slevel));
	basedntt = build_enum_dntt(sym,force);
	break;
	}
#ifdef ANSI
     case SCHAR:
#endif
     case CHAR:
	basedntt = IMMED_DP(T_CHAR, SZCHAR);
	break;
     case SHORT:
	basedntt = IMMED_DP(T_INT, SZSHORT);
	break;
#ifdef ANSI
     case LONG:			
#endif
     case INT:
	basedntt = IMMED_DP(T_INT, SZINT);
	break;
     case UCHAR:
	basedntt = IMMED_DP(T_UNS_INT, SZCHAR);
	break;
     case USHORT:
	basedntt = IMMED_DP(T_UNS_INT, SZSHORT);
	break;
#ifdef ANSI
     case ULONG:
#endif
     case UNSIGNED:
	basedntt = IMMED_DP(T_UNS_INT, SZINT);
	break;
     case FLOAT:
	basedntt = IMMED_DP(T_REAL, SZFLOAT);
	break;
     case DOUBLE:
	basedntt = IMMED_DP(T_REAL, SZDOUBLE);
	break;
#ifdef ANSI
     case LONGDOUBLE:
	basedntt = IMMED_DP(T_REAL, SZLONGDOUBLE);
	break;
#endif
     }

   /* Now we have the basetype-dntt.  If that's all there was, we're
    * done.  Otherwise start building up the type-modifier information
    */

gotbase:
   typedntt = basedntt;
   size = dimtab[sym->sizoff];

   /* begin calculating the size from the typedef modifiers */
   for (i=td_nmods-1; i>=0; i--)
     {
     switch(td_modifier[i])
        {
        default:
          cerror("illegal modifier [%d] in dntt-type for '%s'",
                   modifier[i],sym->sname);
          break;

        case PTR:
          size = SZPOINT;
          break;

        case ARY:
          {
          unsigned long nelements;
          nelements = dimtab[--dimoff];
          if (nelements>0)
            size *= nelements;
          else
            size = 0;
          break;
          }

        case FTN:
          --dimoff;
          size = SZPOINT;
          break;
        }  /* end switch */
     } /* end of for-modifiers loop */


   /* build a dntt for modifier[i] on top of 'typedntt' so far. */
   for (i=nmods-1; i>=0; i--)
     {
     int saveloc;

     switch(modifier[i])
	{
	default:
	  cerror("illegal modifier [%d] in dntt-type for '%s'",
		   modifier[i],sym->sname);
	  break;

	case PTR:
	  if (sym->sclass == STATIC)
	    {  /* if the symbol is static put all modifiers in LNTT */
#ifndef IRIF
	    saveloc = locctr(LNTT);
	    dtindex = dnttentry(K_POINTER,typedntt,SZPOINT);
	    locctr(saveloc);
#else /* IRIF */
	    saveloc = dntt_type;
	    dntt_type = LNTT;
	    dtindex = dnttentry(K_POINTER,typedntt,SZPOINT);
	    dntt_type = saveloc;
#endif /* IRIF */
	    }
	  else
	    dtindex = dnttentry(K_POINTER,typedntt,SZPOINT);
	  size = SZPOINT;
	  break;

	case ARY:  /* need to build a subrange type for the dimension */
	  {
	  unsigned long upper, nelements, subr_index;
	  int element_size;
	  nelements = dimtab[--dimoff];
	  element_size = size;
	  if (nelements>0)
	    {  /* the normal case */
	    upper = nelements - 1;
	    size *= nelements;
	    }
	  else
	    {	/* 0 dimension or no dimensions specified */
	    upper = MAXPOSINT;
	    size = 0;
	    }
	  if (size < 0)
	    {
	    werror("overflow of debug size information,  debug information may not be correct");
	    size = MAXPOSINT;
	    }
	  if (sym->sclass == STATIC)
	    {  /* if the symbol is static put all modifiers in LNTT */
#ifndef IRIF
	    saveloc = locctr(LNTT);
	    subr_index = dnttentry(K_SUBRANGE,0,upper,DNTTINT, SZINT);
	    dtindex = dnttentry(K_ARRAY,size,subr_index,typedntt,element_size);
	    locctr(saveloc);
#else /* IRIF */
	    saveloc = dntt_type;
	    dntt_type = LNTT;
	    subr_index = dnttentry(K_SUBRANGE,0,upper,DNTTINT, SZINT);
	    dtindex = dnttentry(K_ARRAY,size,subr_index,typedntt,element_size);
	    dntt_type = saveloc;
#endif /* IRIF */
	    }
	  else
	    {
	    subr_index = dnttentry(K_SUBRANGE,0,upper,DNTTINT, SZINT);
	    dtindex = dnttentry(K_ARRAY,size,subr_index,typedntt,element_size);
	    }
	  break;
	  }

	case FTN: /* This will only be executed when making FUNCTYPE
		   * entries -- not FUNC's.  (K_FUNC's are generated
		   * by special begin-function code).
		   * PTR FTN ==> a FUNCTYPE */
	  {
	  unsigned long varargs_bit, compatinfo;
	  NODE * ph;
	  SYMLINK psym;
	  unsigned long firstp, nextp;
	  unsigned long ptype;
	  int isindirect; /* is the FPARAM an array,i.e. passed by refernece */
# ifndef  ANSI 
	  varargs_bit = 0;
	  compatinfo = F_ARGMODE_COMPAT_C;
	  firstp = DNTTNIL;
# else /* ANSI */
	  /* generate function prototype info (if available) for use in
	   * command line function calls.  Note that the prototype parameters
	   * are linked from ph->phead in reverse order of declaration.  */
	  ph = (NODE *) dimtab[--dimoff];
	  varargs_bit = (ph->ph.flags&SELLIPSIS) ? 1 : 0;
	  compatinfo = (ph->ph.flags&SPARAM) ?
			F_ARGMODE_ANSI_C_PROTO : F_ARGMODE_ANSI_C;
		
	  /* Generate the param list back to front, leaving
	   * firstparam correctly set when done.  */
	  nextp = DNTTNIL;
	  for (psym=next_param(ph->ph.phead); psym!=NULL_SYMLINK;
	       psym=next_param(psym->slev_link) )
	    {
	    ptype = build_type_dntt(psym, 0);
	    /* if this FPARAM was an array, then we emit info as if it is    */
	    /* an array.  We must also set the indirect bit to indicate this */
	    /* is being passed by reference                                  */
	    if(ISPTR(psym->stype) && WAS_ARRAY(psym->sattrib))
		isindirect = 1;
	    else
		isindirect = 0;
	    nextp = dnttentry(K_FPARAM,0,isindirect,VTNIL,0,ptype,nextp);
	    }
	  firstp = nextp;
#endif /* ANSI */
	  /* allow functypes to come out here, it is needed since */
	  /* the new debug format has to emit dnt_typedefs        */
	  dtindex = dnttentry(K_FUNCTYPE,varargs_bit,
		  	compatinfo, SZPOINT,firstp,typedntt);
	  size = SZPOINT;
	  break;
         }
	}  /* end switch */
     typedntt = dtindex;
     } /* end of for-modifiers loop */

   if (sizeof_type) *sizeof_type = size;
   return(typedntt);
} /* end of build_type_dntt */


/****************************************************************************/
/* Build a K_STRUCT or K_UNION type , including all the K_FIELD info.
 * Return a CCOM_DNTTPTR for the K_STRUCT (or K_UNION) dntt. (This could
 * be INDEXED or SYMBOLIC).
 *
 * 'sym' points to the associated symbol table entry (for a variable, STNAME,
 *	 or ENAME) with basic type STRTY or UNIONTY.
 * 'force' == 1  means to build an empty structure if the member list is
 *	 empty rather than waiting to see if the member list is defined later.
 *
 * This routine uses the STRUCT information stored in the 'dimtab':
 *
 *
 *			_________________________
 *			|			|
 *	sym->sizoff:    | size in bits		|
 *			|_______________________|
 *			|index to stab-list for	|
 *			|members;  OR 		|---
 *			|-1 if no members	|  |
 *			|_______________________|  |
 *			|  alignment		|  |
 *			|_______________________|  |
 *			| symtab index of	|  |
 *			| STNAME OR <=0 if none	|  |
 *			|_______________________|  |
 *			| status word           |  |
 *			| if 01 then str/union  |  |
 *			| contains CONST member |  |
 *			|_______________________|  |
 *			| a CCOM_DNTTPTR 	|  |
 *			|   to access		|  |
 *			|   use CCOM_OFF as the	|  |
 *			|   offset from sizoff	|  |
 *			|_______________________|  |
 *			| 			|  |
 *				...		   |
 *						   |
 *						   |
 *			|			|  |
 *			|_______________________|  |
 *			| stab-pointer-mem1	|<-| stab pointers for members
 *			|_______________________|
 *			|	 ...		|
 *			|_______________________|
 *			|        NULL		|
 *			|_______________________|
 *			|	  .		|
 *
 *
 * The CCOM_DNTTPTR at dimtab[sym->sizoff+CCOM_OFF] is an extra field added to
 * the dimtab info.  It provides information on what has already been done
 * with this STRUCT-type: allowing multiple references to share the same
 * K_STRUCT, and handling recursion and forward references. The relevant
 * values of the dptrkind field of this CCOM_DNTTPTR are:
 *	DP_LNTT_SYMBOLIC:
 *	DP_GNTT_SYMBOLIC:
 *		The structure type was referenced via a symbolic
 * 		label. The K_STRUCT has not been output yet; when
 *		it is output, label it.
 *		This is used for forward and recursive references.
 *	DP_LNTT_INDEXED:
 *	DP_GNTT_INDEXED:
 *		The K_STRUCT has already been output; we can just
 *		copy the index.
 *	DP_NULL:
 *		No work has been done on this STRUCT type yet.
 *	   	Use the 'recursive' bit to avoid getting into an
 *		infinite loop when a structure contains a field
 *		that references the same structure.
 */


LOCAL unsigned long build_struct_dntt(sym,force)
  register struct symtab * sym;
  int force;
  {
  register CCOM_DNTTPTR * typeptr;
			    /* pointer to the COMM_DNTTPTR word in dimtab
				 info. will point to 
				 dimtab[sym->sizoff+CCOM_OFF]
		  	    */
  register struct symtab * memsym;
				/* pointer to symtab entry for a structure
			         member
			      */

  register int nmem;	/* Number of structure/union members */
  register int memi;	/* index into 'dimtab'--moving through list of member
				stab-indices
				*/
  register int i;
  int sizoff;		/* index to relevant info in 'dimtab' array */
  int firstmem_dimoff;	/* index into dimtab where list of */
				/* member stab-indices begins.	   */
  int memclass;		/* storage class of the current member */
  int incr;

  /* build_type_dntt is called for each structure member and the info
   * returned is saved in the following arrays.
  */
  unsigned long nextmem_dntt;	/* used to chain the members */
  unsigned long struct_dntt;	/* dntt-index of the K_STRUCT */
  int bitoffset;
  int bitlength;

  int *mem_size;
				/* of size maxparamsz when building structures,
				 * maxparamsz is already a limit on number of
				 * members in a structure.
				 * These arrays are used to satisfy the
				 * debugger's original desire to see member
				 * names in order, as consecutively as possible.
				 * We could just build the type/names and chain
				 * them is reverse order as we go.
				 */

  unsigned long *memtype_dntt;   /* Since the compiler uses a 'paramstk' */
  int is_struct, nextmem_incr;
  int level;
  int retloc = 0;
  struct symtab * tagsym;
  /* get info on any type-work already done for this STRUCT or UNION */
  sizoff = sym->sizoff;
  typeptr = (CCOM_DNTTPTR *) &dimtab[sizoff+CCOM_OFF];

  /* if the struct has a tag use the level of the tag */
  /* otherwise, use the level of the symbol           */
  tagsym = (struct symtab *)(dimtab[sizoff+3]);
  if( tagsym != NULL )
    {
    level = tagsym->slevel;
    }
  else
    level= sym->slevel;
   /* If the type has already been built and output, we can just
    * return the dntt-pointer that was saved before.
    */
  if ((typeptr->dnttptr_null.dptrkind == DP_LNTT_INDEXED)
     || (typeptr->dnttptr_null.dptrkind == DP_GNTT_INDEXED))
    return(typeptr->dnttptr_ccom);

  /* check to see if this is a recursive call.
   * If so, we return a symbolic pointer, defining a new one if necessary.
   */
  if (typeptr->dnttptr_null.recursive)
    {
    if (typeptr->dnttptr_null.dptrkind == DP_NULL) {

#ifndef IRIF
      if (level > 0)
	   typeptr->dnttptr_ccom = SYMBOLIC_LNTT_DP(get_lntt_labnum());
      else
	   typeptr->dnttptr_ccom = SYMBOLIC_GNTT_DP(get_gntt_labnum());
#else /* IRIF */
      typeptr->dnttptr_ccom = symbolic_id++;
#endif /* IRIF */

      typeptr->dnttptr_symbolic.recursive = 1;
      }
    return(typeptr->dnttptr_ccom);
    }

  /* If the field list is empty and the 'force' option is not on, delay
   * output of this structure -- incase the field list is declared later.
   * Return a symbolic pointer -- defining a new one if necessary.
   */
  firstmem_dimoff = dimtab[sizoff+1];
  /* FSDdt07079 fix - Added level != 0 to force debug info to be generated
   * for incomplete struct types. */

  if ((firstmem_dimoff<=0) && !force && level != 0)
    {
    if (typeptr->dnttptr_null.dptrkind == DP_NULL)
      {
	 /* Set SDBGREQD to insure that tag gets output.
	    This covers case were sym is in enclosed
	    scope and tag was defined in a header file. 
	    Note, no need to check for tag, force is set
	    if isn't one. */
      tagsym->sflags |= SDBGREQD;

#ifndef IRIF
      if (level > 0)
	   typeptr->dnttptr_ccom = SYMBOLIC_LNTT_DP(get_lntt_labnum());
      else
	   typeptr->dnttptr_ccom = SYMBOLIC_GNTT_DP(get_gntt_labnum());
#else /* IRIF */
      typeptr->dnttptr_ccom = symbolic_id++;
#endif /* IRIF */

      }
    return(typeptr->dnttptr_ccom);
    }


  /* At this point we know we are going to have to build the STRUCT/UNION
   * type here and now -- we can't duck it any further !!
   *
   * Set the recursive bit -- in case this structure references itself
   * via a pointer field -- so we won't get into an infinite loop.
   */
  if (level == 0)

#ifndef IRIF
    /* SWFfc00653 fix - set locctr to LNTT if no symbol/tag name
       and the class is STATIC and level is file scope */

    if (dimtab[sizoff+3] <= 0 && sym->sclass == STATIC) 
      retloc = locctr(LNTT);  
    else retloc = locctr(GNTT);

#else /* IRIF */
    { retloc = dntt_type; dntt_type = GNTT; }
#endif /* IRIF */

  typeptr->dnttptr_null.recursive = 1;

  /* Generate the type information for all the fields and members */
  mem_size = ckalloc(maxparamsz * sizeof(int) );
  memtype_dntt = (unsigned long *) ckalloc(maxparamsz * sizeof(long) );
  nmem=0;
  if (firstmem_dimoff>0)
    {
    memi = firstmem_dimoff;
    while ((memsym=((struct symtab *) dimtab[memi++]))!= NULL)
      {
      memclass = memsym->sclass;
      memtype_dntt[nmem] = build_type_dntt(memsym,&mem_size[nmem]);
      nmem++;
      }
    }

   /* Now we've done all the member types.  Put out the K_STRUCT dntt
      and all the K_FIELD dntt's, chaining them together. Note that since
	it's no longer necessary to put the fields out in the order of their
	occurrence we could rewrite build_struct_dntt() without the allocated
	arrays and put the member dntts out as they occur. But it works as
	it's written now so why bother.
    */

   /* Check whether we need to label the K_STRUCT/K_UNION entry   */
#ifndef IRIF  
   /* SWFfc00653 fix - set struct_dntt to INDEXED_LNTT if no symbol/tag name
      and the class is STATIC and level is file scope */

  struct_dntt = (level > 0) ? INDEXED_LNTT_DP(lnttindex) :
    (dimtab[sizoff+3] <= 0 && sym->sclass == STATIC) 
                            ? INDEXED_LNTT_DP(lnttindex)
                            : INDEXED_GNTT_DP(gnttindex);
#else /* IRIF */
  struct_dntt = symbolic_id;
#endif /* IRIF */

  if (dimtab[sizoff+3] <= 0) /* if no tag, label it here */ {

#ifndef IRIF
       if (typeptr->dnttptr_null.dptrkind==DP_LNTT_SYMBOLIC)
	    def_lntt_labnum((int)(typeptr->dnttptr_symbolic.labnum));
       if (typeptr->dnttptr_null.dptrkind==DP_GNTT_SYMBOLIC)
	    def_gntt_labnum((int)(typeptr->dnttptr_symbolic.labnum));

#else /* IRIF */
       /* if a STRUCT is not tagged, would 'typeptr' ever be symbolic ? */
#endif /* IRIF */
  }
  is_struct = (BTYPE(sym->stype)==STRTY);

#ifndef IRIF  
   /* SWFfc00653 fix - set nextmem_dntt to INDEXED_LNTT if no symbol/tag name
      and the class is STATIC and level is file scope */

  nextmem_incr = is_struct ? DNTTSIZE[K_STRUCT] : DNTTSIZE[K_UNION];
  nextmem_dntt = (nmem==0) ? DNTTNIL :
                 (level>0) ? INDEXED_LNTT_DP(lnttindex+nextmem_incr) :
   (dimtab[sizoff+3] <= 0 && sym->sclass == STATIC) 
                           ? INDEXED_LNTT_DP(lnttindex+nextmem_incr)
                           : INDEXED_GNTT_DP(gnttindex+nextmem_incr);
#else /* IRIF */
  nextmem_dntt = (nmem==0) ? DNTTNIL : (symbolic_id + 1);
#endif /* IRIF */
  
  dnttentry(is_struct ? K_STRUCT : K_UNION, nextmem_dntt,dimtab[sizoff]);
  memi = firstmem_dimoff;
  for(i=0; i<nmem; i++)
    {

#ifndef IRIF
# ifdef SA       
    nextmem_incr =  saflag ? DNTTSIZE[K_XREF] + DNTTSIZE[K_FIELD]
                           : DNTTSIZE[K_FIELD];
# else /* not SA */			   
    nextmem_incr =  DNTTSIZE[K_FIELD];
# endif /* SA */
 /* SWFfc00653 fix - set nextmem_dntt to INDEXED_LNTT if no symbol/tag name
    and the class is STATIC and level is file scope */

    nextmem_dntt = (i==nmem-1) ? DNTTNIL :  
		   (level>0) ? INDEXED_LNTT_DP(lnttindex+nextmem_incr) :
     (dimtab[sizoff+3] <= 0 && sym->sclass == STATIC) 
                             ? INDEXED_LNTT_DP(lnttindex+nextmem_incr)
			     : INDEXED_GNTT_DP(gnttindex+nextmem_incr);

#else /* IRIF */
    nextmem_dntt = (i==nmem-1) ? DNTTNIL : (symbolic_id + 1);
#endif /* IRIF */
    
    memsym = (struct symtab *) dimtab[memi++];
    bitlength = (memsym->sclass&FIELD)? (memsym->sclass&FLDSIZ) : mem_size[i];
    bitoffset = memsym->offset;
    (void)dnttentry(K_FIELD,memsym,bitoffset,memtype_dntt[i],bitlength,
		    nextmem_dntt);
# ifdef SA
    if (saflag)
       if (memsym->slevel==blevel)
	  (void)dnttentry(K_XREF, compute_xt_index(memsym), FALSE);
       else
	  (void)dnttentry(K_XREF, compute_symbolic_xt_index(memsym), TRUE);
# endif
    }


  free((char *)memtype_dntt);
  free((char *)mem_size);

  /* generate the tag for the struct */

  if (dimtab[sizoff+3] > 0) {

    tagsym = (struct symtab *)(dimtab[sizoff+3]);

    if (tagsym->vt_file != 0) {
       if (level == 0 && save_ft != tagsym->vt_file){ /*Asok DTS#CLL4600021 */
         dnttentry(K_SRCFILE, tagsym->vt_file, 0);
         save_ft = tagsym->vt_file;  }
    }
    else {
       if (sym->slevel == 0 && save_ft != sym->vt_file) {
       dnttentry(K_SRCFILE, sym->vt_file, 0);
       save_ft = sym->vt_file;  }
         }

#ifndef IRIF
    if (typeptr->dnttptr_null.dptrkind==DP_LNTT_SYMBOLIC)
       def_lntt_labnum((int)(typeptr->dnttptr_symbolic.labnum));
    if (typeptr->dnttptr_null.dptrkind==DP_GNTT_SYMBOLIC)
       def_gntt_labnum((int)(typeptr->dnttptr_symbolic.labnum));

#else /* IRIF */
    if( typeptr->dnttptr_null.dptrkind==DP_NULL )
	 typeptr->dnttptr_ccom = symbolic_id++;
    tagsym->symbolic_id = typeptr->dnttptr_ccom;
#endif /* IRIF */

    struct_dntt = dnttentry( K_TAGDEF, (nmem == 0) ? 0 : 1,
                    	     tagsym, struct_dntt );
# ifdef SA
    if (saflag)
       if (level==blevel)
	  (void)dnttentry(K_XREF, compute_xt_index(tagsym), FALSE);
       else
	  (void)dnttentry(K_XREF, compute_symbolic_xt_index(tagsym), TRUE);
# endif
    }
  
  if (retloc)
#ifndef IRIF
    (void) locctr(retloc);
#else /* IRIF */
     dntt_type = retloc;
#endif /* IRIF */

   /* reset the dnttptr field */
  typeptr->dnttptr_ccom = struct_dntt;
  return(struct_dntt);
  } /* build_struct_dntt */


/****************************************************************************/
/* Build a K_ENUM type, including all the K_MEMENUM info.
 * Return a CCOM_DNTTPTR (as an unsigned long) for the K_ENUM.
 * 
 * This is very similar to build_struct_dntt except that we don't have
 * to build a type dntt for each member.
 *
 * The 'dimtab' info is analagous to the structure case (see above) and
 * will not be repeated here.
 *
 */

LOCAL unsigned long build_enum_dntt(sym,force)
  register struct symtab * sym;
  int force;
  {
  int sizoff;		/* index into 'dimtab' of relevant info */
  int firstmem_dimoff;	/* index into 'dimtab' where member-indices begins */
  register CCOM_DNTTPTR * typeptr; /* points to COMM_DNTTPTR stored in dimtab */

  register int mdimoff;	/* index into 'dimtab'--moves thru the member list */
  register struct symtab * memsym; /* pointer to the symtab struc for member */
  unsigned long nextmem_dntt;	/* used to chain the members */
  unsigned long enum_dntt;	/* dntt-index for the K_ENUM */
  int bitlength;
  int mem_value;
  int level;
  int retloc = 0;
  struct symtab * tagsym;

  /* get info on any type-work already done for this ENUM */
  sizoff = sym->sizoff;
  typeptr = (CCOM_DNTTPTR *) &dimtab[sizoff+CCOM_OFF];
  /* use the level of the tag if there is one */
  /* otherwise use the level of the symbol    */
  if (dimtab[sizoff+3] > 0)
    {
    tagsym = (struct symtab *)(dimtab[sizoff+3]);
    level = tagsym->slevel;
    }
  else
    level= sym->slevel;
  /* If the type has already been built and output, we can just
   * return the dntt-pointer that was saved.  */
  if ((typeptr->dnttptr_null.dptrkind == DP_LNTT_INDEXED)
     || (typeptr->dnttptr_null.dptrkind == DP_GNTT_INDEXED))
    return(typeptr->dnttptr_ccom);

  /* If the member list is empty and the 'force' option is not on, delay
   * output of this enum-type -- incase the member list is declared later.
   * Return a symbolic pointer -- defining a new one if necessary.  */

  firstmem_dimoff = dimtab[sizoff+1];
  if ((firstmem_dimoff<=0) && !force)
    {
    if (typeptr->dnttptr_null.dptrkind==DP_NULL)
      {
         /* Set SDBGREQD to insure that tag gets output.
            This covers case were sym is in enclosed
            scope and tag was defined in a header file.
            Note, no need to check for tag, force is set
            if isn't one. */
      tagsym->sflags |= SDBGREQD;

#ifndef IRIF
      if (level > 0)
        typeptr->dnttptr_ccom = SYMBOLIC_LNTT_DP(get_lntt_labnum());
      else
        typeptr->dnttptr_ccom = SYMBOLIC_GNTT_DP(get_gntt_labnum());
#else /* IRIF */
      typeptr->dnttptr_ccom = symbolic_id++;
#endif /* IRIF */

      }
      
    return(typeptr->dnttptr_ccom);
    }

  /* At this point we know we are going to have to build the ENUM now.
   * Put out the K_ENUM dntt and all the K_MEMENUM dntt's .  */

#ifndef IRIF
  if (level == 0)
       retloc = locctr(GNTT);

  enum_dntt = (level > 0) ? INDEXED_LNTT_DP(lnttindex)
		          : INDEXED_GNTT_DP(gnttindex);
#else /* IRIF */
  if (level == 0) {
       retloc = dntt_type; 
       dntt_type = GNTT; 
  }
  enum_dntt = symbolic_id;
#endif /* IRIF */

#ifdef DEBUGornotIRIF
  /* Check whether we need to label the K_ENUM entry   */
  if (dimtab[sizoff+3] <= 0) /* if no tag, label it here */
    {
    if (typeptr->dnttptr_null.dptrkind == DP_LNTT_SYMBOLIC) 
      def_lntt_labnum((int)(typeptr->dnttptr_symbolic.labnum));
    if (typeptr->dnttptr_null.dptrkind == DP_GNTT_SYMBOLIC) 
      def_gntt_labnum((int)(typeptr->dnttptr_symbolic.labnum));
    }
#endif /* DEBUGornotIRIF */

#ifndef IRIF
  nextmem_dntt =
    (firstmem_dimoff == NULL) ? DNTTNIL
    : (level>0) ? INDEXED_LNTT_DP(lnttindex + DNTTSIZE[K_ENUM])
    : INDEXED_GNTT_DP(gnttindex + DNTTSIZE[K_ENUM]);

#else /* IRIF */
  nextmem_dntt =
    (firstmem_dimoff == NULL) ? DNTTNIL : (symbolic_id + 1);
#endif /* IRIF */

  bitlength = dimtab[sizoff]; /* This is a general calc. Actually, for
			         our implementation we could just use SZINT */
  (void)dnttentry(K_ENUM,nextmem_dntt,bitlength);

  /* Build a chain of K_MEMENUM for all the members.  */
  if (firstmem_dimoff>0)
    {
    mdimoff = firstmem_dimoff;
    while ((memsym = (struct symtab *) dimtab[mdimoff++]) != NULL)
      {

#ifndef IRIF
#ifdef SA	 
      nextmem_dntt =
	(dimtab[mdimoff]==NULL) ? DNTTNIL
	: (level>0) ? INDEXED_LNTT_DP(lnttindex + 
			(saflag ? DNTTSIZE[K_XREF] + DNTTSIZE[K_MEMENUM]
			 : DNTTSIZE[K_MEMENUM]))
	: INDEXED_GNTT_DP(gnttindex + 
			  (saflag ? DNTTSIZE[K_XREF] + DNTTSIZE[K_MEMENUM]
			   : DNTTSIZE[K_MEMENUM]));
#else /* SA */
      nextmem_dntt =
	(dimtab[mdimoff]==NULL) ? DNTTNIL
	: (level>0) ? INDEXED_LNTT_DP(lnttindex + DNTTSIZE[K_MEMENUM])
	: INDEXED_GNTT_DP(gnttindex + DNTTSIZE[K_MEMENUM]);
#endif /* SA */

#else /* IRIF */
      nextmem_dntt =
	(dimtab[mdimoff]==NULL) ? DNTTNIL : (symbolic_id + 1);
#endif /* IRIF */
      
      mem_value = memsym->offset;
      (void)dnttentry(K_MEMENUM,memsym,mem_value,nextmem_dntt);
# ifdef SA
      if (saflag)
       if (memsym->slevel==blevel)
	  (void)dnttentry(K_XREF, compute_xt_index(memsym), FALSE);
       else
	  (void)dnttentry(K_XREF, compute_symbolic_xt_index(memsym), TRUE);
# endif      
      }
    }

  /* generate the tag for the enum */

  if (dimtab[sizoff+3] > 0)
    {

    tagsym = (struct symtab *)(dimtab[sizoff+3]);

    if (tagsym->vt_file != 0) {
       if (level == 0 && save_ft != tagsym->vt_file){ /*Asok CLL4600021 */
         dnttentry(K_SRCFILE, tagsym->vt_file, 0);
         save_ft = tagsym->vt_file;  }
    }
    else {
       if (sym->slevel == 0 && save_ft != sym->vt_file) {
       dnttentry(K_SRCFILE, sym->vt_file, 0);
       save_ft = sym->vt_file;  }
         }

#ifndef IRIF
    if (typeptr->dnttptr_null.dptrkind == DP_LNTT_SYMBOLIC)
       def_lntt_labnum((int)(typeptr->dnttptr_symbolic.labnum));
    if (typeptr->dnttptr_null.dptrkind == DP_GNTT_SYMBOLIC)
       def_gntt_labnum((int)(typeptr->dnttptr_symbolic.labnum));
#else /* IRIF */
    if( typeptr->dnttptr_null.dptrkind==DP_NULL )
	 typeptr->dnttptr_ccom = symbolic_id++;
    tagsym->symbolic_id = typeptr->dnttptr_ccom;
#endif /* IRIF */

    enum_dntt = dnttentry( K_TAGDEF, (firstmem_dimoff == NULL) ? 0 : 1,
			   tagsym, enum_dntt );

# ifdef SA
    if (saflag)
       if (level==blevel)
	  (void)dnttentry(K_XREF, compute_xt_index(tagsym), FALSE);
       else
	  (void)dnttentry(K_XREF, compute_symbolic_xt_index(tagsym), TRUE);
# endif /* SA */
    }

  if (retloc)
#ifndef IRIF
    (void) locctr(retloc);
#else /* IRIF */
    dntt_type = retloc;
#endif /* IRIF */

   /* reset the dnttptr field */
  typeptr->dnttptr_ccom = enum_dntt;
  return(enum_dntt);
} /* build_enum_dntt */


/*************************************************************************/
/* The following routine is called to produce the dntt-name information
 * and dntt-type information required for the symbol (* sym).
 * Return a ccom_dnttptr (as unsigned long) telling where the dntt-name
 * info went.
 *
 * This routine is to be called for symbols representing variable names,
 * STNAMES, UNAMES, ENAMES, TYPEDEFS, and LABELS.
 * You might call these "top level names" -- not a subpart of of any type.
 *
 * It is NOT prepared to handle FUNCTION-names, PARAM, MOS, MOU, MOE --
 * these are all handled elsewhere.
 *
 */

LOCAL unsigned long build_name_dntt(sym)
  register struct symtab * sym;
  {
  int sclass;
  unsigned int dtype;
  unsigned int dnameindex;
  unsigned int namedntt;

  sclass = sym->sclass;
  if (sclass & FIELD )
    {
unexpected:
    werror("unexpected symbol class [%d] in dntt-name for '%s'",
	    sclass,sym->sname);
    return(DNTTNIL);
    }

  switch(sclass)
    {

    default:	/* ??? should this be a cerror ??? */
	werror("unimplemented sclass [%d] for '%s' in dntt-name",
		sclass,sym->sname);
	break;

    case AUTO:
    case REGISTER:
	/* this is a K_DVAR */
	{ 
	int location;
	int reg_var_bit;

	if (sym->slevel<2) goto unexpected;
	dtype = build_type_dntt(sym,0);
	reg_var_bit = (sclass==REGISTER);

#ifndef IRIF
	location = sym->offset;		/* bit offset or register number */
	if (!reg_var_bit) location /= SZCHAR;	/* byte offset */
#else /* IRIF */
	location = 0;    /* IRIF figures out location */
#endif /* IRIF */
	dnameindex=dnttentry(K_DVAR,0,0,reg_var_bit,sym,location,dtype);

# ifdef SA
	if (saflag)
	   dnttentry(K_XREF, compute_xt_index(sym),FALSE);
# endif
	break;
	}

    case STATIC:
    case EXTDEF: /* only for func names and initialized global scalars ??? */
    case EXTERN:
	/* this is a K_SVAR */
	{
	int  public, saveloc;
	char *slabel;
	char slocal[10];
	static char * UNKNOWN_SVAR_LOCATION = "-1";

	/* skip function names -- K_FUNC's are built via cdb_bfunc */
	if (ISFTN(sym->stype))
	  return(DNTTNIL);
	dtype = build_type_dntt(sym,0);
	public = (sclass!=STATIC);

#ifndef IRIF
	if (sym->slevel && sclass==STATIC) 
	  sprntf(slabel=slocal,LABFMT, sym->offset);
	/* special check -- declared extern but never defined emit
	 * a NULL SVAR location and force the linker to fix if
	 * possible. Can't emit an 'exname' here because we can't
	 * be sure that the name is defined anywhere.
	 */
	else if (sclass==EXTERN && !sym->cdb_info.info.extvar_isdefined)
	  slabel = UNKNOWN_SVAR_LOCATION;
	else
	  slabel = EXNAME(sym);

	/* if the symbol is static, put it in the LNTT */
	if (sclass == STATIC)
	  {
	  saveloc = locctr(LNTT);
	  dnameindex = dnttentry(K_SVAR,0,0,sym,slabel,dtype);
	  locctr(saveloc);
	  }
	else
	  dnameindex = dnttentry(K_SVAR,1,0,sym,slabel,dtype);
#else /* IRIF */
	
	/* emit *no* dnttentry for an EXTERN which has not been 
	 *                     defined or referenced
	 */
	if ( sclass != EXTERN || sym->cdb_info.info.extvar_isdefined 
		              || sym->cdb_info.info.buildname ) {

	     saveloc = dntt_type;
	     dntt_type = ( sclass == STATIC ? LNTT : GNTT ); 
	     dnameindex = dnttentry( K_SVAR, dntt_type == GNTT, 0,
				    sym, NULL, dtype);
	     dntt_type = saveloc;
	}
#endif /* not IRIF */

# ifdef SA
	  if (saflag) {
	     if (sclass == STATIC)
#ifndef IRIF
		saveloc = locctr(LNTT);
#else /* IRIF */
		saveloc = dntt_type; dntt_type = LNTT;
#endif /* IRIF */
	     dnttentry(K_XREF, compute_xt_index(sym),FALSE);
	     if (sclass == STATIC)
#ifndef IRIF
		(void)locctr(saveloc);
#else /* IRIF */
		dntt_type = saveloc;
#endif /* IRIF */
	  }
# endif /* SA */
	break;
	}

    case STNAME:
    case UNAME:
    case ENAME:
	{
	dnameindex = build_type_dntt(sym,0);
	break;
	}

    case CLABEL:
	/* when the label was defined in the code, I saved the
	 * corresponding sltindex in the 'cdb_info' field of the
	 * symtab entry for the label-id.
	*/
	dnameindex = dnttentry(K_LABEL,sym,sym->cdb_info.label_slt);
# ifdef SA
	if (saflag)
	   dnttentry(K_XREF, compute_xt_index(sym),FALSE);
# endif
	break;

    case TYPEDEF:
       {
	  int retloc = 0;
	  
	  if (sym->cdb_info.dntt.dnttptr_null.dptrkind == DP_NULL)
	  {
	     if (sym->cdb_info.dntt.dnttptr_null.recursive) {
#ifndef IRIF
		if (sym->slevel > 0)
		   sym->cdb_info.dntt.dnttptr_ccom = SYMBOLIC_LNTT_DP(get_lntt_labnum());
		else
		   sym->cdb_info.dntt.dnttptr_ccom = SYMBOLIC_GNTT_DP(get_gntt_labnum());
#else /* IRIF */
		sym->cdb_info.dntt.dnttptr_ccom = symbolic_id++;
#endif /* IRIF */
		return(sym->cdb_info.dntt.dnttptr_ccom);
	     }
	     if (sym->slevel==0)
#ifndef IRIF
		retloc = locctr(GNTT);
#else /* IRIF */
		{ retloc = dntt_type; dntt_type = GNTT; }
#endif /* IRIF */
	     sym->cdb_info.dntt.dnttptr_null.recursive = 1;
	     dtype = build_type_dntt(sym,0);

             if (sym->slevel == 0 && save_ft != sym->vt_file) {
                dnttentry(K_SRCFILE, sym->vt_file, 0);
                save_ft = sym->vt_file;
             }

#ifdef DEBUGornotIRIF
	     if (sym->cdb_info.dntt.dnttptr_null.dptrkind==DP_LNTT_SYMBOLIC)
		def_lntt_labnum((int)(sym->cdb_info.dntt.dnttptr_symbolic.labnum));
	     if (sym->cdb_info.dntt.dnttptr_null.dptrkind==DP_GNTT_SYMBOLIC)
		def_gntt_labnum((int)(sym->cdb_info.dntt.dnttptr_symbolic.labnum));
#endif /* DEBUGornotIRIF */

	     dnameindex = dnttentry(K_TYPEDEF,0,sym,dtype);

# ifdef SA
	     if (saflag)
		if (sym->slevel==blevel)
		   (void)dnttentry(K_XREF, compute_xt_index(sym), FALSE);
	        else
		   (void)dnttentry(K_XREF, compute_symbolic_xt_index(sym), TRUE);
# endif
	     if (retloc)
#ifndef IRIF
		(void)locctr(retloc);
#else /* IRIF */
		dntt_type = retloc;
#endif /* IRIF */
	     sym->cdb_info.dntt.dnttptr_ccom = dnameindex;
	  }
	  else
	     dnameindex = sym->cdb_info.dntt.dnttptr_ccom;
       }
	break;
    }  /* end of switch */

  namedntt = dnameindex;
  return(namedntt);
  } /* build_name_dntt */


/******************************************************************************/
/* cdb_bfunc is called when a function definition header is processed.
 * This occurs at the point where the '{' opening the function body has just
 * been parsed. 
 * Generate a K_FUNC, SLT_FUNCTION, and K_PARAM's. The symtab.h speicifications
 * require the dntt-entries to be ordered as: K_FUNC, K_FPARAM's (in left-to-
 * right order). To do this, first dump all the type info for the function
 * return type and the PARAM's.  Then go back and do the K_FUNC, K_FPARAM's
 * which will then go into consecutive dntt-entries.
 *
 * The caller must provide an array of symtab indices for the PARAM's and
 * the number of PARAM's.  In the Series 500 compiler, these have already been
 * built in the globals 'paramstk' and 'argno'.
 *
*/

LOCAL unsigned long bfunc_dntt;   /* saves dntt-ptr of last K_FUNC       */
			  	  /* used when generating matching K_END */
LOCAL unsigned int cdb_efunc_lab; /* label number for end of function marker */



  cdb_bfunc(fsym,arg,n)
  register struct symtab *fsym;	/* pts to symtab for function */
  int arg[];		/* array of symbol table indices for PARAM's */
  int n;		/* number of PARAM's */
  {
  register int i;
  register unsigned long *argtype_dntt = atype_dntt;
  struct symtab retsym; /* needed when building dntt for fcn return type */
  int olocctr;
  unsigned long ftype_dntt;  /* dntt-ptr for the function return type */
  struct symtab * argsym;
  int slt;  /* slt-index of the SLT_FUNCTION */
  int public;
  char nextaddress[20];  /* it will be a symbolic label */
  unsigned long firstparam, nextparam;
  char * alias;		/* only "main" can have a nonnull alias */
  int isreg;		/* is the FPARAM really a REGISTER -- series 200 */
  int isindirect;	/* is the FPARAM an array, i.e. passed by reference */
  int offset;
  struct symtab *sptr;
  int varargs_bit;
  int compatinfo;
# ifdef ANSI
  NODE * ph;
# endif

#ifndef IRIF
  olocctr = locctr(LNTT);
#else /* IRIF */
  olocctr = dntt_type; dntt_type = LNTT;
#endif /* IRIF */
  /* build the type info for the function-return-type */
  retsym = *fsym;
  retsym.stype = DECREF(retsym.stype);
  retsym.dimoff++;    /* skip past proto pointer */
  ftype_dntt = build_type_dntt(&retsym,0);

  /* build the type info for all the param's */
  /* Note: because of the way prototypes was implemented, the ANSI compiler
   * ends up doing the psave's in the opposite order.
   */
# ifndef ANSI
  for (i=0; i<n; i++)
    {
# else
  for (i=n-1; i>=0; i--)
    {
# endif
    /* ??? they make a check like this in dclargs() */
    /* when could this happen ???? */
    if ((argsym=((struct symtab *) paramstk[i])) == NULL)
      argtype_dntt[i] = DNTTNIL;
    else
      argtype_dntt[i] = build_type_dntt(argsym,0);
    }

  /* Now we've done all the types. Put out the special SLT_FUNCTION, */
  /* the K_SRCFILE, the K_FUNC dntt, and the K_FPARAM dntt's         */

#ifndef IRIF
  (void)locctr(PROG);
  slt = sltspecial(SLT_SRCFILE,fsym->suse,(unsigned)INDEXED_LNTT_DP(lnttindex));
  (void)locctr(LNTT);
  (void)dnttentry(K_SRCFILE,cdbfile_vtindex,slt);
  bfunc_dntt = INDEXED_LNTT_DP(lnttindex);
  (void)locctr(PROG);
#else /* IRIF */
  slt = sltspecial(SLT_SRCFILE,fsym->suse,symbolic_id);
  dntt_type = LNTT;
  (void)dnttentry(K_SRCFILE,cdbfile_vtindex,slt);
  bfunc_dntt = symbolic_id;
#endif /* IRIF */

  /* The 'lineno' we'll get here is that of the function name. */
  slt=sltspecial(SLT_FUNCTION, fsym->suse, bfunc_dntt);
  cdb_efunc_lab = GETLAB();
  sprntf(nextaddress,LABFMT,cdb_efunc_lab);
  public = fsym->sclass==EXTDEF;

#ifndef IRIF
# ifdef SA  
  firstparam = (n>0) ? (saflag && fsym->sclass == STATIC) ?
                       INDEXED_LNTT_DP(lnttindex+DNTTSIZE[K_FUNCTION]+DNTTSIZE[K_XREF]) 
                     : INDEXED_LNTT_DP(lnttindex+DNTTSIZE[K_FUNCTION])
		     : DNTTNIL;
# else /* not SA */ 
  firstparam = (n>0) ? INDEXED_LNTT_DP(lnttindex+DNTTSIZE[K_FUNCTION])
		     : DNTTNIL;
# endif /* not SA */

#else /* IRIF */
  firstparam = (n>0) ? (symbolic_id + 1) : DNTTNIL;
#endif /* IRIF */

#ifndef IRIF
  (void)locctr(LNTT);
#else /* IRIF */
  dntt_type = LNTT;
#endif /* IRIF */

  alias = strcmp(fsym->sname,"main") ? 0 : "_MAIN_" ;
# ifdef ANSI
  ph = (NODE *) dimtab[fsym->dimoff];
  varargs_bit = (ph->ph.flags&SELLIPSIS) ? 1 : 0;
  compatinfo = (ph->ph.flags&SPARAM) ?  F_ARGMODE_ANSI_C_PROTO :
					F_ARGMODE_ANSI_C;
# else
  varargs_bit = 0;
  compatinfo = F_ARGMODE_COMPAT_C;
# endif
  (void)dnttentry(K_FUNCTION,public,0,varargs_bit,compatinfo,fsym,
                  alias,firstparam,slt,EXNAME(fsym),ftype_dntt,nextaddress);

# ifdef SA
  if (saflag && fsym->sclass == STATIC)
    dnttentry(K_XREF, compute_symbolic_xt_index(fsym), TRUE);
# endif
# ifndef ANSI
  for(i=0; i<n; i++)
    {
# else
  for (i=n-1; i>=0; i--)
    {
# endif
    if ((sptr=(struct symtab *) arg[i]) == NULL) continue; /* ??? */
    if ((sptr=(struct symtab *) arg[i]) == NULL) continue; /* ??? */
    nextparam =
# ifndef ANSI
	    (i==n-1)
# else
	    (i==0)
# endif

#ifndef IRIF
# ifdef SA	       
	    ? DNTTNIL : (saflag) ?
	                INDEXED_LNTT_DP(lnttindex+DNTTSIZE[K_FPARAM]+DNTTSIZE[K_XREF])
	              : INDEXED_LNTT_DP(lnttindex+DNTTSIZE[K_FPARAM]);

# else /* not SA */    
	    ? DNTTNIL : INDEXED_LNTT_DP(lnttindex+DNTTSIZE[K_FPARAM]);
# endif	/* not SA */

#else /* IRIF */
            ? DNTTNIL : (symbolic_id + 1 );
#endif /* IRIF */

    isreg = (sptr->sclass == REGISTER);

#ifndef IRIF
    offset = sptr->offset;	/* bit offset or register number */
    if (!isreg) offset /= SZCHAR;	/* byte offset */
#endif /* IRIF */
    /* if this FPARAM was an array, then we emit info as if it is    */
    /* an array.  We must also set the indirect bit to indicate this */
    /* is being passed by reference                                  */
    if(ISPTR(sptr->stype) && WAS_ARRAY(sptr->sattrib))
        isindirect = 1;
    else
        isindirect = 0;
    (void)dnttentry(K_FPARAM,isreg,isindirect,sptr,offset,argtype_dntt[i],nextparam);
# ifdef SA
    if (saflag)
       dnttentry(K_XREF, compute_symbolic_xt_index(sptr), TRUE);
# endif
    }
#ifndef IRIF
  (void)locctr(olocctr);
#else /* IRIF */
  dntt_type = olocctr;
#endif /* IRIF */
}

/******************************************************************************/
/* cdb_efunc is called at the end of a function body.
 * To mark the end of a function we must:
 *	- generate a K_END dntt to match the K_FUNCTION
 *	- generate a SLT_END
 *	- define the cdb_efunc_lab
 */

cdb_efunc()
  {
  int olocctr;
  int slt;

#ifndef IRIF
  olocctr = locctr(PROG);
  fprntf(outfile,"\tset\tL%d,.-1",cdb_efunc_lab);
  fprntf(outfile,"\n");
  slt = sltspecial(SLT_END,lineno,(unsigned)INDEXED_LNTT_DP(lnttindex));
  (void)locctr(LNTT);
#else /* IRIF */
  olocctr = dntt_type;
  slt = sltspecial(SLT_END,lineno,symbolic_id);
  dntt_type = LNTT;
#endif /* IRIF */

  (void)dnttentry(K_END,K_FUNCTION,slt,bfunc_dntt);
# ifdef SA
  if (saflag) {
     flush_xt_local();
#ifdef DEBUGornotIRIF
     fprntf(outfile, "\n\tlntt\n");
#endif /* DEBUGornotIRIF */ 
  }
# endif /* SA */  

#ifndef IRIF
  (void)locctr(olocctr);
#else /* IRIF */
  dntt_type = olocctr;
#endif /* IRIF */
  }

/*****************************************************************************/
/* cdb_startscope/cdb_endscope handle the start/end of nested scopes.
 *
 * In C, any compound statement can begin with a variable declaration list
 * which will require a K_BEGIN/K_END pair and a corresponding SLT_BEGIN/
 * SLT_END pair.
 *
 * cdb_startscope is called whenever a compound statement begins.  In order
 * to avoid emitting alot of needless K_BEGIN/K_END pairs, the call is 
 * delayed until after the declaration-list (if any) has been parsed far
 * enough to tell if there are declarations.
 * The caller passes a parameter indicating whether any vars were declared
 * (decl_flag), and the lineno of the '{' symbol (lineno).
 * If there are declarations, or we are starting a function body, emit
 * a K_BEGIN and stack the dntt-ptr, otherwise stack a DNTTNIL pointer.
 *
 * The parser (cgram.y) calls cdb_startscope as soon as it can determine
 * that there is a declaration.  This way the K_BEGIN is emitted before
 * the code or sltnormals corresponding to any automatic variable
 * initilization.
 *
 * NOTE: a K_BEGIN must always be emitted for the beginning of a function
 *	 body even if no declarations occurred because the function body
 *	 may contain a LABEL name that will need to be emitted in a nested
 *	 scope.
 *
 *
 * cdb_endscope is called when a compound statement ends. Unstack a
 * dntt-pointer, and if non-nil, generate a K_END.
 *
 * COMMENT: If programmers do strange things like switch files between the
 * '{' and the end of the declaration list (via an "include" file) the 
 * SLT emitted will be screwed up ??? 
 */

LOCAL unsigned long scope_dntt[CDB_NSCOPES];	/* save BEGIN dntt-pointers */
LOCAL int scopeoff = 0;


cdb_startscope(decl_flag,scopeline) 
  int decl_flag;  /* were there any decls ? */
  int scopeline;  /* the line# the '{' occurred in. */
  {
/* begin a compound block ==> a new scope */
/* but only if 1) vars are declared; or 2) start of a new function */

  int slt;
  int olocctr;
 
  if (scopeoff >=CDB_NSCOPES)
    cerror("Too many nested scopes for cdb-symtab");
  if (decl_flag || blevel == 2)
    {

#ifndef IRIF
    scope_dntt[scopeoff++] = INDEXED_LNTT_DP(lnttindex);
    olocctr = locctr(PROG);
    slt = sltspecial(SLT_BEGIN,scopeline,(unsigned)INDEXED_LNTT_DP(lnttindex));
    (void)locctr(LNTT);
    (void)dnttentry(K_BEGIN,slt);
    (void)locctr(olocctr);
#else /* IRIF */
    scope_dntt[scopeoff++] = symbolic_id;;
    olocctr = dntt_type;
    slt = sltspecial(SLT_BEGIN,scopeline,symbolic_id);
    dntt_type = LNTT;
    (void)dnttentry(K_BEGIN,slt);
    dntt_type = olocctr;
#endif /* IRIF */
	
    }
  else 
    scope_dntt[scopeoff++] = DNTTNIL;
}

cdb_endscope()
  { /* end a symtab scope */
  int slt;
  unsigned long begin_dntt;
  int olocctr;

  if (scopeoff <= 0)
    cerror("symtab scoping error in cdb_end");
  if ((begin_dntt=scope_dntt[--scopeoff]) != DNTTNIL)
    { /* a K_BEGIN was generated,  need to close the scope with a K_END. */

#ifndef IRIF
    olocctr = locctr(PROG);
    slt = sltspecial(SLT_END,lineno,(unsigned)INDEXED_LNTT_DP(lnttindex));
    (void)locctr(LNTT);
    (void)dnttentry(K_END,K_BEGIN,slt,begin_dntt);
    (void)locctr(olocctr);
#else /* IRIF */
    olocctr = dntt_type;
    slt = sltspecial(SLT_END,lineno,symbolic_id);
    dntt_type = LNTT;
    (void)dnttentry(K_END,K_BEGIN,slt,begin_dntt);
    dntt_type = olocctr;
#endif /* IRIF */

    }
  }


/****************************************************************************/
/* cdb_global_names is called at the end of compilation to dump out symbols
 * for the global names.
 * Scan the entire symbol table, calling build_name_dntt for each
 * symbol, filtering out the names that are not of "interest".
 */

cdb_global_names()
  {
  register struct symtab *p,*prev,*next;
  int sclass;

#ifndef IRIF
  (void)locctr(GNTT);
#else /* IRIF */
  dntt_type = GNTT;
#endif /* IRIF */
  /* reverse the links to get structs and typdefs out first */
  for (p = stab_lev_head[0], prev = NULL_SYMLINK;
       p != NULL_SYMLINK;
       next = p->slev_link, p->slev_link = prev, prev = p, p = next)
    ;
  /* process each symbol and un-reverse the links */
  for (p = prev, prev = NULL_SYMLINK;
       p != NULL_SYMLINK;
       next = p->slev_link, p->slev_link = prev, prev = p, p = next)
    {
     if(p->stype != TNULL && (!ISFTN(p->stype) || p->sclass == TYPEDEF)) {
	sclass = p->sclass;
	if (sclass & FIELD) continue;
	switch(sclass)
        {
	   case STNAME:
	   case UNAME:
	   case ENAME:
	   case TYPEDEF: 
	      /* tags and typedefs can be forced out
		 later by another name whose type 
		 depends on them */
	      if (!(Allflag || (p->sflags & SDBGREQD)))
		 continue;
	   case EXTERN:
	      /* Test extvar_isdefined so that 
		 "int x;" is always output */
	      if (!(Allflag || (p->sflags & SDBGREQD) || 
		    p->cdb_info.info.extvar_isdefined))
		 continue;
	   case EXTDEF:
	   case STATIC: (void)build_name_dntt(p);
        }
     }
   } /* end for */
# ifdef SA
  if (saflag) {
     flush_xt_global();
     flush_xt_macro();
  }
# endif  
}


/****************************************************************************/
/* cdb_local_names is called at the end of an inner lexical scope to dump
 * out symbols with slevel > lev -- called before clearst starts
 * to remove these symbols.
 * This extra pass through the symbol table is necessary because of
 * the current symbol table structure, which actually moves symtab entries
 * as names are deleted.  This means that we must find and dump all the
 * relevant symbols before the 'clearst' procedure actually begins.
 *
 * Scan the symbol table looking for names of slevel>lev. Then use the
 * storage class to determine whether to dump the name.  For names of
 * "interest" call build_name_dntt to dump the name into the symbol table.
 *
 */

cdb_local_names(lev)
  {
  register struct symtab *p,*prev,*next;
  int olocctr;
  int sclass;
  int stype;

#ifndef IRIF
  olocctr = locctr(LNTT);
#else /* IRIF */
  olocctr = dntt_type; dntt_type = LNTT;
#endif /* IRIF */

  /* reverse the links to get structs and typdefs out first */
  for (p = stab_lev_head[lev], prev = NULL_SYMLINK;
       p != NULL_SYMLINK;
       next = p->slev_link, p->slev_link = prev, prev = p, p = next)
    ;
  /* process each symbol and un-reverse the links */
  for (p = prev, prev = NULL_SYMLINK;
       p != NULL_SYMLINK;
       next = p->slev_link, p->slev_link = prev, prev = p, p = next)
     {
     sclass = p->sclass;
     stype = p->stype;

     /* Ignore some symbols based on stype */
     if (stype == TNULL || ISFTN(stype))
       continue;
     if (stype == UNDEF || (sclass == ULABEL && lev < 2))
       continue;     /* clearst will flag undefined name error */

     /* Use storage class to determine whether we want the name
      * in the symbol table.  */
     if (sclass & FIELD)
       continue;

     switch(sclass)
       {
       case EXTERN:
         /*[Compatability mode comment]
          * ??? this case currently never occurs because externs
          * declared at an inner scope are getting copied out to
          * the global level -- a C "feature" (alias, bug).  */
# ifdef ANSI
         if( lev>1 )continue;  /* block extern */
# endif /*ANSI*/
       case EXTDEF:
         /* ??? does this case ever occur ??? */
         werror("unexpected sclass [%d] for '%s' in cdb-local-names",
                 sclass,p->sname);
         continue;
       case REGISTER:
         /* check REG-var to see if its really an FPARAM.
          * If so, it was already dumped along with the
          * function header. */
         if (p->slevel<2) continue;
       case STATIC:
       case AUTO:
       case STNAME:
       case UNAME:
       case ENAME:
       case TYPEDEF:
       case CLABEL:
         (void)build_name_dntt(p);
       }
     } /* end for */
#ifndef IRIF
  (void)locctr(olocctr);
#else /* IRIF */
  dntt_type = olocctr;
#endif /* IRIF */
  }

/***************************************************************************/
/* This routine is called at the point where a user label is defined (LABEL).
 * Put out the sltnormal to mark the code location and store the sltindex
 * in the p->cdb_info field.  This value will be used when the K_LABEL 
 * dnttentry is generated.
 */

cdb_prog_label(sym)
  struct symtab * sym;
{
  sym->cdb_info.label_slt = sltnormal();
}

/****************************************************************************/
/* Generate a K_MODULE and SLT_MODULE to mark the beginning of a module.
 * For C, a module corresponds to the total compilation unit. I.e., there
 * is always exactly one module.
 */

LOCAL unsigned long module_begin_dntt;	/* saves dntt-ptr of the K_MOULE for
					 * use when generating the matching
					 * K_END.
					 */

cdb_module_begin()
  {
  extern char ftitle[];
  extern int lineno;
  int slt;
  int olocctr;
  char * module_name;

/* A NULL module name could only happen if 'ccom' is invoked directly */
/* with input that has not passed through 'cpp'. Since the K_SRCFILE  */
/* code assuumes the name is is already includes the quotes, the null */
/* name is actually a pair of double quotes.                          */

#ifndef IRIF
  module_begin_dntt = INDEXED_LNTT_DP(lnttindex);
  olocctr = locctr(PROG);
  slt = sltspecial(SLT_MODULE,lineno,(unsigned)module_begin_dntt);
  (void)locctr(LNTT);
  (void)dnttentry(K_MODULE,slt);
  (void)locctr(olocctr);
#else /* IRIF */
  module_begin_dntt = symbolic_id;
  olocctr = dntt_type;	
  slt = sltspecial(SLT_MODULE,lineno,(unsigned)module_begin_dntt);
  dntt_type = LNTT;
  (void)dnttentry(K_MODULE,slt);
  dntt_type = olocctr;
#endif /* IRIF */

}


/***************************************************************************/
/* Generate a K_END/SLT_END to mark the end of the module (compilation
 * unit.
 */

cdb_module_end()
  {
  int slt;
  extern int lineno;

#ifndef IRIF
  (void)locctr(PROG);
  slt = sltspecial(SLT_END,lineno,(unsigned)INDEXED_LNTT_DP(lnttindex));
  (void)locctr(LNTT);
#else /* IRIF */
  slt = sltspecial(SLT_END,lineno,symbolic_id);
  dntt_type = LNTT;
#endif /* IRIF */
  (void)dnttentry(K_END,K_MODULE,slt,module_begin_dntt);
  }

# endif		/* ifdef HPCDB */
