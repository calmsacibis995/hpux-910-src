/*
**	@(#)logging.c:	$Revision: 1.25.109.1 $	$Date: 91/11/19 14:10:40 $  
**	logging.c	--	handle file logging from daemons
**
*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#include <rpc/rpc.h>
nl_catd nlmsg_fd;
char *catgets();
#endif NLS

#include <stdio.h>
#include <time.h>
#include <sys/utsname.h>		/* contains def of UTSLEN	*/

static	char	host_name[UTSLEN],	/* the name of this host	*/
		invo_name[64],		/* the name of this process	*/
		log_file[64],		/* the name of the log file	*/
		Newline;		/* the carriage return		*/

static	int	logging_enabled=0;	/* records whether enabled	*/

static	FILE	*fp;			/* used for writing to file	*/

/*
**	startlog()	--	start logging
**	parms:
**		invo_name:	argv[0]
**		file_name:	the name of the log file to use
**	effects:
**		stores data in static variables, tries to open log file
**		sets host name.  Note: log file can be /dev/console.
**	returns:
**		non-zero if the log file could not be opened
*/
int
startlog(invo, file)
char	*invo, *file;
{
#ifdef NLS
        nl_init(getenv("LANG"));
        nlmsg_fd = catopen("logging",0);
#endif NLS

	/*
	**	set up the static variables ...
	*/
	(void) strcpy(invo_name, invo);
	(void) strcpy(log_file, file);
	(void) gethostname(host_name, UTSLEN);

	if ((fp=fopen(log_file, "a")) == NULL)
		return(-1);
	/*
	**	if we were able to open the file, close it and return OK
	*/
	(void) fclose(fp);
	logging_enabled++;
	/*	if the log_file is a tty device, then output the sequence
	**	"\r" to make sure all messages leave the cursor in col.
	**	zero; if we are logging to a file, then we don't need that.
	*/
	Newline = isatty(log_file) ? '\r' : '\0';
	return(0);
}

/*      logheader()     --      write the message header to the log file
**      parms:          --      none
**      effects: 
**              write the header portion of a log message. This includes
**              the date, hostname, caller's pid, and the invocation name
**              terminated by \r\n.
**      returns:
**             void
*/

logheader()
{
	long   	tv_sec;
	struct	tm	*tp, *localtime();

	/*
	**	determine the current time ...
	*/
	(void) time(&tv_sec);
	tp = localtime(&tv_sec);
	/*
	**	write it to the log file in the format YY.MM.DD hh.mm.ss,
	**	then write the host name, pid, and invo_name.
	*/
	nl_fprintf(fp,(catgets(nlmsg_fd,NL_SETN,1, "%1$02d.%2$02d.%3$02d %4$02d:%5$02d:%6$02d  ")),
		tp->tm_year, tp->tm_mon+1, tp->tm_mday,
		tp->tm_hour, tp->tm_min, tp->tm_sec);
	nl_fprintf(fp,(catgets(nlmsg_fd,NL_SETN,2, "%1$-8.8s  pid=%2$-5d  %3$s%4$s")),
		host_name, getpid(), invo_name, Newline?(catgets(nlmsg_fd, NL_SETN, 3,"\r\n     ")):(catgets(nlmsg_fd, NL_SETN, 4,"\n     ")));
}

/*
**	logmsg()	--	send a message to the log file
**	parms:
**		msg:	the message to be written to the log file
**		a[1-8]:	optional arguments for fprintf(msg) ...
**	effects:
**		write the message argument to the log file, preceeded
**		by the date, hostname, caller's pid, and invocation name;
**		terminates lines with \r\n, so no newline is needed in msg.
**	returns:
**		non-zero if a failure occurred opening or writing
*/

int
logmsg(msg, a1,a2,a3,a4,a5,a6,a7,a8)
char  *msg;
{

	/*
	**	if logging has not been enabled, then just return OK
	*/
	if (logging_enabled == 0)
		return(0);
	/*
	**	attempt to open the log file, return error if failure
	*/
	if ((fp=fopen(log_file, "a+")) == NULL)
		return(-1);

	/* print the message header */
	logheader();

	/*
	**	now write the message we were asked to write, with any
	**	arguments we were passed.
	*/
	nl_fprintf(fp,msg,a1,a2,a3,a4,a5,a6,a7,a8);
	/*
	**	terminate the message and close the log file ...
	**	we make sure we have at least one \r and only one \n
	*/
	if (strchr(msg,'\n') == NULL)
		fprintf(fp, Newline?(catgets(nlmsg_fd, NL_SETN, 5,"\n\r")):(catgets(nlmsg_fd, NL_SETN, 6,"\n")));
	else	/* we have already printed a newline */
		if (Newline != '\0')
			fprintf(fp, Newline);


	/*
	**	this fclose will fflush() out any pending data ...
	*/
	(void) fclose(fp);
}

/*
**	logstr()	--	send a string to the log file,
**                              does not write the message header 
**                              or append the \r\n. This routine 
**                              was added for lockd support. Lockd 
**                              call logheader to print the header 
**                              and logmsg to print the \r\n.
**	parms:
**		msg:	the message to be written to the log file
**		a[1-8]:	optional arguments for fprintf(msg) ...
**	effects:
**		write the message argument to the log file, does not
**              append \r\n.
**	returns:
**		non-zero if a failure occurred opening or writing
*/

int
logstr(msg, a1,a2,a3,a4,a5,a6,a7,a8)
char  *msg;
{

	/*
	**	if logging has not been enabled, then just return OK
	*/
	if (logging_enabled == 0)
		return(0);
	/*
	**	attempt to open the log file, return error if failure
	*/
	if ((fp=fopen(log_file, "a+")) == NULL)
		return(-1);
	
	/*
	**	write the message we were asked to write, with any
	**	arguments we were passed.
	*/
	nl_fprintf(fp,msg,a1,a2,a3,a4,a5,a6,a7,a8);

	/*
	**	this fclose will fflush() out any pending data ...
	*/
	(void) fclose(fp);
}


/*
**	logclnt_perror()	--	send a message to the log file
**	parms:
**              clnt:   the rpc client handle
**		msg:	the message to be written to the log file
**	effects:
**              Imitate clnt_perror, only the output is placed in
**		to the log file, preceeded by the date, hostname, 
**              caller's pid, and invocation name; terminates lines 
**              with \r\n, so no newline is needed in msg.
**	returns:
**		non-zero if a failure occurred opening or writing
*/

int
logclnt_perror(clnt, msg)
CLIENT *clnt;
char  *msg;
{
	char    *clnt_msg;  /* the message that client_perror would 
				   print */
	char *clnt_sperrno();
	/*
	**	if logging has not been enabled, then just return OK
	*/
	if (logging_enabled == 0)
		return(0);
	/*
	**	attempt to open the log file, return error if failure
	*/
	if ((fp=fopen(log_file, "a+")) == NULL)
		return(-1);

	/* print the message header */
	logheader();

	/*
	**      now generate the rest of the message that clnt_perror	
	**      would print and write it to the log file.
	*/
        clnt_msg = clnt_sperror(clnt, msg);
	fprintf(fp, clnt_msg);
	/*
	**	terminate the message and close the log file ...
	**	we make sure we have at least one \r and only one \n
	*/
	if (strchr(clnt_msg,'\n') == NULL)
		fprintf(fp, Newline?(catgets(nlmsg_fd, NL_SETN, 5,"\n\r")):(catgets(nlmsg_fd, NL_SETN, 6,"\n")));
	else	/* we have already printed a newline */
		if (Newline != '\0')
			fprintf(fp, Newline);


	/*
	**	this fclose will fflush() out any pending data ...
	*/
	(void) fclose(fp);
}

/*
**	logclnt_perrno()	--	send a message to the log file
**	parms:
**		num:    the status returned by the server
**	effects:
**              Imitate clnt_perrno, only the output is placed in
**		to the log file, preceeded by the date, hostname, 
**              caller's pid, and invocation name; terminates lines 
**              with \r\n, so no newline is needed in msg.
**	returns:
**		non-zero if a failure occurred opening or writing
*/

int
logclnt_perrno(num)
  enum clnt_stat num;         /* status returned to client */
{
	char    *clnt_msg;  /* the message that clnt_perrno would 
				   print */

	/*
	**	if logging has not been enabled, then just return OK
	*/
	if (logging_enabled == 0)
		return(0);
	/*
	**	attempt to open the log file, return error if failure
	*/
	if ((fp=fopen(log_file, "a+")) == NULL)
		return(-1);

	/* print the message header */
	logheader();

	/*
	**      now generate the rest of the message that clnt_perrno
	**      would print and write it to the log file.
	*/
        clnt_msg = clnt_sperrno(num);
	fprintf(fp, clnt_msg);
	/*
	**	terminate the message and close the log file ...
	**	we make sure we have at least one \r and only one \n
	*/
	if (strchr(clnt_msg,'\n') == NULL)
		fprintf(fp, Newline?(catgets(nlmsg_fd, NL_SETN, 5,"\n\r")):(catgets(nlmsg_fd, NL_SETN, 6,"\n")));
	else	/* we have already printed a newline */
		if (Newline != '\0')
			fprintf(fp, Newline);


	/*
	**	this fclose will fflush() out any pending data ...
	*/
	(void) fclose(fp);
}


/*
**	logclnt_pcreateerror()	--   send a message to the log file
**	parms:
**		msg:	the message to be written to the log file
**	effects:
**              Imitate clnt_pcreateerror, only the output is placed in
**		to the log file, preceeded by the date, hostname, 
**              caller's pid, and invocation name; terminates lines 
**              with \r\n, so no newline is needed in msg.
**	returns:
**		non-zero if a failure occurred opening or writing
*/

int
logclnt_pcreateerror(msg)
char  *msg;
{
	char    *clnt_msg;  /* the message that client_perror would 
				   print */

	/*
	**	if logging has not been enabled, then just return OK
	*/
	if (logging_enabled == 0)
		return(0);
	/*
	**	attempt to open the log file, return error if failure
	*/
	if ((fp=fopen(log_file, "a+")) == NULL)
		return(-1);

	/* print message header */
	logheader();

	/*
	**      now generate the rest of the message that clnt_perror	
	**      would print and write it to the log file.
	*/
        clnt_msg = clnt_spcreateerror(msg);
	fprintf(fp, clnt_msg);
	/*
	**	terminate the message and close the log file ...
	**	we make sure we have at least one \r and only one \n
	*/
	if (strchr(clnt_msg,'\n') == NULL)
		fprintf(fp, Newline?(catgets(nlmsg_fd, NL_SETN, 5,"\n\r")):(catgets(nlmsg_fd, NL_SETN, 6,"\n")));
	else	/* we have already printed a newline */
		if (Newline != '\0')
			fprintf(fp, Newline);

	/*
	**	this fclose will fflush() out any pending data ...
	*/
	(void) fclose(fp);
}

/*      log_perror()    --      log perror message to a file
**      parms:
**		msg:	the message to be written to the log file
**                      before sys_errlist[err];
**              err:    value of errno.
**	effects:
**              Imitate perror, only the output is placed in
**		to the log file, preceeded by the date, hostname, 
**              caller's pid, and invocation name; terminates lines 
**              with \r\n, so no newline is needed in msg.
**	returns:
**		non-zero if a failure occurred opening or writing
*/
int 
log_perror(msg)
char  *msg;

{
	char    *clnt_msg;  /* the message that perror would 
				   print */
	int     save_errno;  /* strerror changes errno and we want the 
			      * user to see the original value of errno
			      * when we return so we save and restore it.
			      */
        extern  int errno;   /* global i/o error variable */
        char    *strerror(); /* function which returns NLSed error message 
			       from sys_errlist */
	/*
	**	if logging has not been enabled, then just return OK
	*/

	/* save errno's value */
	save_errno = errno;
	if (logging_enabled == 0)
		return(0);
	/*
	**	attempt to open the log file, return error if failure
	*/
	if ((fp=fopen(log_file, "a+")) == NULL)
	  {
	    /* restore errno's original value */
	    errno = save_errno;
	    return(-1);
	  }

	/* print the message header */
	logheader();

	/* 
	**if the user specified a message print it first followed by a
	** colon and a blank
	*/
	if (strcmp(msg,"")!=0) 
	  {
	    fprintf(fp,"%s: ",msg);
	  }
	/* print the error string as determined by errno infomation */
	clnt_msg = strerror(save_errno);
	if (clnt_msg != NULL )
	  {
	    fprintf(fp, clnt_msg);
	  }
	
	/*
	**	terminate the message and close the log file ...
	**	we make sure we have at least one \r and only one \n
	*/

	if (strchr(clnt_msg,'\n') == NULL)
		fprintf(fp, Newline?(catgets(nlmsg_fd, NL_SETN, 5,"\n\r")):(catgets(nlmsg_fd, NL_SETN, 6,"\n")));
	else	/* we have already printed a newline */
		if (Newline != '\0')
			fprintf(fp, Newline);

	/*
	**	this fclose will fflush() out any pending data ...
	*/

	(void) fclose(fp);
	/* restore errno's original value */
	errno = save_errno;
}            


/*
**	endlog()	--	quit logging to this file
**	parms:
**		none, terminates the current settings
**	effects:
**		"forgets" about the log file it had open
**	returns:
**		zero (OK)
*/
int
endlog()
{
	/*
	**	set logging_enabled to zero to let logmsg() know we
	**	have no current log file and to return OK...
	**	also set the invo_name and log_file to empty strings
	*/
	logging_enabled = 0;
	invo_name[0] = '\0';
	log_file[0] = '\0';
	
	return(0);
}
