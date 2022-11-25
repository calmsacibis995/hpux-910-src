/* @(#) $Revision: 70.1 $ */      

/*************************** debug.c ***************************************/
/*  Code to handle cdb/xdb pseudo-ops */

# include  <stdio.h>

/*  The 800 cross-compiler needs these files from the 300, not from the 800.
    We need the 300 version on the current directory... */

#ifdef xcomp300_800
# include "symtab.h"
# include "dnttsizes.h"
#else
/* These comes from /usr/contrib/include (at the moment).  There is an */
/* "-I/usr/contrib/include" flag in the makefile so it can be found.   */
# include  <symtab.h>
# include  <dnttsizes.h>
#endif

# include  "symbols.h"
# include  "adrmode.h"
# include  "sdopt.h"

/* External GLOBAL Variables  */
/* -------------------------- */
extern symbol * dot;
extern FILE   * fdout;
extern FILE   * fdlntt, *fdgntt, *fdslt,  *fdvt, *fdxt, *fddntt;
extern long     dotslt;
extern long     newdot;
extern long     line;
extern char   * filenames[];

/* GLOBAL Variables  */
/* ----------------- */
union dnttentry  dnt;
union sltentry   slt;
union xrefentry  xref;

int gfiles_open = 0;

# define VIA_VT_TEMPFILE 1
# define VIA_VT_BYTES    2
int have_vt_data = 0;

# define CHECK_GFILES_OPEN  { if (!gfiles_open) open_debug_tmp_files(); }

int incfile_offset;
int incdnt_size;

/*########################################################################*\
 # Pass 1 routines.
 # There is one routine for each type of dnt-entry, slt-entry, and xt-entry.
 # Fill in the fields of a dntt, slt, or xt entry, and write the 
 # structure to an intermediate file (via calls to "dump_dnttentry"
 # or "dump_sltentry", "dump_xrefentry".
 #
 # Dntt's, slt's and xt's can be built into final form except for forward
 # symbolic reference's.  These will have to be fixed in Pass 2.
\*#########################################################################*/


/* while doing debugging/qa, clear the dnt before starting so that unused */
/* bits will always be zero, and regression tests will pass.		  */
/* ---------------------------------------------------------------------- */

# if ZDNTT
  union dnttentry null_dnt;
  union xrefentry null_xref;
# define CLEAR_DNT(dnt)	  dnt = null_dnt
# define CLEAR_XREF(xref) xref = null_xref
# else
# define CLEAR_DNT(dnt)
# define CLEAR_XREF(xref)
# endif

/*###############*\
 # DNTT routines 
\*###############*/

# define MUST_BE(type, s) if (dot->stype != type) must_be_error(type, s)

void must_be_error(stype, s)
    long  stype;
    char *s;
{
    char buff[80];
    char *s2;
        
    s2 = (stype == SLNTT) ? "LNTT" : "GNTT";
    sprintf(buff, "dntt type <dnt_%s> is only valid in the %s", s, s2);
    uerror(buff);
}

/* ------------------------------------------- */

dntt_srcfile(language, name, address)
    LANGTYPE    language;
    VTPOINTER   name;
    SLTPOINTER  address;

{ 
    CLEAR_DNT(dnt);
    dnt.dsfile.extension = 0;
    dnt.dsfile.kind      = K_SRCFILE;
    dnt.dsfile.language  = language;
    dnt.dsfile.unused    = 0;
    dnt.dsfile.name      = name;
    dnt.dsfile.address   = address;
    dump_dnttentry(&dnt);
}
			  
dntt_module(name, alias, address) 
    VTPOINTER  name;
    VTPOINTER  alias;
    SLTPOINTER address;

{ 
    MUST_BE(SLNTT, "module");
    CLEAR_DNT(dnt);
    dnt.dmodule.extension = 0;
    dnt.dmodule.kind      = K_MODULE;
    dnt.dmodule.unused    = 0;
    dnt.dmodule.name      = name;
    dnt.dmodule.alias     = alias;
    dnt.dmodule.dummy.word= DNTTNIL;
    dnt.dmodule.address   = address;
    dump_dnttentry(&dnt);
}

dntt_function(public, language, level, optimize, varargs, info, inlined, name, 
	      alias, firstparam, address, entryaddr, retval, lowaddr, hiaddr)

    BITS            public, language, level, optimize, varargs, info, inlined;
    VTPOINTER       name, alias;
    DNTTPOINTER     firstparam;
    SLTPOINTER      address;
    ADDRESS         entryaddr;
    DNTTPOINTER     retval;
    ADDRESS         lowaddr, hiaddr;
    
{ 
    MUST_BE(SLNTT, "function");
    CLEAR_DNT(dnt);
    dnt.dfunc.extension  = 0;
    dnt.dfunc.kind       = K_FUNCTION;
    dnt.dfunc.public     = public;
    dnt.dfunc.language   = language;
    dnt.dfunc.level      = level;
    dnt.dfunc.optimize   = optimize;
    dnt.dfunc.varargs    = varargs;
    dnt.dfunc.info       = info;
    dnt.dfunc.inlined    = inlined;
    dnt.dfunc.unused     = 0;
    dnt.dfunc.name       = name;
    dnt.dfunc.alias      = alias;
    dnt.dfunc.firstparam = firstparam;
    dnt.dfunc.address    = address;
    dnt.dfunc.entryaddr  = entryaddr;
    dnt.dfunc.retval     = retval;
    dnt.dfunc.lowaddr    = lowaddr;
    dnt.dfunc.hiaddr     = hiaddr;
    dump_dnttentry(&dnt);
}

dntt_entry(public, language, level, optimize, varargs, info, inlined, name,
	   alias, firstparam, address, entryaddr, retval)

    BITS            public, language, level, optimize, varargs, info, inlined;
    VTPOINTER       name, alias;
    DNTTPOINTER     firstparam;
    SLTPOINTER      address;
    ADDRESS         entryaddr;
    DNTTPOINTER     retval;
    
{ 
    MUST_BE(SLNTT, "entry");
    CLEAR_DNT(dnt);
    dnt.dentry.extension  = 0;
    dnt.dentry.kind       = K_ENTRY;
    dnt.dentry.public     = public;
    dnt.dentry.language   = language;
    dnt.dentry.level      = level;
    dnt.dentry.optimize   = optimize;
    dnt.dentry.varargs    = varargs;
    dnt.dentry.info       = info;
    dnt.dentry.inlined    = inlined;
    dnt.dentry.inlined    = 0;
    dnt.dentry.unused     = 0;
    dnt.dentry.name       = name;
    dnt.dentry.alias      = alias;
    dnt.dentry.firstparam = firstparam;
    dnt.dentry.address    = address;
    dnt.dentry.entryaddr  = entryaddr;
    dnt.dentry.retval     = retval;
    dnt.dentry.lowaddr    = 0;
    dnt.dentry.hiaddr     = 0;
    dump_dnttentry(&dnt);
}

dntt_blockdata(public, language, level, optimize, varargs, info, name, alias,
              firstparam, address, retval)

    BITS            public, language, level, optimize, varargs, info;
    VTPOINTER       name, alias;
    DNTTPOINTER     firstparam;
    SLTPOINTER      address;
    DNTTPOINTER     retval;
    
{ 
    MUST_BE(SLNTT, "blockdata");
    CLEAR_DNT(dnt);
    dnt.dblockdata.extension  = 0;
    dnt.dblockdata.kind       = K_BLOCKDATA;
    dnt.dblockdata.public     = public;
    dnt.dblockdata.language   = language;
    dnt.dblockdata.level      = level;
    dnt.dblockdata.optimize   = optimize;
    dnt.dblockdata.varargs    = varargs;
    dnt.dblockdata.info       = info;
    dnt.dblockdata.inlined    = 0;
    dnt.dblockdata.unused     = 0;
    dnt.dblockdata.name       = name;
    dnt.dblockdata.alias      = alias;
    dnt.dblockdata.firstparam = firstparam;
    dnt.dblockdata.address    = address;
    dnt.dblockdata.entryaddr  = 0;
    dnt.dblockdata.retval     = retval;
    dnt.dblockdata.lowaddr    = 0;
    dnt.dblockdata.hiaddr     = 0;
    dump_dnttentry(&dnt);
}

dntt_begin(classflag, address)
    BITS       classflag;
    SLTPOINTER address;

{ 
    MUST_BE(SLNTT, "begin");
    CLEAR_DNT(dnt);
    dnt.dbegin.extension = 0;
    dnt.dbegin.kind      = K_BEGIN;
    dnt.dbegin.classflag = classflag;
    dnt.dbegin.unused    = 0;
    dnt.dbegin.address   = address;
    dump_dnttentry(&dnt);
}

dntt_end(endkind, classflag, address, beginscope)
    KINDTYPE       endkind;
    BITS           classflag;
    SLTPOINTER     address;
    DNTTPOINTER    beginscope;
    
{ 
    MUST_BE(SLNTT, "end");
    CLEAR_DNT(dnt);
    dnt.dend.extension  = 0;
    dnt.dend.kind       = K_END;
    dnt.dend.endkind    = endkind;
    dnt.dend.classflag  = classflag;
    dnt.dend.unused     = 0;
    dnt.dend.address    = address;
    dnt.dend.beginscope = beginscope;
    dump_dnttentry(&dnt);
}

dntt_import(explicit, module, item)
    BITS        explicit;
    VTPOINTER   module, item;
    
{ 
    MUST_BE(SLNTT, "import");
    CLEAR_DNT(dnt);
    dnt.dimport.extension = 0;
    dnt.dimport.kind      = K_IMPORT;
    dnt.dimport.explicit  = explicit;
    dnt.dimport.unused    = 0;
    dnt.dimport.module    = module;
    dnt.dimport.item      = item;
    dump_dnttentry(&dnt);
}

dntt_label(name, address)
    VTPOINTER   name;
    SLTPOINTER  address;
    
{ 
    MUST_BE(SLNTT, "label");
    CLEAR_DNT(dnt);
    dnt.dlabel.extension = 0;
    dnt.dlabel.kind      = K_LABEL;
    dnt.dlabel.unused    = 0;
    dnt.dlabel.name      = name;
    dnt.dlabel.address   = address;
    dump_dnttentry(&dnt);
}

dntt_with(addrtype, indirect, longaddr, nestlevel, location, address, 
	  type, name, offset, stattype_is_id)

    BITS             addrtype, indirect, longaddr, nestlevel;
    long             location;  /* Could be STATTYPE DYNTYPE REGTYPE */
    SLTPOINTER       address;
    DNTTPOINTER      type;
    VTPOINTER        name;
    unsigned long    offset;	
    int		     stattype_is_id;
        
{ 
    if (addrtype == 0 /* STATTYPE */ ) 
    {
	if ((!stattype_is_id) && (location != STATNIL))
	{
	    uerror("with STATTYPE location must be symbolic or -1 (STATNIL)");
	    return;
	}
    }
    else
	if (stattype_is_id)
	{
	    uerror("with STATTYPE location must not be symbolic for DYNTYPE or REGTYPE");
	    return;
	}

    MUST_BE(SLNTT, "with");
    CLEAR_DNT(dnt);
    dnt.dwith.extension = 0;
    dnt.dwith.kind      = K_WITH;
    dnt.dwith.addrtype  = addrtype;
    dnt.dwith.indirect  = indirect;
    dnt.dwith.longaddr  = longaddr;
    dnt.dwith.nestlevel = nestlevel;
    dnt.dwith.unused    = 0;
    dnt.dwith.location  = location;
    dnt.dwith.address   = address;
    dnt.dwith.type      = type;
    dnt.dwith.name      = name;
    dnt.dwith.offset    = offset;
    dump_dnttentry(&dnt);
}

dntt_common(name, alias)
    VTPOINTER  name, alias;

{ 
    CLEAR_DNT(dnt);
    dnt.dcommon.extension = 0;
    dnt.dcommon.kind      = K_COMMON;
    dnt.dcommon.unused    = 0;
    dnt.dcommon.name      = name;
    dnt.dcommon.alias     = alias;
    dump_dnttentry(&dnt);
}

dntt_fparam(regparam, indirect, longaddr, copyparam, dflt, name, location, 
	    type, nextparam, misc)

    BITS            regparam, indirect, longaddr, copyparam, dflt;
    VTPOINTER       name;
    DYNTYPE         location;
    DNTTPOINTER     type, nextparam;
    int             misc;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dfparam.extension = 0;
    dnt.dfparam.kind      = K_FPARAM;
    dnt.dfparam.regparam  = regparam;
    dnt.dfparam.indirect  = indirect;
    dnt.dfparam.longaddr  = longaddr;
    dnt.dfparam.copyparam = copyparam;
    dnt.dfparam.dflt      = dflt;
    dnt.dfparam.unused    = 0;
    dnt.dfparam.name      = name;
    dnt.dfparam.location  = location;
    dnt.dfparam.type      = type;
    dnt.dfparam.nextparam = nextparam;
    dnt.dfparam.misc      = misc;
    dump_dnttentry(&dnt);
}

dntt_svar(public, indirect, longaddr, staticmem, a_union, name, 
	  location, type, offset, displacement, stattype_is_id)
    BITS             public, indirect, longaddr, staticmem, a_union;
    VTPOINTER        name;
    STATTYPE         location;
    DNTTPOINTER      type;
    unsigned long    offset, displacement;
    int		     stattype_is_id;
    
{ 
    if ((!stattype_is_id) && (location != STATNIL))
    {
	uerror("svar STATTYPE must be symbolic or -1 (STATNIL)");
	return;
    }
    
    CLEAR_DNT(dnt);
    dnt.dsvar.extension    = 0;
    dnt.dsvar.kind         = K_SVAR;
    dnt.dsvar.public       = public;
    dnt.dsvar.indirect     = indirect;
    dnt.dsvar.longaddr     = longaddr;
    dnt.dsvar.staticmem    = staticmem;
    dnt.dsvar.a_union      = a_union;
    dnt.dsvar.unused       = 0;
    dnt.dsvar.name         = name;
    dnt.dsvar.location     = location;
    dnt.dsvar.type         = type;
    dnt.dsvar.offset       = offset;
    dnt.dsvar.displacement = displacement;
    dump_dnttentry(&dnt);
}

dntt_dvar(public, indirect, regvar, a_union, name, location, type, offset)
    BITS              public, indirect, regvar, a_union;
    VTPOINTER         name;
    DYNTYPE           location;
    DNTTPOINTER       type;
    unsigned long     offset;
    
{ 
    MUST_BE(SLNTT, "dvar");
    CLEAR_DNT(dnt);
    dnt.ddvar.extension = 0;
    dnt.ddvar.kind      = K_DVAR;
    dnt.ddvar.public    = public;
    dnt.ddvar.indirect  = indirect;
    dnt.ddvar.regvar    = regvar;
    dnt.ddvar.a_union   = a_union;
    dnt.ddvar.unused    = 0;
    dnt.ddvar.name      = name;
    dnt.ddvar.location  = location;
    dnt.ddvar.type      = type;
    dnt.ddvar.offset    = offset;
    dump_dnttentry(&dnt);
}

dntt_const(public, indirect, locdesc, classmem, name, location, 
	   type, offset, displacement, stattype_is_id)
    BITS              public, indirect;
    LOCDESCTYPE       locdesc;
    BITS              classmem;
    VTPOINTER         name;
    STATTYPE          location;
    DNTTPOINTER       type;
    unsigned long     offset, displacement;
    int		      stattype_is_id;
        
{ 
    if (locdesc == LOC_PTR) 
    {
	if ((!stattype_is_id) && (location != STATNIL))
	{
	    uerror("const STATTYPE must be symbolic or -1 (STATNIL)");
	    return;
	}
    }
    else
	if (stattype_is_id)
	{
	    uerror("const STATTYPE must not be symbolic for LOC_IMMED or LOC_VT");
	    return;
	}
    	    

    CLEAR_DNT(dnt);
    dnt.dconst.extension    = 0;
    dnt.dconst.kind         = K_CONST;
    dnt.dconst.public       = public;
    dnt.dconst.indirect     = indirect;
    dnt.dconst.locdesc      = locdesc;
    dnt.dconst.classmem     = classmem;
    dnt.dconst.unused       = 0;
    dnt.dconst.name         = name;
    dnt.dconst.location     = location;
    dnt.dconst.type         = type;
    dnt.dconst.offset       = offset;
    dnt.dconst.displacement = displacement;
    dump_dnttentry(&dnt);
}

dntt_typedef(public, name, type)
    BITS           public;
    VTPOINTER      name;
    DNTTPOINTER    type;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dtype.extension = 0;
    dnt.dtype.kind      = K_TYPEDEF;
    dnt.dtype.public    = public;
    dnt.dtype.typeinfo  = 1;
    dnt.dtype.unused    = 0;
    dnt.dtype.name      = name;
    dnt.dtype.type      = type;
    dump_dnttentry(&dnt);
}

dntt_tagdef(public, typeinfo, name, type)
    BITS            public, typeinfo;
    VTPOINTER       name;
    DNTTPOINTER     type;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dtag.extension = 0;
    dnt.dtag.kind      = K_TAGDEF;
    dnt.dtag.public    = public;
    dnt.dtag.typeinfo  = typeinfo;
    dnt.dtag.unused    = 0;
    dnt.dtag.name      = name;
    dnt.dtag.type      = type;
    dump_dnttentry(&dnt);
}

dntt_pointer(pointsto, bitlength)
    DNTTPOINTER    pointsto;
    unsigned long  bitlength;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dptr.extension = 0;
    dnt.dptr.kind      = K_POINTER;
    dnt.dptr.unused    = 0;
    dnt.dptr.pointsto  = pointsto;
    dnt.dptr.bitlength = bitlength;
    dump_dnttentry(&dnt);
}

dntt_reference(pointsto, bitlength)
    DNTTPOINTER    pointsto;
    unsigned long  bitlength;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dptr.extension = 0;
    dnt.dptr.kind      = K_REFERENCE;
    dnt.dptr.unused    = 0;
    dnt.dptr.pointsto  = pointsto;
    dnt.dptr.bitlength = bitlength;
    dump_dnttentry(&dnt);
}

dntt_enum(firstmem, bitlength)
    DNTTPOINTER   firstmem;
    unsigned long bitlength;
    
{ 
    CLEAR_DNT(dnt);
    dnt.denum.extension = 0;
    dnt.denum.kind      = K_ENUM;
    dnt.denum.unused    = 0;
    dnt.denum.firstmem  = firstmem;
    dnt.denum.bitlength = bitlength;
    dump_dnttentry(&dnt);
}

dntt_memenum(classmem, name, value, nextmem)
    BITS           classmem;
    VTPOINTER      name;
    unsigned long  value;
    DNTTPOINTER    nextmem;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dmember.extension = 0;
    dnt.dmember.kind      = K_MEMENUM;
    dnt.dmember.classmem  = classmem;
    dnt.dmember.unused    = 0;
    dnt.dmember.name      = name;
    dnt.dmember.value     = value;
    dnt.dmember.nextmem   = nextmem;
    dump_dnttentry(&dnt);
}

dntt_set(declaration, subtype, bitlength)
    BITS           declaration;
    DNTTPOINTER    subtype;
    unsigned long  bitlength;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dset.extension   = 0;
    dnt.dset.kind        = K_SET;
    dnt.dset.declaration = declaration;
    dnt.dset.unused      = 0;
    dnt.dset.subtype     = subtype;
    dnt.dset.bitlength   = bitlength;
    dump_dnttentry(&dnt);
}

dntt_subrange(dyn_low, dyn_high, lowbound, highbound, subtype, bitlength)
    BITS            dyn_low, dyn_high;
    long            lowbound, highbound;
    DNTTPOINTER     subtype;
    unsigned long   bitlength;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dsubr.extension = 0;
    dnt.dsubr.kind      = K_SUBRANGE;
    dnt.dsubr.dyn_low   = dyn_low;
    dnt.dsubr.dyn_high  = dyn_high;
    dnt.dsubr.unused    = 0;
    dnt.dsubr.lowbound  = lowbound;
    dnt.dsubr.highbound = highbound;
    dnt.dsubr.subtype   = subtype;
    dnt.dsubr.bitlength = bitlength;
    dump_dnttentry(&dnt);
}

dntt_array(declaration, dyn_low, dyn_high, arrayisbytes, elemisbytes, elemorder,
           justified, arraylength, indextype, elemtype, elemlength)

    BITS            declaration, dyn_low, dyn_high, arrayisbytes;
    BITS            elemisbytes, elemorder, justified;
    unsigned long   arraylength;
    DNTTPOINTER     indextype, elemtype;
    unsigned long   elemlength;
    
{ 
    CLEAR_DNT(dnt);
    dnt.darray.extension     = 0;
    dnt.darray.kind          = K_ARRAY;
    dnt.darray.declaration   = declaration;
    dnt.darray.dyn_low       = dyn_low;
    dnt.darray.dyn_high      = dyn_high;
    dnt.darray.arrayisbytes  = arrayisbytes;
    dnt.darray.elemisbytes   = elemisbytes;
    dnt.darray.elemorder     = elemorder;
    dnt.darray.justified     = justified;
    dnt.darray.unused        = 0;
    dnt.darray.arraylength   = arraylength;
    dnt.darray.indextype     = indextype;
    dnt.darray.elemtype      = elemtype;
    dnt.darray.elemlength    = elemlength;
    dump_dnttentry(&dnt);
}

dntt_struct(declaration, firstfield, vartagfield, varlist, bitlength)
    BITS           declaration;
    DNTTPOINTER    firstfield, vartagfield, varlist;
    unsigned long  bitlength;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dstruct.extension   = 0;
    dnt.dstruct.kind        = K_STRUCT;
    dnt.dstruct.declaration = declaration;
    dnt.dstruct.unused      = 0;
    dnt.dstruct.firstfield  = firstfield;
    dnt.dstruct.vartagfield = vartagfield;
    dnt.dstruct.varlist     = varlist;
    dnt.dstruct.bitlength   = bitlength;
    dump_dnttentry(&dnt);
}

dntt_union(firstfield, bitlength)
    DNTTPOINTER    firstfield;
    unsigned long  bitlength;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dunion.extension  = 0;
    dnt.dunion.kind       = K_UNION;
    dnt.dunion.unused     = 0;
    dnt.dunion.firstfield = firstfield;
    dnt.dunion.bitlength  = bitlength;
    dump_dnttentry(&dnt);
}

dntt_field(visibility, a_union, name, bitoffset, type, bitlength, nextfield)
    BITS           visibility, a_union;
    VTPOINTER      name;
    unsigned long  bitoffset;
    DNTTPOINTER    type;
    unsigned long  bitlength;
    DNTTPOINTER    nextfield;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dfield.extension  = 0;
    dnt.dfield.kind       = K_FIELD;
    dnt.dfield.visibility = visibility;
    dnt.dfield.a_union    = a_union;
    dnt.dfield.unused     = 0;
    dnt.dfield.name       = name;
    dnt.dfield.bitoffset  = bitoffset;
    dnt.dfield.type       = type;
    dnt.dfield.bitlength  = bitlength;
    dnt.dfield.nextfield  = nextfield;
    dump_dnttentry(&dnt);
}

dntt_variant(lowvarvalue, hivarvalue, varstruct, bitoffset, nextvar)
    long           lowvarvalue, hivarvalue;
    DNTTPOINTER    varstruct;
    unsigned long  bitoffset;
    DNTTPOINTER    nextvar;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dvariant.extension   = 0;
    dnt.dvariant.kind        = K_VARIANT;
    dnt.dvariant.unused      = 0;
    dnt.dvariant.lowvarvalue = lowvarvalue;
    dnt.dvariant.hivarvalue  = hivarvalue;
    dnt.dvariant.varstruct   = varstruct;
    dnt.dvariant.bitoffset   = bitoffset;
    dnt.dvariant.nextvar     = nextvar;
    dump_dnttentry(&dnt);
}

dntt_file(ispacked, bitlength, bitoffset, elemtype)
    BITS           ispacked;
    unsigned long  bitlength, bitoffset;
    DNTTPOINTER    elemtype;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dfile.extension   = 0;
    dnt.dfile.kind        = K_FILE;
    dnt.dfile.ispacked    = ispacked;
    dnt.dfile.unused      = 0;
    dnt.dfile.bitlength   = bitlength;
    dnt.dfile.bitoffset   = bitoffset;
    dnt.dfile.elemtype    = elemtype;
    dump_dnttentry(&dnt);
}

dntt_functype(varargs, info, bitlength, firstparam, retval)
    BITS	    varargs, info;
    unsigned long   bitlength;
    DNTTPOINTER     firstparam, retval;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dfunctype.extension  = 0;
    dnt.dfunctype.kind       = K_FUNCTYPE;
    dnt.dfunctype.varargs    = varargs;
    dnt.dfunctype.info       = info;
    dnt.dfunctype.unused     = 0;
    dnt.dfunctype.bitlength  = bitlength;
    dnt.dfunctype.firstparam = firstparam;
    dnt.dfunctype.retval     = retval;
    dump_dnttentry(&dnt);
}

dntt_cobstruct(hasoccurs, istable, parent, child, sibling, synonym,
               catusage, pointloc, numdigits, table, editpgm, bitlength)

    BITS           hasoccurs, istable;
    DNTTPOINTER    parent, child, sibling, synonym;
    BITS           catusage, pointloc, numdigits;
    DNTTPOINTER    table;
    VTPOINTER      editpgm;
    unsigned long  bitlength;
    
    
{ 
    CLEAR_DNT(dnt);
    dnt.dcobstruct.extension  	= 0;
    dnt.dcobstruct.kind         = K_COBSTRUCT;
    dnt.dcobstruct.hasoccurs	= hasoccurs;
    dnt.dcobstruct.istable	= istable;
    dnt.dcobstruct.unused	= 0;
    dnt.dcobstruct.parent	= parent;
    dnt.dcobstruct.child	= child;
    dnt.dcobstruct.sibling	= sibling;
    dnt.dcobstruct.synonym	= synonym;
    dnt.dcobstruct.catusage	= catusage;
    dnt.dcobstruct.pointloc	= pointloc;
    dnt.dcobstruct.numdigits	= numdigits;
    dnt.dcobstruct.unused2	= 0;
    dnt.dcobstruct.table	= table;
    dnt.dcobstruct.editpgm	= editpgm;
    dnt.dcobstruct.bitlength	= bitlength;
    dump_dnttentry(&dnt);
}

dntt_genfield(visibility, a_union, field, nextfield)
    BITS        visibility, a_union;
    DNTTPOINTER field, nextfield;
{
    CLEAR_DNT(dnt);
    dnt.dgenfield.extension  = 0;
    dnt.dgenfield.kind       = K_GENFIELD;
    dnt.dgenfield.visibility = visibility;
    dnt.dgenfield.a_union    = a_union;
    dnt.dgenfield.unused     = 0;
    dnt.dgenfield.field      = field;
    dnt.dgenfield.nextfield  = nextfield;
    dump_dnttentry(&dnt);
}

dntt_memaccess(classptr, field)
    DNTTPOINTER classptr, field;
{
    CLEAR_DNT(dnt);
    dnt.dmemaccess.extension = 0;
    dnt.dmemaccess.kind      = K_MEMACCESS;
    dnt.dmemaccess.unused    = 0;
    dnt.dmemaccess.classptr  = classptr;
    dnt.dmemaccess.field     = field;
    dump_dnttentry(&dnt);
}

dntt_modifier(m_const, m_static, m_void, m_volatile, m_duplicate, type)
    BITS        m_const, m_static, m_void, m_volatile, m_duplicate;
    DNTTPOINTER type;
{
    CLEAR_DNT(dnt);
    dnt.dmodifier.extension   = 0;
    dnt.dmodifier.kind        = K_MODIFIER;
    dnt.dmodifier.m_const     = m_const;
    dnt.dmodifier.m_static    = m_static;
    dnt.dmodifier.m_void      = m_void;
    dnt.dmodifier.m_volatile  = m_volatile;
    dnt.dmodifier.m_duplicate = m_duplicate;
    dnt.dmodifier.unused      = 0;
    dnt.dmodifier.type        = type;
    dump_dnttentry(&dnt);
}

dntt_vfunc(pure, funcptr, vtbl_offset)
    BITS          pure;
    DNTTPOINTER   funcptr;
    unsigned long vtbl_offset;
{
    CLEAR_DNT(dnt);
    dnt.dvfunc.extension   = 0;
    dnt.dvfunc.kind        = K_VFUNC;
    dnt.dvfunc.pure        = pure;
    dnt.dvfunc.unused      = 0;
    dnt.dvfunc.funcptr     = funcptr;
    dnt.dvfunc.vtbl_offset = vtbl_offset;
    dump_dnttentry(&dnt);
}

dntt_classscope(address, type)
    SLTPOINTER  address;
    DNTTPOINTER type;
{
    CLEAR_DNT(dnt);
    dnt.dclass_scope.extension = 0;
    dnt.dclass_scope.kind      = K_CLASS_SCOPE;
    dnt.dclass_scope.unused    = 0;
    dnt.dclass_scope.address   = address;
    dnt.dclass_scope.type      = type;
    dump_dnttentry(&dnt);
}

dntt_friendclass(classptr, next)
    DNTTPOINTER classptr, next;
{
    CLEAR_DNT(dnt);
    dnt.dfriend_class.extension = 0;
    dnt.dfriend_class.kind      = K_FRIEND_CLASS;
    dnt.dfriend_class.unused    = 0;
    dnt.dfriend_class.classptr  = classptr;
    dnt.dfriend_class.next      = next;
    dump_dnttentry(&dnt);
}

dntt_friendfunc(funcptr, classptr, next)
    DNTTPOINTER funcptr, classptr, next;
{
    CLEAR_DNT(dnt);
    dnt.dfriend_func.extension = 0;
    dnt.dfriend_func.kind      = K_FRIEND_FUNC;
    dnt.dfriend_func.unused    = 0;
    dnt.dfriend_func.funcptr   = funcptr;
    dnt.dfriend_func.classptr  = classptr;
    dnt.dfriend_func.next      = next;
    dump_dnttentry(&dnt);
}

dntt_class(abstract, class_decl, memberlist, vtbl_loc, parentlist, 
	   bitlength, identlist, friendlist, future2, future3)
    BITS          abstract, class_decl;
    DNTTPOINTER   memberlist;
    unsigned long vtbl_loc;
    DNTTPOINTER   parentlist;
    unsigned long bitlength;
    DNTTPOINTER   identlist, friendlist;
    unsigned long future2, future3;
{
    CLEAR_DNT(dnt);
    dnt.dclass.extension  = 0;
    dnt.dclass.kind       = K_CLASS;
    dnt.dclass.abstract   = abstract;
    dnt.dclass.class_decl = class_decl;
    dnt.dclass.unused     = 0;
    dnt.dclass.memberlist = memberlist;
    dnt.dclass.vtbl_loc   = vtbl_loc;
    dnt.dclass.parentlist = parentlist;
    dnt.dclass.bitlength  = bitlength;
    dnt.dclass.identlist  = identlist;
    dnt.dclass.friendlist = friendlist;
    dump_dnttentry(&dnt);
}

dntt_ptrmem(pointsto, memtype)
    DNTTPOINTER pointsto, memtype;
{
    CLEAR_DNT(dnt);
    dnt.dptrmem.extension = 0;
    dnt.dptrmem.kind      = K_PTRMEM;
    dnt.dptrmem.unused    = 0;
    dnt.dptrmem.pointsto  = pointsto;
    dnt.dptrmem.memtype   = memtype;
    dump_dnttentry(&dnt);
}

dntt_inheritance(Virtual, visibility, classname, offset, next)
    BITS          Virtual, visibility;
    DNTTPOINTER   classname;
    unsigned long offset;
    DNTTPOINTER   next;
{
    CLEAR_DNT(dnt);
    dnt.dinheritance.extension  = 0;
    dnt.dinheritance.kind       = K_INHERITANCE;
    dnt.dinheritance.Virtual    = Virtual;
    dnt.dinheritance.visibility = visibility;
    dnt.dinheritance.unused     = 0;
    dnt.dinheritance.classname  = classname;
    dnt.dinheritance.offset     = offset;
    dnt.dinheritance.next       = next;
    dump_dnttentry(&dnt);
}

dntt_objectid(object_ident, offset, next)
    unsigned long object_ident, offset;
    DNTTPOINTER   next;
{
    CLEAR_DNT(dnt);
    dnt.dobject_id.extension    = 0;
    dnt.dobject_id.kind         = K_OBJECT_ID;
    dnt.dobject_id.unused       = 0;
    dnt.dobject_id.object_ident = object_ident;
    dnt.dobject_id.offset       = offset;
    dnt.dobject_id.next         = next;
    dnt.dobject_id.segoffset    = 0;
    dump_dnttentry(&dnt);
}

dntt_memfunc(public, language, varargs, info, inlined, name, 
	      alias, firstparam, retval)

    BITS            public, language, varargs, info, inlined;
    VTPOINTER       name, alias;
    DNTTPOINTER     firstparam;
    DNTTPOINTER     retval;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dmemfunc.extension  = 0;
    dnt.dmemfunc.kind       = K_MEMFUNC;
    dnt.dmemfunc.public     = public;
    dnt.dmemfunc.language   = language;
    dnt.dmemfunc.level      = 0;
    dnt.dmemfunc.optimize   = 0;
    dnt.dmemfunc.varargs    = varargs;
    dnt.dmemfunc.info       = info;
    dnt.dmemfunc.inlined    = inlined;
    dnt.dmemfunc.unused     = 0;
    dnt.dmemfunc.name       = name;
    dnt.dmemfunc.alias      = alias;
    dnt.dmemfunc.firstparam = firstparam;
    dnt.dmemfunc.address    = 0;
    dnt.dmemfunc.entryaddr  = 0;
    dnt.dmemfunc.retval     = retval;
    dnt.dmemfunc.lowaddr    = 0;
    dnt.dmemfunc.hiaddr     = 0;
    dump_dnttentry(&dnt);
}

dntt_block(word1, word2, word3, sym1, sym2, sym3, 
	   st_is_id1, st_is_id2, st_is_id3)
long     word1, word2, word3;
STATTYPE sym1, sym2, sym3;
int      st_is_id1, st_is_id2, st_is_id3;
{
    unsigned long data;
    static int kind = K_NIL;
    static int offset = 0;

    if ((kind == K_NIL) && (word1 & 0x80000000)) {
	uerror("extension bit set in first word of first dnt_block)");
	return;
    }

    if ((kind != K_NIL) && !(word1 & 0x80000000)) {
	uerror("extension bit not set in first word of 2nd or 3rd dnt_block)");
	kind = K_NIL;
	offset = 0;
	return;
    }

    if (st_is_id1) {
	uerror("first block STATTYPE must be -1 (STATNIL)");
	return;
    }
       
    if ((!st_is_id1 && (sym1 != STATNIL)) ||
        (!st_is_id2 && (sym2 != STATNIL)) ||
        (!st_is_id3 && (sym3 != STATNIL))) {
	uerror("block STATTYPE must be symbolic or -1 (STATNIL)");
	return;
    }

    if (kind == K_NIL) {
       CLEAR_DNT(dnt);
       offset = 0;
    }

    if (st_is_id1) {
       data = sym1;
    }
    else {
       data = word1;
    }
    dnt.dgeneric.word[offset] = data;

    if (st_is_id2) {
       data = sym2;
    }
    else {
       data = word2;
    }
    dnt.dgeneric.word[offset + 1] = data;

    if (st_is_id3) {
       data = sym3;
    }
    else {
       data = word3;
    }
    dnt.dgeneric.word[offset + 2] = data;

    if (kind == K_NIL) {
       kind = dnt.dblock.kind;
       if ((kind < K_SRCFILE) || (kind > K_MAX)) {
	  uerror("bad kind field in first dnt_block");
          kind = K_NIL;
       }
    }

    if (DNTTSIZE[kind] == (offset / 3 + 1)) {
       dump_dnttentry(&dnt);
       kind = K_NIL;
       offset = 0;
    }
    else {
       offset += 3;
    }
}

dntt_include(name)
char *name;
{
    filenames[12] = (char *) malloc( strlen(name) + 1 );
    
    if (!filenames[12])
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("could not malloc space for dntt file name");
    
    strcpy(filenames[12], name);
    
    if ((fddntt = fopen(name, "r")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open dntt include file %s",name);

    unlink(name);

    incfile_offset = 0;
    incdnt_size    = 0;
}


dntt_fixup(gotoend, offset, sym)
int     gotoend;
int     offset;
symbol *sym;
{
    int bytesread;
    int bytesneeded;
    int kind;
    static int dnt_size;

    if (!gotoend && (offset & 3)) {
        uerror("dnt_fixup offset not a multiple of 4 bytes");
    }

    for (;;) {

        if (incdnt_size) {
	    if (!gotoend && (offset < incfile_offset + incdnt_size)) {
	        dnt.dgeneric.word[(offset - incfile_offset) / 4] = (int) sym;
	        return;
	    }
            dump_dnttentry(&dnt);
	    incfile_offset += incdnt_size;
        }

        CLEAR_DNT(dnt);

        bytesread = fread(&dnt, 1, DNTTBLOCKSIZE, fddntt);
        if (bytesread == 0) {
	    if (!gotoend) {
	    	uerror("dntt_fixup offset past end of include file");
	    }
	    return;
	}
	else if (bytesread != DNTTBLOCKSIZE) {
	    uerror("dnt_include file size not a multiple of DNTTLOCKSIZE");
	    return;
	}
	if (dnt.dgeneric.word[0] & 0x80000000) {
	    uerror("extension bit set in first word of dntt in dntt_include");
	    return;
	}

        kind = dnt.dblock.kind;
        if ((kind < K_SRCFILE) || (kind > K_MAX)) {
	    uerror("bad kind field in dntt_include");
	    return;
        }

	incdnt_size = DNTTBLOCKSIZE * (DNTTSIZE[kind]);
	bytesneeded = incdnt_size - DNTTBLOCKSIZE;

        bytesread = fread(&dnt.dgeneric.word[3], 1, bytesneeded, fddntt);

	if (bytesread != bytesneeded) {
	    uerror("dnt_include missing extension blocks");
	    return;
	}

    }
}

dntt_xref(language,xreflist, xref_sym)
    BITS	  language;
    XREFPOINTER   xreflist;
    symbol        *xref_sym;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dxref.extension  = 0;
    dnt.dxref.kind       = K_XREF;
    dnt.dxref.language   = language;
    dnt.dxref.unused     = 0;
    dnt.dxref.xreflist   = xreflist;
    dnt.dxref.extra      = 0;
    
    /* For now (until the "extra" field is claimed), we will use it */
    /* as a flag that "xreflist" is really a symbol table pointer.  */
    /* ------------------------------------------------------------ */
    if (xref_sym)
    {
	dnt.dxref.xreflist = (XREFPOINTER) xref_sym;
	dnt.dxref.extra    = -1;
    }
    
    dump_dnttentry(&dnt);
}  

dntt_sa(base_kind, name)
    KINDTYPE      base_kind;
    VTPOINTER     name;
    
{ 
    CLEAR_DNT(dnt);
    dnt.dsa.extension  = 0;
    dnt.dsa.kind       = K_SA;
    dnt.dsa.base_kind  = base_kind;
    dnt.dsa.unused     = 0;
    dnt.dsa.name       = name;
    dnt.dsa.extra      = 0;
    dump_dnttentry(&dnt);
}

dntt_macro(name)
    VTPOINTER     name;
    
{ 
    dntt_sa(K_MACRO, name);
}

/*###############*\
 # SLT routines 
\*###############*/

sltnormal(lineno)
    int lineno;
    
{ 
    slt.snorm.sltdesc = SLT_NORMAL;
    slt.snorm.line    = lineno;
    slt.snorm.address = dot->svalue;
    dump_sltentry(&slt);
# ifdef SDOPT
    if (sdopt_flag) putw(sdi_listtail, fdslt);
# endif
}

sltexit(lineno)
    int lineno;

{ 
    slt.snorm.sltdesc = SLT_EXIT;
    slt.snorm.line    = lineno;
    slt.snorm.address = dot->svalue;
    dump_sltentry(&slt);
# ifdef SDOPT
    if (sdopt_flag) putw(sdi_listtail, fdslt);
# endif
}

sltspecial(type, lineno, backptr)
    unsigned long  type;
    int            lineno;
    DNTTPOINTER    backptr;
    
{ 
    if (type == SLT_MARKER)
	uerror("slt type MARKER is not currently supported");
    
    slt.sspec.sltdesc = type;
    slt.sspec.line    = lineno;
    slt.sspec.backptr = backptr;
    dump_sltentry(&slt);
}

sltasst(address)
    SLTPOINTER  address;

{ 
    slt.sasst.sltdesc = SLT_ASSIST;
    slt.sasst.unused  = 0;
    slt.sasst.address = address;
    dump_sltentry(&slt);
}

/*###############*\
 # XT routines 
\*###############*/

xt_info1(definition, declaration, modification, use, call, column, line)

    unsigned long   definition, declaration, modification, use;
    unsigned long   call, column, line;

{   CLEAR_XREF(xref);
    xref.xrefshort.tag          = XINFO1;
    xref.xrefshort.definition   = definition;
    xref.xrefshort.declaration  = declaration;
    xref.xrefshort.modification = modification;
    xref.xrefshort.use          = use;
    xref.xrefshort.call         = call;
    xref.xrefshort.column       = column;
    xref.xrefshort.line         = line;
    dump_xrefentry(&xref, (symbol *)NULL);
}

xt_info2A(definition, declaration, modification, use, call, column)

    unsigned long   definition, declaration, modification, use;
    unsigned long   call, column;

{   CLEAR_XREF(xref);
    xref.xreflong.tag          = XINFO2;
    xref.xreflong.definition   = definition;
    xref.xreflong.declaration  = declaration;
    xref.xreflong.modification = modification;
    xref.xreflong.use          = use;
    xref.xreflong.call         = call;
    xref.xreflong.extra        = 0;
    xref.xreflong.column       = column;
    dump_xrefentry(&xref, (symbol *)NULL);
}

xt_info2B(line)
    unsigned long   line;

{   CLEAR_XREF(xref);
    xref.xrefline.line = line;
    dump_xrefentry(&xref, (symbol *)NULL);
}

xt_info(definition, declaration, modification, use, call, column, line)

    unsigned long   definition, declaration, modification, use;
    unsigned long   call, column, line;

{
    if (line > 65535)
    {
	xt_info2A(definition, declaration, modification, use, call, column);
	xt_info2B(line);
    }
    else
	xt_info1(definition, declaration, modification, use, call, column, line);
}
	    
xt_link(next, xref_sym)
    unsigned long   next;
    symbol	    *xref_sym;
    
{   CLEAR_XREF(xref);
    xref.xlink.tag  = XLINK;
    xref.xlink.next = next;

    /* If "xref_sym" is non-null, "next" is zero. */
    /* We want to turn it into all 1's as a flag. */
    /* Zero is used to mark EOL.                  */
    /* ------------------------------------------ */
    if (xref_sym)
	xref.xlink.next = ~xref.xlink.next;
    dump_xrefentry(&xref, xref_sym);
}

xt_name(filename)
    VTPOINTER  filename;

{   CLEAR_XREF(xref);
    xref.xfname.tag      = XNAME;
    xref.xfname.filename = filename;
    dump_xrefentry(&xref, (symbol *)NULL);
}


/***************************************************************************\
 *  generate_xtdata(data)
 *
 * This is called when we see a "xt_block xx,xx,xx" pseudo op. 
 * It is called once for each piece of data given.             
 *
\****************************************************************************/

generate_xtdata(data) 
    unsigned long data;
{ 
    dump_xrefentry(&data, (symbol *)NULL);
}


/*#########################################################################*\
 # Routines to dump pass1 structures to intermediate files.
\*#########################################################################*/

/***************************************************************************\
 * dump_sltentry(sltp)
 *
 * copy a slt-entry to the intermediate file. 
\****************************************************************************/

dump_sltentry(sltp)
    union sltentry * sltp;
{ 
    CHECK_GFILES_OPEN;
    
    if (dot->stype != STEXT)
	uerror("slt only legal within TEXT");
    
    if (fwrite(sltp, SLTBLOCKSIZE, 1, fdslt) != 1 ||
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	putw(line, fdslt)!=0 )		/* line number for errors */
	aerror("unable to write to temp (slt) file");
    
    dotslt += 1;
}


/***************************************************************************\
 * dump_dnttentry(dnttp)
 *
 * copy a dntt-entry to the intermediate file. 
\****************************************************************************/

dump_dnttentry(dnttp)
    union dnttentry *dnttp;

{ 
    CHECK_GFILES_OPEN;
    
    if ( (dot->stype != SGNTT) && (dot->stype != SLNTT) )
	uerror("dnt only legal within LNTT or GNTT");
    
    if (dot->stype == SGNTT)
    {
	if (fwrite(dnttp, sizeof(union dnttentry), 1, fdgntt) != 1 ||
	    putw(line, fdgntt)!=0 )		/* line number for errors */
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("unable to write temp (gntt) file");
    }
    else /* LNTT */
    {
	if (fwrite(dnttp, sizeof(union dnttentry), 1, fdlntt) != 1 ||
	    putw(line, fdlntt)!=0 )		/* line number for errors */
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("unable to write temp (lntt) file");
    }
    
    newdot += DNTTSIZE[dnttp->dblock.kind];
}


/***************************************************************************\
 * dump_xrefentry(xrefp, xref_sym)
 *
 * copy a xref-entry to the intermediate file. 
\****************************************************************************/

dump_xrefentry(xrefp, xref_sym)
    union xrefentry *xrefp;
    symbol          *xref_sym;
{ 
    CHECK_GFILES_OPEN;

    if (dot->stype != SXT)
	uerror("xt_xxx only legal within XT");

    if (fwrite(xrefp, sizeof(union xrefentry), 1, fdxt) != 1)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("unable to write temp (xt) file");

    newdot++;  /* ALL XT entries are 1 block long. */

    /* If the "next" field was a label, dump the   */
    /* symbol table entry pointer to the file now. */
    /* ------------------------------------------- */
    if (xref_sym)
	putw(xref_sym, fdxt);
}


/*#########################################################################*\
 # The following routines are the pass2 routines to read back from the
 # temporary files, fix up any dnttpointer's and stattypes, and then
 # write the final info to the object file.
\*#########################################################################*/

/****************************************************************************\
 *
 * void fix_stattype(location, displacement, sym)
 *
\****************************************************************************/

void fix_stattype(location, displacement, sym)
    STATTYPE	    *location;
    unsigned long   *displacement;
    symbol	    *sym;
{
    if (*location != STATNIL) {
	if (sym->slstindex != -1 ) 
	    *location = 0x80000000 | sym->slstindex; 
	else {  
	    /* mask off EXTERN or ALIGN before call to set symbol type */
	    /* ------------------------------------------------------- */
	    *location =  linker_sym_type(sym->stype&STYPE);
	    *displacement += sym->svalue;
	}
    }
}

/****************************************************************************\
 * fix_dnttpointer(dntp)
 *
 * If the dnttpointer was symbolic, make sure the symbol has been
 * properly resolved, and replace the field with dntt-index type
 * pointer.
 * a symbolic dnttpointer :
 *	This is really kludgy and non-portable.
 * 	In order to use the <symtab.h> structures, and avoid adding extra
 *	boolean fields to tell whether a dnttpointer is symbolic.
 *	In order to distinguish a symbol dntt from an immediate or indexed,
 *	we rely on the fact that our machine is aligning symbol
 *	structures on an even boundary.  So the low address bit is always 0.
 *	We shifted it off in the parser routines before the value is stored
 *	to be sure the high bit is zer0.  Then we'll shift it back in the
 *	dnttfixup.
 *
\****************************************************************************/

/* PCAL's are expensive --- don't make one for a simple check */

#define fix_dnttpointer(dntp) if (dntp.dnttp.extension == 0) \
                                 really_fix_dnttpointer(&dntp)

/* same thing as above only in function form */

fix_dnttpointer_func(dntp)
    register DNTTPOINTER *dntp;
{
    if (dntp->dnttp.extension == 0) 
	really_fix_dnttpointer(dntp);
}

/****************************************************************************\
 *  really_fix_dnttpointer(dntp)
 *
 * This routine is called if a dntt pointer is a symbol table entry.
 * See the above description in "fix_dntt_pointer" for the kludge
 * that tells us this.  Fix up the dntt pointer with actual data here.
 *
\****************************************************************************/

really_fix_dnttpointer(dntp)
    register DNTTPOINTER * dntp;

{ 
    register symbol * sym;

    if (dntp->dnttp.extension != 0) return;
    
    sym =  (symbol*)((unsigned int)dntp->word<<1);

    if ((sym->stype&STYPE)==SUNDEF)
	uerror("dntt symbol (%s) never resolved",sym->snamep);

    if ( ((sym->stype&STYPE)==SLNTT) && ((sym->stype&STYPE)==SGNTT) )
	uerror("symbol (%s) found in both LNTT and GNTT",sym->snamep);

    if ( ((sym->stype&STYPE)!=SLNTT) && ((sym->stype&STYPE)!=SGNTT) )
	uerror("symbol (%s) not of type LNTT/GNTT",sym->snamep);

    dntp->dnttp.extension = 1;
    dntp->dnttp.immediate = 0;
    dntp->dnttp.global    = ((sym->stype&STYPE) == SGNTT) ? 1 : 0;
    dntp->dnttp.index     = sym->svalue;
}

/****************************************************************************\
 * fix_dnttentry(dntp)
 *
 * This is called for each dntt entry read from the temp file on pass2.
 * We call the appropriate fixup routines to fix all appropriate fields.
 *
\****************************************************************************/

fix_dnttentry(dntp)
    register union  dnttentry * dntp;

{ 
    symbol * sym;
    int      dummy_displacement;

    switch(dntp->dblock.kind) 
    {
    default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("bad dntt kind in pass 2");
	break;
	
    case K_SRCFILE:
    case K_MODULE:
    case K_BEGIN:
    case K_COMMON:
    case K_IMPORT:
    case K_LABEL:
    case K_SA:
    case K_MACRO:
	/* nothing to fixup */
	break;
	
    case K_FUNCTION:
	fix_address(&dntp->dfunc.lowaddr);
	fix_address(&dntp->dfunc.hiaddr);
    case K_ENTRY:
	fix_address(&dntp->dfunc.entryaddr);
    case K_BLOCKDATA:
    case K_MEMFUNC:
	fix_dnttpointer(dntp->dfunc.firstparam);
	fix_dnttpointer(dntp->dfunc.retval);
	break;
	
    case K_WITH:
	fix_dnttpointer(dntp->dwith.type);

	/* OVERSIGHT??? There is no displacement in the with record for  */
	/*              pre indirection displacement.  Since no compiler */
	/*              on earth generates these anyway, we will ignore  */
	/*              it for now.                                      */
	/* ------------------------------------------------------------- */
	if (dntp->dwith.addrtype == 0 /* STATTYPE */)
	    fix_stattype(&dntp->dwith.location, 
			 &dummy_displacement,
			 (symbol *) dntp->dwith.location);

	break;
	
    case K_END:
	fix_dnttpointer(dntp->dend.beginscope);
	break;
	
    case K_FPARAM:
	fix_dnttpointer(dntp->dfparam.type);
	fix_dnttpointer(dntp->dfparam.nextparam);
	break;
	
    case K_SVAR:
	fix_dnttpointer(dntp->dsvar.type);
	fix_stattype(&dntp->dsvar.location, 
		     &dntp->dsvar.displacement, 
		     (symbol *) dntp->dsvar.location);
	break;
	
    case K_DVAR:
	fix_dnttpointer(dntp->ddvar.type);
	break;
	
    case K_CONST:
	if (dntp->dconst.locdesc == LOC_PTR)
	    fix_stattype(&dntp->dconst.location, 
			 &dntp->dconst.displacement, 
			 (symbol *) dntp->dconst.location);

	fix_dnttpointer(dntp->dconst.type);
	break;
	
    case K_TYPEDEF:
    case K_TAGDEF:
	fix_dnttpointer(dntp->dtype.type);
	break;
	
    case K_REFERENCE:
    case K_POINTER:
	fix_dnttpointer(dntp->dptr.pointsto);
	break;
	
    case K_ENUM:
	fix_dnttpointer(dntp->denum.firstmem);
	break;
	
    case K_MEMENUM:
	fix_dnttpointer(dntp->dmember.nextmem);
	break;
	
    case K_SET:
	fix_dnttpointer(dntp->dset.subtype);
	break;
	
    case K_SUBRANGE:
	fix_dnttpointer(dntp->dsubr.subtype);
	break;
	
    case K_ARRAY:
	fix_dnttpointer(dntp->darray.indextype);
	fix_dnttpointer(dntp->darray.elemtype);
	break;
	
    case K_STRUCT:
	fix_dnttpointer(dntp->dstruct.firstfield);
	fix_dnttpointer(dntp->dstruct.vartagfield);
	fix_dnttpointer(dntp->dstruct.varlist);
	break;
	
    case K_UNION:
	fix_dnttpointer(dntp->dunion.firstfield);
	break;
	
    case K_FIELD:
	fix_dnttpointer(dntp->dfield.type);
	fix_dnttpointer(dntp->dfield.nextfield);
	break;
	
    case K_VARIANT:
	fix_dnttpointer(dntp->dvariant.varstruct);
	fix_dnttpointer(dntp->dvariant.nextvar);
	break;
	
    case K_FILE:
	fix_dnttpointer(dntp->dfile.elemtype);
	break;
	
    case K_FUNCTYPE:
	fix_dnttpointer(dntp->dfunctype.firstparam);
	fix_dnttpointer(dntp->dfunctype.retval);
	break;
	
    case K_COBSTRUCT:
	fix_dnttpointer(dntp->dcobstruct.parent);
	fix_dnttpointer(dntp->dcobstruct.child);
	fix_dnttpointer(dntp->dcobstruct.sibling);
	fix_dnttpointer(dntp->dcobstruct.synonym);
	fix_dnttpointer(dntp->dcobstruct.table);
	break;

#ifdef CPLUSPLUS
	
    case K_GENFIELD:
	fix_dnttpointer(dntp->dgenfield.field);
	fix_dnttpointer(dntp->dgenfield.nextfield);
	break;
	
    case K_MEMACCESS:
	fix_dnttpointer(dntp->dmemaccess.classptr);
	fix_dnttpointer(dntp->dmemaccess.field);
	break;
	
    case K_MODIFIER:
	fix_dnttpointer(dntp->dmodifier.type);
	break;
	
    case K_VFUNC:
	fix_dnttpointer(dntp->dvfunc.funcptr);
	break;
	
    case K_CLASS_SCOPE:
	fix_dnttpointer(dntp->dclass_scope.type);
	break;
	
    case K_FRIEND_CLASS:
	fix_dnttpointer(dntp->dfriend_class.classptr);
	fix_dnttpointer(dntp->dfriend_class.next);
	break;
	
    case K_FRIEND_FUNC:
	fix_dnttpointer(dntp->dfriend_func.funcptr);
	fix_dnttpointer(dntp->dfriend_func.classptr);
	fix_dnttpointer(dntp->dfriend_func.next);
	break;
	
    case K_CLASS:
	fix_dnttpointer(dntp->dclass.memberlist);
	fix_dnttpointer(dntp->dclass.parentlist);
	fix_dnttpointer(dntp->dclass.identlist);
	fix_dnttpointer(dntp->dclass.friendlist);
#ifdef TEMPLATES
	fix_dnttpointer(dntp->dclass.templateptr);
	fix_dnttpointer(dntp->dclass.nextexp);
#endif
	break;
	
    case K_PTRMEM:
    case K_PTRMEMFUNC:
	fix_dnttpointer(dntp->dptrmem.pointsto);
	fix_dnttpointer(dntp->dptrmem.memtype);
	break;
	
    case K_INHERITANCE:
	fix_dnttpointer(dntp->dinheritance.classname);
	fix_dnttpointer(dntp->dinheritance.next);
	break;
	
    case K_OBJECT_ID:
	fix_dnttpointer(dntp->dobject_id.next);
	fix_stattype(
		     &dnt.dobject_id.object_ident,
		     &dnt.dobject_id.segoffset,
		     (symbol *) dnt.dobject_id.object_ident
		    );
	break;

#ifdef TEMPLATES
	
    case K_TEMPLATE:
	fix_dnttpointer(dntp->dtemplate.memberlist);
	fix_dnttpointer(dntp->dtemplate.parentlist);
	fix_dnttpointer(dntp->dtemplate.identlist);
	fix_dnttpointer(dntp->dtemplate.friendlist);
	fix_dnttpointer(dntp->dtemplate.arglist);
	fix_dnttpointer(dntp->dtemplate.expansions);
	break;
	
    case K_TEMPL_ARG:
	fix_dnttpointer(dntp->dtempl_arg.type);
	fix_dnttpointer(dntp->dtempl_arg.nextarg);
	break;
	
    case K_FUNC_TEMPLATE:
	fix_dnttpointer(dntp->dfunctempl.firstparam);
	fix_dnttpointer(dntp->dfunctempl.retval);
	fix_dnttpointer(dntp->dfunctempl.arglist);
	break;
	
    case K_LINK:
	fix_dnttpointer(dntp->dlink.ptr1);
	fix_dnttpointer(dntp->dlink.ptr2);
	break;
#endif

#endif
	
    case K_XREF:
	/* If the "extra" field (usually zero), is "-1", we know   */
	/* that the "xreflist" field contains a symbol table ptr.  */
	/* ------------------------------------------------------- */
	if (dntp->dxref.extra == -1)
	{
	    sym = (symbol *) dntp->dxref.xreflist;
	    
	    if ((sym->stype&STYPE)==SUNDEF)
		uerror("xt symbol (%s) for dnt_xref never resolved",sym->snamep);
	    
	    if ((sym->stype&STYPE)!=SXT)
		uerror("symbol (%s) for dnt_xref not of type XT",sym->snamep);
	    
	    dntp->dxref.xreflist = sym->svalue;
	}
	break;
    }
}

/****************************************************************************\
 *  fix_xrefentry(xrefp, ifd)
 *
 *  The only work to do is to resolve LINK "next" addresses that were
 *  specified as labels.
 *
\****************************************************************************/

fix_xrefentry(xrefp, ifd)
  register union xrefentry  *xrefp;
  register FILE		    *ifd;
{ 
    register symbol * sym;
    struct XREFLINK link;
    
    switch (xrefp->xlink.tag)
    {
    case XINFO1:
    case XINFO2:
    case XNAME:     
	/* nothing to do */
	break;
	
    case XLINK:
	/* We stuffed all 1's into the "next" field (zero is used for */
	/* EOL conditions) if we had a label as the "next" address.   */
	/* We then dumped a ptr to the symbolentry into the temp file.*/
	/* ---------------------------------------------------------- */
	link.next = ~xrefp->xlink.next;
	if (link.next == 0)
	{
	    if ((sym = (symbol *)getw(ifd)) == (symbol *)EOF)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("EOF looking for symbol table ptr after XREFLINK entry");

	    if ((sym->stype&STYPE)==SUNDEF)
		uerror("xt symbol (%s) never resolved",sym->snamep);
	    
	    if ((sym->stype&STYPE)!=SXT)
		uerror("symbol (%s) not of type XT",sym->snamep);
	    
	    xrefp->xlink.next = sym->svalue;
	}
	break;
    }
}

/****************************************************************************\
 * fix_address(addrp)
 *
 * This is called to fix up an text address reference in a dntt entry.
 * All such references are symbol table (label) references.
 *
\****************************************************************************/

fix_address(addrp) 
    ADDRESS * addrp;

{ 
    symbol * sym;
    
    sym = (symbol *) * addrp;
    if ((sym->stype&STYPE) != STEXT)
	uerror("dntt address symbol (%s) must refer to TEXT", sym->snamep);
    * addrp = sym->svalue;
}


/****************************************************************************\
 * dump_debug_name_table(ifd, segsize, stype)
 *
 * Need to do fixup for dnttpointer's and statyptes that were symbolic
 * This routine is used to dump the LNTT and GNTT                     
 *
\****************************************************************************/

dump_debug_name_table(ifd, segsize, stype)
    FILE 	  *ifd;
    long 	  segsize;
    unsigned long stype;
        
{   register int segdot = 0;

    (void) rewind(ifd);

    while (fread ( &dnt, sizeof(union dnttentry) , 1, ifd) == 1) {
	if ((line = getw(ifd)) == EOF)	/* line info for error messages */
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("EOF getting line number following a gntt/lntt entry");
	fix_dnttentry(&dnt);
	segdot += fwrite(&dnt, DNTTBLOCKSIZE, DNTTSIZE[dnt.dblock.kind], fdout);
    }

    if (segdot != segsize)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	if (stype == SLNTT)
	    aerror("lntt segment size error in pass2");
        else
	    aerror("gntt segment size error in pass2");
}

/****************************************************************************\
 * dump_cross_reference_table(ifd, segsize)
 *
 * Just copy the XT table to the output file.  
 *
\****************************************************************************/

dump_cross_reference_table(ifd, segsize)
    FILE *ifd;
    long segsize;

{ 
    register int  w;
    register int  segdot = 0;

    (void) rewind(ifd);

    while( fread( &xref, sizeof(union xrefentry), 1, ifd) == 1 )
    {
	if (xref.xlink.tag == XLINK)
	    fix_xrefentry(&xref, ifd);

	segdot += fwrite(&xref, XTBLOCKSIZE, 1, fdout);

	/* If its and "info2A", the next thing in the file MUST */
	/* be and "info2B" --- these do not have "tag" fields!  */
	/* ---------------------------------------------------- */
        if (xref.xlink.tag == XINFO2)
        {
	    if ((w=getw(ifd)) == EOF) 
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("EOF looking for xref_info2B following an info2A");
	    
	    putw(w,fdout);
	    segdot++;
	}
    }
    
    if (segdot != segsize)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("xt segment size error in pass2");
}

/****************************************************************************\
 * dump_source_line_table(ifd, segsize)
 *
 * Read through the temp SLT file.  "normal" entries may need readjustment
 * if code was moved due to sd-optimiztion.  "special" entries need their
 * dntt pointers fixed up.  Copy the records to the output file.
 *
\****************************************************************************/

dump_source_line_table(ifd, segsize)
    FILE *ifd;
    long segsize;
{ 
    int 	segdot = 0;
    int 	sdi_delta;
    struct sdi  *psdi;

    (void) rewind(ifd);

    while ( fread(&slt, SLTBLOCKSIZE, 1, ifd)==1 ) 
    {
	if ((line = getw(ifd)) == EOF)	/* line info for error messages */
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("EOF getting line number following a slt entry");
	
	if ((slt.sspec.sltdesc == SLT_NORMAL) ||
	    (slt.sspec.sltdesc == SLT_EXIT) )
	{
	    
# ifdef SDOPT
	    /* if doing sd-optimization need to fixup the */
	    /* offset on every sltnormal.		  */
	    /* ------------------------------------------ */
	    if (sdopt_flag) 
	    {
		if ((psdi = (struct sdi *) getw(fdslt)) == (struct sdi *)EOF)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		    aerror("EOF looking for psdi struct after slt entry");
		
		sdi_delta = sdi_dotadjust(psdi,slt.snorm.address);
		slt.snorm.address += sdi_delta;
# ifdef SDDEBUG
		printf("adjusting sltnorm.address for %d from %d to %d\n",
		       slt.snorm.line, slt.snorm.address-sdi_delta,
		       slt.snorm.address);
# endif
	    }
# endif /* SDOPT */
	}
	
        else if (slt.sspec.sltdesc == SLT_ASSIST)
	    /* nothing to do, the address is an SLT index */;
		
        else if (slt.sspec.sltdesc == SLT_MARKER)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("unsupported SLT type MARKER found in slt table");
	
	else /* some other kind of "special" slt item */
	{
	    fix_dnttpointer_func(&slt.sspec.backptr);
	}

	segdot += fwrite(&slt, SLTBLOCKSIZE, 1, fdout);
    }
    
    if (segdot != segsize)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("slt segment size error in pass2");
}


/****************************************************************************\
 * dump_value_table(ifd, vtsiz)
 *
 * The value table was build by the compiler.  It was passed in one
 * of two ways:
 *   1) The file name was passed to us in a pseudo-op.  Just copy the file to 
 *      the object file.
 *   2) 'vtbytes' pseudo-ops were used to pass the contents of the file.
 *      We have written it to a tempfile in this case.
 *
\****************************************************************************/

dump_value_table(ifd, vtsiz)
    FILE	*ifd;
    long	*vtsiz;
        
{ 
    register char *vtfilename;
    register int  c;
    register long segdot= 0;
    
    if (!have_vt_data)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("required 'vt' or 'vtfile' pseduo-op never encountered.");
    
    rewind(ifd);   /* Rewind in case we wrote the tempfile ourselves */
    
    while((c=getc(ifd)) != EOF) 
    {
	segdot++;
	putc(c, fdout);
    }
	

    if (have_vt_data == VIA_VT_TEMPFILE)
	*vtsiz = segdot;
    else
	if (segdot != *vtsiz)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("vt segment size error in pass2");
    
    fclose(ifd);
}

/***************************************************************************\
 *  generate_vtbyte(c)
 *
 * This is called when we see a "vtbyte xx,xx,xx" pseudo op. 
 * It is called once for each piece of data given.             
 *
\****************************************************************************/

generate_vtbyte(c) 
    int c;
{ 
    if (have_vt_data == VIA_VT_TEMPFILE)
    {
	uerror("'vtbytes' not valid with 'vtfile' pseudo-op -- ignored");
	return ;
    }
    
    if (dot->stype != SVT)
	uerror("'vtbytes' only legal within VT");
    
    if (!have_vt_data) 
    {
	if ((fdvt = fopen(filenames[6], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("Unable to open temporary (svt) file");
	unlink(filenames[6]);
	have_vt_data = VIA_VT_BYTES;
    }

    if (putc(c, fdvt) == EOF)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("unable to write temp (vt) file");

    newdot++;
}


/****************************************************************************\
 * open_vt_temp_file(name)
 *
 * This routine is called when the grammer recognizes the "vtfile" 
 * pseudo-op.  We open the file and unlink it so it goes away if
 * we die.
 *
\****************************************************************************/

open_vt_temp_file(name)
    char *name;
{
    if (have_vt_data == VIA_VT_TEMPFILE)
    {
	uerror("Only one 'vtfile' pseudo-op is allowed");
	return;
    }
    
    if (have_vt_data == VIA_VT_BYTES)
    {
	uerror("'vtfile' not valid with previous 'vt' or 'vtbytes'");
	return;
    }
    
    filenames[6] = (char *) malloc( strlen(name) + 1 );
    
    if (!filenames[6])
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("could not malloc space for vt file name");
    
    strcpy(filenames[6], name);
    
    if ((fdvt = fopen(name, "r")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open temporary (vt) file %s",name);

    unlink(name);
    have_vt_data = VIA_VT_TEMPFILE;
}

/*****************************************************************************\
 * open_debug_tmp_files()
 * 
 * open intermediate files for gntt, lntt, slt, and xt section.
 * This routine is called the first time a debug pseduo-op is encountered.
 * set the "gfiles_open" flag, to prevent further calls.
 *
\*****************************************************************************/

open_debug_tmp_files()
{
    gfiles_open = 1;
    
    if ((fdgntt = fopen(filenames[4], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open temporary (gntt) file");
    
    if ((fdslt = fopen(filenames[5], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open temporary (slt) file");
    
    if ((fdlntt = fopen(filenames[10], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open temporary (lntt) file");
    
    if ((fdxt = fopen(filenames[11], "w+")) == NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("Unable to open temporary (xt) file");
    
    unlink(filenames[4]);
    unlink(filenames[5]);
    unlink(filenames[10]);
    unlink(filenames[11]);
}
