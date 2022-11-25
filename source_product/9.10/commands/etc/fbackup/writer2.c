/* @(#) $Revision: 70.3 $ */

/***************************************************************************
****************************************************************************

	writer2.c

    This file contains the functions necessary to write the three sections
    at the beginning out an output volume (label, volume header, and index),
    plus code to recover from write errors to magnetic tape drives.

****************************************************************************
***************************************************************************/

#include "head.h"
#include <sys/mtio.h>

#ifdef NLS
#define NL_SETN 3	/* message set number */
extern nl_catd nlmsg_fd;
#endif NLS

extern int qicflag;     /* for QIC support */
extern int SuccessfulAutoLoad;     /* for 3480 support. Defined in writer.c */
extern int is3480;     /* for 3480 support. Defined in vdi.c */

void vhdr_cksum(), newvol();
char *myctime();

extern int	recsize,
		outfd,
		trecnum,
		filenum,
		retrynum,
		resumeflag,
                mttype,
		pipefd;
extern char	*recbuf;
extern PADTYPE	*pad;
extern CKPTTYPE	r_ckpt,
		v_ckpt;



/***************************************************************************
    This function fills in the appropriate data for a volume header, and
    then writes that structure to the output file.
***************************************************************************/
int
writevolhdr(outfiletype, n_uses)
     int outfiletype;
     int n_uses;
{
  VHDRTYPE volhdr;
  struct utsname name;
  char str[64];
  int n;
  int size;
  char *type = "volume header";

  (void) strncpy(volhdr.magic, VOLMAGIC, sizeof(volhdr.magic));
  (void) uname(&name);
  (void) strncpy(volhdr.machine,  name.machine,  sizeof(volhdr.machine));
  (void) strncpy(volhdr.sysname,  name.sysname,  sizeof(volhdr.sysname));
  (void) strncpy(volhdr.release,  name.release,  sizeof(volhdr.release));
  (void) strncpy(volhdr.nodename, name.nodename, sizeof(volhdr.nodename));
  (void) cuserid(str);
  (void) strncpy(volhdr.username, str, sizeof(volhdr.username));
  (void) sprintf(str, "%d", recsize);
  (void) strncpy(volhdr.recsize, str, sizeof(volhdr.recsize));
  (void) strcpy(str, myctime(&((long)(pad->begtime))));
  (void) strncpy(volhdr.time, str, sizeof(volhdr.time));
  (void) sprintf(str, "%d", n_uses);
  (void) strncpy(volhdr.mediause, str, sizeof(volhdr.mediause));
  (void) sprintf(str, "%03d", pad->vol);
  (void) strncpy(volhdr.volno, str, sizeof(volhdr.volno));
  (void) sprintf(str, "%d", pad->ckptfreq);
  (void) strncpy(volhdr.check, str, sizeof(volhdr.check));
  (void) sprintf(str, "%d", pad->fsmfreq);
  (void) strncpy(volhdr.fsmfreq, str, sizeof(volhdr.fsmfreq));
  (void) sprintf(str, "%d", pad->idxsize);
  (void) strncpy(volhdr.indexsize, str, sizeof(volhdr.indexsize));

					    /* NOTE: pid is used to make ppid */
  (void) sprintf(str, INTSTRFMT, pad->pid);
  (void) strncpy(volhdr.backupid.ppid, str, sizeof(volhdr.backupid.ppid));

  (void) sprintf(str, INTSTRFMT, pad->begtime);
  (void) strncpy(volhdr.backupid.time, str, sizeof(volhdr.backupid.time));
  (void) sprintf(str, "%d", pad->nfiles);
  (void) strncpy(volhdr.numfiles, str, sizeof(volhdr.numfiles));
  (void) strncpy(volhdr.lang, pad->envlang, sizeof(volhdr.lang));
  (void) strncpy(volhdr.pad, (char*)NULL, sizeof(volhdr.pad));
  
  vhdr_cksum(&volhdr);
  size = sizeof(VHDRTYPE);

  switch (outfiletype) {
  case VDI_REMOTE:
    n = rmt_write(outfd, (char*)&volhdr, size);
    if (n != size) {
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,111, "fbackup(3111): write error on a record in the %s\n")), type);
      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "Remote writevol successful\n");
#endif

    if (!mtoper(outfd, MTWEOF, 1)) {
      msg((catgets(nlmsg_fd,NL_SETN,112, "fbackup(3112): write error on a EOF in the %s\n")), type);
      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "Remote writevol EOF successful\n");
#endif

    break;
  case VDI_MAGTAPE:
    n = vdi_write(outfiletype, outfd, (char*)&volhdr, size);
    if (n != size) {
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,113, "fbackup(3113): write error on a record in the %s\n")), type);
      return(TRUE);
    }
/* test comment for compiler problem */

#ifdef DEBUG_WRTR
    fprintf(stderr, "Mag writevol successful\n");
#endif

    if ((vdi_ops(outfiletype, outfd, VDIOP_WTM1, 1)) != 0) {
      msg((catgets(nlmsg_fd,NL_SETN,114, "fbackup(3114): write error on an EOF in the %s\n")), type);
      return(TRUE);
    }
/* test comment for compiler problem */

#ifdef DEBUG_WRTR
    fprintf(stderr, "Mag writevol EOF successful\n");
#endif

    break;
  case VDI_DAT:
  case VDI_DAT_FS:
    n = vdi_write(outfiletype, outfd, (char*)&volhdr, size);
    if (n != size) {
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,127, "fbackup(3127): write error on a record in the %s\n")), type);
      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "DAT FS writevol successful\n");
#endif

    if ((vdi_ops(outfiletype, outfd, VDIOP_WTM2, 1)) != 0) {
      msg((catgets(nlmsg_fd,NL_SETN,128, "fbackup(3128): write error on an EOF in the %s\n")), type);
      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "DAT FS writevol FSM successful\n");
#endif

    break;
  case VDI_MO:
  case VDI_DISK:
    n = vdi_write(outfiletype, outfd, (char*)&volhdr, size);
    if (n != size) {
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,129, "fbackup(3129): write error on a record in the %s\n")), type);
      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "MO writevol successful\n");
#endif

    break;
  case VDI_REGFILE:
  case VDI_STDOUT:
    n = vdi_write(outfiletype, outfd, (char*)&volhdr, size);
    if (n != size) {
      perror("fbackup(9999)");
      if ((outfd == 1) && ((vdi_errno == EPIPE) || (vdi_errno == ENOSPC)))
        wrtrabort();
      msg((catgets(nlmsg_fd,NL_SETN,115, "fbackup(3115): write error on a record in the %s\n")), type);
      return(TRUE);
    }

#ifdef DEBUG_WRTR
    fprintf(stderr, "File writevol successful\n");
#endif

    break;
  default:
    break;
  } /* end switch */

    return(FALSE);
  
}  /* end writevolhdr */



/***************************************************************************
    This function computes a checksum for the volume header in precisely the
    same fashion as for header and trailer blocks.  It then stores this
    result in the appropriate place within this structure.
***************************************************************************/
void
vhdr_cksum(volhdr)
VHDRTYPE *volhdr;
{
    int lim, sum = 0;
    int *intptr;
    char *chptr;

    chptr = volhdr->checksum;
    lim = sizeof(volhdr->checksum);
    while (lim--)
	*chptr++ = '\0';

    intptr = (int*)volhdr;
    lim = sizeof(VHDRTYPE)/sizeof(int);
#ifdef DEBUG_CKSUM
fprintf(stderr, "VOL HDR: lim = %d\n", lim);
#endif 
    while (lim--)
	sum += *intptr++;
#ifdef DEBUG_CKSUM
fprintf(stderr, "VOL HDR: sum = %d\n", sum);
#endif 
    (void) sprintf(volhdr->checksum, INTSTRFMT, sum);
}  /* end vhdr_cksum */


/***************************************************************************
    This function is used when the writer wants to update the volume numbers
    in the flist.  When the writer requires a new volume, it would like
    to have the volume numbers incremented for all the files in the flist
    which didn't get put on any previous volumes.  Unfortunately, the main
    process is the keeper of the flist, so the writer must request that this
    update take place.  That is what callupdateidx does.  It arranges so the
    main process will call updateidx by setting pad->idxcmd to UPDATEIDX
    (The main process' writer signal handler checks for things to do every
    time is gets an interrupt.)  When it is done updating the index (by
    changing the volume number values in the flist) updateidx signals that
    it is done by sending a null byte in the pipe between the main and the
    writer process.
***************************************************************************/
void
callupdateidx(afterfile)
int afterfile;
{
    char ch = 'A';	/* could be any non '\0' char */

    pad->updateafter = afterfile;
    pad->idxcmd = UPDATEIDX;
    (void) kill(pad->ppid, SIGUSR1);
    while (ch != '\0') {
	(void) read(pipefd, &ch, 1);
    }
    pad->idxcmd = NOACTION;
}  /* end callupdateidx */


/***************************************************************************
    This function causes the main process to send the index over the pipe
    between it and this (the writer) process.  Writeidx receives the data
    and buffers it up until it gets a full recsize bytes of it, and then
    writes it all to the output file.  There may be one partial record at
    the end.
***************************************************************************/
int
writeidx(outfiletype)
     int outfiletype;
{
    int n, idx = 0;
    int m;
    char *type = "index";

    pad->idxcmd = SENDIDX;
    (void) kill(pad->ppid, SIGUSR1);
    for (;;) {
	n = read(pipefd, &recbuf[idx], (unsigned)(recsize-idx));
	idx += n;					/* n always >= 1 */
	if (recbuf[idx-1] == '\0') {
	    n = rndup(idx);
	    while (idx < n)
		recbuf[idx++] = FILLCHAR;
	    switch (outfiletype) {
	    case VDI_REMOTE:
	      m = rmt_write(outfd, recbuf, n);
 	      if (m != n) {
		perror("fbackup(9999)");
		msg((catgets(nlmsg_fd,NL_SETN,116, "fbackup(3116): write error on a record in the %s\n")), type);
		return(TRUE);
	      }
	      break;
	    case VDI_MAGTAPE:
	      m = vdi_write(outfiletype, outfd, recbuf, n);
	      if (m != n) {
		perror("fbackup(9999)");
		msg((catgets(nlmsg_fd,NL_SETN,117, "fbackup(3117): write error on a record in the %s\n")), type);
		return(TRUE);
	      }
	      break;
	    case VDI_DAT:
	    case VDI_DAT_FS:
	      m = vdi_write(outfiletype, outfd, recbuf, n);
	      if (m != n) {
		perror("fbackup(9999)");
		msg((catgets(nlmsg_fd,NL_SETN,130, "fbackup(3130): write error on a record in the %s\n")), type);
		return(TRUE);
	      }
	      break;
	    case VDI_MO:
	    case VDI_DISK:
	      m = vdi_write(outfiletype, outfd, recbuf, n);
	      if (m != n) {
		perror("fbackup(9999)");
		msg((catgets(nlmsg_fd,NL_SETN,130, "fbackup(3130): write error on a record in the %s\n")), type);
		return(TRUE);
	      }
	      break;
	    case VDI_REGFILE:
	    case VDI_STDOUT:
	      m = vdi_write(outfiletype, outfd, recbuf, n);
	      if (m != n) {
		perror("fbackup(9999)");
		if ((outfd == 1) && ((vdi_errno == EPIPE) || (vdi_errno == ENOSPC)))
		wrtrabort();
		msg((catgets(nlmsg_fd,NL_SETN,118, "fbackup(3118): write error on a record in the %s\n")), type);
		return(TRUE);
	      }
	      break;
	    default:
	      break;
	    } /* end switch */

	    break;
      }
    
	if (idx >= recsize) {
	  switch (outfiletype) {
	  case VDI_REMOTE:
	    m = rmt_write(outfd, recbuf, recsize);
	    if (m != recsize) {
	      perror("fbackup(9999)");
	      msg((catgets(nlmsg_fd,NL_SETN,119, "fbackup(3119): write error on a record in the %s\n")), type);
	      return(TRUE);
	    }
	    break;
	  case VDI_MAGTAPE:
	    m = vdi_write(outfiletype, outfd, recbuf, recsize);
	    if (m != recsize) {
	      perror("fbackup(9999)");
	      msg((catgets(nlmsg_fd,NL_SETN,120, "fbackup(3120): write error on a record in the %s\n")), type);
	      return(TRUE);
	    }
	    break;
	  case VDI_DAT:
	  case VDI_DAT_FS:
	    m = vdi_write(outfiletype, outfd, recbuf, recsize);
	    if (m != recsize) {
	      perror("fbackup(9999)");
	      msg((catgets(nlmsg_fd,NL_SETN,120, "fbackup(3120): write error on a record in the %s\n")), type);
	      return(TRUE);
	    }
	    break;
	  case VDI_MO:
	  case VDI_DISK:
	    m = vdi_write(outfiletype, outfd, recbuf, recsize);
	    if (m != recsize) {
	      perror("fbackup(9999)");
	      msg((catgets(nlmsg_fd,NL_SETN,120, "fbackup(3120): write error on a record in the %s\n")), type);
	      return(TRUE);
	    }
	    break;
	  case VDI_REGFILE:
	  case VDI_STDOUT:
	    m = vdi_write(outfiletype, outfd, recbuf, recsize);
	    if (m != recsize) {
	      perror("fbackup(9999)");
	      if ((outfd == 1) && ((vdi_errno == EPIPE) || (vdi_errno == ENOSPC)))
	      wrtrabort();
	      msg((catgets(nlmsg_fd,NL_SETN,121, "fbackup(3121): write error on a record in the %s\n")), type);
	      return(TRUE);
	    }
	    break;
	  } /* end switch */
	  idx = 0;
	}
      }
    pad->idxcmd = NOACTION;
    return(FALSE);
}  /* end writeidx */


/***************************************************************************
    This function is called whenever a write error to a tape drive occurs.
    It attempts to let the user decide whether to keep the good data (before
    the error) on the volume and continue on another (have a partially used
    volume N, and continue on volume N+1), or toss this volume and continue
    from the appropriate point in the session on a new volume (start volume
    N over from its beginning).
	First tapeerr rewinds the tape to the previous checkpoint record and
    reads it.  If it can't read this checkpoint record, the user gets no
    choice, the volume must be tossed.  The recovered checkpoint record is
    compared with the in-memory volume checkpoint record.  If they match,
    again, the user gets no choice, the volume is tossed.  (There is no good
    data to save on this volume anyway).
	If the recoverd ckpt rec does NOT match the in-memory vol ckpt rec,
    but it DOES match the in-mem record ckpt record, the user is asked if
    s/he would like to save what can be salvaged of this volume or toss it
    and start this vol over.  If he chooses to toss it, it so behaves.
	In either case, tapeerr resets the appropriate variables to the
    values they had at the time the corresponding ckpt was written.  A new
    volume is requested, the main process is reset (similar to what happens
    when an active file is discovered), as is this (the writer) process.
    This eventually results in a "resume" occurring in the main process.
***************************************************************************/
int
tapeerr(outfiletype, outfptr, readingfname, ht_ctr, d_ctr, tmpdatasize,
	r_internalckpt, v_internalckpt)
     int outfptr;
     int outfiletype;
     int *readingfname, *ht_ctr, *d_ctr;
     off_t *tmpdatasize;
     INTCKPTTYPE *r_internalckpt, *v_internalckpt;
{
    CKPTTYPE *ckpt = (CKPTTYPE*)recbuf;


    switch (outfiletype) {
    case VDI_REMOTE:
      msg((catgets(nlmsg_fd,NL_SETN,101, " at media record %d\n")), trecnum);
      msg((catgets(nlmsg_fd,NL_SETN,102, "fbackup(3102): attempting to make this volume salvagable\n")));
      (void) mtoper(outfd, MTBSF, 1);
      break;
    case VDI_MAGTAPE:
      msg((catgets(nlmsg_fd,NL_SETN,101, " at media record %d\n")), trecnum);
      msg((catgets(nlmsg_fd,NL_SETN,102, "fbackup(3102): attempting to make this volume salvagable\n")));
      vdi_ops(outfiletype, outfd, VDIOP_BSF, 1);
      break;
    case VDI_DAT:
    case VDI_DAT_FS:
      msg((catgets(nlmsg_fd,NL_SETN,101, " at media record %d\n")), trecnum);
      msg((catgets(nlmsg_fd,NL_SETN,102, "fbackup(3102): attempting to make this volume salvagable\n")));
      vdi_ops(outfiletype, outfd, VDIOP_BSF, 1);
      break;
    }
    if (!readtrec(outfiletype, recbuf, recsize))
	*ckpt = v_ckpt;		    /* ensure this vol is tossed (see below) */
    switch (outfiletype) {
    case VDI_REMOTE:
      (void) mtoper(outfd, MTBSF, 1);
      rmt_close(outfd);
      break;
    case VDI_MAGTAPE:
      vdi_ops(outfiletype, outfd, VDIOP_BSF, 1);
      if (qicflag)       /* To avoid single EOF in QIC */
	 vdi_ops(outfiletype, outfd, VDIOP_WTM1, 1);
	 /* on 3480 don't close the device. we have to put it offline later. */
      if (!is3480)
      	vdi_close(outfiletype, outfptr, &outfd, mttype);
      break;
    case VDI_DAT:
    case VDI_DAT_FS:
      vdi_ops(outfiletype, outfd, VDIOP_BSF, 1);
      vdi_close(outfiletype, outfptr, &outfd, mttype);
      break;
    }
    if (pad->err_file != (char*)NULL)
	(void) system(pad->err_file);

    if (ckptmatch(ckpt, &r_ckpt) && !ckptmatch(&r_ckpt, &v_ckpt) &&
	!query((catgets(nlmsg_fd,NL_SETN,103, "fbackup(3103): rewrite this volume from the beginning?\n")))) {

		    /* no, keep the good data up to here on this (bad) volume */
      switch (outfiletype) {
      case VDI_REMOTE:
	mtoffl(outfptr);
	break;
      case VDI_MAGTAPE:
     /* in 3480 use load_next_reel() to auto load and set SuccessfulAutoLoad. */
	if (is3480) {
		SuccessfulAutoLoad = load_next_reel(outfd);
	} else
		vdi_ops(outfiletype, outfd, VDIOP_SOFFL, 1);
	break;
      case VDI_DAT:
      case VDI_DAT_FS:
	vdi_ops(outfiletype, outfd, VDIOP_SOFFL, 1);
	break;
      }
      filenum = pad->reset.filenum = atoi(r_ckpt.filenum);
      trecnum = atoi(r_ckpt.trecnum);
      pad->reset.blknum = atoi(r_ckpt.blknum);
      retrynum = atoi(r_ckpt.retrynum);
      pad->reset.datasize = atoi(r_ckpt.datasize);
      pad->reset.blktype = r_ckpt.blktype;
      v_ckpt = r_ckpt;
      *readingfname = r_internalckpt->readingfname;
      *tmpdatasize = r_internalckpt->tmpdatasize;
      *ht_ctr = r_internalckpt->ht_ctr;
      *d_ctr = r_internalckpt->d_ctr;
      if (r_ckpt.blktype == HDRFIRST)
        callupdateidx(filenum-1);
      else
        callupdateidx(filenum);
      (pad->vol)++;
    } else {
		/* yes, toss this vol and begin it again on another reel */

      switch (outfiletype) {
      case VDI_REMOTE:
	msg((catgets(nlmsg_fd,NL_SETN,104, "fbackup(3104): writing 2 EOFs and rewinding the tape\n")));
	/* (void) openoutfd(outfptr, outfd); */
	(void) mtoper(outfd, MTWEOF, 2);
	rmt_close(outfd);
	break;
      case VDI_MAGTAPE:
	msg((catgets(nlmsg_fd,NL_SETN,105, "fbackup(3105): writing 2 EOFs and rewinding the tape\n")));
	vdi_ops(outfiletype, outfd, VDIOP_WTM1, 2);
	/* Set 3480 Autoload flag false to force operator intervention on bad tape */
	SuccessfulAutoLoad = 0;
	vdi_close(outfiletype, outfptr, &outfd, mttype);
	break;
      case VDI_DAT:
      case VDI_DAT_FS:
	msg((catgets(nlmsg_fd,NL_SETN,105, "fbackup(3105): writing 2 EOFs and rewinding the tape\n")));
	vdi_ops(outfiletype, outfd, VDIOP_WTM1, 2);
	vdi_close(outfiletype, outfptr, &outfd, mttype);
	break;
      }
      msg((catgets(nlmsg_fd,NL_SETN,106, "fbackup(3106): please mount a good tape\n")));
      filenum = pad->reset.filenum = atoi(v_ckpt.filenum);
      trecnum = atoi(v_ckpt.trecnum);
      pad->reset.blknum = atoi(v_ckpt.blknum);
      retrynum = atoi(v_ckpt.retrynum);
      pad->reset.datasize = atoi(v_ckpt.datasize);
      pad->reset.blktype = v_ckpt.blktype;
      *readingfname = v_internalckpt->readingfname;
      *tmpdatasize = r_internalckpt->tmpdatasize;
      *ht_ctr = v_internalckpt->ht_ctr;
      *d_ctr = v_internalckpt->d_ctr;
      r_ckpt = v_ckpt;
    }
    if (pad->chgvol_file != (char*)NULL)
	(void) system(pad->chgvol_file);
    newvol();
    resumeflag = TRUE;
} /* end tapeerr */






/***************************************************************************
    This function compares two checkpoint records.  It is called after a
    tape write error, when the tape has been rewound to the previous
    checkpoint, and the checkpoint record read from the tape.  The one just
    read from the tape is compared with the copy kept in memory.
***************************************************************************/
int
ckptmatch(ckpt1, ckpt2)
CKPTTYPE *ckpt1, *ckpt2;
{
    return(
	!strcmp(ckpt1->filenum, ckpt2->filenum) &&
	!strcmp(ckpt1->trecnum, ckpt2->trecnum) &&
	!strcmp(ckpt1->retrynum, ckpt2->retrynum) &&
	!strcmp(ckpt1->datasize, ckpt2->datasize) &&
	(ckpt1->blktype == ckpt2->blktype) &&
	((ckpt1->blktype == HDRFIRST) ||
				!strcmp(ckpt1->blknum, ckpt2->blknum))
    );
}  /* end ckptmatch */



/***************************************************************************
    This function calls xquery function with same argumet.  If xquery returns
    EOF, fbackup will abort.  Otherwise this function returns the value of
    xquery function.
***************************************************************************/

query(inputstr)
char *inputstr;
{
    int retvalue = xquery(inputstr);

    if (retvalue == EOF)
	wrtrabort();

    return (retvalue);
}


/***************************************************************************
    This function is called to read a tape record of not more than "size",
    bytes into the buffer buf.  It is presently used for reading checkpoint
    records after a write error to a tape drive.
***************************************************************************/
int readtrec(outfiletype, buf, size)
     int outfiletype;
     char *buf;
     int size;
{
  switch (outfiletype) {
  case VDI_REMOTE:
    if ((rmt_read(outfd, buf, (unsigned)size) == 0) &&
	(rmt_read(outfd, buf, (unsigned)size) == sizeof(CKPTTYPE)))
    return(TRUE);
    break;
  case VDI_MAGTAPE:
    if ((vdi_read(outfiletype, outfd, buf, (unsigned)size) == 0) &&
	(vdi_read(outfiletype, outfd, buf, (unsigned)size) == sizeof(CKPTTYPE)))
    return(TRUE);
    break;
  case VDI_DAT:
  case VDI_DAT_FS:
    if ((vdi_read(outfiletype, outfd, buf, (unsigned)size) == 0) &&
	(vdi_read(outfiletype, outfd, buf, (unsigned)size) == sizeof(CKPTTYPE)))
    return(TRUE);
    break;
  case VDI_MO:
  case VDI_DISK:
    break;
  case VDI_REGFILE:
  case VDI_STDOUT:
    break;
  }
    msg((catgets(nlmsg_fd,NL_SETN,123, "fbackup(3123): could not read the previous checkpoint record\n")));
    return(FALSE);
}
