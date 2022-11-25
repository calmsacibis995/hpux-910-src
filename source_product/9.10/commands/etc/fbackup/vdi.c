/* -*-C-*-
********************************************************************************
*
* File:         vdi.c
* RCS:          $Header: vdi.c,v 70.6.1.1 94/10/12 15:36:14 hmgr Exp $
* Description:  This routine contains the top level virtual device
*               interface functions.
* Author:       Dan Matheson, CSB R&D
* Created:      Mon Nov 26 10:39:52 1990
* Modified:     Wed Feb 20 13:19:06 1991 (Dan Matheson) danm@kheldar.BBN.HP.COM
* Language:     elec-C
* Package:      N/A
* Status:       Experimental (Do Not Distribute)
*
* (c) Copyright 1990, Hewlett-Packard Company, all rights reserved.
*
********************************************************************************
*/

#ifndef lint
 static char RCSid[]="$Header: vdi.c,v 70.6.1.1 94/10/12 15:36:14 hmgr Exp $";
#endif

#include "vdi.h"

#ifndef MT_IS3480
#define MT_IS3480       0x0B /* 3480 device */
#endif

int is3480=0;           /* flag is set to indicate 3480 device */
int is8mm;		/* flag to indicate 8MM device */

/* added the following three lines for EXABYTE 8mm tape drive support.
 * It should be removed when these definitions appear in sys/mtio.h
 */
#ifndef MT_ISEXABYTE
#define MT_ISEXABYTE            0x0a
#endif

/*
   The following three lines is added for 8.0 support.  It should be
   taken out after we stop support 8.0 release.
*/
#ifndef MT_ISQIC
#define MT_ISQIC	0x09
#endif

int qicflag=0;          /* flag to indicate QIC device */
extern int errno;

/* device identification stuff */

#define sec_nbr(x) 	(int)((unsigned)(x)&0xf)

static char *scsi_types[] =
{
    /* Temporary */ "SCSI"
};

static char *cs80_types[] =
{
    /* Free to convert BCD code (6 digits)*/ "      "
};

/*
 * Device types recognized by disk_info.
 */
#define CS80		0x000
#define SCSI		0x001
#define SCSI_TAPE	0x002
#define AUTOCH		0x003
#define ALINK		0x004
#define NIO		0x005
#define UNSUPP_MAJOR	-1

/*
 * Tables used for mapping major numbers to device types.
 */
struct major_map
{
    short major_num;	/* device major number */
    short dev_type;	/* type of device */
};

struct major_map s300_maj_map[] =
{
    {  4, CS80		},
    { 47, SCSI		},
    { 54, SCSI_TAPE	},
    { 55, AUTOCH	},
    { -1, UNSUPP_MAJOR	}
};

struct major_map s700_maj_map[] =
{
    { 47, SCSI		},
    { 54, SCSI_TAPE	},
    { 55, AUTOCH	},
    { -1, UNSUPP_MAJOR	}
};

struct major_map s800_maj_map[] =
{
    {  4, CS80		},
    { 13, SCSI		},
    { 19, AUTOCH	},
    { 12, ALINK		},
    {  7, NIO		},
    { -1, UNSUPP_MAJOR	}
};


/*
 *  The purpose of the vdi_identify function is to figure out what
 *  type of device is out there.  The type of device is returned.
 */

int vdi_identify(path, type)
     char *path;  /* input, pathname of device */
     int *type;    /* output, type of device */
{
  struct stat statbuf;
  *type = VDI_UNKNOWN;  /* initialize to unknown device type*/
  vdi_errno = VDIERR_NONE;
  qicflag = 0;          /* assume that it is not QIC at beginning */

  if (strchr(path, ':') != (char *)NULL) {
    *type = VDI_REMOTE;
  } else if (is_datfs(path)) {
    *type = VDI_DAT_FS;
  } else if (is_dat(path)) {
    *type = VDI_DAT;
  } else if (is_magtape(path)) {
    *type = VDI_MAGTAPE;
  } else if (is_mo(path)) {
    *type = VDI_MO;
  } else if (is_disk(path)) {
    *type = VDI_DISK;
  } else if (is_stdout(path)) {
    *type = VDI_STDOUT;
  } else if (is_file(path)) {
    *type = VDI_REGFILE;
  } else if ((stat(path, &statbuf)) < 0) {
    /* if stat fails assume device does not exist */
    vdi_errno = errno;
    return(VDIERR_NOD);
  }
    
  if (*type == VDI_UNKNOWN) {
    return VDIERR_ID;
  }
  else {
    return VDIERR_NONE;
  }
} /* end vdi_identify */


/*
 *  The purpose of this routine is to open the device for reading or
 *  writing.
 */

int vdi_open(type, path, oflag)
     char *path;  /* input, pathname of device */
     int type;    /* input, type of device */
     int oflag;   /* input, open flags */
{
  int fd;

  vdi_errno = 0;
  
  switch (type) {
  case VDI_REGFILE:
    if ((fd = open(path, oflag, PROTECT)) < 0) {
      vdi_errno = errno;
      fd = CLOSED;
    } 
    break;
  case VDI_MAGTAPE:
    if ((fd = open(path, oflag, PROTECT)) < 0) {
      vdi_errno = errno;
      fd = CLOSED;
    } 
    break;
  case VDI_DAT:
  case VDI_DAT_FS:
    if ((fd = open(path, oflag, PROTECT)) < 0) {
      vdi_errno = errno;
      fd = CLOSED;
    } 
    break;
  case VDI_MO:
    if ((fd = open(path, oflag, PROTECT)) < 0) {
      vdi_errno = errno;
      fd = CLOSED;
    } 
    break;
  case VDI_DISK:
    if ((fd = open(path, oflag, PROTECT)) < 0) {
      vdi_errno = errno;
      fd = CLOSED;
    } 
    break;
  default:
    fd = VDIERR_TYPE;
    break;
  }

  return fd;
  
} /* end vdi_open */


int vdi_close(type, path, fd, mttype)
     int type;  /* input, type of device */
     char *path;  /* input, pathname of device */
     int *fd;    /* input-output, open file descriptor */
     int mttype;    /* input, type of magtape */
{
  char s[128];

  vdi_errno = 0;
  
  switch (type) {
  case VDI_REGFILE:
  case VDI_STDOUT:
    (void) close(*fd);
    *fd = CLOSED;
    break;
  case VDI_MAGTAPE:
    (void) close(*fd);
    *fd = CLOSED;
    if (mttype == UCBNOREW) {
      (void) sprintf(s, "mt -t %s rewind &", path);
      (void) system(s);
    }
    break;
  case VDI_DAT:
  case VDI_DAT_FS:
    (void) close(*fd);
    *fd = CLOSED;
    break;
  case VDI_MO:
  case VDI_DISK:
    (void) close(*fd);
    *fd = CLOSED;
    break;
  default:
    *fd = VDIERR_TYPE;
    break;
  }

}  /* end vdi_close */


off_t vdi_seek(type, fd, offset, whence)
  off_t offset;      /* input, offset to start from*/
  int whence;        /* input, number of bytes to move */
  int type;          /* input, type of device */
  int fd;            /* input, open file desccriptor */
{
  off_t n;
  char *buf;

  vdi_errno = 0;
  
  switch (type) {
  case VDI_STDOUT:
    if ((n = lseek(fd, offset, whence)) == -1) {
      vdi_errno = errno;
    }
    break;
  case VDI_REGFILE:
    if ((n = lseek(fd, offset, whence)) == -1) {
      vdi_errno = errno;
    }
    break;
  case VDI_MAGTAPE:
    if ((buf = (char *) malloc((unsigned)whence)) == (char *)NULL) {
      vdi_errno = errno;
      n = VDIERR_IN;
      break;
    }
    if ((n = read(fd, buf, (unsigned)whence)) < 0) {
      vdi_errno = errno;
    }
    free(buf);
    break;
  case VDI_DAT:
  case VDI_DAT_FS:
    if ((buf = (char *) malloc((unsigned)whence)) == (char *)NULL) {
      vdi_errno = errno;
      n = VDIERR_IN;
      break;
    }
    if ((n = read(fd, buf, (unsigned)whence)) < 0) {
      vdi_errno = errno;
    }
    free(buf);
    break;
  case VDI_MO:
    if ((n = lseek(fd, offset, whence)) == -1) {
      vdi_errno = errno;
    }
    break;
  case VDI_DISK:
    if ((n = lseek(fd, offset, whence)) == -1) {
      vdi_errno = errno;
    }
    break;
  default:
    vdi_errno = VDIERR_TYPE;
    return(VDIERR_TYPE);
    break;
  }
  
  return(n);

}  /* end vdi_seek */


int vdi_read(type, fd, buf, nbyte)
  char *buf;         /* output, buffer to put bytes read into */
  unsigned nbyte;    /* input, number of bytes to read */
  int type;          /* input, type of device */
  int fd;            /* input, open file desccriptor */
{
  int n;

  vdi_errno = 0;
  
  switch (type) {
  case VDI_STDOUT:
  case VDI_REGFILE:
  case VDI_MAGTAPE:
  case VDI_DAT:
  case VDI_DAT_FS:
  case VDI_MO:
  case VDI_DISK:
    if ((n = read(fd, buf, nbyte)) < 0) {
      vdi_errno = errno;
    }
    break;
  case VDI_REMOTE:
    if ((n = rmt_read (fd, buf, nbyte)) < 0) {
      vdi_errno = errno;
    }
    break;
  default:
    vdi_errno = VDIERR_TYPE;
    return(VDIERR_TYPE);
    break;
  }
  
  return(n);

}  /* end vdi_read */


int vdi_write(type, fd, buf, nbyte)
  char *buf;         /* input, buffer write out */
  unsigned nbyte;    /* input, number of bytes to write */
  int type;          /* input, type of device */
  int fd;            /* input, open file desccriptor */
{
  int n;
  
  vdi_errno = VDIERR_NONE;
  
  switch (type) {
  case VDI_STDOUT:
  case VDI_REGFILE:
  case VDI_MAGTAPE:
  case VDI_DAT:
  case VDI_DAT_FS:
  case VDI_MO:
  case VDI_DISK:
    if ((n = write(fd, buf, nbyte)) <0) {
      vdi_errno = errno;
    }
    break;
  default:
    vdi_errno = VDIERR_TYPE;
    return(VDIERR_TYPE);
    break;
  }

  return(n);
  
}  /* end vdi_write */



int vdi_get_att(type, fd, ops, buf)
  long ops;               /* input, attributes to get */
  struct vdi_gatt *buf;   /* output, buffer to put values into */
  int type;               /* input, type of device */
  int fd;                 /* input, open file descriptor */
{
  union inquiry_data inq;
  struct capacity cap;
  disk_describe_type des;

  struct mtget mtget_buf;
  struct describe_type db;
  struct stat statbuf;
  off_t offset;
  off_t filepos;
  int whence;
  int size;
  
  int allowed_ops;
    
  vdi_errno = 0;
  
  buf->wrt_protect = VDIGAT_NO;  
  buf->media = VDIGAT_NO;          
  buf->on_line = VDIGAT_NO;        
  buf->bom = VDIGAT_NO;            
  buf->eom = VDIGAT_NO;            
  buf->tm1 = VDIGAT_NO;            
  buf->tm2 = VDIGAT_NO;            
  buf->im_report = VDIGAT_NO;      
  buf->door_open = VDIGAT_NO;      
  buf->density = VDIGAT_NA;        
  buf->optimum_block = VDIGAT_NA;  
  buf->wait_time = VDIGAT_NA;      
  buf->hog_time = VDIGAT_NA;       
  buf->queue_size = VDIGAT_NA;     
  buf->queue_mix = VDIGAT_NA;      

  switch (type) {
  case VDI_STDOUT:
  case VDI_REGFILE:
    allowed_ops = (VDIGAT_ISOB);
    if (!ops || (ops & ~allowed_ops)) {
      return(VDIERR_OP);
    }
    if (ops & VDIGAT_ISOB) {
      buf->optimum_block = 32768;
    }
    break;
  case VDI_MAGTAPE:
  case VDI_DAT:
  case VDI_DAT_FS:
  case VDI_REMOTE:
    allowed_ops = (VDIGAT_ISWP  | VDIGAT_ISOL  | VDIGAT_ISBOM |
		   VDIGAT_ISEOM | VDIGAT_ISTM1 | VDIGAT_ISIR  |
		   VDIGAT_ISEOD | VDIGAT_ISDO  | VDIGAT_ISOB  |
		   VDIGAT_ISTM2 | VDIGAT_ISDEN);
    if (!ops || (ops & ~allowed_ops)) {
      return(VDIERR_OP);
    }
    
    if ((type == VDI_REMOTE && rmt_ioctl (fd, MTIOCGET, &mtget_buf) < 0) ||
        (type != VDI_REMOTE &&     ioctl (fd, MTIOCGET, &mtget_buf) < 0)) {
      vdi_errno = errno;
      return(VDIERR_ACC); /* error accuracy */
    }
    
    if (ops & VDIGAT_ISDEN) {
      if (GMT_D_6250(mtget_buf.mt_gstat)) {
	buf->density = 6250;
      }
      if (GMT_D_1600(mtget_buf.mt_gstat)) {
	buf->density = 1600;
      }
      if (GMT_D_800(mtget_buf.mt_gstat)) {
	buf->density = 800;
      }
    }
    if (ops & VDIGAT_ISWP) {
      buf->wrt_protect = GMT_WR_PROT(mtget_buf.mt_gstat);
    }
    if (ops & VDIGAT_ISOL) {
      buf->on_line = GMT_ONLINE(mtget_buf.mt_gstat);
    }
    if (ops & VDIGAT_ISBOM) {
      buf->bom = GMT_BOT(mtget_buf.mt_gstat);
    }
    if (ops & VDIGAT_ISEOM) {
      buf->eom = GMT_EOT(mtget_buf.mt_gstat);
    }
    if (ops & VDIGAT_ISTM1) {
      buf->tm1 = GMT_EOF(mtget_buf.mt_gstat);
    }
    if (ops & VDIGAT_ISTM2) {
      buf->tm2 = GMT_SM(mtget_buf.mt_gstat);
    }
    if (ops & VDIGAT_ISIR) {
      buf->im_report = GMT_IM_REP_EN(mtget_buf.mt_gstat);
    }
    if (ops & VDIGAT_ISEOD) {
      buf->eod = GMT_EOD(mtget_buf.mt_gstat);
    }
    if (ops & VDIGAT_ISDO) {
      buf->door_open = GMT_DR_OPEN(mtget_buf.mt_gstat);
    }
    if (ops & VDIGAT_ISOB) {
      buf->optimum_block = 32768;
    }
    break;

  case VDI_MO:
    allowed_ops = (VDIGAT_ISWP  | VDIGAT_ISBOM | VDIGAT_ISEOM |
		   VDIGAT_ISOB);
    if (!ops || (ops & ~allowed_ops)) {
      return(VDIERR_OP);
    }

    if (ioctl(fd, SIOC_INQUIRY, &inq) < 0) {
      vdi_errno = errno;
      return(VDIERR_ACC);
    }
    if (ioctl(fd, SIOC_CAPACITY, &cap) < 0) {
      if (ioctl(fd, DIOC_DESCRIBE, &des) < 0) {
	vdi_errno = errno;
	return(VDIERR_ACC);
      }
    }
    whence = SEEK_CUR;
    offset = 0;
    if ((filepos = lseek(fd, offset, whence)) == -1) {
      vdi_errno = errno;
      return(VDIERR_ACC);
    }

    if (ops & VDIGAT_ISWP) {
      buf->wrt_protect = 0;
    }
    if (ops & VDIGAT_ISBOM) {
      buf->bom = (filepos == 0);
    }
    if (ops & VDIGAT_ISEOM) {
      buf->eom = (filepos == (cap.lba * cap.blksz));
    }
    if (ops & VDIGAT_ISOB) {
      buf->optimum_block = 65536;
    }
    break;

  case VDI_DISK:
    allowed_ops = (VDIGAT_ISBOM | VDIGAT_ISEOM |VDIGAT_ISOB);
    if (!ops || (ops & ~allowed_ops)) {
      return(VDIERR_OP);
    }

    fstat(fd, &statbuf);
    
    switch (get_disk_type(major(statbuf.st_rdev))) {
    case ALINK:
    case NIO:
    case CS80:
      if (ioctl(fd, CIOC_DESCRIBE, &db) < 0) {
	vdi_errno =errno;
	return(VDIERR_ACC);
      }
      whence = SEEK_CUR;
      offset = 0;
      if ((filepos = lseek(fd, offset, whence)) == -1) {
	vdi_errno = errno;
	return(VDIERR_ACC);
      }

      if (ops & VDIGAT_ISBOM) {
	buf->bom = (filepos == 0);
      }
      if (ops & VDIGAT_ISEOM) {
#ifdef __hp9000s300
	size = db.volume_tag.volume.maxsvadd.lfb;
#else
	size = db.volume_tag.volume.maxsvadd_lfb;
#endif  /* _hp9000s300 */	
	buf->eom = (filepos == size);
      }
      if (ops & VDIGAT_ISOB) {
	buf->optimum_block = 65536;
      }
      break;
    case SCSI:
      if (ioctl(fd, SIOC_INQUIRY, &inq) < 0) {
	vdi_errno = errno;
	return(VDIERR_ACC);
      }
      if (ioctl(fd, SIOC_CAPACITY, &cap) < 0) {
	if (ioctl(fd, DIOC_DESCRIBE, &des) < 0) {
	  vdi_errno = errno;
	  return(VDIERR_ACC);
	}
      }
      whence = SEEK_CUR;
      offset = 0;
      if ((filepos = lseek(fd, offset, whence)) == -1) {
	vdi_errno = errno;
	return(VDIERR_ACC);
      }

      if (ops & VDIGAT_ISWP) {
	buf->wrt_protect = 0;
      }
      if (ops & VDIGAT_ISBOM) {
	buf->bom = (filepos == 0);
      }
      if (ops & VDIGAT_ISEOM) {
	buf->eom = (filepos == (cap.lba * cap.blksz));
      }
      if (ops & VDIGAT_ISOB) {
	buf->optimum_block = 65536;
      }
      break;
    }

    if (ops & VDIGAT_ISOB) {
      buf->optimum_block = 65536;
    }
    break;
  default:
    /* error */
    break;
  }

  return(VDIERR_NONE);
  
}  /* end vdi_get_att */


int vdi_set_att(type, fd, ops, buf)
  long ops;               /* input, single attribute to set */
  struct vdi_satt *buf;   /* input, buffer with values */
  int type;               /* input, type of device */
  int fd;                 /* input, open file descriptor */
{
  struct queue_const qconst;
  struct stat statbuf;
  
  vdi_errno = 0;
  
  switch (type) {
  case VDI_STDOUT:
  case VDI_REGFILE:
    return(VDIERR_OP);
    break;
  case VDI_MAGTAPE:
    return(VDIERR_OP);
    break;
  case VDI_DAT:
  case VDI_DAT_FS:
    return(VDIERR_OP);
    break;
  case VDI_MO:
    if (ops = VDISAT_QCONS) {
      fstat(fd, &statbuf);
      if (get_disk_type(major(statbuf.st_rdev)) == AUTOCH) {
	qconst.wait_time = buf->wait_time;
	qconst.hog_time = buf->hog_time;
	if ((ioctl(fd, ACIOC_WRITE_Q_CONST, &qconst)) < 0) {
	  return(VDIERR_ACC);
	}
      } else {
	return(VDIERR_OP);
      }
    }
    break;
  case VDI_DISK:
    return(VDIERR_OP);
    break;
  default:
    return(VDIERR_OP);
    /* error */
    break;
  }

  return(VDIERR_NONE);
  
}  /* end vdi_set_att */


int vdi_ops(type, fd, ops, count)
  long ops;      /* input, single operation to do */
  long count;    /* input, number of times to do operation */
  int type;      /* input, type of device */
  int fd;        /* input, open file descriptor */
{
  struct mtop mtop_buf;
  
  vdi_errno = 0;

  /* check for legal operations */

  switch (type) {
  case VDI_STDOUT:
  case VDI_REGFILE:
    return(VDIERR_OP);
    break;
  case VDI_MAGTAPE:
    if ((ops == VDIOP_WTM2) || (ops == VDIOP_FSS) || (ops == VDIOP_BSS)) {
      return(VDIERR_OP);
    }
    mtop_buf.mt_count = count;
    if (ops == VDIOP_NOP) {
      mtop_buf.mt_op = MTNOP;
    }
    if (ops == VDIOP_SOFFL) {
      mtop_buf.mt_op = MTOFFL;
      mtop_buf.mt_count = 1;
    }
    if (ops == VDIOP_WTM1) {
      mtop_buf.mt_op = MTWEOF;
    }
    if (ops == VDIOP_FSF) {
      mtop_buf.mt_op = MTFSF;
    }
    if (ops == VDIOP_BSF) {
      mtop_buf.mt_op = MTBSF;
    }
    if (ops == VDIOP_FSR) {
      mtop_buf.mt_op = MTFSR;
    }
    if (ops == VDIOP_BSR) {
      mtop_buf.mt_op = MTBSR;
    }
    if (ops == VDIOP_REW) {
      mtop_buf.mt_op = MTREW;
      mtop_buf.mt_count = 1;
    }
    
    if (ioctl(fd, MTIOCTOP, &mtop_buf) < 0) {
      vdi_errno = errno;
      return(VDIERR_ACC);  /* error accuracy */
    }
    break;
  case VDI_DAT:
  case VDI_DAT_FS:
    mtop_buf.mt_count = count;
    if (ops == VDIOP_NOP) {
      mtop_buf.mt_op = MTNOP;
    }
    if (ops == VDIOP_SOFFL) {
      mtop_buf.mt_op = MTOFFL;
      mtop_buf.mt_count = 1;
    }
    if (ops == VDIOP_WTM1) {
      mtop_buf.mt_op = MTWEOF;
    }
    if (ops == VDIOP_WTM2) {
      mtop_buf.mt_op = MTWSS;
    }
    if (ops == VDIOP_FSS) {
      mtop_buf.mt_op = MTFSS;
    }
    if (ops == VDIOP_BSS) {
      mtop_buf.mt_op = MTBSS;
    }
    if (ops == VDIOP_FSF) {
      mtop_buf.mt_op = MTFSF;
    }
    if (ops == VDIOP_BSF) {
      mtop_buf.mt_op = MTBSF;
    }
    if (ops == VDIOP_FSR) {
      mtop_buf.mt_op = MTFSR;
    }
    if (ops == VDIOP_BSR) {
      mtop_buf.mt_op = MTBSR;
    }
    if (ops == VDIOP_REW) {
      mtop_buf.mt_op = MTREW;
      mtop_buf.mt_count = 1;
    }
    
    if (ioctl(fd, MTIOCTOP, &mtop_buf) < 0) {
      vdi_errno = errno;
      return(VDIERR_ACC);  /* error accuracy */
    }
    break;
  case VDI_MO:
    return(VDIERR_OP);
    break;
  case VDI_DISK:
    return(VDIERR_OP);
    break;
  default:
    return(VDIERR_OP);
    /* error */
    break;
  }

  return(VDIERR_NONE);
  
}  /* end vdi_ops */


check_remote(path)
     char *path;
{
  return (strchr(path, ':') != NULL);
}  /* end check_remote */

int is_magtape(path)
     char *path;
{
    struct mtget mtget_buf;
    struct stat statbuf;
    int outfd;
    
    if ((stat(path, &statbuf)) < 0) {
      vdi_errno = errno;
      return(FALSE);
    }
    else {
      if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
	return(FALSE);
      }
    }

    if ((outfd = open(path, O_RDONLY)) < 0) {
      vdi_errno =errno;
      return(FALSE);
    }
    
    if (ioctl(outfd, MTIOCGET, &mtget_buf) == -1) {
      close(outfd);
      return(FALSE);
    }
    else {

/* Check for SCSI Mag tape is made in addition to Stream type device */
      if ((mtget_buf.mt_type == MT_ISSTREAM) ||
          (mtget_buf.mt_type == MT_ISSCSI1)  ||
	  (mtget_buf.mt_type == MT_ISQIC)   ) {
        if(mtget_buf.mt_type == MT_ISQIC)
	   qicflag = 1;              /* QIC tape */
        else
	   qicflag = 0;
	close(outfd);
	return(TRUE);
      } else if (mtget_buf.mt_type == MT_IS3480) {
		 is3480 = 1;
		 close(outfd);
		 return(TRUE);
      } else {
	close(outfd);
	return(FALSE);
      }
    }
}  /* end is_magtape */

int is_dat(path)
     char *path;
{
    struct mtget mtget_buf;
    struct stat statbuf;
    int outfd;
    
    if ((stat(path, &statbuf)) < 0) {
      vdi_errno = errno;
      return(FALSE);
    }
    else {
      if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
	return(FALSE);
      }
    }

    if ((outfd = open(path, O_RDONLY)) < 0) {
      vdi_errno =errno;
      return(FALSE);
    }
    
    if (ioctl(outfd, MTIOCGET, &mtget_buf) == -1) {
      close(outfd);
      return(FALSE);
    }
    else {
      if (mtget_buf.mt_type == MT_ISDDS1) {
	close(outfd);
	return(TRUE);
      }
      else {
	close(outfd);
	return(FALSE);
      }
    }
}  /* end is_dat */


int is_datfs(path)
     char *path;
{

    struct mtget mtget_buf;
    struct stat statbuf;
    int outfd;

    if ((stat(path, &statbuf)) < 0) {
      vdi_errno = errno;
      return(FALSE);
    }
    else {
      if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
	return(FALSE);
      }
    }

    if ((outfd = open(path, O_RDONLY)) < 0) {
      vdi_errno =errno;
      return(FALSE);
    }
    
    if (ioctl(outfd, MTIOCGET, &mtget_buf) == -1) {
      close(outfd);
      return(FALSE);
    }
    else {
      if (mtget_buf.mt_type == MT_ISDDS2) {
	close(outfd);
	return(TRUE);
      }
    /*
     * At this point we don't know if this 8MM tape device can support
     * setmarks in the selected tape format (thru device file).
     * The 8MM tape driver determines the actual format only after a
     * read/write to the media. We assume that all 8MM formats
     * support setmarks to begin with and return the device type as
     * VDI_DAT. The actual check is done in writelabel(writer.c)
     * after writing the label and the device type is re-identified
     * thru setmarks_allowed_8mm(vdi.c).
     */
      else if (mtget_buf.mt_type == MT_ISEXABYTE) {
	is8mm = 1;
        close(outfd);
        return(TRUE);
      } else {
	close(outfd);
	return(FALSE);
      }
    }
}  /* end is_datfs */

int is_mo(path)
     char * path;
{

  struct stat statbuf;
  int outfd;
  struct describe_type db;
  int                  fildes;
  union inquiry_data inq;
   
  if ((stat(path, &statbuf)) < 0) {
    vdi_errno = errno;
    return(FALSE);
  }
  else {
    if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
      return(FALSE);
    }
  }
  
  if ((outfd = open(path, O_RDONLY)) < 0) {
    vdi_errno =errno;
    return(FALSE);
  }

  if (get_disk_type(major(statbuf.st_rdev)) == SCSI) {
    ioctl(outfd, SIOC_INQUIRY, &inq);
    if (strncmp(inq.inq1.product_id, "S6300.650A", strlen("S6300.650A")) == 0) {
      close(outfd);
      return(TRUE);
    }
  }
  if (get_disk_type(major(statbuf.st_rdev)) == AUTOCH) {
    close(outfd);
    return(TRUE);
  }
  
  close(outfd);
  return(FALSE);
}  /* end is_mo */

int is_disk(path)
     char * path;
{

  struct stat statbuf;
  int outfd;
  struct describe_type db;
  int                  fildes;
  union inquiry_data inq;
    
  if ((stat(path, &statbuf)) < 0) {
    vdi_errno = errno;
    return(FALSE);
  }
  else {
    if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
      return(FALSE);
    }
  }
  
  if ((outfd = open(path, O_RDONLY)) < 0) {
    vdi_errno =errno;
    return(FALSE);
  }

  switch (get_disk_type(major(statbuf.st_rdev))) {
  case ALINK:
  case NIO:
  case CS80:
    ioctl(outfd, CIOC_DESCRIBE, &db);
    if (db.unit_tag.unit.dt < 2) {
      close(outfd);
    return(TRUE);
    }
    break;
  case SCSI:
    ioctl(outfd, SIOC_INQUIRY, &inq);
    if (inq.inq2.dev_type == 0) {
      close(outfd);
      return(TRUE);
    }
    break;
  }

    close(outfd);
    return(FALSE);
}  /* end is_disk */

int is_stdout(path)
     char * path;
{
  struct stat statbuf;
  int outfd;
  
  if ((strcmp(path, "-")) == 0) {
    return(TRUE);
  }
  return(FALSE);
}  /* end is_stdout */

int is_file(path)
     char * path;
{
  struct stat statbuf;
  int outfd;
    
  if ((stat(path, &statbuf)) < 0) {
    vdi_errno = errno;
    return(FALSE);
  }
  else {
    if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
      return(TRUE);
    }
  }

  return(FALSE);
}  /* end is_file */

int get_disk_type(major_num)
  int major_num;
{
    struct major_map *table;

#ifdef __hp9000s300
    table = s300_maj_map;
#else
    if (sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO)
	table = s700_maj_map;
    else
	table = s800_maj_map;
#endif

    while (table->major_num != major_num && table->major_num != -1)
	table++;

    return table->dev_type;
}  /* end get_disk_type */


int
setmarks_allowed_8mm(fd)
    int	fd;     /* IN, Open File descriptor of tape device */
{

    register int	retval = FALSE;	/* Return value */
    struct mtget	mtget_buf;	/* Local MTIOCGET buffer */

    /*
     * Check for 8MM format supporting setmarks:
     * 8mm device with 8500c format.
     */
    if ((ioctl(fd, MTIOCGET, &mtget_buf) != -1) &&
	(mtget_buf.mt_type == MT_ISEXABYTE) &&
        (GMT_8mm_FORMAT(mtget_buf.mt_gstat) == FORMAT8mm8500c)) {
	    retval = TRUE;

    } /* Check for 8MM format supporting setmarks. */

    return (retval);

}  /* end setmarks_allowed_8mm */
