/* @(#) $Revision: 70.2 $ */      
#
#include "rcv.h"
#include <sys/stat.h>
#include <ctype.h>

#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 2	/* set number */
#include <locale.h>
extern int __nl_langid[];
#endif NLS

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * User commands.
 */


/*
 * Print the current active headings.
 * Don't change dot if invoker didn't give an argument.
 */

static int screen;

headers(msgvec)
	int *msgvec;
{
	register int n, mesg, flag;
	register struct message *mp;
	int size;

	size = screensize();
	n = msgvec[0];
	if (n != 0)
		screen = (n-1)/size;
	if (screen < 0)
		screen = 0;
	mp = &message[screen * size];
	if (mp >= &message[msgCount])
		mp = &message[msgCount - size];
	if (mp < &message[0])
		mp = &message[0];
	flag = 0;
	mesg = mp - &message[0];
	if (dot != &message[n-1])
		dot = mp;
	if (Hflag)
		mp = message;
	for (; mp < &message[msgCount]; mp++) {
		mesg++;
		if (mp->m_flag & MDELETED)
			continue;
		if (flag++ >= size && !Hflag)
			break;
		printhead(mesg);
		sreset();
	}
	if (flag == 0) {
		printf((catgets(nl_fn,NL_SETN,1, "No more mail.\n")));
		return(1);
	}
	return(0);
}

/*
 * Scroll to the next/previous screen
 */

scroll(arg)
	char arg[];
{
	register int s, size;
	int cur[1];

	cur[0] = 0;
	size = screensize();
	s = screen;
	switch (*arg) {
	case 0:
	case '+':
		s++;
		if (s * size > msgCount) {
			printf((catgets(nl_fn,NL_SETN,2, "On last screenful of messages\n")));
			return(0);
		}
		screen = s;
		break;

	case '-':
		if (--s < 0) {
			printf((catgets(nl_fn,NL_SETN,3, "On first screenful of messages\n")));
			return(0);
		}
		screen = s;
		break;

	default:
		printf((catgets(nl_fn,NL_SETN,4, "Unrecognized scrolling command \"%s\"\n")), arg);
		return(1);
	}
	return(headers(cur));
}

/*
 * Compute what the screen size should be.
 * We use the following algorithm:
 *	If user specifies with screen option, use that.
 *	If baud rate < 1200, use  5
 *	If baud rate = 1200, use 10
 *	If baud rate > 1200, use 20
 */
screensize()
{
	register char *cp;
	register int s;

	if (baud < B1200)
		s = 5;
	else if (baud == B1200)
		s = 10;
	else
		s = 20;
	if ((cp = value("screen")) != NOSTR) {
		s = atoi(cp);
		if (s > 0)
			return(s);
	}
	return(s);
}

/*
 * Print out the headlines for each message
 * in the passed message list.
 */

from(msgvec)
	int *msgvec;
{
	register int *ip;

	for (ip = msgvec; *ip != NULL; ip++) {
		printhead(*ip);
		sreset();
	}
	if (--ip >= msgvec)
		dot = &message[*ip - 1];
	return(0);
}

/*
 * Print out the header of a specific message.
 * This is a slight improvement to the standard one.
 */

printhead(mesg)
{
	struct message *mp;
	FILE *ibuf;
	char headline[LINESIZE], *subjline, dispc, curind;
	char *toline, *fromline;
	char pbuf[BUFSIZ];
	struct headline hl;
	register char *cp;
	int showto;
#ifdef NLS
	int cvtdate = 1; /* Tell parse() to convert date */
#endif NLS

	mp = &message[mesg-1];
	ibuf = setinput(mp);
	readline(ibuf, headline);
	toline = hfield("to", mp);
	subjline = hfield("subject", mp);
	if (subjline == NOSTR)
		subjline = hfield("subj", mp);

	curind = (!Hflag && dot == mp) ? '>' : ' ';
	dispc = ' ';
	showto = 0;
	if (mp->m_flag & MSAVED)
		dispc = '*';
	if (mp->m_flag & MPRESERVE)
		dispc = 'P';
	if ((mp->m_flag & (MREAD|MNEW)) == MNEW)
		dispc = 'N';
	if ((mp->m_flag & (MREAD|MNEW)) == 0)
		dispc = 'U';
	if (mp->m_flag & MBOX)
		dispc = 'M';
#ifdef NLS
	parse(headline, &hl, pbuf, cvtdate);
#else
	/* Tell parse() NOT to do NLS date conversion */
	parse(headline, &hl, pbuf, 0);
#endif NLS

	/*
	 * Netnews interface?
	 */

	if (newsflg) {
	    if ( (fromline=hfield("newsgroups",mp)) == NOSTR 	/* A-news */
	      && (fromline=hfield("article-id",mp)) == NOSTR ) 	/* B-news */
	          fromline = "<>";
	    else 
		  for(cp=fromline; *cp; cp++) {		/* limit length */
			if( any(*cp, " ,\n")){
			      *cp = '\0';
			      break;
			}
		  }
	/*
	 * else regular.
	 */

        } else {
		fromline = nameof(mp, 0);
		if (toline && value("showto")) {
			if (value("allnet")) {
				cp = do_allnet(fromline);
			} else
				cp = fromline;
			if (strcmp(cp, myname)==0) {
				showto = 1;
				fromline = toline;
				while (*toline && !isspace(*toline))
					toline++;
				*toline = '\0';
				if ((cp = rindex(fromline, '!'))==NOSTR)
					cp = fromline;
				else while (cp > fromline) {
					if (*--cp=='!')
						break;
				}
				fromline = cp;
			}
		}
	}
	if (showto) {
#ifdef NLS
	    if (__nl_langid[LC_TIME] != 0 && __nl_langid[LC_TIME] != 99) {
		if (subjline != NOSTR)
			printf("%c%c%3d To %-15s %21.21s %4d/%-5d %-.20s\n",
			    curind, dispc, mesg, fromline, hl.l_date,
			    mp->m_lines, mp->m_size, subjline);
		else
			printf("%c%c%3d To %-15s %21.21s %4d/%-5d\n", curind, dispc, mesg,
			    fromline, hl.l_date, mp->m_lines, mp->m_size);
	    }
	    else
#endif NLS
		if (subjline != NOSTR)
			printf("%c%c%3d To %-15s %16.16s %4d/%-5d %-.25s\n",
			    curind, dispc, mesg, fromline, hl.l_date,
			    mp->m_lines, mp->m_size, subjline);
		else
			printf("%c%c%3d To %-15s %16.16s %4d/%-5d\n", curind, dispc, mesg,
			    fromline, hl.l_date, mp->m_lines, mp->m_size);
	} else {
#ifdef NLS
	    if (__nl_langid[LC_TIME] != 0 && __nl_langid[LC_TIME] != 99) {
		if (subjline != NOSTR)
			printf("%c%c%3d %-18s %21.21s %4d/%-5d %-.20s\n",
			    curind, dispc, mesg, fromline, hl.l_date,
			    mp->m_lines, mp->m_size, subjline);
		else
			printf("%c%c%3d %-18s %21.21s %4d/%-5d\n", curind, dispc, mesg,
			    fromline, hl.l_date, mp->m_lines, mp->m_size);
	    }
	    else
#endif NLS
		if (subjline != NOSTR)
			printf("%c%c%3d %-18s %16.16s %4d/%-5d %-.25s\n",
			    curind, dispc, mesg, fromline, hl.l_date,
			    mp->m_lines, mp->m_size, subjline);
		else
			printf("%c%c%3d %-18s %16.16s %4d/%-5d\n", curind, dispc, mesg,
			    fromline, hl.l_date, mp->m_lines, mp->m_size);
	}
}

/*
 * Print out the value of dot.
 */

pdot()
{
	printf("%d\n", dot - &message[0] + 1);
	return(0);
}

/*
 * Print out all the possible commands.
 */

pcmdlist()
{
	register struct cmd *cp;
	register int cc;
	extern struct cmd cmdtab[];

	printf((catgets(nl_fn,NL_SETN,5, "Commands are:\n")));
	for (cc = 0, cp = cmdtab; cp->c_name != NULL; cp++) {
		cc += strlen(cp->c_name) + 2;
		if (cc > 72) {
			printf("\n");
			cc = strlen(cp->c_name) + 2;
		}
		if ((cp+1)->c_name != NOSTR)
			printf("%s, ", cp->c_name);
		else
			printf("%s\n", cp->c_name);
	}
	return(0);
}

/*
 * Type out messages, honor ignored fields.
 */
type(msgvec)
	int *msgvec;
{

	return(type1(msgvec, 1));
}

/*
 * Type out messages, even printing ignored fields.
 */
Type(msgvec)
	int *msgvec;
{

	return(type1(msgvec, 0));
}

/*
 * Type out the messages requested.
 */
jmp_buf	pipestop;

type1(msgvec, doign)
	int *msgvec;
{
	register *ip;
	register struct message *mp;
	register int mesg;
	register char *cp;
	int c, nlines;
	int brokpipe();
	FILE *ibuf, *obuf;
	void (*saveint)();

	saveint = signal(SIGINT, SIG_IGN);
	obuf = stdout;
	if (setjmp(pipestop)) {
		if (obuf != stdout) {
			pipef = NULL;
			pclose(obuf);
		}
		goto ret0;
	}
	if (intty && outtty && (cp = value("crt")) != NOSTR) {
		for (ip = msgvec, nlines = 0; *ip && ip-msgvec < msgCount; ip++)
			nlines += message[*ip - 1].m_lines;
		if (nlines > atoi(cp)) {
			obuf = popen(PG, "w");
			if (obuf == NULL) {
				perror(PG);
				obuf = stdout;
			}
			else {
				pipef = obuf;
				sigset(SIGPIPE, brokpipe);
			}
		} else
			signal(SIGINT, saveint);
	} else
		signal(SIGINT, saveint);
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		dot = mp;
		print(mp, obuf, doign);
	}
	if (obuf != stdout) {
		pipef = NULL;
		pclose(obuf);
	}
ret0:
	sigset(SIGPIPE, SIG_DFL);
	signal(SIGINT, saveint);
	return(0);
}

/*
 * Respond to a broken pipe signal --
 * probably caused by user quitting pg.
 */

brokpipe()
{
# ifndef VMUNIX
	signal(SIGPIPE, brokpipe);
# endif
	longjmp(pipestop, 1);
}

/*
 * Print the indicated message on standard output.
 */

print(mp, obuf, doign)
	register struct message *mp;
	FILE *obuf;
{

	if (!doign || !isign("message"))
		fprintf(obuf, (catgets(nl_fn,NL_SETN,6, "Message %2d:\n")), mp - &message[0] + 1);
	touch(mp - &message[0] + 1);
#ifdef NLS
	doign |= PBIT; /* Set the PBIT */
#endif NLS
	send(mp, obuf, doign); /* PBIT (for print()) is used to
			        * tell send() that it is OK to
				* do the NLS date conversions.
				* All other calls to send() do
				* not set this bit; they could
				* possibly mangle the "From "
				* line without this feature.
				*/
}

/*
 * Print the top so many lines of each desired message.
 * The number of lines is taken from the variable "toplines"
 * and defaults to 5.
 */

top(msgvec)
	int *msgvec;
{
	register int *ip;
	register struct message *mp;
	register int mesg;
	int c, topl, lines, lineb;
	char *valtop, linebuf[LINESIZE];
	FILE *ibuf;
#ifdef NLS
	char *nl_cxtime(), *nldp, nldate[LINESIZE];
	extern long mailx_getdate(); /* converts ASCII date string to long */
	long fromdate;
#endif NLS

	topl = 5;
	valtop = value("toplines");
	if (valtop != NOSTR) {
		topl = atoi(valtop);
		if (topl < 0 || topl > 10000)
			topl = 5;
	}
	lineb = 1;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		dot = mp;
		if (value("quiet") == NOSTR)
			printf((catgets(nl_fn,NL_SETN,7, "Message %2d:\n")), mesg);
		ibuf = setinput(mp);
		c = mp->m_lines;
		if (!lineb)
			printf("\n");
		for (lines = 0; lines < c && lines < topl; lines++) {
			if (readline(ibuf, linebuf) <= 0)
				break;
#ifdef NLS
			if ((lines == 0) &&(__nl_langid[LC_TIME] != 0 && __nl_langid[LC_TIME] != 99)) {
			/* Only do the conversion if this is the
			 * first line ("From " line).
			 */
				nldp = strchr(linebuf, ' ');
				nldp = strchr(++nldp, ' ');
				/* Copy ASCII date string
				 * to nldate for conversion
				 */
				strcpy(nldate,++nldp);
				/* Terminate "From " line
				 * at space after username
				 */
				*nldp = '\0';
				fromdate = mailx_getdate(nldate, NULL);
				if (fromdate != -1)   /* Fix of DSDe414531 */
				   strcpy(nldate, nl_cxtime(&fromdate, ""));
				/* nldate now contains the new
				 * NLS-formatted date
				 */
				strcat(linebuf, nldate);
			}
#endif NLS
			puts(linebuf);
			lineb = blankline(linebuf);
		}
	}
	return(0);
}

/*
 * Touch all the given messages so that they will
 * get mboxed.
 */

stouch(msgvec)
	int msgvec[];
{
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		dot->m_flag |= MTOUCH;
		dot->m_flag &= ~MPRESERVE;
	}
	return(0);
}

/*
 * Make sure all passed messages get mboxed.
 */

mboxit(msgvec)
	int msgvec[];
{
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		dot->m_flag |= MTOUCH|MBOX;
		dot->m_flag &= ~MPRESERVE;
	}
	return(0);
}

/*
 * List the folders the user currently has.
 */
folders()
{
	char dirname[BUFSIZ], cmd[BUFSIZ];
	int pid, s, e;

	if (getfold(dirname) < 0) {
		printf((catgets(nl_fn,NL_SETN,8, "No value set for \"folder\"\n")));
		return(-1);
	}
	sprintf(cmd, "%s %s", LS, dirname);
	return(system(cmd));
}
