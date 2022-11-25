/* HPUX_ID: @(#)conf.c	27.1     85/04/19  */
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>

#include "saio.h"

extern boot_errno;

devread(io)
	register struct iob *io;
{
	register int cc, bn;

	io->i_flgs |= F_RDDATA;
	io->i_error = 0;
	/*cc = (*devsw[io->i_ino.i_dev].dv_strategy)(io, READ);*/
	cc = 0x01000100;		/* device relative */
	bn = io->i_bn;
	if (io->i_flgs & F_REMOTE) {
		cc = 0;			/* file relative */
	} else {
		bn <<= (DEV_BSHIFT - 8);  /* mread: 256 byte sectors */
	}
	cc = call_bootrom(M_READ, bn, io->i_cc, io->i_ma, cc);
	io->i_flgs &= ~F_TYPEMASK;
	if (cc >= 0) cc = io->i_cc;
	return (cc);
}

/*
/*devwrite(io)
/*	register struct iob *io;
/*{
/*	int cc;
/*
/*	io->i_flgs |= F_WRDATA;
/*	io->i_error = 0;
/*	/*cc = (*devsw[io->i_ino.i_dev].dv_strategy)(io, WRITE);*/
/*	io->i_bn <<= (DEV_BSHIFT - 8);  /* mwrite expects 256 byte sectors */
/*	cc = -1;
/*	io->i_flgs &= ~F_TYPEMASK;
/*	return (cc);
/*}
 */

/*
/*devopen(io)
/*	register struct iob *io;
/*{
/*	/*(*devsw[io->i_ino.i_dev].dv_open)(io);*/
/*}
 */

/*devclose(io)
/*	register struct iob *io;
/*{
/*
/*	/*(*devsw[io->i_ino.i_dev].dv_close)(io);*/
/*}
 */
