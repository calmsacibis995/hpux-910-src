/* @(#) $Revision: 66.1 $ */   
#ifdef NLS
#define NL_SETN 12
#include        <msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif

# include	"../../hdr/defines.h"

/*
	Routine to write out either the current line in the packet
	(if newline is zero) or the line specified by newline.
	A line is actually written (and the x-file is only
	opened) if pkt->p_upd is non-zero.  When the current line from 
	the packet is written, pkt->p_wrttn is set non-zero, and
	further attempts to write it are ignored.  When a line is
	read into the packet, pkt->p_wrttn must be turned off.
*/

int	Xcreate;
FILE	*Xiop;


putline(pkt,newline)
register struct packet *pkt;
char *newline;
{
	static char obf[BUFSIZ];
	char *xf, *auxf();
	register char *p;
	FILE *fdfopen();

	if(pkt->p_upd == 0) return;

	if(!Xcreate) {
		stat(pkt->p_file,&Statbuf);
		xf = auxf(pkt->p_file,'x');
		Xiop = xfcreat(xf,Statbuf.st_mode);
		setbuf(Xiop,obf);
		chown(xf,Statbuf.st_uid,Statbuf.st_gid);
	}
	if (newline)
		p = newline;
	else {
		if(!pkt->p_wrttn++)
			p = pkt->p_line;
		else
			p = 0;
	}
	if (p) {
		if(fputs(p,Xiop)==EOF)
			FAILPUT;
		if (Xcreate)
			while (*p)
				pkt->p_nhash += *p++;
	}
	Xcreate = 1;
}


flushline(pkt,stats)
register struct packet *pkt;
register struct stats *stats;
{
	register char *p;
	char ins[6], del[6], unc[6], hash[6];

	if (pkt->p_upd == 0)
		return;
	putline(pkt,(char *) 0);
	rewind(Xiop);

	if (stats) {
		sprintf(ins,"%.05u",stats->s_ins);
		sprintf(del,"%.05u",stats->s_del);
		sprintf(unc,"%.05u",stats->s_unc);
		for (p = ins; *p; p++)
			pkt->p_nhash += (*p - '0');
		for (p = del; *p; p++)
			pkt->p_nhash += (*p - '0');
		for (p = unc; *p; p++)
			pkt->p_nhash += (*p - '0');
		/*
		** Don't allow the file under control to grow beyond 99999 lines.
		*/
		if ((stats->s_ins - stats->s_del + stats->s_unc) > 99999) {
		    sprintf(Error,"%s (co27)",(nl_msg(221,"file longer than 99999 lines")));
		    fatal(Error);
		}
	}

	sprintf(hash,"%5u",pkt->p_nhash&0xFFFF);
	zeropad(hash);
	/*
	** Add checks on fprintf and fclose to report error if
	**  the print or close did not succeed because the final
	**  flush of the file may cause some failure.
	*/
	if (
	    fprintf(Xiop,"%c%c%s\n",CTLCHAR,HEAD,hash) < 0 
	   )
	  {
	    xmsg( pkt->p_file, "flushline" );
	    exit(1);
	  }
	if (stats)
	  if (
	      fprintf(Xiop,"%c%c %s/%s/%s\n",CTLCHAR,STATS,ins,del,unc) < 0
	     )
	    {
	      xmsg( pkt->p_file, "flushline" );
	      exit(1);
	    }
	if ( fclose(Xiop) != 0 )
	  {
	    xmsg( pkt->p_file, "flushline" );	    
	    exit(1);
	  }
}


xrm(pkt)
struct packet *pkt;
{

  /*
  **  No error checking on this fclose because a this final
  **   close is done as part of the cleanup and so should be
  **   complete.
  */
  if (Xiop)
    fclose(Xiop);
  Xiop = 0;
  Xcreate = 0;
}
