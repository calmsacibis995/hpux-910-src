
/* @(#) $Revision: 70.1 $ */     

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

     Command:      Series 200 ld   ( HP -UX link editor )
     File   :      deb.c

     Purpose:      update of symbolic debugger information

* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include "ld.defs.h"
#include <dnttsizes.h>

#if 0

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* fix_slts - for all slt special entries, add on the base for the dntt entry */

fix_slts(sloc)
{
	union sltentry silt;
	register int i;

	fseek(text, sloc+SLTPOS, 0);
	for(i=filhdr.a_sltsize/SLTBLOCKSIZE; i != 0; --i)
	{	dread(&silt, SLTBLOCKSIZE, 1, text);
		if (silt.sspec.special == 1)
			silt.sspec.backptr.word += dntt_base;
		else
			silt.snorm.address += torigin;
		fwrite(&silt, sizeof silt, 1, sltout);
	}
}

#endif

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
fix_dntts(sloc,globals)
long sloc;
long globals;
{
	union dnttentry dent;
	register long size, word, i;
	register unsigned char *ip;
	register long index;
	register long offset;
	FILE *rtext;

	rtext = fopen(filename,"r");
	if (globals) {
	   fseek(text, sloc+debugheader.e_spec.debug_header.gntt_offset, 0);
	   i = debugheader.e_spec.debug_header.gntt_size / DNTTBLOCKSIZE;
	}
	else {
	   fseek(text, sloc+debugheader.e_spec.debug_header.lntt_offset, 0);
	   i = debugheader.e_spec.debug_header.lntt_size / DNTTBLOCKSIZE;
	}
	while (i != 0) 
	{
		dread(&dent, DNTTBLOCKSIZE, 1, text);
		/* check for reasonable .kind */
		if( (dent.dblock.kind != K_NIL) &&
			(dent.dblock.kind >= (sizeof DNTTSIZE) / (sizeof DNTTSIZE[1])) )
#ifdef	BBA
#pragma BBA_IGNORE
#endif
			fatal("unknown debug information in %s",filename);

		if (dent.dblock.kind == K_NIL) {
		    size = 1;
                }
                else {
		    size = DNTTSIZE[dent.dblock.kind];
                }
		i -= size;
		if (size-1)
			dread(&dent.dgeneric.word[3], size-1, DNTTBLOCKSIZE, text);

		/* fix all fields of type address and STATTYPE */

		switch (dent.dblock.kind) {

		default:
#pragma		BBA_IGNORE
			break;

		case K_FUNCTION:
			dent.dfunc.lowaddr += torigin;
			dent.dfunc.hiaddr += torigin;

		case K_ENTRY:
			dent.dfunc.entryaddr += torigin;
			break;

		case K_CONST:
			if ((dent.dconst.locdesc == LOC_IMMED) ||
			    (dent.dconst.locdesc == LOC_VT)) {
			    break;
                        }
			/* locdesc must be LOC_PTR, fall through
			   and handle it like a SVAR              */

		case K_SVAR:
			if (dent.dsvar.location == -1)
			/* special case symbol--look it up in symbol table */
			{
				register int s;
				register int i,n;
				register char *cp;
				fseek(
				    rtext,
				    debugheader.e_spec.debug_header.vt_offset + 
				    sloc + dent.dsvar.name,
				    0
				);
				cp = csymbuf;
				while (*cp++ = getc(rtext))
#pragma		BBA_IGNORE_ALWAYS_EXECUTED
					if (feof(rtext))
#pragma		BBA_IGNORE
						fatal(e3,filename);
				cursym.sname = csymbuf;
				n = strlen(csymbuf);

				cursym.s.n_length = n;
				cursym.s.n_type = EXTERN;
				s = lookup();
				if (s == -1)
				/* try it with an underbar in front */
				{	for (i=n; i != 0; --i)
						csymbuf[i] = csymbuf[i-1];
					csymbuf[0] = '_';
					cursym.s.n_length += 1;
					s = lookup();
				}
				if (s != -1)
				{
				  if (rflag) dent.dsvar.location = (0x80000000 | s);
				  else dent.dsvar.location = symtab[s].s.n_value;
				}
				break;
			}

			if (dent.dsvar.location & 0x80000000)
			/* location is an index into local symbol table */
			{	index = dent.dsvar.location & 0x7fffffff;
#ifdef	XGFIX
				if (local[index] == NULL || (xflag && (local[index]->s.n_type & EXTERN) == 0))
#else
				if (xflag)
#endif
				{
					/* remember to warn user about
					   not using "-x" on a debuggable file
					 */
					warn_xg = 1;
					dent.dsvar.location = -1;
				}
				else if (index > last_local)
#ifdef	BBA
#pragma BBA_IGNORE
#endif
					error(e20,filename);
				else if (rflag) dent.dsvar.location = ( 0x80000000 | local[index]->sindex);
				else dent.dsvar.location = local[index]->s.n_value;
				break;
			}

			/* the MSB of location is zero => the location field
			   gives the segment and the displacement field holds
			   the offset of this object from the segment base. */
			switch (dent.dsvar.location)
			{
			   case TEXT: 
				 offset = dent.dsvar.displacement  + torigin;
				 break;
                           case DATA:
				 offset = dent.dsvar.displacement  + dorigin - filhdr.a_text;
				 break;
                           case BSS:
				 offset = dent.dsvar.displacement  + borigin - (filhdr.a_text + filhdr.a_data);
				 break;
                           case ABS:
				 offset = dent.dsvar.displacement;
				 break;
                           default:
#ifdef	BBA
#pragma BBA_IGNORE
#endif
				 error(e20, filename);
                        }
	         	if (rflag) dent.dsvar.displacement = offset;
	         	else 
			{
				if (align_seen)
					offset = align_inc(offset,dent.dsvar.location);
				dent.dsvar.location = offset;
			}
			break;


		case K_OBJECT_ID:
			if (dent.dobject_id.object_ident & 0x80000000)
			/* object_ident is an index into local symbol table */
			{
				index = dent.dobject_id.object_ident & 
					    0x7fffffff;

#ifdef	XGFIX
				if (local[index] == NULL || (xflag && (local[index]->s.n_type & EXTERN) == 0))
#else
				if (xflag)
#endif
				{
					/* remember to warn user about
					   not using "-x" on a debuggable file
					 */
					warn_xg = 1;
					dent.dobject_id.object_ident = -1;
				}
				else if (index > last_local) {
#ifdef	BBA
#pragma BBA_IGNORE
#endif
					error(e20,filename);
				}
				else if (rflag) {
					dent.dobject_id.object_ident =
					    (0x80000000 | local[index]->sindex);
				}
				else {
					dent.dobject_id.object_ident =
					    local[index]->s.n_value;
				}
			}
			else {
			/* the MSB of object_ident is zero => the object_ident 
			   field gives the segment and the segoffset field 
			   holds the offset of this object from the segment
			   base. 
			 */
			    switch (dent.dobject_id.object_ident)
			    {
			       case TEXT: 
				 offset = dent.dobject_id.segoffset + torigin;
				 break;

                               case DATA:
				 offset = dent.dobject_id.segoffset + dorigin
					  - filhdr.a_text;
				 break;

                               case BSS:
				 offset = dent.dobject_id.segoffset + borigin
					  - (filhdr.a_text + filhdr.a_data);
				 break;

                               case ABS:
				 offset = dent.dobject_id.segoffset;
				 break;

			                   case UNDEF:
				 break;

                               default:
#ifdef	BBA
#pragma BBA_IGNORE
#endif
				 error(e20, filename);
				 break;
                            }
	         	    if (rflag) 
			    {
				dent.dobject_id.segoffset = offset;
			    }
	         	    else 
			    {
				if (align_seen)
			        {
				    offset = align_inc(
						  offset,
						  dent.dobject_id.object_ident
						  );
			        }
				dent.dobject_id.object_ident = offset;
			    }
			}
			break;
		}

		if (globals) {
		   fwrite(&dent, size, DNTTBLOCKSIZE, gnttout);
		}
		else {
		   fwrite(&dent, size, DNTTBLOCKSIZE, lnttout);
		}
	}
	fclose (rtext);
}



