#if defined NLS || defined NLS16
#define NL_SETN 2	/* set number */
#endif

#include "frecover.h"

/********************************************************************/
/*  	    PUBLIC PART OF IO	    	    	    	    	    */
char *buf;			    /* buffer to hold data	    */
char *gbp;	    	    	    /* global pointer to buf	    */
int ckptcount = 0;		    /* count of checkpoints	    */
int noread = FALSE;		    /* used by UNGETBLK and getblk  */
int tryagain = TRUE;		    /* dev changes between vols     */
/********************************************************************/

/********************************************************************/
/*  	    VARIABLES FROM UTILITIES	    	    	    	    */
extern int datarecsize;       	    /* set in volume header read    */
extern int checkpoints;       	    /* set in volume header read    */
extern int fsmfreq;       	    /* set in volume header read    */
extern int maxsize;      	    /* set in volume header read    */
/********************************************************************/

/********************************************************************/
/*  	    VARIABLES FROM MAIN	    	    	    	    	    */
extern int  pflag;		    /* partial recovery flag	    */
extern int  vflag;		    /* verbose flag		    */
extern int  mflag;		    /* print file marker info	    */
extern int synclimit;		    /* max number of tries to resync*/
extern int blocksize;

extern int outfiletype;
extern int fd;

extern char   *outfptr;

#if defined NLS || defined NLS16
extern nl_catd nlmsg_fd;	    /* used by catgets		    */
#endif
/********************************************************************/

/********************************************************************/
/*	    VARIABLES FROM FILES				    */
extern int filedone;		    /* used to detect termination   */
extern int newvol;		    /* flag to require a new volume */
extern char msg[MAXS];		    /* message buffer 		    */
extern int fsm_move;                /* indicates did a FSS, nuke buf*/
/********************************************************************/

/********************************************************************/
/*	    VARIABLES FROM VOLHEADERS				    */
extern int expect_eof;		    /* backup on disk or tape	    */
/********************************************************************/


char *
getblk()
{
    static char *localptr;		    /* local pointer to buf */
    static int first = 1;		    /* initializer	    */
    char *getbuf();	    	    	    /* performs io  	    */

    if(first || fsm_move) {
      /* this "throws away" the current buffer, so a new one will be read */
	first = 0;
	fsm_move = FALSE;
	localptr = buf + datarecsize;
    }

    if(noread) {
	noread = FALSE;
	return(localptr);
    }
    
    localptr += blocksize;
    if((localptr >= (buf+datarecsize)) || newvol)
    	localptr = getbuf();

    return(localptr);
}


char *
gettbuf()
{
  CKPTTYPE 	    ckprec;	    	    /* used to verify read  */
  int done=0,      /* Used to flag when we've got a successful read */
      eof_count=0; /* Used to count the number of consecutive EOFs  */
  struct vdi_gatt gatt_buf;

  do {
    if (newvol) {        /* Need to change media */
      chgreel();
      newvol = FALSE;    /* Update newvol after reel is changed*/
      frindex(1);
    }
  
    if ((datarecsize = vdi_read(outfiletype, fd, buf, (unsigned)maxsize)) < 0){
      (void) sprintf (msg, catgets (nlmsg_fd,NL_SETN,114, "(2114): read error from input device (%s)"), strerror(vdi_errno));
      warn (msg);
      resync();
      return((char *)IOERROR);
    }

    if (datarecsize == 0) {       /*Either an EOF or a setmark*/

      if (vdi_get_att(outfiletype, fd, VDIGAT_ISTM1 | VDIGAT_ISTM2 |
                                       VDIGAT_ISEOM | VDIGAT_ISEOD,
                                                 &gatt_buf) == VDIERR_ACC){
	resync();
	return((char *)IOERROR);
      }
      if (gatt_buf.tm2) {  /*This is a set mark*/
	eof_count = 0;

	if (mflag)
          warn (catgets(nlmsg_fd,NL_SETN,111,"(2111): DDS setmark read"));

      } else if (gatt_buf.tm1 || gatt_buf.eom || gatt_buf.eod){            
                                         /*This is an EOF mark or end-of-data*/
	if (eof_count) {        /*This is our second consecutive EOF*/
	  eof_count=0;
	  newvol = TRUE;        /*We are at EOM, mount tape before continuing*/
	} else {
	  eof_count++;

	  if (mflag)
	    warn (catgets(nlmsg_fd,NL_SETN,109, "(2109): file marker read"));
	}

      } else {
	eof_count = 0;
          if (mflag) {
            sprintf (msg, catgets (nlmsg_fd,NL_SETN,112, "(2112): unexpected DDS marker read.  Status:"));
            warn (msg);

            if (vdi_get_att(outfiletype, fd, VDIGAT_ISWP | VDIGAT_ISOL |
                                             VDIGAT_ISOB | VDIGAT_ISIR |
                                             VDIGAT_ISDO | VDIGAT_ISBOM,
                                                 &gatt_buf) == VDIERR_ACC){
              resync();
              return((char *)IOERROR);
            }

	    if (gatt_buf.wrt_protect)
              fprintf (stderr, "\tTape is write protected\n");
	    if (gatt_buf.on_line)
              fprintf (stderr, "\tTape drive is on line\n");
	    if (gatt_buf.bom)
              fprintf (stderr, "\tTape is at beginning of media\n");
	    if (gatt_buf.im_report)
              fprintf (stderr, "\tImmediate Reporting enabled\n");
	    if (gatt_buf.door_open)
              fprintf (stderr, "\tDrive door is open\n");
          }
      }
    } else {  /* If datarecsize !=0, it's either a checkpoint or a good read */
      if (datarecsize == sizeof (ckprec) || 
	  datarecsize == sizeof (ckprec)-1) { /*It's a checkpoint record*/
	eof_count = 0;

	if (mflag)
	  warn (catgets(nlmsg_fd,NL_SETN,110,"(2110): checkpoint record read"));
      } else {
	done = 1;    /*!EOF, !SetMark, !Chkpt... must be a good read: leave*/
      }
    }
  } while (!done);

  return(buf);
}  /* end gettbuf */


resync()
{
  static int synccount = 0;

  switch(synclimit) {
  case SYNCDEF:       /*If we're not going to count resync's, then at
                        least use synccount to prevent recursive resync's
		       */
    if (synccount == 0) {
      synccount++;    /*Set the static flag to say we've been here*/
    } else {
      (void) sprintf(msg, catgets(nlmsg_fd,NL_SETN,113, "(2113): unable to resync backup media"));
      panic(msg, !USAGE);
    }
    break;
  default:
    if(synccount > synclimit) {
      (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,102, "(2102): maximum number (%d) resynchronizations exceeded")), synccount);
      panic(msg, !USAGE);
    }
    synccount++;
    break;
  }
  getnexthdr(RESYNC);

  if (synclimit == SYNCDEF) {/*Successful resync: clear the no-recursion flag*/
    synccount = 0;
  }
}  /* end resync() */

char *
getdbuf()
{
  int eom;

retry:
  eom = FALSE;
  datarecsize = vdi_read(outfiletype, fd, buf, (unsigned)maxsize);

  if (datarecsize != maxsize) {  /* check End-Of-Media conditions */
    if (outfiletype == VDI_REMOTE) {
      if( (datarecsize == 0) && (filedone))
        done(0);			/* recovery complete */
      if (datarecsize < 0) {
	panic((catgets(nlmsg_fd,NL_SETN,107, "(2107): IOERROR on a disk device. Frecover exiting")), !USAGE);
      }
      if(datarecsize == 0)
        panic((catgets(nlmsg_fd,NL_SETN,108, "(2108): zero length read from disk device. Frecover exiting")), !USAGE);

    return(buf);
    }  /* end if VDI_REMOTE */

/*
    if( (datarecsize == 0) && (filedone))
        done(0);			/* recovery complete */

    if (datarecsize >= 0) {  /* read a partial record, assume EOM, ignore what was read */
      eom = TRUE;
    }
#ifdef _WSIO  /* 300 and 700 */
    if ((datarecsize < 0) && (vdi_errno == ENOSPC)) {
      eom = TRUE;
    }
#else  /* 800 */
    if ((datarecsize < 0) && (vdi_errno == ENXIO)) {
      eom = TRUE;
    }
#endif
    if (eom) {
      newvol = TRUE;
      chgreel();
      newvol = FALSE;
      frindex(1);
      goto retry;
    }
    else {
      panic((catgets(nlmsg_fd,NL_SETN,107, "(2107): IOERROR on a disk device. Frecover exiting")), !USAGE);
    } 
  }
    return(buf);
} /* end getdbuf */


char *
getfbuf()
{
    datarecsize = vdi_read(outfiletype, fd, buf, (unsigned)maxsize);
    if(datarecsize < 0)
        panic((catgets(nlmsg_fd,NL_SETN,103, "(2103): IOERROR on a disk file. Frecover exiting")), !USAGE);
    if( (datarecsize == 0) && (filedone))
        done(0);			/* recovery complete */
    if(datarecsize == 0)
        panic((catgets(nlmsg_fd,NL_SETN,104, "(2104): zero length read from disk file. Frecover exiting")), !USAGE);
    return(buf);
} /* end getfbuf */


/* Used to read past single marks on the tape: this routine expects that the
 * tape is currently positioned directly in front of a marker either EOF, TM1
 * or TM2.  It returns 0 if there are problems reading from the device, or if
 * the expected marker was not found.
 */
int 
readmark()
{
    int temp_char;
    
    /* If we are sitting at a marker, then read better return 0 bytes */

    if (vdi_read (outfiletype, fd, &temp_char, 1) != 0) {
        warn (catgets(nlmsg_fd,NL_SETN,105, "(2105): did not find expected file marker"));
        return (0);
    }

    return (1);  /*Successful read past the marker*/
}

char *
getbuf()
{
    char *res;
    
    tryagain = TRUE;
    while(tryagain) {
	tryagain = FALSE;		/* set by gettbuf of chgreel */
	switch (outfiletype) {
	case VDI_REMOTE:
	  if(expect_eof)
	    res = gettbuf();
	  else
	    res = getdbuf();
	  break;
	case VDI_MAGTAPE:
	case VDI_DAT:
	case VDI_DAT_FS:
	  res = gettbuf();
	  break;
	case VDI_MO:
	case VDI_DISK:
	  res = getdbuf();
	  break;
	case VDI_REGFILE:
	  res = getfbuf();
	  break;
	case VDI_STDOUT:
	  res = getfbuf();
	  break;
	}
    }
    return(res);
}

