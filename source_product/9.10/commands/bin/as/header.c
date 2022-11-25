/* @(#) $Revision: 70.1 $ */     

# include <stdio.h>
# ifdef SEQ68K
# include "seq.a.out.h"
# include "exthdr.h"
# else
# include <a.out.h>
# endif
# include "header.h"

#ifdef xcomp300_800
# include "symtab.h"
#else
/* This comes from /usr/contrib/include (at the moment).  There is an */
/* "-I/usr/contrib/include" flag in the makefile so it can be found.  */
# include <symtab.h>
#endif

#ifdef PIC
extern long drelocs;
int         highwater = 0;
int         highwater_seen = 0;
extern short pic_flag;
extern short shlib_flag;
#else
long 	    drelocs = 0L;
int         highwater = 0;
#endif

extern int gfiles_open;

/*****************************************************************************
 * header
 *	Fill in the entries of the global struct exec_header based on the
 *	values of global variables txtsiz, etc.
 *	If the "wrtflag" parameter is nonzero, then write the structure
 *	to the output file "fdout".
 */


/* PORTABILITY NOTE:  For a cross assembler, may have to define a local
 * file to replace the s300 a.out.h, so that structure sizes and alignments
 * come out correct on the host machine.
 */

/* Gobal Variables for header records */
/* ---------------------------------- */
struct header_extension	debug_ext_header;	/* Debug extension hdr */
#ifdef SEQ68K
struct seqhdr            exec_header;	      	/* a.out header */
struct seqhdr *filhdr = &exec_header;
#else
struct exec              exec_header;	      	/* a.out header */
#endif

# define dbghdr(f) debug_ext_header.e_spec.debug_header.f
# define DEBUG_HEADER_VERSION 0
    

/* size of a relocation record */
# ifdef SEQ68K
# define RELSZ sizeof (struct reloc)
# else
# define RELSZ sizeof (struct r_info)
# endif

/* Assembler's global variables for the size of all the various
 * sections of an a.out file.
 */
extern long txtsiz, datsiz, bsssiz, lstsiz, lstent, supsize, 
	    modsiz, txtrel, datrel;

extern long gnttsiz, lnttsiz, sltsiz, vtsiz, xtsiz;
extern long debug_ext_offset;

extern FILE * fdout;
long    debug_ext_offset = 0L;

/******************************************************************************/

header(wrtflag) {
#ifdef SEQ68K
	exec_header.fmagic = OFT_HP_LINKER_INPUT;
	exec_header.tsize = txtsiz;
	exec_header.dsize = datsiz;
	exec_header.bsize = bsssiz;
	exec_header.ssize = lstsiz;
	exec_header.rtsize = txtrel * RELSZ;
	exec_header.rdsize = datrel * RELSZ;
	exec_header.entry = 0;
	exec_header.dstart = 0;
	exec_header.priv_flag = 0;

	/* Debug extension header will IMMEDIATELY follow the */
	/* data relocation area in the file. 4-byte align for */
	/* easy debugging of data.                            */
	/* -------------------------------------------------- */
	if (gfiles_open)
	{
	    debug_ext_offset = RDATA_PPOS + exec_header.rdsize;
	    if (3 & debug_ext_offset) 
		debug_ext_offset = (debug_ext_offset & ~3) + 4;
	}

	exec_header.hp_debug = debug_ext_offset;
#else
	exec_header.a_magic.file_type = RELOC_MAGIC;
	exec_header.a_magic.system_id = HP9000S200_ID;
	exec_header.a_stamp = version;
	exec_header.a_highwater = highwater;
	exec_header.a_miscinfo = M_VALID;
	if (version == VER_20_FLT) {
	   exec_header.a_miscinfo |= M_FP_SAVE;
	}
	if (gfiles_open) {
	   exec_header.a_miscinfo |= M_DEBUG;
	}
#ifdef PIC
	if (pic_flag) {
	   exec_header.a_miscinfo |= M_PIC;
	}
	if (shlib_flag) {
	   exec_header.a_miscinfo |= M_DL;
	}
#endif
	exec_header.a_text = txtsiz;
	exec_header.a_data = datsiz;
	exec_header.a_bss = bsssiz;
	exec_header.a_trsize = txtrel * RELSZ;
	exec_header.a_drsize = datrel * RELSZ;
	exec_header.a_pasint = modsiz;
	exec_header.a_lesyms = lstsiz;
	exec_header.a_spared = 0L;       /* Not used */
	exec_header.a_entry = 0L;
	exec_header.a_spares = 0L;	   /* Not used */
	exec_header.a_supsym = supsize;
	exec_header.a_drelocs = drelocs;

	/* Debug extension header will IMMEDIATELY follow the */
	/* data relocation area in the file. 4-byte align for */
	/* easy debugging of data.                            */
	/* -------------------------------------------------- */
	if (gfiles_open)
	{
	    debug_ext_offset = RDATA_OFFSET(exec_header) + exec_header.a_drsize;
	    if (3 & debug_ext_offset) 
		debug_ext_offset = (debug_ext_offset & ~3) + 4;
	}

	exec_header.a_extension = debug_ext_offset;   /* No Shared lib hdr yet */

#endif

	if (wrtflag) {
	   if ( fseek(fdout, 0L, 0) != 0 )
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("seek error in header");
#ifdef SEQ68K
	   if ( fwrite(&exec_header, sizeof(struct seqhdr), 1, fdout) < 1)
#else
	   if ( fwrite(&exec_header, sizeof(struct exec), 1, fdout) < 1)
#endif
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("write error in header");
	   }
}

/*****************************************************************************
 * fill_debug_ext_header(wrtflag)
 *
 */

fill_debug_ext_header(wrtflag)
    int wrtflag;
{
    debug_ext_header.e_header    = DEBUG_HEADER;
    debug_ext_header.e_version   = DEBUG_HEADER_VERSION;
    debug_ext_header.e_size	 = 0;
    debug_ext_header.e_extension = 0;

    /* This assumes we are the first extension header in the file. */
    /* This is true at the moment, but not after the linker adds   */
    /* shared library stuff headers.                               */
    /* ----------------------------------------------------------- */
#ifdef SEQ68K
    dbghdr(header_offset) = exec_header.hp_debug + sizeof(debug_ext_header);
#else
    dbghdr(header_offset) = EXT_OFFSET(exec_header) + sizeof(debug_ext_header);
#endif
    dbghdr(header_size)	  = sizeof(struct XDB_header);
    dbghdr(gntt_offset)	  = dbghdr(header_offset) + dbghdr(header_size);
    dbghdr(gntt_size)	  = gnttsiz * DNTTBLOCKSIZE;    
    dbghdr(lntt_offset)	  = dbghdr(gntt_offset) + dbghdr(gntt_size);
    dbghdr(lntt_size)	  = lnttsiz * DNTTBLOCKSIZE;    
    dbghdr(slt_offset)	  = dbghdr(lntt_offset) + dbghdr(lntt_size); 
    dbghdr(slt_size)	  = sltsiz * SLTBLOCKSIZE;     
    dbghdr(vt_offset)	  = dbghdr(slt_offset) + dbghdr(slt_size);
    dbghdr(vt_size)	  = vtsiz;      
    dbghdr(xt_offset)	  = dbghdr(vt_offset) + dbghdr(vt_size);
    dbghdr(xt_size)	  = xtsiz * XTBLOCKSIZE;      
    dbghdr(spare1)	  = 0L;       

    if (wrtflag)
    {
#ifdef SEQ68K
	if ( fseek(fdout, exec_header.hp_debug, 0) != 0 )
#else
	if ( fseek(fdout, EXT_OFFSET(exec_header), 0) != 0 )
#endif
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("seek error seeking to debug extension header");

	if ( fwrite(&debug_ext_header, sizeof(struct header_extension), 1, fdout) < 1)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	    aerror("write error writing debugger extension header");
    }
}

/*****************************************************************************
 * write_XDB_header()
 *
 * WARNING:  write_debug_ext_header() MUST be called first!
 */

write_XDB_header()
{
    struct XDB_header	xdb_header;
    
    xdb_header.gntt_length = gnttsiz * DNTTBLOCKSIZE;
    xdb_header.lntt_length = lnttsiz * DNTTBLOCKSIZE;
    xdb_header.slt_length  = sltsiz  * SLTBLOCKSIZE;
    xdb_header.vt_length   = vtsiz;
    xdb_header.xt_length   = xtsiz   * XTBLOCKSIZE;
    
    if ( fseek(fdout, dbghdr(header_offset), 0) != 0 )
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("seek error seeking to debugger XDB header");

    if ( fwrite(&xdb_header, sizeof(struct XDB_header), 1, fdout) < 1)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("write error writing debugger XDB header");
}

/*****************************************************************************
 * fix_version

 * Fix the a_stamp field in the a.out header.  The a_stamp field is being
 * used to distinguish pre- and post- 6.5 code;  the conventions for the
 * use of floating point registers changed at 6.5, and we are trying to
 * flag potential mismatched of .o files.
 *
 * If a -V<num> option was used, or a version pseudo-op was used, use
 * that version number (without question);  otherwise set a default.
 */


unsigned short 	version;	/* value for the a_stamp field */
short	version_seen = 0;	/* Initially 0; set to 1 when a version
				 * pseudo-op is seen.
				 */
short	vcmd_seen = 0;		/* Set to 1 when a -V option is seen. */
short	floatop_seen;		/* Initially 0; set to 1 when any
				 * dragon or 68881 op is used.
				 */



#ifndef SEQ68K
fix_version()  {

   if (!(version_seen||vcmd_seen)) {
#ifdef M68020
	if (floatop_seen) {
	   werror("no version specified and floating point\n    ops present; version may not be properly set (see Assembler Reference\n    Manual)");
	   version = VER_OLD;	/* ?? or VER_20_NOFLT ?? 
					 * It's not clear what's best to
					 * put for a version here.  Maybe
					 * we need another code for "I don't
					 * know, so the linker can always
					 * warn. */
	   }
	else version = VER_20_NOFLT;
#endif
#ifdef M68010
	version = VER_10;
#endif
	}

   exec_header.a_stamp = version;

}
#endif


#ifdef PIC
ShlibVersion(month,year)
long month;
long year;
{
   if (highwater_seen) {
      werror("multiple shlib_version pseudo-op; previous value overwritten");
   }
   highwater_seen = 1;
   if ((month < 1) || (month > 12)) {
      werror("month outside range of 1 to 12 in shlib_version pseudo-op");
      month = 1;
   }
   if ((year > 99) && (year < 1990)) {
      werror("invalid year in shlib_version pseudo-op");
      year = 90;
   }
   if (year < 90) {
      year += 10;
   }
   else if (year >= 1990) {
      year -= 1990;
   }
   else {
      year -= 90;
   }
   highwater = 12 * year + month - 1;
}
#endif
