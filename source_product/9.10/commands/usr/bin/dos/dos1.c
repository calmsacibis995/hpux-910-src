/* @(#) $Revision: 56.1 $ */     
/* -------------------------------------------------------------------------- */
/*	
	Issues  - Need to eliminate the dependency on an incore FAT
		- should disc space be zero'd on writes to file 
		- when to update directory entries?  each time eof is changed ?
		- where possible move data by words instead of bytes 
		- does the last entry in a directory have to mark the end of
		  the directory or does the code have to watch out for physical
		  end of directory also?
		- Could compare duplicate FATS and tell the user if they are
		  different.
*/
/* -------------------------------------------------------------------------- */
#include	<stdio.h>
#include	<errno.h>
#include	<fcntl.h>
#include	"dos.h"
#include	<signal.h>
#include	<time.h>

char	*strchr (), *strrchr();	

#ifdef	DEBUG
/* ----------------------------- */
/* bugout - debugging print tool */
/* ----------------------------- */
bugout (s, a, b, c, d, e, f, g, h, i)
char	*s;
{
	if (DEBUGON)
		printf (s, a, b, c, d, e, f, g, h, i);
}
#endif	DEBUG

/* ---------------------------------------------- */
/* stoupper - converts a string to upper case     */
/*            conversion starts after finding ':' */
/* ---------------------------------------------- */
stoupper (s)
char	*s;	/* source string */
{
	char	*c;

	/* Only convert to upper case after the : */

	c = strchr (s, ':');
	if (c == NULL) return;
	
	while (*(++c) != '\0') {
		*c = toupper (*c);
	}
}


/* ---------------------------------------------------------- */
/* dostail - find the last last component of a DOS file name. */
/* ---------------------------------------------------------- */
char * dostail (s)
char	*s;
{
	char	*p;

	p = strchr (s, ':');	/* find leftmost colon */
	if (p!=NULL)		/* if we found one     */
		s = p+1;	/* 	skip past it.  */
	p = strrchr (s, '/');	/* find rightmost /    */
	if (p!=NULL)		/* if we find one skip */
		s = p+1;	/* skip past it.       */
	return (s);
}

/* ----------------------------------------------------------- */
/* d.to.u.fname - Convert a dos file name to a unix file name. */
/*		  Trailing blanks in the name and extension    */
/*		  are truncated.                               */
/* ----------------------------------------------------------- */
d_to_u_fname (hpuxname, dosname, dosext)
char	*hpuxname; 	/* must hold at least 13 characters */
char	*dosname;
char	*dosext;
{
	int	i;
	char	*fb;

	/* copy the file name component */
	i = 0;
	*hpuxname = ' ';	/* just in case dosname is NULL string */
	fb = hpuxname+1;	/* Keep at least one blank.		*/
	while (1){
		if ((i++ >= 8) || (*dosname == '\0')) break;
		if (*dosname == ' '){
			if (fb == NULL) fb = hpuxname;
		} else	fb = NULL;
		*hpuxname++ = *dosname++;
	}

	/* Eliminating trailing blanks in the filename.   
	   Note that the use of 'fb' will eliminate the . in hpuxname if 
	   the extension component consists entirely of blanks            
	*/
	if (fb != NULL) hpuxname = fb;
		else fb = hpuxname;
	*hpuxname++ = '.';
		
	/* now copy the extension component elimninating trailing blanks */
	i = 0;
	while (1){
		if ((i++ >= 3) || (*dosext == '\0')) break;
		if (*dosext == ' ') {
			if (fb == NULL) fb = hpuxname;
		} else	fb = NULL;
		*hpuxname++ = *dosext++;
	}
	if (fb != NULL) hpuxname = fb;
	*hpuxname = '\0';
	return (0);
}


/* ----------------------------------------------------------- */
/* u.to.d.fname - Convert a HPUX file name to a dos file name. */
/*		  DOS names are truncated to eight characters  */
/*                with a 3 character extension.  HPUX file     */
/*		  names can't contain the null character.  No  */
/*		  restictions are put on the characters in a   */
/*		  names except for ".".                        */
/* ----------------------------------------------------------- */
u_to_d_fname (fname, name, ext)
char	*fname;
char	*name;
char	*ext;
{
	int	namelen;
	int	extlen;		/* length of file name extension */
	char	*c;

	/* padd the name and extension with blanks */
	strcpy (name, "        ");
	strcpy (ext, "   ");

	if (strcmp (".", fname) == 0) strcpy (name, ".       ");
	  else if (strcmp ("..", fname) == 0) strcpy (name, "..      ");
	    else { c = strchr (fname, '.');
		   namelen = strlen(fname);
		   if (c!=NULL) {
			namelen -= strlen(c);
			c++;
			extlen = strlen(c);
			if (extlen>3) {
				extlen = 3;
fprintf (stderr, "DOS file name extension truncated to three characters: %s\n", fname);
			}
			strncpy (ext, c, extlen);
			}
		   if (namelen > 8) {
			namelen = 8;
fprintf (stderr, "DOS file name truncated to eight characters: %s\n", fname);
		    }
		   strncpy (name, fname, namelen);
		   }
	 return (0);
}

/* -------------------------------------------------------------- */
/* write_block -  Write the contents of the buffer to the device. */
/* -------------------------------------------------------------- */
write_block (cb)
struct	cachebuf	*cb;
{

	int	blocksize;
	int	fd;

	blocksize = cb->hd->blocksize;
	fd = cb->hd->fd;

	/* seek to the correct spot on the device */
	if (   lseek(fd, (long)(cb->block*blocksize+cb->hd->part_off), 0) 
	    != (long)(cb->block*blocksize+cb->hd->part_off))
		{
		fprintf (stderr, "Error seeking. block = %d", cb->block);
		fprintf (stderr, " on device %s\n", cb->hd->dname);
		fprintf (stderr, "Errno = %d\n", errno);
		return(-1);
		}

	/* write the data */
	if (write (fd, cb->buf, blocksize) != blocksize) 
		{
		fprintf (stderr, "Error writing. block = %d", cb->block);
		fprintf (stderr, " on device %s\n", cb->hd->dname);
		fprintf (stderr, "Errno = %d\n", errno);
		return(-1);
		}

	return(0);
}

/* ------------------------------------------------------------------- */
/* release_block -  This routine may not be a no-op depending upon how */
/*                  caching is implemented.                            */
/* ------------------------------------------------------------------- */
release_block (cb)
struct	cachebuf	*cb;
{
	return (0);

}


/* ------------------------------------- */
/* get_block -  */
/* ------------------------------------- */
struct	cachebuf	*get_block (hd, newblock, doread)
struct header *hd;	/* file descriptor for device */
int	newblock;	/* find and return this block */
boolean	doread;		/* when true don't bother reading the block from the */
			/* device if it is not in the cache.  Possible 	     */
			/* performance boost for block copies. */
{

	struct cachebuf	*cb;
	int		fd;
	unsigned	blocksize;
	int		n;
	
	cb = &hd->cb;
	fd = hd->fd;
	blocksize = hd->blocksize;

	/* ---------------------------------------------------------------- */
	/*  check if block is in the cache.  If not, seek to the start of   */
	/* the block and read it in. 					    */
	/* ---------------------------------------------------------------- */
	if ((cb->block != newblock) && doread) {
		if (   lseek(fd, (long)(newblock*blocksize+hd->part_off), 0) 
		    != (long)(newblock*blocksize+hd->part_off))
		  {
		  fprintf (stderr, "Error seeking. block = %d", newblock);
		  fprintf (stderr, " on device %s\n", hd->dname);
		  fprintf (stderr, "Errno = %d\n", errno);
		  return(NULL);
		  }

		/* invalidate block first in case read fails.  No  */
		cb->block = -1;		
		n = read (fd, cb->buf, blocksize);
		if (n != blocksize) {
		  if (n >= 0) {
			/* no error on partial reads */
			/* BUG should the buffer be initialized? */
		  } else {
		    fprintf (stderr, "Error reading. block = %d", newblock);
		    fprintf (stderr, " on device %s\n", hd->dname);
		    fprintf (stderr, "Errno = %d\n", errno);
		    return(NULL);
		  }
		}
	}

	cb->block = newblock;
	return(cb);
}




/* -------------------------------------------------------------------------- */
/* signal_catcher - terminates program when a signal is received.             */
/* -------------------------------------------------------------------------- */
signal_catcher ()
{

	/* first set up to igore all signals */
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGIOT, SIG_IGN);
	signal (SIGTERM, SIG_IGN);
	signal (SIGUSR1, SIG_IGN);
	signal (SIGUSR2, SIG_IGN);

	fprintf (stdout, "Interrupt!  terminating...\n");
	mif_quit ();
	exit (1);
}


/****************************/
/* MIF_QUIT                 */
/****************************/

mif_quit ()
{
	struct	info	*mp;

	/* close all open files? */
	for (mp=info; mp<&info[INFOSIZE]; mp++) {
		if ((mp->openc > 0)  && (mp->dir_modified == TRUE))
			update_dir_entry (mp);
	}


	/* flush all buffers to the disc? */
	while (devices!=NULL)
		close_device (devices); 
}


/* -------------------------------------------------------------------------- */
/* mif_init - initialize data structures.  In general this routine must be    */
/*	      called before any of these support routines are called.         */
/* -------------------------------------------------------------------------- */
mif_init ()
{
	int	i;


	devices = NULL;

	/* initialize file descriptors */
	for (i=0; i<INFOSIZE; i++) info[i].openc = 0;

	/* set up signal catchers */
	signal (SIGINT, signal_catcher);
	signal (SIGQUIT, signal_catcher);
	signal (SIGIOT, signal_catcher);
	signal (SIGTERM, signal_catcher);
	signal (SIGUSR1, signal_catcher);
	signal (SIGUSR2, signal_catcher);
}

/* ------------------------------------------------------------------ */
/* get_disc_data - read raw data from msdos disc.  Transfers data     */
/*                 across block boundaries.                           */
/*		   Returns the number of bytes actually transferred.  */
/* ------------------------------------------------------------------ */
get_disc_data (hd, offset, dest, size)
struct header	*hd;	/* HPUX file descriptor */
int		offset;	/* offset in bytes from start of disc */
uchar		*dest;	/* destination for data */
int		size;	/* number of bytes to read */
{
	uchar	*source;
	int	i;
	int	blocksize;
	int	transfer;
	int	ret;
	struct	cachebuf	*cb;


	blocksize = hd->blocksize;
	if (size <=0)
		return(0);

	if ((cb = get_block (hd, offset/blocksize, TRUE)) == NULL) {
		return(-1);
	}
	ret = size;
	
	/* move data from successive blocks to memory */
	source = cb->buf + offset%blocksize;
	while (size > 0) {
		transfer = MIN (blocksize - offset%blocksize, size);
		for (i=0; i<transfer; i++)  
			*dest++ = *source++;
		size -= transfer;
		offset += transfer;
		release_block (cb);

		if (size > 0) {
		  if ((cb = get_block (hd, offset/blocksize, TRUE)) == NULL)
			return(ret-size);
		  source = cb->buf;
		}
	}
	return(ret);
}


/* ------------------------------------------------------------------ */
/* put_disc_data - write raw data to msdos disc                       */
/*		   returns the number of bytes actually transferred.  */
/* ------------------------------------------------------------------ */
put_disc_data (hd, offset, source, size)
struct	header	*hd;
int		offset;	/* offset in byt from start of disc */
uchar		*source;/* source for data */
int		size;	/* number of bytes to write */
{
	uchar	*dest;
	int	i;
	int	transfer;
	struct	cachebuf	*cb;
	int	blocksize;
	int	ret;


	blocksize = hd->blocksize;
	ret = size;
	if (size <= 0)
		return(0);

	/* don't bother to read the block if our transfer starts on a block 
	   boundary and we are transfering at least one full block of data */
	if ((cb = get_block (hd, 
			     offset/blocksize, 
			     (   (offset%blocksize!=0) 
			      || (size<blocksize)))   ) == NULL) {
		return(-1);
	}

	/* move data from memory to successive blocks on disc*/
	dest = cb->buf + offset%blocksize;
	while (size > 0) {
		transfer = MIN (blocksize - offset%blocksize, size); 
		for (i=0; i<transfer; i++)  
			*dest++ = *source++;
		size -= transfer;
		offset += transfer;

		if (write_block (cb) == -1)
			return(-1);

		/* don't bother to read the block if our transfer starts on a 
		   block boundary and we are transfering at least one full 
		   block of data */
		if (size > 0) {
		  if ((cb = get_block (hd, 
				       offset/blocksize,
				       (size<blocksize))) == NULL)  {
		   	return(ret-size);
		    } else dest = cb->buf;
		}
	}
	return(ret);
}

