/* @(#) $Revision: 70.1 $ */      
#

#include "rcv.h"
#include <sys/stat.h>
#include <errno.h>

#ifndef NLS
#define catgets(i,mn,sn, s) (s)
#else NLS
#define NL_SETN 14	/* set number */
#endif NLS

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Rcv -- receive mail rationally.
 *
 * Termination processing.
 */


/*
 * Save all of the undetermined messages at the top of "mbox"
 * Save all untouched messages back in the system mailbox.
 * Remove the system mailbox, if none saved there.
 */

quit()
{
	int mcount, p, modify, autohold, anystat, holdbit, nohold;
	FILE *ibuf, *obuf, *fbuf, *rbuf, *readstat, *mbuf;
	register struct message *mp;
	register int c;
	extern char tempQuit[], tempResid[];
	struct stat minfo;
	char *id;
	int appending, fd;
	char *mbox = expand(Getf("MBOX"));

	/*
	 * If we are read only, we can't do anything,
	 * so just return quickly.
	 */

	mcount = 0;
	if (readonly)
		return;
	/*
	 * See if there any messages to save in mbox.  If no, we
	 * can save copying mbox to /tmp and back.
	 *
	 * Check also to see if any files need to be preserved.
	 * Delete all untouched messages to keep them out of mbox.
	 * If all the messages are to be preserved, just exit with
	 * a message.
	 *
	 * If the luser has sent mail to himself, refuse to do
	 * anything with the mailbox, unless mail locking works.
	 */

	fbuf = lock(mailname, "r+", OUT);
	if (fbuf == NULL)
		goto newmail;
#ifndef CANLOCK
	if (selfsent) {
		printf((catgets(nl_fn,NL_SETN,1, "You have new mail.\n")));
		unlock(fbuf);
		return;
	}
#endif
	rbuf = NULL;
	if (stat(mailname, &minfo) >= 0 && minfo.st_size > mailsize) {
		printf((catgets(nl_fn,NL_SETN,2, "New mail has arrived.\n")));
		rbuf = fopen(tempResid, "w");
		if (rbuf == NULL)
			goto newmail;
#ifdef APPEND
		fseek(fbuf, mailsize, 0);
		while ((c = getc(fbuf)) != EOF)
			putc(c, rbuf);
#else
		p = minfo.st_size - mailsize;
		while (p-- > 0) {
			c = getc(fbuf);
			if (c == EOF)
				goto newmail;
			putc(c, rbuf);
		}
#endif
		fclose(rbuf);
		if ((rbuf = fopen(tempResid, "r")) == NULL)
			goto newmail;
		remove(tempResid);
	}

	/*
	 * Adjust the message flags in each message.
	 */

	anystat = 0;
	autohold = value("hold") != NOSTR;
	appending = value("append") != NOSTR;
	holdbit = autohold ? MPRESERVE : MBOX;
	nohold = MBOX|MSAVED|MDELETED|MPRESERVE;
	if (value("keepsave") != NOSTR)
		nohold &= ~MSAVED;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & MSTATUS)
			anystat++;
		if ((mp->m_flag & MTOUCH) == 0)
			mp->m_flag |= MPRESERVE;
		if ((mp->m_flag & nohold) == 0)
			mp->m_flag |= holdbit;
	}
	modify = 0;
	if (Tflag != NOSTR) {
		if ((readstat = fopen(Tflag, "w")) == NULL)
			Tflag = NOSTR;
	}
	for (c = 0, p = 0, mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MBOX)
			c++;
		if (mp->m_flag & MPRESERVE)
			p++;
		if (mp->m_flag & MODIFY)
			modify++;
		if (Tflag != NOSTR && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			id = hfield("article-id", mp);
			if (id != NOSTR)
				fprintf(readstat, "%s\n", id);
		}
	}
	if (Tflag != NOSTR)
		fclose(readstat);
	if (p == msgCount && !modify && !anystat) {
		if (p == 1)
			printf((catgets(nl_fn,NL_SETN,3, "Held 1 message in %s\n")), mailname);
		else if (p !=0)
			printf((catgets(nl_fn,NL_SETN,4, "Held %2d messages in %s\n")), p, mailname);
		unlock(fbuf);
		return;
	}
	if (c == 0) {
		if (p != 0) {
			writeback(rbuf, fbuf);
			return;
		}
		goto cream;
	}

	/*
	 * Create another temporary file and copy user's mbox file
	 * darin.  If there is no mbox, copy nothing.
	 * If he has specified "append" don't copy his mailbox,
	 * just copy saveable entries at the end.
	 */

	mcount = c;
	if (!appending) {
		if ((obuf = fopen(tempQuit, "w")) == NULL) {
			perror(tempQuit);
			unlock(fbuf);
			return;
		}
		if ((ibuf = fopen(tempQuit, "r")) == NULL) {
			perror(tempQuit);
			remove(tempQuit);
			fclose(obuf);
			unlock(fbuf);
			return;
		}
		remove(tempQuit);
		if ((mbuf = fopen(mbox, "r+")) != NULL) {
			while ((c = getc(mbuf)) != EOF)
				putc(c, obuf);
		} else if ((mbuf = fopen(mbox, "w")) == NULL) {
			perror(mbox);
			fclose(obuf);
			fclose(ibuf);
			unlock(fbuf);
			return;
		}
		if (ferror(obuf)) {
			perror(tempQuit);
			fclose(mbuf);
			fclose(ibuf);
			fclose(obuf);
			unlock(fbuf);
			return;
		}
		if (ferror(mbuf)) {
			perror(mbox);
			fclose(mbuf);
			fclose(ibuf);
			fclose(obuf);
			unlock(fbuf);
			return;
		}
		fclose(obuf);
	/* when we leave this if, obuf should be mbox where it needs to be
	 * written to
	 */
		obuf = mbuf;
		mbuf = NULL;
		rewind(obuf);

	} else		/* we are appending */
		if ((obuf = fopen(mbox, "a")) == NULL) {
			perror(mbox);
			unlock(fbuf);
			return;
		}
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if (mp->m_flag & MBOX)
			if (send(mp, obuf, 0) < 0) {
				perror(mbox);
				fclose(ibuf);
				fclose(obuf);
				unlock(fbuf);
				return;
			}

	/*
	 * Copy the user's old mbox contents back
	 * to the end of the stuff we just saved.
	 * If we are appending, this is unnecessary.
	 */

	if (!appending) {
		rewind(ibuf);
		c = getc(ibuf);
		while (c != EOF) {
			putc(c, obuf);
			if (ferror(obuf))
				break;
			c = getc(ibuf);
		}
		fclose(ibuf);
		fflush(obuf);
	}
	fflush(obuf);		/* get file up-to-date for ftruncate */
	ftruncate(fileno(obuf), ftell(obuf));
	if (ferror(obuf)) {
		perror(mbox);
		fclose(obuf);
		unlock(fbuf);
		return;
	}
	fclose(obuf);
	if (mcount == 1)
		printf((catgets(nl_fn,NL_SETN,5, "Saved 1 message in %s\n")), mbox);
	else
		printf((catgets(nl_fn,NL_SETN,6, "Saved %d messages in %s\n")), mcount, mbox);

	/*
	 * Now we are ready to copy back preserved files to
	 * the system mailbox, if any were requested.
	 */

	if (p != 0) {
		writeback(rbuf,fbuf);
		return;
	}

	/*
	 * Finally, remove his /usr/mail file.
	 * If new mail has arrived, copy it back.
	 */

cream:
	if (rbuf != NULL) {
		fflush(fbuf);
		rewind(fbuf);
		while ((c = getc(rbuf)) != EOF)
			putc(c, fbuf);
		fclose(rbuf);
		fflush(fbuf);	/* get contents up-to-date for ftruncate */
		ftruncate(fileno(fbuf), ftell(fbuf));
		unlock(fbuf);
		alter(mailname);
		return;
	}
	demail();
	return;

newmail:
	printf((catgets(nl_fn,NL_SETN,7, "You have new mail.\n")));
}

/*
 * Preserve all the appropriate messages back in the system
 * mailbox, and print a nice message indicating how many were
 * saved.  On any error, just return -1.  Else return 0.
 * Incorporate any new mail that we found.
 */

writeback(res, obuf)
	register FILE *res;
	register FILE *obuf;
{
	register struct message *mp;
	register int p, c, fd;

	
	p = 0;

	fflush(obuf);

	/* we are no longer truncating the file to zero length here.  If 
	 * there is no disk space left on a disk, we should be able to put
	 * the mail back into the system mailbox because it is probably getting
	 * smaller.  The only possible hitch comes from the v command, which
	 * could be used to make a mail messsage bigger.  If this happens,
	 * we might not be able to write everything out.  Isn't UNIX great ??
	 */

	rewind(obuf);
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if ((mp->m_flag&MPRESERVE)||(mp->m_flag&MTOUCH)==0) {
			p++;
			if (send(mp, obuf, 0) < 0) {
				perror(mailname);
				unlock(obuf);
				return(-1);
			}
		}
#ifdef APPEND
	if (res != NULL)
		while ((c = getc(res)) != EOF)
			putc(c, obuf);
#endif
	fflush(obuf);
	ftruncate(fileno(obuf), ftell(obuf));
	if (ferror(obuf)) {
		perror(mailname);
		unlock(obuf);
		return(-1);
	}

	if (res != NULL)
		fclose(res);
	alter(mailname);
	if (p == 1)
		printf((catgets(nl_fn,NL_SETN,8, "Held 1 message in %s\n")), mailname);
	else
		printf((catgets(nl_fn,NL_SETN,9, "Held %d messages in %s\n")), p, mailname);
	unlock(obuf);
	return(0);
}
