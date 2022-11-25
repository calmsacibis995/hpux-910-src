/* @(#) $Revision: 37.2 $ */     
#ifdef NLS
#define NL_SETN 12
#include	<msgbuf.h>
#else
#define nl_msg(i, s) (s)
#endif
# include	"../../hdr/defines.h"

/*
	Does initialization for sccs files and packet.
*/

sinit(pkt,file,openflag)
register struct packet *pkt;
register char *file;
{
	extern	char	*satoi();
	register char *p;
	FILE *fdfopen();
	char *getline();

	zero(pkt,sizeof(*pkt));
	if (size(file) > FILESIZE)
	{
		sprintf(Error,"%s (co7)",nl_msg(211,"too long"));
		fatal(Error);
	}
	if (!sccsfile(file))
	{
		sprintf(Error,"%s (co1)",nl_msg(212,"not an SCCS file"));
		fatal(Error);
	}
	copy(file,pkt->p_file);
	pkt->p_wrttn = 1;
	pkt->do_chksum = 1;	/* turn on checksum check for getline */
	if (openflag) {
		pkt->p_iop = xfopen(file,0);
		setbuf(pkt->p_iop,pkt->p_buf);
		fstat(fileno(pkt->p_iop),&Statbuf);
		if (Statbuf.st_nlink > 1)
		{
			sprintf(Error,"%s (co3)",nl_msg(213,"more than one link"));
			fatal(Error);
		}
		if ((p = getline(pkt)) == NULL || *p++ != CTLCHAR || *p++ != HEAD) {
			fclose(pkt->p_iop);
			fmterr(pkt);
		}
		p = satoi(p,&pkt->p_ihash);
		if (*p != '\n')
			fmterr(pkt);
	}
	pkt->p_chash = 0;
}
