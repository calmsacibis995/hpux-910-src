/* @(#) $Revision: 70.1 $ */   

#ifdef LISTER2
/* Routines to support a post-pass2 lister generator.
 *
 * make_list_marker:	called during pass1 to generate a "list_marker"
 *			structure to the temporary file for listing info.
 *			Uses global variables: listdot, dot, newdot, lineno,
 *			sdi_listtail.
 *
 * lister:	called after the assembler's second pass to generate
 *		the listing.
 *		Uses global variables: sdi_listhead.
 *		Access files for input, list info, generated .o file
 */

# include <stdio.h>
# include "symbols.h"
# include "adrmode.h"
# include "sdopt.h"

/* extern declarations for global variables used */
extern long	listdot;
extern long	newdot;
extern long	line;
extern symbol * dot;
extern struct sdi *sdi_listtail, *sdi_listhead;
extern long	codestart;
extern long	txtsiz, datsiz;

extern FILE	*fdin, *fdout, *fdrel, *fdlistinfo;
extern char	*filenames[];


/* structure for the lister info temp file */
struct list_marker {
	short lm_stype;		/* what segment are we in */
	long  lm_dotval0;	/* current (start) location counter */
	long  lm_dotval1;	/* current (end) location counter */
	long  lm_lineno;	/* line number in the input file */
#ifdef SDOPT
	struct sdi * lm_sdi;	/* point to most recent sdi node */
#endif
	};

FILE *fdlist;
FILE	*fdtxtbytes, *fddatbytes, *fdsource;

struct list_marker lmarker;
int skip_next_list_marker = 0;	/* when this is != 0 skip the next
				 * call to make_list_marker.
				 * This is used to avoid double counting
				 * when the special bss maker call is
				 * used for lcomm's that are not in bss.
				 */

# define MAXCODEBYTES 24	/* Maximum code bytes to be emited per 
				 * statement.
				 */


/* make_list_marker: this routine is called to mark a source line.
 *		It is called at the end of a line.  It's called from
 *		the parser when processing the statement NL, and
 *		also from the scanner when a NL is escaped inside
 *		a string literal.
 */
make_list_marker() {
  /* fill in the list_marker from global variables, and then write
   * it to the list-info temp file.
   */
  register struct list_marker *lm = &lmarker;

  if (skip_next_list_marker) {
	skip_next_list_marker = 0;
	return;
	}
  lm->lm_stype = dot->stype;
  lm->lm_dotval0 = listdot;
  lm->lm_dotval1 = newdot;
  lm->lm_lineno = line;
# ifdef SDOPT
  lm->lm_sdi = (sdopt_flag && lm->lm_stype==STEXT) ? sdi_listtail : 0;
# endif
  if (fwrite(lm, sizeof(struct list_marker), 1, fdlistinfo) != 1)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("unable to write to temp (listinfo) file");

# ifdef LISTER_DEBUG
  printf("lister_mark: type <%d>, dot0 <%x>, dot1 <%x>, line <%d>\n",
	  lm->lm_stype, lm->lm_dotval0, lm->lm_dotval1, lm->lm_lineno);
# endif
}


make_list_marker_bss(loc0, loc1) {
  /* fill in the list_marker from global variables, and then write
   * it to the list-info temp file.
   */
  register struct list_marker *lm = &lmarker;
  lm->lm_stype = SBSS;
  lm->lm_dotval0 = loc0;
  lm->lm_dotval1 = loc1;
  lm->lm_lineno = line;
# ifdef SDOPT
  lm->lm_sdi = 0;
# endif
  if (fwrite(lm, sizeof(struct list_marker), 1, fdlistinfo) != 1)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("unable to write to temp (listinfo) file");
  skip_next_list_marker = 1;

# ifdef LISTER_DEBUG
  printf("lister_mark: type <%d>, dot0 <%x>, dot1 <%x>, line <%d>\n",
	  lm->lm_stype, lm->lm_dotval0, lm->lm_dotval1, lm->lm_lineno);
# endif
}


/* lister:	Use the list-info tempfile, together with the .s source file,
 *	the .o object file, and the sdi-info chain to produce a listing.
 */

char sourcebuf[256];

lister() {
  register struct list_marker *lm;
  register struct sdi *sdp;
  long txtoffs, datoffs; 
  register long *poffs;
  register long line;
  long bssoffs, gntoffs, lntoffs, vtoffs, xtoffs;
  long datastart;
  register FILE *fdbytes;
  long slinelen;
  long offset_adjust;	/* for BSS and DATA need to adjust offset values. */

  int n;
  long ncodebytes, ncbytesout;

  datastart = codestart + txtsiz;

  /* reset the .s, .o, and listinfo file pointers */
  /* we reuse file descriptors that are (now) left open, rather than
   * opening new files.
   * Probably cleaner code to just reopen the files here.
   */
  fdtxtbytes = fdout;
  fddatbytes = fdrel;
  fdsource = fdin;

  if (fseek(fdtxtbytes, codestart, 0)!=0 || fseek(fdsource, 0, 0)!=0
      || fseek(fddatbytes, datastart,0)!=0 || fseek(fdlistinfo, 0, 0)!= 0)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
	aerror("unable to reread files for listing");

  /* open the listing output file */
  if (filenames[19] != NULL) {
	if((fdlist = fopen(filenames[19],"w"))==NULL)
#ifdef  BBA
#pragma BBA_IGNORE
#endif
 	   aerror("unable to open listing file (%s)", filenames[19]);
	setvbuf(fdlist, NULL, _IOFBF, 8192);
	}
  else {
	fdlist = stdout;
	/* can we safely setvbuf here.  Only conflict (ie, already have
	 * written to stdout would be from debug prints (a don't care),
	 * ??? but also from error messages if user has done something
	 * like 2>&1 in the command line.
	 */
	}

  lm = &lmarker;
  txtoffs = 0;
  datoffs = 0;
  bssoffs = 0;
  vtoffs = lntoffs = gntoffs = xtoffs = 0; 
  line = 0;

  while (fread(lm, sizeof(*lm), 1, fdlistinfo)==1) {
# ifdef LISTER_DEBUG
	printf("lister_mark read: type <%d>, dot0 <%x>, dot1 <%x>, line <%d>\n",
	   lm->lm_stype, lm->lm_dotval0, lm->lm_dotval1, lm->lm_lineno);
# endif
	ncbytesout = 0;
	offset_adjust = 0;
	switch(lm->lm_stype) {
	   default:
	   case SSLT:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
		aerror("unknown lister marker type");
		break;

	   case STEXT:
#ifdef SDOPT
		if (sdopt_flag) {
			lm->lm_dotval0 += sdi_dotadjust(lm->lm_sdi,lm->lm_dotval0);
			lm->lm_dotval1 += sdi_dotadjust(lm->lm_sdi,lm->lm_dotval1);
# ifdef LISTER_DEBUG
		printf("lister locations updated to: dot0 <%x>, dot1 <%x>\n",
			lm->lm_dotval0, lm->lm_dotval1);
# endif  /* LISTER_DEBUG */
			}
#endif /* SDOPT */
		fdbytes = fdtxtbytes;
		poffs = &txtoffs;
		break;

	   case SDATA:
		offset_adjust = txtsiz;
		fdbytes = fddatbytes;
		poffs = &datoffs;
		break;

	   case SBSS:
		offset_adjust = txtsiz + datsiz;
		fdbytes = NULL;
		poffs = &bssoffs;
		break;

	    case SLNTT:  
		fdbytes = NULL;
		poffs = &lntoffs;
		break;

	   case SGNTT:
		fdbytes = NULL;
		poffs = &gntoffs;
		break;

	   case SVT:
		fdbytes = NULL;
		poffs = &vtoffs;
		break;

	   case SXT:
		fdbytes = NULL;
		poffs = &xtoffs;
		break;
	   }
	
	/* linenumber field */
	if (lm->lm_lineno > line)
	   fprintf(fdlist, "%5d\t", lm->lm_lineno);
	else
	   fprintf(fdlist, "\t");

	/* offset field */
	fprintf(fdlist, "%04X\t", lm->lm_dotval0+offset_adjust);

	/* Code bytes field
	 * Only dump bytes for STEXT and SDATA. (old assembler only dumped
	 * bytes * for instructions, nothing for data -- should we try to
	 * add enough info to handle that????)
	 *
	 * The bytes to dump will be [dotval0..dotval1-1] since dotval1
	 * represenets the dot-locator AFTER the bytes were generated.
	 */
	if ((lm->lm_dotval1 > lm->lm_dotval0) &&
	    (lm->lm_stype & (STEXT|SDATA))) {
	   /* skip over any code bytes between where we are in the .o file
	    * and the bytes we want to list.  Theoretically these are
	    * filler bytes, or bytes left over from a long previous line.
	    * We could getc these bytes, or use an fseek.
	    */
	   if (*poffs < lm->lm_dotval0) {
		fseek(fdbytes, lm->lm_dotval0 - *poffs, 1);
		*poffs = lm->lm_dotval0;
		}
	   ncodebytes = lm->lm_dotval1-lm->lm_dotval0;
	   ncbytesout = list_bytes(fdbytes, ncodebytes, 8);
	   }
	else {
	   /* output tabs past the code bytes field */
	   fprintf(fdlist, "\t\t\t");
	   ncodebytes = 0;
	   ncbytesout = 0;
	   }
	   

	if (lm->lm_lineno > line) {
	   /* put source line */
	   fgets(sourcebuf, 200, fdsource);
	   slinelen = strlen(sourcebuf);

	   /* if this read didn't get the entire source line, skip
	    * past the rest of the bytes on the line.
	    */
	   if (sourcebuf[slinelen-1] != '\n') {
		while(n=fgetc(fdsource)!='\n' && n!=EOF);
		}
	   /* plant a NULL on top the the '\n' left by fgets. */
	   sourcebuf[slinelen-1] = '\0';
	   fprintf(fdlist, "%s", sourcebuf);

	   }

	/* dump the output line */
	fputc('\n', fdlist);

	/* if the code bytes didn't fit on one line output additional lines
	 * with code bytes only up to the limit (currently 24 bytes).
	 */
	while ((ncbytesout < MAXCODEBYTES) && (ncbytesout<ncodebytes)) {
		fprintf(fdlist, "\t\t");
		ncbytesout += list_bytes(fdbytes, ncodebytes-ncbytesout, 8);
		fprintf(fdlist, "\n");
		}

	/* update current line and offset info.
	 * Only update *poffs if bytes were actually put out;  this is
	 * because the fseek to skip past unlisted bytes was only done
	 * above if dotval1-dotval0 > 0 .
	 */
	line = lm->lm_lineno;
	/* *poffs = lm->lm_dotval1; ?? */
	if (ncbytesout>0)
	    *poffs = lm->lm_dotval0 + ncbytesout;  /* only updates STEXT, 
				 * SDATA, but we don't care about the others.
				 */

	}
   fclose(fdlist);

}


long  list_bytes(fd, nbytes, nlimit)
  FILE * fd;
  long nbytes, nlimit;
{ register long n;
  register long limit;
  int ntab;

  limit = (nbytes<nlimit) ? nbytes : nlimit;

  for (n=0; n<limit; ) {
	fprintf(fdlist,"%02X", fgetc(fd)&0xff);
	if (++n % 2 == 0) fprintf(fdlist, " ");
	}
  /* put out tabs to adjust output */
  ntab = (nlimit -n+ 3) / 4 + 1;
  while (ntab-->0) fprintf(fdlist, "\t");

  return(n);
}
#endif  /* LISTER2 */
