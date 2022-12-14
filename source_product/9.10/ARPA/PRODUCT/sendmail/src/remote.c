/* $Source: /source/hpux_source/networking/rcs/arpa90_800/sendmail/src/RCS/remote.c,v $
 * $Revision: 1.2.109.5 $	$Author: mike $
 * $State: Exp $   	$Locker:  $
 * $Date: 95/03/22 17:19:34 $
 */

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.8
 */

# ifndef lint
static char rcsid[] = "$Header: remote.c,v 1.2.109.5 95/03/22 17:19:34 mike Exp $";
# 	ifndef hpux
static char sccsid[] = 	"@(#)remote.c	1.2 90/08/02 4.1NFSSRC Copyr 1990 Sun Micro";
# 	endif /* not hpux */
# endif	/* not lint */
# ifdef PATCH_STRING
static char *patch_5384="@(#) PATCH_9.X: remote.o $Revision: 1.2.109.5 $ 94/03/24 PHNE_5384";
# endif	/* PATCH_STRING */

# define	_DEFINE

# include "sendmail.h"
# include "pathnames.h"
# include <sys/types.h>
# include <sys/stat.h>
# include <mntent.h>

/*
**  REMOTE -- handle simple sending of outgoing mail via SMTP,
** without any kind of queuing or scanning at all.
**
** av is the argument vector of people to deliver the mail to.
** the message comes in over stdin.  
**/

/*
 * yet to do:
 * handle all the SMTP error return right (test with bad addresses)
 */

# define REPLYTYPE(r)	((r) / 100)		/* first digit of reply code */
# define REPLYCLASS(r)	(((r) / 10) % 10)	/* second digit of reply code */
# define SMTPCLOSING	421			/* "Service Shutting Down" */

# define RemoteTimeout	300			/* seconds to wait */

extern char SmtpReplyBuffer[];			/* from usersmtp.c */

/*
 * if the -t option was specified, we need to wait until after the
 * message collection to open the remote mode.  Otherwise we can get
 * away without even needing spool files.
 */
RemoteMode(from, argv, queue, e)
    char *from;		/* send address if non-null */
    char **argv;	/* recipient vector */
    ADDRESS *queue;	/* queue if above was null */
    ENVELOPE *e;	/* the current envelope */
  {
    /*
     * Open an SMTP connection to the Remote Server, then send the
     * appropriate SMTP commands to deliver the message.
     */
    FILE *smtpOut, *smtpIn;
    int err, r, tried;
    char line[256];
    char **av;		/* copy the arguments */
    char **avp;
    ADDRESS *q;

    if (argv) {
        av = copyplist(argv, TRUE);
	dropenvelope(e);
    }
    if (from == NULL) 
    	from = newstr(username());

    tried = 0;
    while (1) {
tryagain:
	if (tried) {
		sleep(2);
	}
	tried = 1;
	err = makeconnection(RemoteServer, 0, &smtpOut, &smtpIn);
	if (err == EX_OK) {
	    r = RemoteReply(smtpIn);
	    if (REPLYTYPE(r) != 2)
	    	continue;
	    RemoteSend(smtpOut, "HELO %s\r\n", MyHostName );
	    r = RemoteReply(smtpIn);
	    if (REPLYTYPE(r) != 2)
	    	continue;
	    RemoteSend(smtpOut, "MAIL FROM:<%s>\r\n", from);
	    r = RemoteReply(smtpIn);
	    if (REPLYTYPE(r) != 2)
	    	continue;
	    if (argv)
	       avp = av;
	    else {
	       q = queue;
	       avp = &q->q_paddr;
	    }
	    while (1) {
		register char *p = *avp;
		char *angle;

		/*
		 * Check for already existing angle-brackets
		 */
		angle = strchr(p, '<');
		if (angle) {
			p = angle + 1;
			angle = strrchr(p, '>');
			if (angle) {
				*angle = '\0';
			}
		}
	    	RemoteSend(smtpOut,"RCPT TO:<%s>\r\n", p);
		r = RemoteReply(smtpIn);
		if (REPLYTYPE(r) == 4)
	    		goto tryagain;
		if (REPLYTYPE(r) != 2) {
	    	  /*
		   * Fatal error - have to return the mail!
		   */
		   FILE *p;
		   char cmd[MAXLINE];

badmail:
# ifdef V4FS
		   sprintf(cmd,"/usr/bin/mail -d %s", from);
# else	/* ! V4FS */
		   sprintf(cmd,"/bin/mail -d %s", from);
# endif	/* V4FS */
		   p = popen(cmd, "w");
		   if (p == NULL)
		   	return(EX_OSERR);
		    fprintf(p,"Subject: returned mail for %s\n\n", *avp);
		    fprintf(p,"Mail error was: %s\n",
		       SmtpReplyBuffer);
		    fprintf(p,"--- returned mail follows ---\n");
		    while (fgets(line, sizeof(line), stdin) != NULL) {
		        if (!IgnrDot && line[0] == '.' && line[1] == '\n') 
					break;
			if (line[0] == '.') fputc('.',p);
			fputs(line, p);
	    	    }
		   pclose(p);
		   return(EX_UNAVAILABLE);
		}
	      if (argv) {
	      	++avp;
		if (*avp == NULL) break;
	      }
	      else {
	      	q = q->q_next;
		if (q == NULL) break;
		avp = &q->q_paddr;
	      }
	    }
	    
	    RemoteSend(smtpOut,"DATA\r\n");
	    if (REPLYTYPE(r) == 4)
	    	continue;
	    r = RemoteReply(smtpIn);
	    if (REPLYTYPE(r) > 4)
	    	goto badmail;
	    if (argv == NULL) {
	    	/*
		 * send the header from the qf file, then open df file
		 */
		MAILER nullmailer;

		bzero((char *) &nullmailer, sizeof nullmailer);
		nullmailer.m_r_rwset = nullmailer.m_s_rwset = -1;
		nullmailer.m_eol = "\n";
# ifdef SUN
		nullmailer.m_argvsize = 10000;
# endif /* SUN */
		putheader(smtpOut, &nullmailer, e);
		if (freopen(e->e_df, "r", stdin) == NULL)
			continue;
	    }
	    while (fgets(line, sizeof(line), stdin) != NULL) {
	    	   /* 
		    * Pass the message through from
		    * standard input to the remote system.
		    */
	        if (!IgnrDot && line[0] == '.' && line[1] == '\n') break;
		if (line[0] == '.') fputc('.',smtpOut);
		fputs(line, smtpOut);
	    }
	    RemoteSend(smtpOut,".\r\n");
	    r = RemoteReply(smtpIn);

	    RemoteSend(smtpOut,"QUIT\r\n");
	    r = RemoteReply(smtpIn);
	    closeconnection(fileno(smtpOut));
	    fclose(smtpOut);
     	    fclose(smtpIn);
	    return(EX_OK);
	    }

	if (err != EX_TEMPFAIL)  {
	    printf("Error contacting remote server %s\n", RemoteServer);
	    exit(err);
        }
    }  /* continue until we suceede */
     
}


/*
 * Simple case of handling SMTP replies in remote mode.
 * Just buffer them up for possible error messages.
 */
RemoteReply(f)
  {
     int r;

     while (sfgets( SmtpReplyBuffer, MAXLINE, f) ) {
	if (Verbose)
	    printf(SmtpReplyBuffer);
       if (SmtpReplyBuffer[3] == '-' || !isdigit(SmtpReplyBuffer[0]) )
       	continue;
       r = atoi(SmtpReplyBuffer);
       if (r < 100) continue;
       return(r);
     }
     /*
      * If we got a timeout, close and retry
      */
    return(450);
  }


/*
 * Send a string (with optional argument) to the remote system
 * as well as echoing it for verbose mode.
 */
RemoteSend(f,s,arg)
    FILE *f;
    char *s, *arg;
{
    fprintf(f, s, arg);
    fflush(f);
    if (Verbose) {
	  printf(">>");
	  printf(s, arg);
    }
}


RemoteDefault()
  {
    /*
     * Search through mtab to see which server /etc/spool/mail
     * is mounted from.  Called when remote mode is set, but no
     * server is specified.  Deliver locally if mailbox is local.
     */
    FILE *mfp;
    struct mntent *mnt;
    char *endhost;			/* points past the colon in name */
    int bestlen = 0, bestnfs = 0;
    int len;
    char mailboxdir[256];		/* resolved symbolic link */
    static char bestname[256];		/* where the name ends up */
    char linkname[256];			/* for symbolic link chasing */

    (void) strcpy(mailboxdir, _PATH_MAILBOXDIR);
    while (1) {
	if ((len = readlink(mailboxdir, linkname, sizeof(linkname))) < 0)
		break;
	linkname[len] = '\0';
	(void) strcpy(mailboxdir, linkname);
    }

    mfp = setmntent(MNT_MNTTAB, "r");
    if (mfp == NULL)  {
       printf("Unable to open mount table\n");
       return(EX_OSERR);
   }
   (void) strcpy(bestname, "");

    while ((mnt = getmntent(mfp)) != NULL) {
		len = preflen(mailboxdir, mnt->mnt_dir);
		if (len > bestlen) {
			bestlen = len;
			(void) strcpy(bestname, mnt->mnt_fsname);
		}
	}
    endmntent(mfp);
    endhost = strchr(bestname,':');
    if (endhost && bestlen > 4) {
	/*
	 * We found a remote mount-point for /var/mail --
	 * save the host name. The test against "4" is because we do not
	 * want to be fooled by mounting "/" or "/var" only.
	 */
	 RemoteServer = bestname;
	 *endhost = 0;
	 return(EX_OK);
    }
    /*
     * No remote mounts - assume local
     */
     RemoteServer = NULL;
  }

/*
 * Returns: length of second argument if it is a prefix of the
 * first argument, otherwise zero.
 */
preflen(str, pref)
	char *str, *pref;
{
	int len; 

	len = strlen(pref);
	if (strncmp(str, pref, len) == 0)
		return (len);
	return (0);
}
