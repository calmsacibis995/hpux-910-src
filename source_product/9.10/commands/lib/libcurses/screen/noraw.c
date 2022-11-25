/* @(#) $Revision: 62.1 $ */   
#include "curses.ext"

noraw()
{
#ifdef USG
	(cur_term->Nttyb).c_cc[VINTR] = (cur_term->Ottyb).c_cc[VINTR];
	(cur_term->Nttyb).c_cc[VQUIT] = (cur_term->Ottyb).c_cc[VQUIT];
#ifdef NONLS
	(cur_term->Nttyb).c_iflag |= ISTRIP;
	(cur_term->Nttyb).c_cflag &= ~CSIZE;
	(cur_term->Nttyb).c_cflag |= CS7;
	(cur_term->Nttyb).c_cflag |= PARENB;
#endif
	nocrmode();
#else
	(cur_term->Nttyb).sg_flags&=~RAW;
# ifdef DEBUG
	if(outf) fprintf(outf, "noraw(), file %x, SP %x, flags %x\n", SP->term_file, SP, cur_term->Nttyb.sg_flags);
# endif
#endif
	SP->fl_rawmode=FALSE;
	reset_prog_mode();
}
