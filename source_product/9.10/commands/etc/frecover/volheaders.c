#if defined NLS || defined NLS16
#define NL_SETN 5	/* set number */
#endif

#include "frecover.h"

#define INITBKUPID "NOT YET"
extern char *strcpy();
extern int is3480;	/* For 3480 support. Defined in vdi.c */
extern int is8mm;	/* For 8MM support. Defined in vdi.c */



/********************************************************************/
/*  	    VARIABLES FROM IO	    	    	    	    	    */
extern char *buf;	  	    /* buffer to hold data  	    */
extern char *gbp;    	    	    /* global pointer to buf	    */
/********************************************************************/

/********************************************************************/
/*  	    VARIABLES FROM UTILITIES	    	    	    	    */
extern int datarecsize;       	    /* set in volume header read    */
extern int checkpoints;       	    /* set in volume header read    */
extern int fsmfreq;       	    /* set in volume header read    */
extern int maxsize;      	    /* set in volume header read    */
extern RESTART p;		    /* used for restart only        */
/********************************************************************/

/********************************************************************/
/*  	    VARIABLES FROM MAIN	    	    	    	    	    */
extern char chgvolfile[MAXPATHLEN]; /* volume change file	    */
extern int  volnum;	    	    /* current volume number	    */
extern int  fflag;		    /* full recovery flag	    */
extern int  pflag;		    /* partial recovery flag	    */
extern int  rflag;		    /* restart flag;		    */
extern int  Eflag;		    /* toggle expect expect_eof flag*/
extern int  bflag;		    /* manually select block size   */
extern int  Dflag;		    /* Prevent any fastsearching    */
extern int  dochgvol;		    /* mark of config file	    */
extern char *indexfile;	    	    /* selected file name   	    */
extern FILE *terminal;		    /* descriptor for terminal	    */
extern int Zflag;		    /* response from sampipe not tty*/
extern LISTNODE *ilist;		    /* pointer to include list	    */
extern int indexonly;		    /* 1 if want index or vol hdr   */
extern int fd;   	   	    /* file desc. for backup dev    */
extern int outfiletype;
extern int blocksize;
extern int do_fs;

extern int     n_outfiles;
extern char   *outfptr;
extern struct fn_list *outfile_head;
extern struct fn_list *outfile_temp;

extern struct index_list *index_head;
extern struct index_list *index_tail;

#if defined NLS || defined NLS16
extern nl_catd nlmsg_fd;	    /* used by catgets		    */
#endif
/********************************************************************/

/********************************************************************/
/*	    VARIABLES FROM FILES				    */
extern int filedone;		    /* used to detect termination   */
extern int filenum;		    /* current file number	    */
extern int newvol;		    /* which new volume is expected */
extern char msg[MAXS];		    /* message buffer		    */
extern char firstname[MAXPATHLEN+1];/* first name on tape	    */
extern char lastname[MAXPATHLEN+1]; /* last name read from tape	    */
extern int dircount;		    /* number of entries in a dir   */
extern struct stat statbuf;	    /* file information		    */
extern struct index_list *index_short;
/********************************************************************/

/********************************************************************/
/*		PUBLIC PART OF VOLHEADERS			    */
BKUPID certify;			    /* certify each volume	    */
int indexlen;	    	    	    /* shared by volhdr and index   */
int expect_eof;	    	    	    /* chgreel, ansilab, volhdr	    */
int numfiles;			    /* number of files on backup    */
int pipein = 0;			    /* input form a pipe	    */
int prevvol;			    /* previous volume		    */
int firstfile;			    /* first name on a tape	    */
int firstfile_on_vol;               /* number of first file on current volume */
VHDRTYPE vol;			    /* volume header		    */
/********************************************************************/


char *data;   /* this needs to be global so value lives between frindex calls */

chgreel()
{
  struct stat sbuf;
  static int first = TRUE;	    /* don't prompt first time      */
  int clean;
  int deverr;
  int nomore;
  LISTNODE *t1;
  int statres;
  struct mtop mtop_buf;
  int res;
  char *oldoutfptr;
  int e;
  int SuccessfulAutoLoad = 0;
	/* This is the return value of load_next_reel.  */
	/* It is set to 1 after a successful auto load. */
	/* on the 3480.                                 */
  int manualmode = 0; /* set to 1 if user desires to position the magazine.   */
		      /* in manual mode after incorrect mode was encountered. */


#ifdef DEBUG_VOLHDR
fprintf(stderr, "volheaders.c:: entered proc chgreel \n");
fflush(stderr);
#endif
  clean = TRUE;		    /* assume no errors		    */
  deverr = FALSE;		    /* assume no device errors	    */
  firstfile = TRUE;		    /* first file name to be saved  */
  while(1) {
    if( (fflag) && (numfiles == filenum) && (filedone) )
      done(0);

/*
  If we are doing a partial recovery then we should check if
  we have already recovered all the files wanted, before
  asking to change the volume. Since changing the volume
  might not be necessary.
*/

    if(pflag) {
      while(TRUE) {
	nomore = TRUE;		/* assume we're done */
	if(!filedone)
	  break;		/* in the middle of a file */
	t1 = ilist->ptr;
	while(t1 != (LISTNODE *)NULL) {
	  if(t1->aflag < RECOVER) {
	    if(t1->aflag < SUBSET) {
	      if((mystrcmp(t1->val, firstname) < 0) &&
		 (mystrcmp(t1->val, lastname) > 0)) {
		nomore = FALSE;	/* more to do */
		break;
	      }
	    }
	    nomore = FALSE;		/* more to do */
	    break;
	  }
	  t1 = t1->ptr;
	}
	if(!nomore)		/* we have more to do */
	  break;
	if(((statbuf.st_mode & S_IFMT) == S_IFDIR) &&
	   (dircount > 0) && (numfiles != filenum))
	  break;		/* dir w/ ents on diff vol. */
	if(numfiles == filenum)
	  done(0);		/* all finished */
	break;			/* all other cases */
      } /* while(TRUE) */
    } /* if(pflag) */

/*
  The following code contains the logic for finding out what to
  do at the end of a volume. It is skipped for the first volume.
*/

    if(!first) {

      if(clean) {
	(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,401, "(5401): Volume %d completed.")),volnum);
	warn(msg);
      }
      
      if(dochgvol)
        (void) system(chgvolfile);

/*	
      if(!deverr) {
	(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,428, "(5428): Do you wish to continue")));
	if(reply(msg) != GOOD)
	  done(1);
      }
*/

      if (outfiletype == VDI_REMOTE) {
	mtop_buf.mt_op = MTREW;
	mtop_buf.mt_count = 1;
	res = rmt_ioctl(fd, MTIOCTOP, &mtop_buf);
      }
      else {
	res = vdi_ops(outfiletype, fd, VDIOP_REW, 1);
      }

      if ((res < 0) && (res != VDIERR_OP)) {
	(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,432, "(5432): Media rewind failed")));
	warn(msg);
      }

      /* in 3480 try to auto load the next tape and then proceed. */
      if (is3480 && !manualmode)
	SuccessfulAutoLoad = load_next_reel(fd);
      else
	SuccessfulAutoLoad = 0;
      manualmode = 0; /* reset */

      /* SuccessfulAutoLoad == 1 indicates that the auto load was successful in the 3480. */

      if (outfiletype == VDI_REMOTE) {
	res = rmt_close(fd, MTIOCTOP, &mtop_buf);
      }
      else {
	res = vdi_close(outfiletype, outfptr, &fd, 0);
      }

      if (res < 0) {
	(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,402, "(5402): failed close on device %s")), outfptr);
	warn(msg);
      }

      /* get next device from list, assume that it is ready, unless the
	 new device == old device
       */

/* If the auto load was successful in 3480, skip scanning the output device
 * filename list.
 */
      if (SuccessfulAutoLoad == 1)
		goto end_3480;
      oldoutfptr = outfptr;
      outfile_temp = outfile_temp->next;
      outfptr = outfile_temp->name;

      if (oldoutfptr == outfptr) {
        if (newvol < 0) {
	  (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,410, "frecover(5410): Press return when the previous volume is ready on %s:\n")), outfptr);
        } else {
	  (void) fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,404, "frecover(5404): Press return when the next volume is ready on %s:\n")), outfptr);
        }
	(void) fflush(stderr);
	(void) openterminal();	/* open /dev/tty for interaction */
	(void) getc(terminal);
	(void) fflush(stdin);
      }
end_3480:;		/* label added for 3480 changes. */
	
    } /* end if(!first) */

/* 
  This next block of code primarily checks that the device is usable,
  i.e. can be opened without a problem. 
*/

      /* vdi_identify and open here */
      is3480 = 0;   /* reset is3480 */
      is8mm = 0;   /* reset is8mm */
      if ((e = vdi_identify(outfptr, &outfiletype)) < 0) {
#ifdef DEBUG_VOLHDR
fprintf(stderr, "vdi_identify errno: %d\n", vdi_errno);
fprintf(stderr, "vdi_identify outfptr: %s\n", outfptr);
fprintf(stderr, "vdi_identify outfiletype: %d\n", outfiletype);
#endif
        if (e == VDIERR_NOD) {
	  (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,437, "(5437): Unable to find the device file %s\n")), outfptr);
	}
	else {    /*VDIERR_ID*/
	  (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,403, "(5403): Unable to open or identify device at %s\n")), outfptr);
	}
	warn(msg);
	clean = FALSE;
	first = FALSE;
	continue;
      }
      
      switch (outfiletype) {
      case VDI_REMOTE:
	fd = rmt_open(outfptr, O_RDONLY);
	expect_eof = TRUE;
	break;
      case VDI_MAGTAPE:
	fd = vdi_open(outfiletype, outfptr, O_RDONLY);
	expect_eof = TRUE;
	break;
      case VDI_DAT:
      case VDI_DAT_FS:
	fd = vdi_open(outfiletype, outfptr, O_RDONLY);
	expect_eof = TRUE;
	break;
      case VDI_MO:
      case VDI_DISK:
	fd = vdi_open(outfiletype, outfptr, O_RDONLY);
	expect_eof = FALSE;
	break;
      case VDI_STDOUT:
	pipein++;
	fd = STDIN;
	expect_eof = FALSE;
	break;
      case VDI_REGFILE:
	fd = vdi_open(outfiletype, outfptr, O_RDONLY);
	expect_eof = FALSE;
	break;
      }
/* fprintf(stderr, "special debug for stdin fd = %d\n", fd); */

      if(fd < 0) {
	clean = FALSE;
	first = FALSE;
	deverr = TRUE;
	(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,405, "(5405): unable to open %s")), outfptr);
	warn(msg);
	if(reply((catgets(nlmsg_fd,NL_SETN,406, "(5406): Do you wish to continue"))) == GOOD) 
	  continue;
	panic((catgets(nlmsg_fd,NL_SETN,407, "(5407): abort selected")), !USAGE);
      }
      else {
	deverr = FALSE;
	if (outfiletype == VDI_REMOTE) {
	  statres = rmt_fstat(fd, &sbuf);
	  if ((sbuf.st_mode & S_IFMT) == S_IFREG) {
	    expect_eof = FALSE;
	  }
	}
      }

      /* Toggle expect_eof if -E was specified. */
      if (Eflag) 
        expect_eof = !expect_eof;
    
/*
  Read the ansi label and volume header to "get them out of the way".
  In eariler versions both were un-wanted, but now we need to provide for 
  possible use (dump to a file or stdout) by the admin person.
*/

    if(readvolheader() < 0) {
      if (!pipein) {

	if(reply((catgets(nlmsg_fd,NL_SETN,412, "(5412): Do you wish to try to salvage this volume"))) == GOOD) {
	  warn((catgets(nlmsg_fd,NL_SETN,413, "(5413): attempting to read backup following bad volume header")));
          first = FALSE;
	  return;
	}

	if(reply((catgets(nlmsg_fd,NL_SETN,411, "(5411): Do you wish to try a different volume"))) == GOOD) {
	  first = FALSE;
	  clean = FALSE;
	  if (is3480)
		manualmode = 1;
	  continue;
	}
      }
      panic((catgets(nlmsg_fd,NL_SETN,414, "(5414): abort selected")),!USAGE);
    }
    first = FALSE;
    return;
  } /* while(1) */
}

readvolheader()
{
    int ret;        /*Store the return value to insure that both ansilab
                     *and volhdr get called, even if ansilab fails
                     */
    ret = ansilab();

    if(!volhdr() || !ret)
        return(-1);

    return(0);
}

ansilab()
{
  LABELTYPE vollabel;

  if ((vdi_read(outfiletype, fd, (char *)&vollabel, sizeof(vollabel))) < 0) {
    warn((catgets(nlmsg_fd,NL_SETN,408, "(5408): not an fbackup volume; unable to read volume label")));
    return(0);
  }
	
  if(expect_eof) {
    readmark();        /*If there is a file marker here, read it*/
  }

  return(1);
}

volhdr()
{
  int badvol;			
  int vhsize;       /*Size of the vol header read: determines header rev*/
  int wantvol;      /*The volume number we expect to find*/
  static int vals_not_set = TRUE;
  static int made_up_vals = FALSE;
  char *lang;
  static char *envp1, *envp2;   /*The new values for the LANG variable*/
  int res;
	
#ifdef DEBUG_VOLHDR
fprintf(stderr, "volheaders.c:: entered proc volhdr\n");
fflush(stderr);
#endif
  badvol = FALSE;			/* assume it's a good volume */

  /* Because we rely on empty fields on old backups (pre-8.0), we need 
   * to be sure the the volheader is filled with 0s before we read-in
   * a volheader which is possibly short.
   */
  memset (&vol, 0, (size_t) sizeof(vol));

  /* On devices with no EOF marks, in order to accomodate the smaller header
   * size of pre-8.0 backups, we need to read a small volume header (since
   * the larger ones have only null-padding beyond the smaller size, this 
   * does not cause problems), then read the extra bytes as necessary.
   * 
   * MO drives cannot have the smaller header sizes, since all transfers
   * must be in even multiples of the sector size.
   *
   * On devices with EOF marks, the read of the larger size will terminate
   * early for smaller headers, so this can be used to determine the correct
   * blocksize for the rest of the backup.
   */
#define SMALLHEADER 1536   /*The size of the smaller of the two headers*/

  if ((expect_eof || outfiletype == VDI_MO || bflag == 1024) && bflag != 512) {
    vhsize = sizeof (vol); /* New, bigger header (2048 bytes) */ 
  } else {
    vhsize = SMALLHEADER;
  }

  if ((vhsize = vdi_read(outfiletype, fd, (char *)&vol, vhsize)) < 0) {
    warn((catgets(nlmsg_fd,NL_SETN,409, "(5409): unable to read volume header")));
    badvol = TRUE;
  }

  /* If there are EOF's, that size of the last read is used to
   * determine the blocksize for the rest of the backup.  
   * If the device does not have EOFs, then we rely on the position of the
   * index relative to the header to check blocksize (see the code in frindex).
   * Another good reason to reset the blocksize is that the user requested it.
   */
  if ((expect_eof && vhsize == SMALLHEADER && bflag != 1024) || bflag == 512) {
      blocksize = BLOCKSIZE_PRE_8_0;
  }

  if(vhsize != SMALLHEADER  &&  vhsize != sizeof (vol)) {
    warn((catgets(nlmsg_fd,NL_SETN,418, "(5418): not an fbackup volume; unable to read volume header")));
    badvol = TRUE;
  } else {

    if(vhdrcksum(&vol, vhsize) != 0) {
      warn((catgets(nlmsg_fd,NL_SETN,419, "(5419): checksum on volume header is incorrect.")));
      badvol = TRUE;
    }
  
    if(strcmp(vol.magic, VOLMAGIC) != 0) {
      warn((catgets(nlmsg_fd,NL_SETN,420, "(5420): not an fbackup volume; magic value is incorrect")));
      badvol = TRUE;
    }
  }
	
  if(!badvol) {
    if( (strcmp(certify.ppid, INITBKUPID) == 0) &&
       (strcmp(certify.time, INITBKUPID) == 0) ) {
      (void) strcpy(certify.ppid, vol.backupid.ppid);
      (void) strcpy(certify.time, vol.backupid.time);
    }
    if( (strcmp(vol.backupid.ppid, certify.ppid) != 0) ||
       (strcmp(vol.backupid.time, certify.time) != 0) ) {
      warn((catgets(nlmsg_fd,NL_SETN,421, "(5421): volume identification does not match the current session")));
      badvol = TRUE;
    }
  }

  if(!rflag) {
    prevvol = volnum;
  }
  else {
    if( (strcmp(vol.backupid.ppid, p.vol.backupid.ppid) != 0) ||
       (strcmp(vol.backupid.time, p.vol.backupid.time) != 0) ) {
      panic((catgets(nlmsg_fd,NL_SETN,422, "(5422): incorrect backup selected on restart")), !USAGE);
    }
    rflag = 0;
  }

  if(badvol && vals_not_set) {		/* try to use this volume */
    checkpoints = 32;			/* default values chosen at */
    datarecsize = 16384;		/* fbackup defaults */
    fsmfreq = 200;
    gbp = buf = fmalloc(datarecsize);
    indexlen = 16384;
    maxsize = datarecsize;
    volnum = 1;
    made_up_vals = TRUE;
    vals_not_set = FALSE;
    do_fs = FALSE;
  }
  
  if(!badvol) {
    checkpoints = atoi(vol.check);
    fsmfreq = atoi(vol.fsmfreq);
    datarecsize = atoi(vol.recsize);
    if (made_up_vals)
    free(buf);
    gbp = buf = fmalloc(datarecsize);
    indexlen = atoi(vol.indexsize);
    maxsize = datarecsize;
    volnum = atoi(vol.volno);
    made_up_vals = FALSE;
    vals_not_set = FALSE;


    /*
      If we are dealing with a DAT device and the tape was made on a
      7.0 system then the tape does not have setmarks.  This difference
      will be remembered by setting outfiletype = VDI_MAGTAPE.  Outfiletype
      = VDI_DAT_FS along with the do_fs flag indicates that setmarks are
      expected and frecover can fast search to them.  Fastsearching is also
      turned off (to save memory for the in-core copy of the index) if there
      were no included files specified for the recovery (the index would
      be useless here, since there's nothing to fastsearch to).

      The 8.01 700 system was shipped with the 7.X version so if we have
      a DAT tape made there we need to pretend that we have a 7.X version.

      Also, given customer penchant for running new and old versions on
      different OS releases, the existance of setmarks is determined by
      checking for the existance of the field "fsmfreq" in the volume header,
      rather than relying on the release string.
    */
/*
 * for 8MM devices check if the format allows setmarks. The 8MM tape driver
 * allows the determination of the format only after a read/write to the media
 * hence the re-identification is done here.
 */

    if ((outfiletype == VDI_DAT) || (outfiletype == VDI_DAT_FS)) {
      if (strlen (vol.fsmfreq) == 0 || fsmfreq == 0 ||
          ilist->ptr == NULL || Dflag || (is8mm && !setmarks_allowed_8mm(fd))) {
	outfiletype = VDI_MAGTAPE;
      }
      else {  /* expect set marks */
	do_fs = TRUE;
      }
    }

#if defined NLS || defined NLS16

    lang = getenv("LC_COLLATE");
    if (lang == (char *)NULL || strcmp(lang, "") == 0)
      lang = getenv("LANG");
    if ( strcmp(lang, "n-computer") == 0 || strcmp(lang, "C") == 0)
      lang = "";
    if ( strcmp(vol.lang, "n-computer") == 0 || strcmp(vol.lang, "C") == 0)
      strcpy(vol.lang, "");
    if(strcmp(vol.lang, lang) != 0) {
      (void)sprintf(msg, (catgets(nlmsg_fd,NL_SETN,429, "(5429): backup volume was created with LC_COLLATE or LANG environment\nvariable set as %s, and must be recovered with same: Resetting to %s.")), vol.lang, vol.lang);
      if(strcmp(vol.lang, "") == 0) {
        (void)sprintf(msg, (catgets(nlmsg_fd,NL_SETN,431, "(5431): backup volume was created with LANG and LC_COLLATE\nvariables unset.  Unsetting LANG and LC_COLLATE and continuing.")));
      }
      warn(msg); 
      
      /*Set both, for good measure*/
      envp1 = malloc (sizeof(char) * (12 + strlen(vol.lang)));
      envp2 = malloc (sizeof(char) * (6  + strlen(vol.lang)));
      sprintf (envp1, "LC_COLLATE=%s", vol.lang); 
      putenv (envp1);
      sprintf (envp2, "LANG=%s", vol.lang);
      putenv (envp2);

      catclose (nlmsg_fd);
      if (!setlocale(LC_ALL, "")) {
        fputs(_errlocale("frecover"), stderr);
        putenv("LANG=");
      }
      nlmsg_fd = catopen("frecover", 0);
    }
#endif  /* NLS || NLS16 */

  }

/*
  It can be that a recover starts in the middle of a set of
  tapes.  This is not an error.  We then need to check with
  the user and set up previous volume so it looks like
  we have read all the earilier ones.  The following logic
  is skipped if we are only extracting the index or volume
  header.

  Determine the value of wantvol (the expected volume) based on 
  the positive or negative value of newvol.
*/

  if (newvol >= 0)          /*General case: go to _next_ volume*/
    wantvol = prevvol + 1;
  else 
    wantvol = prevvol - 1;

  if (wantvol < 1)          /*We're trying to go before the first vol*/
    wantvol = 0;

  if((wantvol != volnum)  && (indexonly == 0) && (!badvol)) {
    warn((catgets(nlmsg_fd,NL_SETN,423, "(5423): incorrect volume mounted;")));
    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,424, "(5424): expected volume %d, and got %d")),wantvol,volnum);
    warn(msg);
    if (!pipein) {
      if(reply((catgets(nlmsg_fd,NL_SETN,433, "(5433): Do you wish to continue using this volume"))) == GOOD) {
	prevvol = volnum -1;
	warn((catgets(nlmsg_fd,NL_SETN,434, "(5434): attempting to resynchronize after volume header")));
      }
      else {
	volnum = prevvol;
	return(0);
      }
    }
  }

  numfiles = atoi(vol.numfiles);
  
  if(expect_eof) {
    readmark();        /* If there should be a file marker here, read it*/
  }

  if(badvol)
    return(0);
  else
    return(1);
}  /* end volhdr */

frindex(val)
int val;
{
  int bp, i, count;
  int n;
  int indexpad;
  char *datapos;
  char *s1;
  char *tok, *linkstr;
  char c[10];

  struct index_list *in;
  struct index_list *in2;
  struct index_list *in3;
  struct index_list *tmp_head;
  struct index_list *tmp_tail;
  struct index_list tmp;
  char pathname[MAXPATHLEN];
  int fnum;
  int obp = 1;
  int num = 1;
  int oldvolnum = 1;
  FILE *indfp = 0;

  fnum = TRUE;
  tmp.path = pathname;

  if(!val) {
    do_fs = FALSE;
    if(strcmp(indexfile, "-") == 0)
      indexfile = "/dev/tty";
    if( (indfp = fopen(indexfile, "w")) == 0) {
      panic((catgets(nlmsg_fd,NL_SETN,425, "(5425): unable to open index file")),!USAGE);
    }
  }

  count = 0;
  indexpad = (blocksize - (indexlen % blocksize)) + indexlen;

  if (val && do_fs) {
    if (index_head == NULL) {
      if ((data = malloc((unsigned) indexpad)) == (char *)NULL) {
	panic((catgets(nlmsg_fd,NL_SETN,435, "(5435): malloc error while attempting to read index")), !USAGE);
      }
    }
    datapos = data;
  }

  /* If the device does not have EOF markers, then we are not yet sure of
   * the size of the blocks used.  Since these devices (except MO) won't mind
   * reading one byte at a time, we read the byte which follows the short
   * volume header, which should be the first character of the index ('#')
   * for a short volume header and a NULL for the larger blocksize.  
   * If the user specified the blocksize as 1024, then we already confidently
   * read the entire volume header, and can safely skip this chunk of code.
   */
  if (!expect_eof && outfiletype != VDI_MO && bflag == 0) {

    bp = vdi_read(outfiletype, fd, buf, 1);

    if (*buf != '#' || bp != 1) { /* The header is of the newer variety, or
				   * there's a read error, assume new sizes*/
      
      bp = vdi_read(outfiletype, fd, buf, 511); /*Read past the padding*/

    } else {           /* We have read the first character of the index*/
      count = -1;      /* Use this as a signal to the following loop that
	        	* the first character has been read already.
			* This also means that the smaller headers being
		 	* used are part of the smaller blocksizes from
			* the pre-8.0 days.  Reset the blocksize
			*/

      blocksize = BLOCKSIZE_PRE_8_0;
      indexpad = (blocksize - (indexlen % blocksize)) + indexlen;
    }
  }

  while(count < indexpad) {
    if (count < 0){    /* First character has already been read */
      
      count = -count;
      i = MIN(indexpad - count, datarecsize - count);

      if(i == 0)
        i = blocksize;		/* force an error */

      bp = vdi_read(outfiletype, fd, buf + count, (unsigned)i);
      
      bp += count;
      count = bp;

    } else {
      i = MIN(indexpad - count, datarecsize);

      if(i == 0)
        i = blocksize;		/* force an error */

      bp = vdi_read(outfiletype, fd, buf, (unsigned)i);

      count += bp;
    }

    if(bp < 0 || (obp == 0 && bp == 0))  {
      warn(catgets(nlmsg_fd,NL_SETN,426, "(5426): read error while attempting to read index.  Skipping.")); 
      do_fs = FALSE;
      break;
    }
    obp =  bp;

/* 
  Keep in memory copy of index for DAT fast search.
  First put all blocks into one big string, then after
  all the index is in memory parse and cut up into pieces
  for the linked list of names.
*/

    if (val && do_fs) {
      memcpy(datapos, buf, bp);
      datapos += bp;
    }
    if(!val) {
      if(count > indexlen)
        bp = bp - (count - indexlen);
      (void) fwrite(buf, sizeof(char), bp, indfp);
    }


/*  The following is to add tolerance into reading a broken
    tape, i.e. where the index size in the header is bigger than
    the actual index size.  This is seen when bp (bytes read) < i
    (the bytes requested).  This comes from hitting the EOF on the
    tape. The EOF cannot be "eaten" here as several other places
    in the code depend on reading the EOF-checkpoint combination.

    However, if we are reading from stdin then there are no EOFs.
    The read statement is set up to read more than PIPE_BUF, so 
    ( bp < i and obp < i ) will always be true.  We have to believe
    that the indexlen is true.
*/

    if ((obp < i) && !pipein) {
      break;
    }
  }
  if(!val)
    (void) fclose(indfp);
  else 
    initname();

/*
  The next block of code deals with the index list that is used for
  DAT fast search.  A major restriction is that the tapes must be 
  accessed in the proper order (first to last).  With the first tape
  a simple linked list is built of all the index entries.  Later 
  (in files.c:preen_index()) This linked list will be shorten to
  only those files which might be of interest according to the include
  options.

  Although the in-memory index could be checked to determine which volume
  should be mounted first (i.e. read the index on the last volume, then
  use the volume numbers in that index to ask for the correct volume),
  the rest of frecover's code does not implement this, so it is currently
  a waste of time to update the in-memory index at the start of each new vol.
 */

    if (do_fs && (index_head == NULL)) {

      s1 = data;
      while ((tok = strtok(s1, "\n")) != NULL) {
	s1 = NULL;

        /* Scanning-in the data from the index can get tricky
         * if a filename contains spaces, so the path is considered the
         * string that goes to the end of the line in the index, and the
         * link indicator (if there is one) is cut off from the string
         * by placing a NULL where that string begins.
         */
        n = sscanf(tok,"%s %*d %[^\n]", c, tmp.path);
        if ((linkstr = strstr (tmp.path, " <LINK> #")) != NULL)
          *linkstr = '\0';

	if ((in = malloc(sizeof(struct index_list))) == (char *)NULL ||
	    (in->path = malloc (strlen (tmp.path) + 1)) == NULL) {

	  warn(catgets(nlmsg_fd,NL_SETN,436, "(5436): malloc error: not enough virtual memory to use fastsearch.\nContinuing without fastsearch capability."));

	  do_fs = FALSE;

	  /*Free the memory held by the index*/
	  for (in=index_head; in != NULL; in=index_tail) {
	    index_tail = in->next;
	    free (in->path);
	    free (in);
	  }

	  index_head = index_tail = NULL;
	  break;
	}

	strcpy (in->path, tmp.path);
	in->num = num++;
	in->next = NULL;

	if (index_head == NULL) {
	  index_head = in;
	  index_tail = in;
	} else {
	  index_tail->next = in;
	  index_tail = in;
	}
      }
    }

  return;
}  /* end frindex() */



setinput()
{
  (void) strcpy(certify.ppid, INITBKUPID);
  (void) strcpy(certify.time, INITBKUPID);

  if (!Zflag) {
    terminal = NULL;	/* mark for not opened */
  }
  else {
    if ((terminal = fopen("/tmp/sampipe", "r+")) == NULL)
    panic((catgets(nlmsg_fd,NL_SETN, 430, "(5430): unable to open /tmp/sampipe")), !USAGE);
  }
}  /* end setinput */

openterminal()
{
    if(terminal == NULL && (terminal = fopen("/dev/tty", "r")) == NULL)
	panic((catgets(nlmsg_fd,NL_SETN,427, "(5427): unable to open controlling terminal")), !USAGE);
}

setup(val)
int val;
{
  newvol = TRUE;
  chgreel();
  newvol = FALSE;
  if (val) {
    frindex(val);
  }
}


int
vhdrcksum(vol,size)
  VHDRTYPE *vol;
  int size;
{
    int checkval, sum=0, lim;
    int *intptr;
    char *chptr;
    int res;

    chptr = vol->checksum;
    checkval = atoi(chptr);
    lim = sizeof(vol->checksum);
    while (lim--)
	*chptr++ = '\0';

    intptr = (int *)vol;  /*integer checksum, stored in an integer*/

   /* Note that the checksum is calculated based on the number of bytes
    * read to compensate for the differences in new and old header sizes.
    */

    lim = size/sizeof(int);
    
    while (lim-- > 0) {
        sum += *intptr++;
    }

    if (sum == checkval) {   
      return (0);
    }

    return((int)IOERROR);
}

/* The following code is for 3480 support. */

/* load_next_reel() attempts to load the next cartridge. It returns
 * 1 if successful, otherwise returns 0.
 */
load_next_reel(mt)
int mt;
{
	int f;
	struct mtop mt_com;

	mt_com.mt_op = MTOFFL;
	mt_com.mt_count = 1;
        if ( ioctl(mt, MTIOCTOP, &mt_com) < 0 ) {
	 	panic(catgets(nlmsg_fd,NL_SETN,438,"(5438): ioctl to offline device failed. aborting...\n"), !USAGE);
	}  

/*
 * The previous ioctl may put the 3480 device offline in the following cases:
 * 1. End of Magazine condition in auto mode.
 * 2. Device in manual/system mode.
 */

/* Check if cartridge is loaded to the drive. */
	if (DetectTape(mt)) {
	  	warn(catgets(nlmsg_fd,NL_SETN,439,"(5439): auto loaded next media.\n"));
		return(1);
	} else {
	  	warn(catgets(nlmsg_fd,NL_SETN,440,"(5440): unable to auto load next media.\n"));
		return(0);
	}
}


/* This function checks if a cartridge is loaded into the 3480 drive. */
DetectTape(mt)
{
	struct mtget mtget;

/*	NOTE:  The code below is commented out due to the 3480 driver
 *	returning an error if the device is offline instead of allowing
 *	ioctl to return an OFFLINE indication (DLM 8/10/93)
 *
 *	if ( ioctl(mt, MTIOCGET, &mtget) < 0 ) {
 *		panic(catgets(nlmsg_fd,NL_SETN,441, "(5441): ioctl to determine device online failed. aborting...\n"), !USAGE);
 *	}
 *
 *	if (GMT_ONLINE(mtget.mt_gstat)) return(1);
 *	else return(0);
 */

	if ((ioctl(mt, MTIOCGET, &mtget) < 0 ) ||
	    (!GMT_ONLINE(mtget.mt_gstat))) return(0);
	else return(1);

}
