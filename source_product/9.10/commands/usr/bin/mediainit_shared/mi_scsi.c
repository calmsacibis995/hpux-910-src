/* HPUX_ID: @(#) $Revision: 70.7 $  */
/*
 * DISC3 mediainit
 */

/*
 * include files
 */
#ifdef _WSIO
#include <sys/sysmacros.h>
#else
#define _WSIO
#include <sys/sysmacros.h>
#undef _WSIO
#endif /* _WSIO */
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#ifdef _WSIO
#include <sys/scsi.h> 
#else
#define _WSIO
#include <sys/scsi.h> 
#undef _WSIO
#endif /* _WSIO */
#include <sys/diskio.h> 
#include <unistd.h>

/*
 * globals from the parser
 */
extern int verbose;
extern int recertify;
extern int guru;
extern int fmt_optn;
extern int interleave;
extern int debug;
extern int blkno;
extern int verify_pass;
extern int maj;
extern int minr;
extern int fd;
extern int unit;
extern int volume;
extern int iotype;
extern char *name;

#define DISK 0
#define TAPE 1
#define WORM 4
#define OPTICAL 7

/* HP Sony MO */
#define HP_MO(inq) (!strncmp((inq).inq1.vendor_id, "HP      ", 8) && \
                    !strncmp((inq).inq1.product_id, "S6300.650A", 10))

/*
 * other globals
 */
extern int errno;

/*
 * Disc3 (SCSI & NIOFL) driver ioctl interface (s800)
 * SCSI driver ioctl interface (s700)
 */

mi_scsi()
{
	int flag=1, ret;
        disk_describe_type describe;
	union inquiry_data inq;
	struct sioc_format data;

	/* Make sure that we have exclusive access to the device */
	if (iotype == IO_TYPE_WSIO) {
		verb("locking SCSI device");
	} else {
		verb("locking disc3 device");
	}
	if ((ret=ioctl(fd, DIOC_EXCLUSIVE, &flag))<0)	{
		perror("scsi_mi: Unable to set EXCLUSIVE mode");
		exit(1);
		}

        /* See if SCSI or NIOFL */
        if ((ret = ioctl(fd, DIOC_DESCRIBE, &describe))<0) {
                perror("dioc_describe: (describe) ioctl failed");
                exit(1);
                }
        if (describe.intf_type == SCSI_INTF) {

		/* Make sure we have a disc */
		if ((ret = ioctl(fd, SIOC_INQUIRY, &inq))<0)	{
			perror("scsi_inquiry: (inquiry) ioctl failed");
			exit(1);
			}

		if (inq.inq1.dev_type != DISK &&
				inq.inq1.dev_type != WORM &&
				inq.inq1.dev_type != OPTICAL)
			err(0, "Not a read/write Direct Access Device");

		if (iotype == IO_TYPE_WSIO)
			if (blkno != -1) {
				verb("sparing block 0x%x", blkno);
				if (mi_scsi_spare_blk(blkno) != 0)
					exit(1);
				exit(0);
			}

		verb("initializing media");
		if (iotype == IO_TYPE_SIO)
			ret = ioctl(fd, _SIO_SIOC_FORMAT, &interleave);
		else {
			data.fmt_optn = fmt_optn;
			data.interleave = interleave;
			ret = ioctl(fd, _WSIO_SIOC_FORMAT, &data);
		}

		if (ret < 0) {
			err(errno, "initialize media command failed");
			exit(1);
			}

		if (iotype == IO_TYPE_WSIO &&
				inq.inq1.dev_type == DISK &&
				!HP_MO(inq) &&
				verify_pass) {
			verb("verifying media");
			if (mi_scsi_verify(fd) < 0) {
				err(0, "verification of media failed");
				exit(1);
			}
		}
	} else {   /* must be NIOFL */

		if (iotype == IO_TYPE_SIO) {
			verb("initializing FL disk media");
			if ((ret=ioctl(fd, DIOC_FORMAT, &interleave))<0) {
				err(errno, "initialize media command failed");
				exit(1);
			}
		} else {
			verb("incorrect Interface-type per driver"); 
			perror("scsi_mi: describe returned incorrect intf_type");
			exit(1);
		}

	}
	exit(0);
}

#include <sys/stat.h>

#define	CHUNK_SIZE	128
#define	MAX_DLIST_SPAN	128

struct dlist {
	char		rsv[2];
	unsigned short	length;
	unsigned long	lba;
};

struct scsi_cmd {
	char		flags;		/* READ, WRITE, DISCONNECT */
	char		cmd_len;	/* 6, 10, 12 */
	char		*cmd;
	char		*buf;
	int		buf_len;
	int		timeout;	/* seconds */
	struct xsense	*sense;
};

#define	SCSI_CMD_NO_DISCON	0x01
#define	SCSI_CMD_READ		0x02
#define	SCSI_CMD_WRITE		0x04

/*
** Use the SCSI verify command to insure that all blocks on the disk are
** accessible.
*/
mi_scsi_verify(fd)
int	fd;
{
	struct capacity		cap;
	int			lba, nblks, on = 1;

	/*
	** get capacity info
	*/
	if (ioctl(fd, SIOC_CAPACITY, &cap) < 0) {
		perror("SIOC_CAPACITY");
		return(1);
	}

	for (lba = 0; lba < cap.lba; lba += nblks) {
		nblks = MIN(0x400, cap.lba - lba);
		if (mi_scsi_verify_chunk(lba, nblks) < 0)
			if (mi_scsi_fix_chunk(lba, nblks) < 0)
				return(-1); 
	}

	return(0);
}

mi_scsi_verify_chunk(lba, nblks)
int	lba;
int	nblks;
{
	int	i, ret;

	if (mi_scsi_fscsil(fd, SCSI_CMD_READ, &i, 4, 600, 10,
			0x2f, 0,
			lba >> 24, lba >> 16 & 0xff,
			lba >> 8 & 0xff, lba & 0xff,
			0, nblks >> 8 & 0xff, nblks & 0xff, 0) != 0)
		return(-1);

	return(0);
}

mi_scsi_fix_chunk(lba, nblks)
int	lba, nblks;
{
	int	i, size;

	if (nblks <= CHUNK_SIZE) {
		for (i = 0; i < nblks; i++)
			if (mi_scsi_verify_chunk(lba + i, 1) < 0)
				if (mi_scsi_spare_blk(lba + i) < 0)
					return(-1);
		if (mi_scsi_verify_chunk(lba, nblks) < 0) {
			fprintf(stderr, "mediainit: unable to fix disk\n");
			return(-1);
		}
	} else for (i = lba; i < lba + nblks; i += size) {
		size = MIN(CHUNK_SIZE, lba + nblks - i);
		if (mi_scsi_verify_chunk(i, size) < 0)
			if (mi_scsi_fix_chunk(i, size) < 0)
				return(-1);
	}

	return(0);
}

/*
** Due to the possibility of multiple bad blocks on the same track, it may
** be necessary to build a dlist with more than one entry.
*/
mi_scsi_spare_blk(lba)
int	lba;
{
	struct dlist	*dlist;
	unsigned long	*ddesc;

	/*
	** get space for the dlist
	*/
	if ((dlist = (struct dlist *)malloc(MAX_DLIST_SPAN + 2, 4)) == NULL) {
		perror("malloc");
		return(-1);
	}
	ddesc = &dlist->lba;

	*ddesc++ = lba;
	dlist->length = 4;

	while (mi_scsi_reassign_blocks(dlist) < 0) {

		/*
		** find the next bad block
		*/
		do {
			lba++;

			/*
			** A crude check to insure that we don't waste too much
			** time on a no-win situation.  MAX_DLIST_SPAN is
			** intended to be a value larger than the number of
			** sectors on any track in any device.
			*/
			if (lba - dlist->lba > MAX_DLIST_SPAN) {
			    fprintf(stderr,
				"mediainit: unable to spare bad block(s)\n");
			    return(-1);
			}

		} while (mi_scsi_verify_chunk(lba, 1) == 0) ;

		*ddesc++ = lba;
		dlist->length += 4;
	}

	if (dlist->length > 4)
		verb("reassigned track containing block 0x%x", lba);
	else
		verb("reassigned block 0x%x", lba);

	return(0);

}

mi_scsi_reassign_blocks(dlist)
struct dlist	*dlist;
{
	int	ret, len;

	len = dlist->length + 4;
	if (mi_scsi_fscsil(fd, SCSI_CMD_WRITE, dlist, len, 600, 6,
			0x07, 0, 0, 0, 0, 0) != len)
		return(-1);

	return(0);
}

/*
** mi_scsi_fscsiv is a stripped down version of what I hope will eventually
** be an ioctl in the scsi driver.
*/
mi_scsi_fscsiv(fd, scsi_cmd)
int		fd;
struct scsi_cmd	*scsi_cmd;
{
	int			cmd_mode_flag;
	int			lun, ret;
	struct stat		statbuf;
	struct scsi_cmd_parms	cmd_parms;

	cmd_parms.cmd_type = scsi_cmd->cmd_len;
	cmd_parms.cmd_mode = (scsi_cmd->flags & SCSI_CMD_NO_DISCON) ? 0 : 1;
	cmd_parms.clock_ticks = (scsi_cmd->timeout == 0)
				? 50 * 10 : 50 * scsi_cmd->timeout;
	memcpy(cmd_parms.command, scsi_cmd->cmd, scsi_cmd->cmd_len);

	/*
	** initialize the lun field of the command -- do all commands have
	** it in the same place?
	*/
	if (fstat(fd, &statbuf) < 0) {
		perror("fstat");
		return(-2);
	}
	lun = m_unit(statbuf.st_rdev);
	cmd_parms.command[1] &= 0x1f;		/* clear the lun bits */
	cmd_parms.command[1] |= lun << 5;	/* set the lun bits */

	cmd_mode_flag = 1;
	if (ioctl(fd, SIOC_CMD_MODE, &cmd_mode_flag) < 0) {
		perror("ioctl(SIOC_CMD_MODE)");
		return(-3);
	}
	if (ioctl(fd, SIOC_SET_CMD, &cmd_parms) < 0) {
		perror("ioctl(SIOC_SET_CMD)");
		return(-4);
	}

	/*
	** execute the command
	*/
	if (scsi_cmd->flags & SCSI_CMD_READ) {
		if ((ret = read(fd, scsi_cmd->buf, scsi_cmd->buf_len)) < 0)
			ret = -10;
	} else {
		if ((ret = write(fd, scsi_cmd->buf, scsi_cmd->buf_len)) < 0) {
			ret = -11;
		}
	}

	cmd_mode_flag = 0; ioctl(fd, SIOC_CMD_MODE, &cmd_mode_flag);

	return(ret);
}

/*
** mi_scsi_fscisl is a stripped down version of what I hope will eventually
** be part of a set of library routines which sit on top of fscsiv.
*/
/*VARARGS13*/	/* Keep lint from complaining */
mi_scsi_fscsil(fd, flags, buf, len, timeout, cmd_len,
			c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11)
int		fd;
int		flags;
char		*buf;
int		len;
int		timeout;
int		cmd_len;
char		c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11;
{
	char		cmd[12];
	struct scsi_cmd	scsi_cmd;

	scsi_cmd.flags = flags;
	scsi_cmd.cmd = cmd;
	scsi_cmd.cmd_len = cmd_len;
	scsi_cmd.buf = buf;
	scsi_cmd.buf_len = len;
	scsi_cmd.timeout = timeout;
	scsi_cmd.sense = 0;

	switch (cmd_len) {
	case 12:
		cmd[11] = c11; cmd[10] = c10;
	case 10:
		cmd[9] = c9; cmd[8] = c8; cmd[7] = c7; cmd[6] = c6;
	case 6:
		cmd[5] = c5; cmd[4] = c4; cmd[3] = c3; cmd[2] = c2;
		cmd[1] = c1; cmd[0] = c0;
		break;
	default:
		fprintf(stderr, "mi_scsi_fscsil: invalid cmd_len\n");
		return(-17);
	}

	return(mi_scsi_fscsiv(fd, &scsi_cmd));
}

/*
 * SCSI tape driver ioctl interface
 */

#define SELECT_DATA_SZ 0x16
unsigned char select_data[SELECT_DATA_SZ];

struct scsi_cmd_parms scsi_mode_select = {
	6, 1, 360000, /* 2 hour timeout */
	0x15, 0x10, 0x00, 0x00, SELECT_DATA_SZ, 0x00 
};

struct scsi_cmd_parms scsi_mode_sense = {
	6, 1, 3000, /* 1 minute timeout */
	0x1a, 0x00, 0x11, 0x00, SELECT_DATA_SZ, 0x00 
};


mi_scsitape(partition)
{
	int flag=1, pred, ret; 
	struct inquiry inq;
	char buf[SELECT_DATA_SZ];

	/* Put scsitape driver into special command mode */
	verb("locking SCSI device");
	if ((ret=ioctl(fd, SIOC_CMD_MODE, &flag))<0)	{
		perror("scsi_mi Unable to set CMD_MODE");
		exit(1);
		}

	/* Make sure we have a tape */
	if ((ret = ioctl(fd, SIOC_INQUIRY, &inq))<0)	{
		perror("scsi_inquiry: (inquiry) ioctl failed");
		exit(1);
		}
	if (inq.dev_type != TAPE)	
		err(0, "Not a Sequential Access Device");

	verb("initializing tape");
	if (partition < 0 || partition > 1200) {
		fprintf(stderr, "Sorry.  %d unacceptable partition size\n",
			partition);
		exit(1);
		}
	if (partition)
		pred = 0.115*partition + 5;
	else
		pred = 2;
	fprintf(stderr, "Please wait.  The initialisation will take approx. %d minutes\n", pred);

	if (ioctl(fd, SIOC_SET_CMD, &scsi_mode_sense) <0 ||
			(ret = read(fd, buf, SELECT_DATA_SZ)) < 0 ) {
		perror("error in ioctl SET CMD MODE: mode sense");
		exit(1);
		}

	select_data[2]  = 0x10;
	select_data[3]  = 0x08;
	select_data[12] = 0x11;
	select_data[13] = 0x08;
	select_data[14] = 0x01;
	select_data[15] = partition ? 0x01 : 0;
	select_data[16] = 0x30;
	select_data[17] = 0x03;
	select_data[20] = 0xff & partition >> 8;
	select_data[21] = 0xff & partition;

	if (ioctl(fd, SIOC_SET_CMD, &scsi_mode_select) <0 ||
			(ret = write(fd, select_data, SELECT_DATA_SZ)) < 0 ) {
		perror("error in ioctl SET CMD MODE: mode select");
		exit(1);
		}

	exit(0);
}
