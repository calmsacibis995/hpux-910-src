/* @(#) $Revision: 63.3 $ */     
/* -------------------------------------------------------------------------- */
/*	
	Issues  - Need to eliminate the dependency on an incore FAT
		- should disc space be zero'd on writes to file 
		- when to update directory entries?  each time eof is changed ?
		- where possible move data by words instead of bytes 
		- does the last entry in a directory have to mark the end of
		  the directory or does the code have to watch out for physical
		  end of directory also? RESOLVED: code added for physical eof.
		- Could compare duplicate FATS and tell the user if they are
		  different.
*/
/* -------------------------------------------------------------------------- */
#include	<sys/types.h>
#include	<stdio.h>
#include	<errno.h>
#include	<fcntl.h>
#include	"dos.h"
#include	<ndir.h>
#include	<signal.h>
#include	<time.h>
#include	<sys/stat.h>
#include	<unistd.h>	/* needed for lockf() */
char	*strchr (), *strrchr();	

/* ------------------------------------------------------------ */
/* closefd - used to allocate a file descriptor for an open file */ 
/* ------------------------------------------------------------ */
closefd (mp)
struct	info	*mp;
{
	if (mp->dir_modified)
		update_dir_entry (mp);
	--(mp->openc);
	
}



/* ------------------------------------------------------------ */
/* openfd - used to allocate a file descriptor for an open file */ 
/* ------------------------------------------------------------ */
struct info *openfd (hd, daddr)
struct	header	*hd;
int	daddr;
{
	struct	info	*mp;
	struct	info	*avail;

	/* find an empty slot in the file descriptor table */
	avail = NULL;
	for (mp=info; mp<&info[INFOSIZE]; mp++) {
		if ((mp->openc > 0) && (mp->daddr == daddr) 
				    && (mp->hdr == hd)) {
			mp->openc++;
			return (mp);
		} 
		if (mp->openc <= 0)
			avail = mp;
	}

	/* did we find one */
	if (avail == NULL) {
		errno = ENFILE;
		return(NULL);
	}
	avail->hdr = hd;
	avail->openc = 1;
	avail->daddr = daddr;
	avail->fp = 0;
	avail->clsoffset = -1;
	avail->dir_modified = FALSE;
	return (avail);
}


/* ------------------------------------------------------------ */
/* get_fat_entry - read an entry from the file allocation table */
/* ------------------------------------------------------------ */
get_fat_entry (hd, cls)
struct header	*hd;
int		cls;	/* FAT entry (cluster) to read */
{
	int		index;
	unsigned int	new_cls;

	/* quick parameter check */
	if (cls > hd->max_cls || cls < 0) {
	  fprintf (stderr, "Cluster out of range: %d, %d\n", cls, hd->max_cls);
	  fprintf (stderr, "Possible corrupted disk on %s\n", hd->dname);
	  return(-1);
	}

	if (hd->twelve_bit_fat == TRUE) { 	/* 12 or 16 bit FAT entries? */
		index  = cls + cls/2;
		if (cls%2 == 0) {
			new_cls =   (hd->fat[index] & 0xff) 
				  | ((hd->fat[index+1]<<8) & 0xf00);
		} else {
			new_cls =   ((hd->fat[index]>>4) & 0xf) 
				  | ((hd->fat[index+1]<<4) & 0xff0);
		}
	} else {
		index = cls*2;
		new_cls  = BUILDHW(hd->fat[index], hd->fat[index+1]);
	}

	return(new_cls);
}


/* ----------------------------------- */
/* flush_fat - write FAT back to disc  */
/* ----------------------------------- */
flush_fat (hd)
struct	header *hd;
{
	/* update FAT on disc */
	hd->fat_modified = FALSE;
	if (put_disc_data (hd, 
			   hd->reserved_sec*hd->byt_p_sector, 
			   hd->fat, 
			   hd->fatsize) 		!= hd->fatsize) {
		fprintf (stderr, "Could not write primary FAT\n");
		return(-1);
	}
	/* update secondary FAT on disc */
	if (put_disc_data (hd, 
			   hd->reserved_sec*hd->byt_p_sector+hd->fatsize, 
			   hd->fat, 
			   hd->fatsize) 		!= hd->fatsize) {
		    fprintf (stderr, "Could not write secondary FAT\n");
		    return(-1);
	}
	return (0);
}


/* ------------------------------------------------------------ */
/* put_fat_entry - write an entry in the file allocation table  */
/* ------------------------------------------------------------ */
put_fat_entry (hd, cls, value)
struct header	*hd;
int		cls;		/* FAT entry to write */
unsigned int	value;		/* Value to write in low order 12/16 bits */
{
	int	index;
	uint	new_cls;

	/* quick parameter check */
	if (cls > hd->max_cls || cls < 0) {
	  fprintf (stderr, "Cluster out of range: %d, %d\n", cls, hd->max_cls);
	  fprintf (stderr, "Possible corrupted disk on %s\n", hd->dname);
	  return(-1);
	}

	if (hd->twelve_bit_fat == TRUE) {
		index  = cls + cls/2;
		if (cls%2 == 0) {
			hd->fat[index] = (value & 0xff); 
			new_cls =  hd->fat[index+1];
			hd->fat[index+1] = (new_cls&0xf0)|((value>>8) & 0xf);
		} else {
			new_cls =  hd->fat[index];
			hd->fat[index] = (new_cls&0xf) | ((value & 0xf)<<4); 
			hd->fat[index+1] = (value>>4) & 0xff;
		}
	} else {
		index = cls*2;
		hd->fat[index] = value&0xff;
		hd->fat[index+1] = (value>>8)&0xff;
	}

	hd->fat_modified = TRUE;
	return(0);
}


/* ------------------------------------------------ */
/* close_device - */
/* ------------------------------------------------ */
close_device  (hd)
struct	header	*hd;
{

	struct	header	*d;

	if (--hd->openc <= 0) {

		/* write FAT to disc and release memory */
		if (hd->fat != NULL) {
			if (hd->fat_modified) 
				flush_fat (hd);
			free (hd->fat);
			hd->fat = NULL;
			}

		/* remove from list of devices */
		if (hd == devices) {
			devices = devices->next;
		} else {
			for (d = devices; (d->next != hd); d = d->next);
			d->next = hd->next;
			}

		/* get rid of cache buffers */
		if (hd->cb.buf != (uchar *) 0)
			free (hd->cb.buf);
			
		/* release memory for header */
		free (hd);
	} else { 
		/* flush the FAT each time a file is closed */
		if (hd->fat != NULL) {
			if (hd->fat_modified) 
				return (flush_fat (hd));
	
		}
	}
	return(0);
}


/* ------------------------------------------- */
/* determine_disk_type - read FAT id from disc */
/* ------------------------------------------- */
determine_disc_type (hd)
struct	header	*hd;
{
	unsigned char	temp[3];

#ifdef DEBUG
bugout ("determine disk type:");
#endif DEBUG


	/* Assume - Sectors are 512 bytes 
		    The FAT is located in the second sector on the disk
	*/
	if ((get_disc_data (hd, 512, temp, 3) != 3) || (temp[1] != 0xff) 
						    || (temp[2] != 0xff)) 
		return (-1);

#ifdef DEBUG
bugout ("fat id = %x\n", temp[0]);
#endif DEBUG

	hd->fats = 2;
	hd->hidden_sec = 1;
	hd->reserved_sec = 1;
	hd->byt_p_sector = 512;
	switch(temp[0]) {
		case 0xff: /* dual sided, 8 sectors per track */
			hd->sec_p_cls = 2;
			hd->dir_entries = (7 * 512) / DIRSIZE;
			hd->total_sec = 640;
			hd->media_type = 0xff;
			hd->sec_p_fat = 1;
			hd->sec_p_track = 8;
			hd->heads = 2;
			break;
		case 0xfe: /* single sided, 8 sectors per track */
			hd->sec_p_cls = 1;
			hd->dir_entries = (4 * 512) / DIRSIZE;
			hd->total_sec = 320;
			hd->media_type = 0xfe;
			hd->sec_p_fat = 1;
			hd->sec_p_track = 8;
			hd->heads = 1;
			break;
		case 0xfd: /* dual sided, 9 sectors per track */
			hd->sec_p_cls = 2;
			hd->dir_entries = (7 * 512) / DIRSIZE ;
			hd->total_sec = 720;
			hd->media_type = 0xfd;
			hd->sec_p_fat = 2;
			hd->sec_p_track = 9;
			hd->heads = 2;
			break;
		case 0xfc: /* single sided, 9 sectors per track */
			hd->sec_p_cls = 1;
			hd->dir_entries = (4 * 512) / DIRSIZE;
			hd->total_sec = 360;
			hd->media_type = 0xfc;
			hd->sec_p_fat = 2;
			hd->sec_p_track = 9;
			hd->heads = 1;
			break;
		case 0xf9: /* dual sided, 15 sectors per track */
			hd->sec_p_cls = 1;
			hd->total_sec = 2400;
			hd->media_type = 0xf9;
			hd->sec_p_fat = 7;
			hd->sec_p_track = 15;
			hd->heads = 2;
			hd->dir_entries = (14 * 512) / DIRSIZE;
			break;
		default: return (-1);
		}
	return (0);
}

/* ------------ */
/* check_header */
/* ------------ */
check_header (hd, temp)
uchar	*temp;
struct	header	*hd;
{

	strncpy (hd->idstring, &temp[3], 8); 
	hd->idstring[8] = '\0';
	hd->byt_p_sector = BUILDHW(temp[11], temp[12]);
	hd->sec_p_cls = temp[13];
	hd->reserved_sec = BUILDHW(temp[14], temp[15]);
	hd->fats = temp[16];
	hd->dir_entries = BUILDHW(temp[17], temp[18]);
	hd->total_sec = BUILDHW(temp[19], temp[20]);
	hd->media_type = temp[21];
	hd->sec_p_fat = BUILDHW(temp[22], temp[23]);
	hd->sec_p_track = BUILDHW(temp[24], temp[25]);
	hd->heads = BUILDHW(temp[26], temp[27]);
	hd->hidden_sec = BUILDHW(temp[28], temp[ 29]);

#ifdef DEBUG
	bugout ("DISK HEADER:\n");
	bugout ("\tid string =  %s\n", hd->idstring);
	bugout ("\tbyt  per sector =  %d\n", hd->byt_p_sector);
	bugout ("\tsec per cls =  %d\n", hd->sec_p_cls);
	bugout ("\treserved sec =  %d\n", hd->reserved_sec);
	bugout ("\tnumber of fats = %d\n", hd->fats);
	bugout ("\tdir entries = %d\n", hd->dir_entries);
	bugout ("\ttotal number of sec =  %d\n", hd->total_sec);
	bugout ("\tmedia type =  %d\n", hd->media_type);
	bugout ("\tsec per fat = %d\n", hd->sec_p_fat);
	bugout ("\tsec per track =  %d\n", hd->sec_p_track);
	bugout ("\tnumber of heads =  %d\n", hd->heads);
	bugout ("\thidden sec = %d\n", hd->hidden_sec);
#endif DEBUG


	/* data consistency check */
	if (   (hd->sec_p_cls <= 0)
	     ||(hd->byt_p_sector <= 0)
	     ||(hd->sec_p_track <= 0)
	     ||(hd->sec_p_fat <= 0)
	     ||(hd->fats <= 0) || (hd->fats > 2)
	     ||(hd->reserved_sec <= 0)
	     ||(hd->dir_entries <= 0)
	     ||(hd->total_sec <= 0)
	     ||((temp[0] != 0xe9) && (temp[0] != 0xeb)) /* near jump to boot */
	   ) return (-1);

   return (0); /* success */
}


/* ------------------------------------------------ */
/* open_device - read the DOS boot sector from disc */
/* ------------------------------------------------ */
struct header *open_device  (device, mode)
char		*device;
int		mode;		/* masked to O_RDWR|O_RDONLY */
{
	uchar	temp[66]; 	/* portion of boot sector from disc.  must be */
				/* at least as large as the partition table   */
				/* or the disk header.			      */
	struct	header	*hd;
	int		fd;
	int		i;
	unsigned int	off;
	
	/* if read-only, look for an existing device */
	/* doing this when read-write causes doscp to break
	   when source device same as destination device */
	if (mode == O_RDONLY) {
	  for (hd=devices; 
	       (hd!=NULL) && ((strcmp(hd->dname, device) != 0)); 
	       hd=hd->next) ; 
	  if (hd != NULL) {
	      hd->openc++;
	      return (hd);
	      }
	  }

	/* none found; create one */
	hd =  (struct header *) malloc(sizeof (struct header));
	if (hd == (struct header *) 0) {
		fprintf (stderr, "Could not allocate memory for FAT\n");
		return(NULL);
		}
	hd->next = devices;
	devices = hd;
	strcpy (hd->dname, device); 

	hd->part_off = 0;
	hd->openc = 1;
	hd->fd = -1;
	hd->fat = NULL;
	hd->cb.buf =  (uchar *) 0;

	/* initialize the cache buffers */
	hd->cb.hd = hd;
	hd->cb.block = -1;
	hd->blocksize = DEV_BSIZE; /* from <sys/param.h> */
	hd->cb.buf =  (uchar *) malloc(hd->blocksize);
	if (hd->cb.buf == (uchar * )0) {
		close_device (hd);
		return (NULL);
		}
	
	/* Be sure to mask mode, it may include O_TRUNC, causing disaster */
	hd->fd = open(device, mode & (O_RDWR|O_RDONLY));
	if (hd->fd == -1) {
		fprintf (stderr, "Could not open %s\n", device);
		fprintf (stderr, "Errno = %d\n", errno);
		close_device (hd);
		return(NULL);
		}

	/* Lock the file, unless read-only. */
	/* If already locked (errno is EAGAIN|ACCESS), stop. */
	if (mode != O_RDONLY)
	  if (lockf (hd->fd, F_TLOCK, 0) == -1) {
	    fprintf(stderr,"Could not lock \"%s\" for exclusive access:",device);
	    fprintf(stderr,"  System errno = %d\n",errno);
	    if (errno == EAGAIN || errno == EACCES)
	 	fprintf(stderr,"\"%s\" is in use by some other application.\n",device);
	    close_device (hd);
	    return (NULL);
	  }

	/* check for 5.25 floppy format */
	if (get_disc_data (hd, 0, temp, HEADER_SIZE) != HEADER_SIZE) {
		close_device (hd);
		return(NULL);	
		}
	if (check_header (hd, temp) >=  0) goto success;

	/* look for a partition header on a fixed disk */
	get_disc_data (hd, 512-PART_SIZE, temp, PART_SIZE);

	/*	Each partition table entry looks like:
		00	boot indicator
		01	beginning head
		02	beginning sector
		03	beginning cylinder
		04	system indicator
		05	ending head
		06	ending sector
		07	ending cylinder
		08	relative sector (low word)
		0A	relative sector (high word)
		0C	number of sectors (low word)
		0E	number of sectors (high word)
	*/
	if ((temp[64] == 0x55) && (temp[65] == 0xaa)) {/* signature at end of */
	    i = 0;					/* partition	      */
	    while (i<64) {
		if (   (temp[i] == 0x80) 	/* boot indicator */
		    && ((temp[i+4] == 0x04)	/* DOS indicator 16 bit FAT */
		        || (temp[i+4] == 0x01))){ /* DOS indicator 12 bit FAT */
		  off = BUILDHW(temp[i+10],temp[i+11]);
		  off = (off << 16) | BUILDHW(temp[i+8],temp[i+9]);
		  off *= 512;
		  get_disc_data (hd, off, temp, HEADER_SIZE);
		  if (check_header (hd, temp) >=  0) {
		  	/* invalidate the cache buffer */
		  	hd->cb.block = -1;
		  	hd->part_off = off;
			goto success;
		  }
		  break;
		  
		}
		i += 16;
	    }
	}


	if (determine_disc_type (hd) == -1) {
		fprintf (stderr, "Unrecognizable disc format on %s\n",
			 hd->dname);
  		close_device (hd);
  		return (NULL);
	}

success:

	hd->byt_p_cls = 
		hd->sec_p_cls * hd->byt_p_sector;
	hd->root_dir_start = 
		hd->reserved_sec + (hd->fats * hd->sec_p_fat); 
	hd->data_area_start = hd->root_dir_start 
		+ (hd->dir_entries * DIRSIZE + hd->byt_p_sector -1)
		/hd->byt_p_sector;

	if (init_fat (hd) != -1) {
		return(hd);
	} else  {
		close_device (hd);
		return(NULL);
		}
}


/* ----------------------------------------------------- */
/* init_fat - read the file allocation table into memory */
/* ----------------------------------------------------- */
init_fat  (hd)
struct	header	*hd;
{

	hd->fat_modified = FALSE;

	/* Clusters are numbered 2 thru max_cls, there are (max_cls - 1)
	   physical clusters. */
	hd->max_cls = 1 + (
	          hd->total_sec 
		- hd->reserved_sec 
		- hd->fats * hd->sec_p_fat 
		- (hd->dir_entries*DIRSIZE)/hd->byt_p_sector) 
		/ hd->sec_p_cls;

	/* Allocate_cluster increments acls before use, initting to
	   max_cls will cause it to wrap around to beginning of FAT
	   for first cluster to check. */
	hd->acls = hd->max_cls;

	/* Does the disc use 12 bit or 16 bit FAT entries? */
	if (hd->max_cls > 4086)
		hd->twelve_bit_fat = FALSE;
	   else hd->twelve_bit_fat = TRUE;

	hd->fatsize = hd->sec_p_fat * hd->byt_p_sector;
	hd->fat =  (uchar *) malloc(hd->fatsize);
	if (hd->fat == (uchar *) 0) {
		fprintf (stderr, "Could not allocate memory for FAT\n");
		return(-1);
	}
	if (get_disc_data (hd, 
			   hd->reserved_sec*hd->byt_p_sector, 
			   hd->fat, 
			   hd->fatsize) 		!= hd->fatsize) {
		fprintf (stderr, "Could not read primary FAT\n");
		fprintf (stderr, "Reading secondary FAT\n");
		if (get_disc_data (hd, 
			   hd->reserved_sec*hd->byt_p_sector+hd->fatsize, 
			   hd->fat, 
			   hd->fatsize) 		!= hd->fatsize) {
		    fprintf (stderr, "Could not read secondary FAT\n");
		    return(-1);
		}
	}

	return(0);
}





/* -------------------------------------------------------------------------- */
/* get_daddr - obtain the disc address of data in a file.  Mainly used to get */
/*	       the disc address of a directory entry.                         */
/* -------------------------------------------------------------------------- */
get_daddr (mfd, offset)
struct	info	*mfd;
int		offset;
{
	struct	dir_entry	*dir;
	struct	header		*hd;

	hd = mfd->hdr;
	dir = &(mfd->dir);

	if (dir->cls == 0) { 		/* special case root directory */

		/* bounds check */
		if (offset > hd->dir_entries * DIRSIZE)
			return (-1);
		offset = hd->root_dir_start*hd->byt_p_sector + offset;

	} else {
		if (gdaddr (mfd, offset, FALSE) != 1) 
			return(-1);
		offset = ((mfd->cls - 2) * hd->sec_p_cls + hd->data_area_start) 
			* hd->byt_p_sector + offset%hd->byt_p_cls;
	}

	return(offset);
}





/* ------------------------------------------- */
/* dos_seek -  */
/* ------------------------------------------- */
dos_seek (mfd, offset)
struct	info	*mfd;
int		offset;
{
	mfd->fp = offset;
	return(mfd->fp);
}



/* ------------------------------------------- */
/* get_file_data - read data from a msdos file */
/* ------------------------------------------- */
get_file_data (mfd, buf, size)
struct	info	*mfd;
uchar		*buf;
int		size;
{
	int			ret;
	int			byt_transfered;
	int			transfer_size;
	struct	header		*hd;
	int			offset;


	hd = mfd->hdr;

	byt_transfered = 0;

	if (size < 0) {
	  fprintf (stderr, "Outside range of file. size = %d\n", 
	           size);
	  return(-1);
	}

	/* transfer data up the cluster boundry */
	transfer_size = MIN (size, hd->byt_p_cls - mfd->fp%hd->byt_p_cls);
	if ((ret = gdaddr(mfd, mfd->fp, FALSE)) != 1)
			return(ret);
	offset = ((mfd->cls - 2) * hd->sec_p_cls + hd->data_area_start) 
		* hd->byt_p_sector + mfd->fp%hd->byt_p_cls;
	
	while (size > 0) {
		if (get_disc_data(hd, 
				  offset, 
				  buf, 
				  transfer_size) != transfer_size){
			fprintf (stderr, "Get file data size error\n");
			return(-1);
		}
		buf += transfer_size;
		size -= transfer_size;
		byt_transfered += transfer_size;
		mfd->fp += transfer_size;
		if (size >0) {
			if ((ret = gdaddr(mfd, mfd->fp, FALSE)) != 1)
				return(ret);
			offset = (  (mfd->cls - 2) * hd->sec_p_cls 
				  + hd->data_area_start) * hd->byt_p_sector;
			transfer_size = MIN (size, hd->byt_p_cls);
		}
	}
	return(byt_transfered);
}

/* ----------------------------------------------------------------------- */
/* allocate_cluster - allocate an unused cluster.  An end of file mark is  */ 
/*		      written to the newly allocated cluster.		   */
/* ----------------------------------------------------------------------- */
allocate_cluster (hd)
struct	header		*hd;
{
	int	c;
	int	start;
	int	ret;
	uint	eof;
	short	first;

	/* begin the search where we last left off */
	c = hd->acls;
	eof = CLSEOF(hd->twelve_bit_fat);
	
	/* unallocated clusters contain zero.  linear search of the FAT with
	   wrap around. */
	start = c;
	first = 1;
	for (;;) {
		c++;
		if (c > hd->max_cls)
			c = 2;
		if ((c==start+1 || (c==2 && start==hd->max_cls)) && first==0)
			return(-1);
		first = 0;
		ret = get_fat_entry (hd, c);
		if (ret < 0) 
			return(ret);
		if (ret == 0) {
			hd->acls = c;
			if (put_fat_entry (hd, c, eof) == -1)
				return (-1);
			return(c);
		}
	}
}


/* ----------------------------------------- */
/* convert_dir_entry - a directory entry */
/* ----------------------------------------- */
convert_dir_entry (dir, buf, direction)
struct dir_entry	*dir;			/* converted directory entry */
uchar			*buf;			/* raw directory entry */
int			direction;		/* use CDIR_IN or CDIR_OUT */
{
	int	i;


	if (direction == CDIR_IN) {
		strncpy (dir->name, buf, 8);
		dir->name[8] = 0;
		strncpy (dir->ext, &buf[8], 3);
		dir->ext[3] = 0;
		dir->attr = buf[11];
		for(i=12; i<22; i++) dir->reserved[i-12] = buf[i];
		dir->year = (buf[25]>>1) & 0x7f;
		dir->month = ((buf[24]>>5) & 0x7) | ((buf[25] & 0x1) << 3);
		dir->day = buf[24] & 0x1f;
		dir->hour = (buf[23]>>3) & 0x1f;
		dir->minute = ((buf[22]>>5) & 0x7) | ((buf[23] & 0x7)<<3);
		dir->second = buf[22] & 0x1f;
		dir->cls = BUILDHW (buf[26],buf[27]);
		dir->size =   (BUILDHW(buf[28],buf[29])) 
		    	| ((BUILDHW(buf[30],buf[31]))<<16);
	} else {

#ifdef DEBUG
if (direction != CDIR_OUT) {
	fprintf (stderr, "Bad argument to convert_dir_entry\n");
	}
#endif DEBUG

		strncpy (buf, dir->name, 8);
		strncpy (&buf[8], dir->ext, 3);
		buf[11] = dir->attr; 
		for(i=12; i<22; i++) buf[i] = dir->reserved[i-12]; 
		for(i=23; i<26; i++) buf[i] = 0; 
		buf[25] = ((dir->year<<1) &0xfe) | ((dir->month>>3) & 0x1);
		buf[24] = ((dir->month & 0x7) << 5) | (dir->day&0x1f);
		buf[23] = (dir->hour<<3) | ((dir->minute>>3) & 0x7);
		buf[22] = ((dir->minute & 0x7) << 5) | (dir->second);
		buf[26] = dir->cls & 0xff;
		buf[27] = (dir->cls >> 8) & 0xff;
		buf[28] = dir->size & 0xff;
		buf[29] = (dir->size >> 8) & 0xff;
		buf[30] = (dir->size >> 16) & 0xff;
		buf[31] = (dir->size >> 24) & 0xff;
	}
}



/* ------------------------------------------------------------------- */
/* cls_eof - returns "TRUE" if the FAT entry represents an end of file */
/* ------------------------------------------------------------------- */
cls_eof (hd, cls)
struct	header		*hd;
int			cls;
{

	if (hd->twelve_bit_fat == TRUE) {
		if (cls >= 0xff8 && cls <= 0xfff)
			return(TRUE);
	} else  {
		if (cls >= 0xfff8 && cls <= 0xffff)
			return(TRUE);
	}
	return(FALSE);
}


/* -------------------------------------------------------------------------- */
/* gdaddr - Find the cluster holding the data which is "offset" bytes from    */
/*          the start of the file.  If "allocate" is true, clusters are       */
/*          allocated as neccessary.   Returns -1 if some catatrophic error   */
/*          occurs; 0 if an end of file is encounted and "allocate" is false; */
/*          and 1 is returned if success.  Leaves the file positioned at EOF  */
/* -------------------------------------------------------------------------- */
gdaddr (mfd, offset, allocate)
struct info	*mfd;
int		offset;
boolean		allocate;
{
	struct	dir_entry	*dir;
	struct  header		*hd;
	int			cls;


	dir = &(mfd->dir);
	hd = mfd->hdr;

	offset -= offset %hd->byt_p_cls;
	if (mfd->clsoffset > offset || mfd->clsoffset < 0) {
		
		/* start with the cluster in the directory entry */
		mfd->clsoffset = -1;
		mfd->cls = dir->cls;
		if (cls_eof(hd, mfd->cls)) {
			if (!allocate) return(0);
			if ((cls = allocate_cluster (hd)) == -1) return(-1);
			dir->cls = cls;
			if (update_dir_entry (mfd) == -1) {
				dir->cls = CLSEOF (hd->twelve_bit_fat);
				return (-1);
				}
			mfd->cls = cls;
			}
		if ((mfd->clsvalue = get_fat_entry (hd, mfd->cls)) == -1)
			return(-1);
		mfd->clsoffset = 0;
		}
	/* traverse down the list of clusters until we locate the proper one */
	while (offset > mfd->clsoffset) {
		if (cls_eof(hd, mfd->clsvalue)) {
			if (!allocate) return(0);
			if ((cls = allocate_cluster (hd)) == -1) return(-1);
			if (put_fat_entry(hd, mfd->cls, cls) == -1)
				return(-1);
			mfd->clsvalue = cls;
			}
		if ((cls = get_fat_entry (hd, mfd->clsvalue)) == -1)
			return(-1);
		mfd->cls = mfd->clsvalue;
		mfd->clsvalue = cls;
		mfd->clsoffset += hd->byt_p_cls;
		}
	return(1);
}







/* ------------------------------------------- */
/* put_file_data - write data to a msdos file */
/* ------------------------------------------- */
put_file_data (mfd, buf, size)
struct	info	*mfd;
uchar		*buf;
int		size;
{
	struct	header		*hd;
	int	byt_transfered;		/* number bytes actually transfered */
	int	transfer_size;
	int	offset;		


	hd = mfd->hdr;

	byt_transfered = 0;

	if (size < 0 ) 
		return(0);
	
	transfer_size = MIN (size, hd->byt_p_cls - mfd->fp%hd->byt_p_cls);
	if (gdaddr (mfd, mfd->fp, TRUE) != 1)
		return(-1);
	offset = ((mfd->cls - 2) * hd->sec_p_cls + hd->data_area_start) 
		* hd->byt_p_sector + mfd->fp%hd->byt_p_cls;
	while (size > 0) {
		if (put_disc_data(hd, 
				  offset, 
				  buf, 
				  transfer_size) != transfer_size){
			fprintf (stderr, "put_file_data size error\n");
			return(-1);
		}
		buf += transfer_size;
		size -= transfer_size;
		byt_transfered += transfer_size;
		mfd->fp += transfer_size;
		if (size >0) {
			if (gdaddr (mfd, mfd->fp, TRUE) != 1)
				return(-1);
			offset = (  (mfd->cls - 2) * hd->sec_p_cls 
				  + hd->data_area_start) * hd->byt_p_sector;
			transfer_size = MIN (size, hd->byt_p_cls);
		}
	}

	return(byt_transfered);
}


/* -------------------------------------------------------- */
/* update_dir_entry - update the directory entry for a file */
/* -------------------------------------------------------- */
update_dir_entry (mfd)
struct info		*mfd;
{
	uchar	buf[DIRSIZE];
	
	convert_dir_entry (&(mfd->dir), buf, CDIR_OUT);

	mfd->dir_modified = FALSE;
	if (mfd->daddr==0xffffffff && mfd->dir.cls==0 ) {
		fprintf (stderr, "This operation not allowed on root directory\n");
		return(-1);
	} else return(put_disc_data ( mfd->hdr, mfd->daddr, buf, DIRSIZE));
}


/* ------------------------------------------------------------------------ */
/* read_dir_entry - read a directory entry into "dir".  Return the raw disc */
/*                  address of the directory entry.			    */
/* ------------------------------------------------------------------------ */
read_dir_entry (mfd, entry, dir)
struct info		*mfd;
int			entry;
struct dir_entry	*dir;
{
	struct	header *hd;
	int	offset;
	uchar	buf[DIRSIZE];


	hd = mfd->hdr;
	offset = entry * DIRSIZE;

	/* Make sure its a dir */
	if ((mfd->dir.attr & ATTDIR) != ATTDIR) {
		fprintf (stderr, "read_dir_entry: not directory.\n");
		return(-1);
		}

	if (mfd->dir.cls == 0) {	/* special case root directory */

		if (entry >= hd->dir_entries) {
			fprintf (stderr, "EOF reading root.\n");
			return(-1);
			}
  		if  (get_disc_data (hd, 
				hd->root_dir_start*hd->byt_p_sector + offset, 
				buf, 
  				DIRSIZE) != DIRSIZE)
			return(-1);;
	} else {

		mfd->fp = offset;
		if (get_file_data (mfd, buf, DIRSIZE) != DIRSIZE)
			return(-1);
		}

	/* Convert dir entries */
	convert_dir_entry (dir, buf, CDIR_IN);

	return(get_daddr (mfd, offset));
}



/* ----------------------------------------- */
/* write_dir_entry - write a directory entry */
/* ----------------------------------------- */
write_dir_entry (mfd, entry, dir)
struct info		*mfd;
int			entry;
struct dir_entry	*dir;
{
	int	offset;
	uchar	buf[DIRSIZE];
	struct	header	*hd;


	hd = mfd->hdr;

	/* Convert dir entries */
	convert_dir_entry (dir, buf, CDIR_OUT);
	offset = entry * DIRSIZE;

	/* Make sure its a dir */
	if ((mfd->dir.attr & ATTDIR) != ATTDIR) {
		fprintf (stderr, "write_dir_entry: not directory.\n");
		return(-1);
		}

	if (mfd->dir.cls == 0) {	/* special case root directory */

		if (entry >= hd->dir_entries) {
		 fprintf (stderr, "Root directory full.\n");
		 return(-1);
		 }
  		if  (put_disc_data (hd, 
				hd->root_dir_start*hd->byt_p_sector + offset, 
				buf, 
  				DIRSIZE) != DIRSIZE)
			return(-1);;
		return(0);
	} else {

		/**************************************************************
		  When disc space is allocated to a directory, it is filled 
		  with "unused" entries to mark the logical end of directory.
		  Seek out to the desired location to write.  if the physical
		  end of file is encountered, then "unused" entries are 
		  written in the gap between the physical end of directory and
		  the entry we wish to write.
		**************************************************************/
		switch (gdaddr (mfd, offset, FALSE)) {

		  case 0 : /* end of file was encountered */
		  { int	filb;
		    int	ts;
		    int	i;
		    char	filbuf[512];

		    if (mfd->clsoffset < 0) 
			mfd->fp = 0;
		     else mfd->fp = mfd->clsoffset + hd->byt_p_cls;
		    filb = (offset - mfd->fp + hd->byt_p_cls);
		    filb = filb - filb%hd->byt_p_cls;
		    for (i=0; i<512; i++) filbuf[i] = 0xf6;
		    for (i=0; i<512; i+=DIRSIZE) filbuf[i] = DIRUNUSED;

		    while (filb > 0) {
		      ts = (filb > 512) ? 512 : filb;
		      if (put_file_data (mfd, filbuf, ts) != ts)
			return (-1);
		      filb -= ts;
		    }
		    break;
		  }
		  case 1 : /* no end of file was encountered */
		    break;

		  default: return (-1);
		}
		mfd->fp = offset;
		return(put_file_data (mfd, buf, DIRSIZE));
		}
}



/* --------------------------------------------- */
/* dostruncate - */
/* --------------------------------------------- */
dostruncate (mp, size)
struct	info	*mp;		/* file descriptor for DOS directory         */
int		size;		/* truncate the file to this number of bytes */
				/* rounded up to the next cluster boundary   */
{
	int			newcls;
	int			cls;
	struct	header		*hd;
	struct	dir_entry	*dir;

	dir = &mp->dir;
	hd = mp->hdr;

	/* find the first cluster which needs to be deallocated */
	if (size <=0) {
		cls = dir->cls;
		if (dir->name[0] != DIRERASED)
			dir->cls = CLSEOF(hd->twelve_bit_fat);
	} else {
		/* seek out to the proper cluster */
		switch (gdaddr(mp, size, FALSE)) {
		  case  1 : 	break;
		  case  0 : 	return (0);
		  case -1 :	return(-1);
		  default : 	return (-1);
		  } 
		cls = mp->clsvalue;
		put_fat_entry (hd, mp->cls, CLSEOF(hd->twelve_bit_fat));
		}
	mp->clsoffset = -1;
		
	/* Mark the clusters available */
	while (!cls_eof(hd, cls)) {
		if ((newcls = get_fat_entry (hd, cls)) == -1)
		    return(-1);
		if (put_fat_entry (hd, cls, 0) == -1)
			return (-1);
		cls = newcls;
	}

	if (dir->name[0] != DIRERASED)
		dir->size = MIN(size, dir->size);
	return(0);
}


/* --------------------------------------------- */
/* find_entry - locate a file in a dos directory */
/* --------------------------------------------- */
find_entry (mp, fname, mode)
struct	info	*mp;
uchar		*fname;
int		mode;	/* fcntl.h for explanation. same as mode */
{
	uchar			name[9];
	uchar			ext[4];
	struct	dir_entry	dir;
	int			empty;
	int			phys_end;
	long			onceapon;
	int			i;
	int			ret;


	u_to_d_fname (fname, name, ext);

	/* search the directory */
	for (i=0, empty = -1, phys_end = 0;; i++)  {

		/* If root directory, check for physical end of directory.
		   Entries are numbered from 0 thru (dir_entries-1). */
		if (mp->dir.cls == 0 && i >= mp->hdr->dir_entries) {
			phys_end = 1;
			break;
		}

		/* cant check directory size; usually 0 */
		if ((ret = read_dir_entry (mp, i, &dir)) < 0)
			return(ret);

		/* check for an erased directory entry */
		if (dir.name[0] == DIRERASED) {
			if (empty == -1) empty = i;
			continue;
			}

		/* check for end of directory */
		if (dir.name[0] == DIRUNUSED) 
			break;

		/* compare file names */
		if (   strcmp (dir.name, name)== 0 
		    && strcmp (dir.ext, ext) == 0){
			if ((mode & O_EXCL) && (mode & O_CREAT)) {
			  fprintf (stderr, "Error: file already exists (%s)\n",
				   fname);
			  return (-1);
			  }
			else return(i);
		}
	}
	
	/* file not found */
	if ((mode & O_CREAT) != O_CREAT) return(-1);

	/* Root directory full. */
	if (phys_end == 1 && empty == -1) {
		fprintf (stderr, "Root directory full.\n");
		return (-1);
	}
	
	/* ------------------------------------------------------------ */
	/*  If an end of directory is reached without finding an empty  */
	/*  directory entry we must write a new end of directory marker */
	/*  Do not do write the marker if this is last entry in root.   */
	/* ------------------------------------------------------------ */
	if (empty == -1) {
		if (mp->dir.cls != 0 || i != mp->hdr->dir_entries - 1)
			if (write_dir_entry (mp, i+1, &dir) == -1)
				return (-1);
		empty = i;
		}


	/* --------------------------------------------------------------- */
	/* build up a new directory entry. An "ordinary" file is created.  */
	/* This can be changed or embellished later.                       */
	/* --------------------------------------------------------------- */
	strcpy (dir.name, name);
	strcpy (dir.ext, ext);
	dir.size = 0;
	dir.attr = ATTARCHIVE;
	dir.cls = CLSEOF(mp->hdr->twelve_bit_fat);
	{	struct	tm *tm;
		onceapon = time((long *)0);
		tm = localtime (&onceapon);
		if (tm->tm_year < 80)
			dir.year = 0;
	   	else dir.year = tm->tm_year - 80;
		dir.month =  tm->tm_mon + 1;
		dir.day =  tm->tm_mday;
		dir.hour =  tm->tm_hour;
		dir.minute =  tm->tm_min;
		dir.second =  tm->tm_sec;
	}
	for (i=0; i<10; i++) dir.reserved[i] = 0;
	if (write_dir_entry (mp, empty, &dir) == -1)
		return (-1);
	return(empty);

}


/* --------------------------------------- */
/* find_file - locate a file on a dos disc */
/* --------------------------------------- */
struct	info*
find_file (hd, path, mode)
struct	header	*hd;
char		*path;
int		mode;
{
	struct	info		*mp;
	struct	dir_entry	dir;
	int			daddr;
	uchar			fname[MAXNAMLEN];
	uchar			*p;
	int			ii;


	/* start search at the root directory */
	if ((mp = openfd (hd, -1)) == NULL)
		return (NULL);
	mp->dir.attr = ATTDIR;
	mp->dir.cls = 0;

	for (;;) {

		/* skip past '/' in the search path */
		while (*path=='/') path++;
		if (*path=='\0')  {
			/* Don't truncate directories.  No error if you try */
			if ((mode & O_TRUNC) && ((mp->dir.attr & ATTDIR)!=ATTDIR)
			    && (dostruncate (mp, 0) == -1))
				return (NULL);
			return(mp);
			}	

		/* make sure we have a directory */
		if (mp->dir.attr & ATTDIR != ATTDIR) {
			fprintf (stderr, "%s ocurred in a file name", fname);
			fprintf (stderr, " path, but is not a directory.\n");
			return (NULL);
			}
		
		/* locate the next component of the filename */
		for (p=fname; *path!='\0' && *path!='/';)
			*p++ = *path++;
		*p='\0';
		
		/* look through directory for file */
		ii = find_entry(mp, fname, (*path=='\0') ? mode : O_RDONLY);
		if (ii==-1) {
			errno = ENOENT;
			return(NULL);
		}
		
		if ((daddr = read_dir_entry (mp, ii, &dir)) < 0)
			return(NULL);

		closefd (mp);
		if ((mp = openfd (hd, daddr)) == NULL)
			return (NULL);
		mp->dir = dir;

			
	}	
}


/* ------------------------------------- */
/* doswrite - write to  DOS or HPUX file */
/* ------------------------------------- */
doswrite (dp, buf, len)
int	dp;
char	*buf;
int	len;
{

	struct	info	*p;
	int		ret;

	if(!dosfile(dp)) {
		ret = write(dp, buf, len);
		if (ret == -1) {
			fprintf (stderr, "Error writing to HPUX file: ");
			fprintf (stderr, "errno = %d\n", errno);
			}
		return (ret);
		}

	p = CFD(dp);
	ret = put_file_data (p, buf, len);	

	/* update the end of file pointer if neccessary */
	if (p->fp > p->dir.size)  {
		p->dir.size = p->fp;
		p->dir_modified = TRUE;
		}
	return (ret);
}
	

/* ------------------------------------ */
/* dosread - read from DOS or HPUX file */
/* ------------------------------------ */
dosread (dp, buf, len)
int	dp;
uchar	*buf;
int	len;
{

	struct	info	*p;
	int		ret;

	if(!dosfile(dp)) {
		ret = read(dp, buf, len);
		if (ret == -1) {
			fprintf (stderr, "Error reading from HPUX file: ");
			fprintf (stderr, "errno = %d\n", errno);
			}
		return (ret);
		}

	
	/* truncate request to amount of data left in file */
	p = CFD(dp);
	if (len+p->fp > p->dir.size)
		len = p->dir.size - p->fp;
	
	if (len == 0)
		return(0);

	return(get_file_data (p, buf, len));
	
}




/* ------------------------ */
/* dosstat - stat DOS  file */
/* ------------------------ */
dosstat (path, dir)
char	*path;
struct	dir_entry	*dir;
{
	int	dp;
	struct	info	*p;

	dp = dosopen (path, O_RDONLY);
	if(!dosfile(dp)) {
		dosclose (dp);
		return(-1);
	}

	p = CFD(dp);
	*dir = p->dir;
	dosclose (dp);
	return(0);
}

/****************/
/* IS_DIRECTORY */
/****************/
is_directory (dp)
int	dp;
{
	struct	info	*p;
	struct	stat	buf;

	if (dp < 0) return (0);
	if(!dosfile(dp)) {
		if (fstat (dp, &buf) != 0)
			return (0);
		
		if ((buf.st_mode & 0170000) == 0040000)
			return (1);
		else	return (0);
	}

	p = CFD(dp);
	if ((p->dir.attr & ATTDIR) == ATTDIR) 
		return (1);
	else	return (0);
}


/* ------------------------------- */
/* dosopen - open DOS or HPUX file */
/* ------------------------------- */
dosopen (path, mode)
char	*path;
int	mode;
{
	char		device[MAXPATHLEN];
	char		*p;
	struct	header  *hd;
	struct	info	*mp;	
	

	/* get the device name */
	for (p=device; *path!=':' && *path!='\0';)
		*p++ = *path++;
	*p = '\0';

	/* if only a filename is present then open HPUX file */
	if (*path=='\0') {
		return(open(device, mode, 0666));
	}	

	path++;

	if ((hd = open_device (device, mode)) == NULL) 
		return(-1);

	mp = find_file (hd, path, mode);
	if (mp == NULL) {
		close_device (hd);
		return(-1);
		}
	return((mp-&info[0])+OFFSET);
}


/* -------------------------------------------------------------------------- */
/* dosclose - close a DOS or HPUX file.  Returns -1 in the event of an error. */
/* -------------------------------------------------------------------------- */
dosclose (dp) 
int	dp;
{
	struct	info	*mp;

	/* check for a HPUX file */
	if (!dosfile(dp))
		return(close(dp));

	mp = CFD(dp);
	if (mp->openc <= 0) {	/* then serious internal error */
		return(-1);
		}

	closefd (mp);
	return (close_device (mp->hdr));
}


