/* @(#) $Revision: 70.1 $ */   

# include <stdio.h>
# include "pass0.h"

# include "symbols.h"
# include "adrmode.h"

extern long newdot;		/* up-to-date value of "." */
extern symbol *dot;

extern short ofile_open;	/* will be 1 if .o file opened */
extern short rflag;
extern short listflag;

extern FILE *fdtext, *fddata, *fdgntt, *fdlntt, *fdslt, *fdvt, *fdxt;
extern FILE *fdtextfixup, *fddatafixup;
extern FILE *fdcsect, *fdcsectfixup;
   
long	dottxt = 0L;
long	dotdat = 0L;
long	dotbss = 0L;
long	dotgntt = 0L;
long	dotlntt = 0L;
long	dotslt = 0L;
long	dotvt = 0L;
long	dotxt = 0L;

long	fixtxt = 0;
long	fixdat = 0;
long	fixsz = 0;

#ifdef LISTER2
extern long listdot;
#endif

cgsect(newtype)
	register short newtype;

{
	/* save the location counter for the current segment in the
	 * appropriate dotxxx
	 */
	switch(dot->stype){
		default:
#ifdef  BBA
#pragma BBA_IGNORE
#endif
			aerror("bad stype in cgsect");
			break;

		case STEXT:{
			dottxt = newdot;
			fixtxt = fixsz;
			break;
			}
		case SDATA:{
			dotdat = newdot;
			fixdat = fixsz;
			break;
			}
		case SBSS:{
			dotbss = newdot;
			break;
			}
		case SGNTT:{
			dotgntt = newdot;
			break;
			}
		case SLNTT:{
			dotlntt = newdot;
			break;
			}
		case SSLT:{		  /* Never gets called. SLT is put */
			dotslt = newdot;  /* directly into the TEXT space  */
			break;
			}
		case SVT:{                /* Never gets called.  VT is now */
			dotvt = newdot;   /* passed as a complete file.    */
			break;
		    }
		case SXT:{
			dotxt = newdot;
			break;
			}
		}

	/* Setup the new segment and set dot appropriately */
	dot->stype = newtype;
	fdcsect = NULL;
	switch(newtype){
		case STEXT:{
			newdot = dottxt;
			fixsz = fixtxt;
			fdcsect = fdtext;
			fdcsectfixup = fdtextfixup;
			break;
			}
		case SDATA:{
			newdot = dotdat;
			fixsz = fixdat;
			fdcsect = fddata;
			fdcsectfixup = fddatafixup;
			break;
			}
		case SBSS:{
			newdot = dotbss;
			break;
			}
		case SGNTT:{
			newdot = dotgntt;
			break;
			}
		case SLNTT:{
			newdot = dotlntt;
			break;
			}
		case SSLT:{               /* Never gets called. SLT is put */
			newdot = dotslt;  /* directly into the TEXT space  */
			break;
			}
		case SVT:{                /* Never gets called.  VT is now */
			newdot = dotvt;	  /* passed as a complete file.    */
			break;
		    }
		case SXT:{
			newdot = dotxt;
			break;
			}
		}
	dot->svalue = newdot;
#ifdef LISTER2
	listdot = newdot;
#endif
}

onintr() {
  unlink_tfiles();
  unlink_ofile();
  EXIT(1);
}


fpe_catch(sig,code,scp) 
  int sig, code;
  struct sigcontext *scp;
{
#ifdef  BBA
#pragma BBA_IGNORE
#endif
  if (code==5)
	aerror("division by 0");
  else
	aerror("floating point exception");
}



extern char * filenames[];

/* Unlink temporary files.
 * Currently only called for error conditions.
 */

unlink_tfiles() {
  int n;
  if (rflag) unlink(filenames[0]);

  /* unlink the temporary files in filenames[2]..filenames[MAXTMPF+1] */
  for (n=2; n<=MAXTMPF+1; n++) {
  	if (filenames[n]) unlink(filenames[n]);
	}
}


unlink_ofile() {
  if (ofile_open)
	unlink(filenames[1]);
}

/* null out unlink so can look at temp files when debugging */
# ifdef NOUNLINK
unlink() {
}
#endif

