/* @(#) $Revision: 64.3 $ */      
#

#include "rcv.h"
#include <errno.h>

#ifndef NLS
#define catgets(i,sn,mn,s) (s)
#else NLS
#define NL_SETN 5	/* set number */
#endif NLS

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * More commands..
 */


/*
 * pipe messages to cmd.
 */
 jmp_buf pipe_broken;

dopipe(str)
	char str[];
{
	register int *ip, mesg;
	register struct message *mp;
	char *cp, *cmd;
	int f, *msgvec, lc, t, nowait=0;
	long cc;
	register int pid;
	int page, s, pivec[2];
	void (*sigint)();
	char *Shell;
	FILE *pio;
	int ispipe;
	int broken_pipe();

	if(setjmp(pipe_broken)) {
		goto break0;
	}
	signal(SIGPIPE, broken_pipe);
	msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	if ((cmd = snarf(str, &f, 0, &ispipe)) == NOSTR) {
		if (f == -1) {
			printf((catgets(nl_fn,NL_SETN,1, "pipe command error\n")));
			return(1);
			}
		if ( (cmd = value("cmd")) == NOSTR) {
			printf((catgets(nl_fn,NL_SETN,2, "\"cmd\" not set, ignored.\n")));
			return(1);
			}
		}
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			printf((catgets(nl_fn,NL_SETN,3, "No messages to pipe.\n")));
			return(1);
		}
		msgvec[1] = NULL;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return(1);
	if (*(cp=cmd+strlen(cmd)-1)=='&'){
		*cp=0;
		nowait++;
		}
	if ((cmd = expand(cmd)) == NOSTR)
		return(1);
	printf((catgets(nl_fn,NL_SETN,4, "Pipe to: \"%s\"\n")), cmd);
	flush();

					/*  setup pipe */
	if (pipe(pivec) < 0) {
		perror("pipe");
		/* signal(SIGINT, sigint) */
		return(0);
	}

	if ((pid = fork()) == 0) {
		close(pivec[1]);	/* child */
		fclose(stdin);
		dup(pivec[0]);
		close(pivec[0]);
		if ((Shell = value("SHELL")) == NOSTR || *Shell=='\0')
			Shell = SHELL;
		execlp(Shell, Shell, "-c", cmd, 0);
		perror(Shell);
		_exit(1);
	}
	if (pid == -1) {		/* error */
		perror("fork");
		close(pivec[0]);
		close(pivec[1]);
		return(0);
	}

	close(pivec[0]);		/* parent */
	pio=fdopen(pivec[1],"w");

					/* send all messages to cmd */
	page = (value("page")!=NOSTR);
	cc = 0L;
	lc = 0;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		if ((t = send(mp, pio, 0)) < 0) {
			perror(cmd);
			return(1);
		}
		lc += t;
		cc += mp->m_size;
		if (page) putc('\f', pio);
	}

	fflush(pio);
	if (ferror(pio))
	      perror(cmd);
	fclose(pio);

					/* wait */
	if (!nowait){
		while (wait(&s) != pid);
		if (s == 256) {
			printf((catgets(nl_fn,NL_SETN,5, "\nExit status = %d.\n")), s>>8);
			goto err;
		}
		s &= 0377;
		if (s != 0) {
			printf((catgets(nl_fn,NL_SETN,6, "Pipe to \"%s\" failed\n")), cmd);
			goto err;
		}
	}

	printf("\"%s\" %d/%ld\n", cmd, lc, cc);
	return(0);

break0:
	sigset(SIGPIPE, SIG_DFL);
	/* signal(SIGINT, sigint); */
	return(0);
err:
	/* signal(SIGINT, sigint); */
	return(0);
}

broken_pipe()
{
# ifndef VMUNIX
      signal(SIGPIPE, broken_pipe);
# endif
      longjmp(pipe_broken, 1);
}
