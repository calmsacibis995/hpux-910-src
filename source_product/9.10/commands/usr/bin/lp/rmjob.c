/* $Revision: 66.5 $ */

/*
 * rmjob - remove the specified jobs from the queue.
 */

#include "lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif
#define	TYPE1	1
#define	TYPE2	2
#define	TYPE98	98
#define	TYPE99	99

/*
 * Stuff for handling lprm specifications
 */
extern char	work[BUFSIZ];		/* work buffer */
extern char	*user[];		/* users to process */
extern int	users;			/* # of users in user array */
extern int	requ[];			/* job number of spool entries */
extern int	requests;		/* # of spool requests */
extern char	*person;		/* name of person doing lprm */
extern char	*RM;
extern char	*RP;
extern char	*from;
extern char	host[SP_MAXHOSTNAMELEN];
extern char	*printer;

static char	root[] = "root";
extern	int	all;		/* eliminate all files (root only) */
/*
static int	all = 0;*/		/* eliminate all files (root only) */
static char	current[SP_MAXHOSTNAMELEN+7];		/* active control file name */
extern	struct	pstat	p;
extern	struct	outq o;
static	int	interrupt;
static	char	errmsg[200];

static	int	rem;
static	FILE	*sfd;
static char	arg[BUFSIZ];		/* work buffer */
rmjob()
{
	char	RM_buf[SP_MAXHOSTNAMELEN];
	char	RP_buf[DESTMAX+1];
	char	rname[BUFSIZ];

	if (!isdest(printer))
	    fatal("cannot find the printer",1);

	if (getpdest(&p,printer) != EOF){
	    RM = RM_buf;
	    RP = RP_buf;
	    strncpy (RM, p.p_remotedest, SP_MAXHOSTNAMELEN);
	    strncpy (RP, p.p_remoteprinter, DESTMAX+1);
	}

	/*
	 * If the format was `lprm -' and the user isn't the super-user,
	 *  then fake things to look like he said `lprm user'.
	 */
	if (users < 0) {
		if (((int) getuid()) == 0)
			all = 1;	/* all files in local queue */
		else {
			user[0] = person;
			users = 1;
		}
	}
	if (!strcmp(person, "-all")) {
		if (from == host)
			fatal("The login name \"-all\" is reserved",1);
		all = 1;	/* all those from 'from' */
		person = root;
	}

		/*
		 * process the files
		 */
	while (getoent(&o) != EOF){
		if ((o.o_rflags & O_OB3) != 0){
			sprintf(rname, "cfA%03d%s", o.o_seqno, o.o_host);
		}else{
			sprintf(rname, "cA%04d%s", o.o_seqno, o.o_host);
		}
		process(rname);
	}
	endoent();
	endpent();
	chkremote(FALSE);
	exit(0);
}

/*
 * Process a control file.
 */
static
process(file)
char *file;
{
	FILE *cfp;
	char	arg[BUFSIZ];
	char	type;
	char	file_buf[BUFSIZ];

	if (!chk(file))
		return;
		setsigs();
		o.o_flags |= O_DEL;
		putoent(&o);

	sprintf(file_buf,"%s/%s/%s",REQUEST,printer,file);
	if ((cfp = fopen(file_buf, "r")) == NULL){
		sprintf(work,"cannot open %s", file);
		fatal(work,1);
	}
	while (getrent(&type,arg,cfp) != EOF) {
		switch (type) {
		case 'U':  /* unlink associated files */
			if (from != host)
				printf("%s: ", host);
			sprintf(file_buf,"%s/%s/%s",REQUEST,printer,arg);
			printf(unlink(file_buf) ? "cannot dequeue %s\n" :
				"%s dequeued\n", arg);
		}
	}
	(void) fclose(cfp);
	if (from != host)
		printf("%s: ", host);
	sprintf(file_buf,"%s/%s/%s",REQUEST,printer,file);
	printf(unlink(file_buf) ? "cannot dequeue %s\n" : "%s dequeued\n", file);
	if(o.o_flags & O_PRINT) {
	    if(getpdest(&p, o.o_dev) != EOF)
		killit(&p);
	    endpent();
	}
	sprintf(errmsg, "%s %d %s", o.o_dest, o.o_seqno, o.o_host);
	enqueue(F_CANCEL, errmsg);
	reset();
}

/*
 * Do the dirty work in checking
 */
static
chk(file)
char *file;
{
	int *r, n;
	char **u, *cp;
	FILE *cfp;
	extern FILE *lockf_open();
	char	type;
	char	arg[BUFSIZ];
	char	file_buf[BUFSIZ];

	if (all && (from == host || !strcmp(from, file+6)))
		return(1);

	/*
	 * get the owner's name from the control file.
	 */
	sprintf(file_buf,"%s/%s/%s",REQUEST,printer,file);
#ifdef SecureWare
	if(ISSECURE)
		lp_change_mode(SPOOL, file_buf, 0640, ADMIN,
			"request file to remove job");
	else
		chmod(file_buf, 0640);
#else
	chmod(file_buf, 0640);
#endif

	if ((cfp = lockf_open(file_buf, "r", FALSE)) == NULL){
		return(0);
	}

	while (getrent(&type,arg,cfp) != EOF) {
		if (type == 'P')
			break;
	}
	(void) fclose(cfp);
	if (type != 'P')
		return(0);

	if (users == 0 && requests == 0)
		return(!strcmp(file, current) && isowner(arg, file));
	/*
	 * Check the request list
	 */
	if ((o.o_rflags & O_OB3) != 0)
	{	/* BSD control file format, cfAddd<hostname> */
	    for (n = 0, cp = file+3; isdigit(*cp); )
		n = n * 10 + (*cp++ - '0');
	}
	else
	{	/* SYS5 control file format, cAdddd<hostname> */
	    for (n = 0, cp = file+2; isdigit(*cp); )
		n = n * 10 + (*cp++ - '0');
	}
	for (r = requ; r < &requ[requests]; r++)
		if (*r == n && isowner(arg, file))
			return(1);
	/*
	 * Check to see if it's in the user list
	 */
	for (u = user; u < &user[users]; u++)
		if (!strcmp(*u, arg) && isowner(arg, file))
			return(1);
	return(0);
}

/*
 * If root is removing a file on the local machine, allow it.
 * If root is removing a file from a remote machine, only allow
 * files sent from the remote machine to be removed.
 * Normal users can only remove the file from where it was sent.
 */
static
isowner(owner, file)
	char *owner, *file;
{
	if (!strcmp(person, root) && (from == host || !strcmp(from, file+6)))
		return(1);
	if (!strcmp(person, owner) && !strcmp(from, file+6))
		return(1);
	if (from != host)
		printf("%s: ", host);
	printf("%s: Permission denied\n", file);
	return(0);
}

/*
 * Check to see if we are sending files to a remote machine. If we are,
 * then try removing files on the remote machine.
 */

chkremote(remote_only)
int	remote_only;	/* indicates this is for remote operation */
			/* called from rcancel, not rlpdaemon */
{
	char *cp;
	int i;
	char buf[BUFSIZ];

	if ((*RP == NULL) || (*RM == NULL))
		return;	/* not sending to a remote machine */

	/*
	 * Flush stdout so the user can see what has been deleted
	 * while we wait (possibly) for the connection.
	 */
	fflush(stdout);

	sprintf(buf, "\5%s %s", RP, all ? "-all" : person);
	cp = buf;
	for (i = 0; i < users; i++) {
		cp += strlen(cp);
		*cp++ = ' ';
		strcpy(cp, user[i]);
	}
	for (i = 0; i < requests; i++) {
		cp += strlen(cp);
		(void) sprintf(cp, " %d", requ[i]);
	}
	strcat(cp, "\n");
	rem = getport(RM);
	if (rem < 0) {
		if (from != host)
			printf("%s: ", host);
		printf("connection to %s is down\n", RM);
	} else {
		i = strlen(buf);
		if (write(rem, buf, i) != i)
			fatal("Lost connection",1);
		if (remote_only){
			trans();
		}else{
			while ((i = read(rem, buf, sizeof(buf))) > 0)
				(void) fwrite(buf, 1, i, stdout);
		}
		(void) close(rem);
	}
}
killit(pp)
struct pstat *pp;
{

	if(pp->p_pid != 0)
		kill(-(pp->p_pid), SIGTERM);
	pp->p_flags &= ~P_BUSY;
	pp->p_pid = 0;
	pp->p_seqno = -1;
	strcpy(pp->p_rdest, "-");
	putpent(pp);
}

setsigs()
{
	int saveint();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, saveint);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, saveint);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, saveint);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, saveint);
}

reset()
{
	int catch();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, catch);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, catch);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, catch);
	if(interrupt) {
		cleanup();
		exit(1);
	}
}

/* catch -- catch signals */

catch()
{
	int cleanup();
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	cleanup();
	exit(1);
}

saveint()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	interrupt = TRUE;
}
int
trans()
{
	char	org_message[BUFSIZ + 1];/* original message */

	int	mtype;			/* holds type of request */
	int	seqno;			/* holds sequence number */

	int	temp;			/* temporary storage */
	FILE	*fdopen();

	sfd	= fdopen(rem,"r");

	while ((mtype = getamessage(&seqno, org_message)) != 0){
		switch(mtype){
		case TYPE1:
			putcancelline(seqno);
			break;
		case TYPE2:
			break;
		default:
			temp = strlen(org_message);
			(void) fwrite(org_message, 1, temp, stdout);
			break;
		}
	}
}
int
getamessage(seqno, org_message)
int	*seqno;		/* holds sequence number */
char	*org_message;	/* original message */
{
	int	index;		/* used as index to token pointers */
	int	length;		/* holds the length of the record */
	int	token_length;	/* holds the length of a token */
	char	*token[32];	/* pointers to tokens */
	char	*token_ptr;	/* pointers to a token */
	char	*end_token_ptr;	/* pointers to end of a token */
	char	token_sep[3];	/* seperators for tokens */
	extern	char	*strtok();

	token_sep[0] = ' ';	/* seperators for tokens */
	token_sep[1] = '\n';	/* seperators for tokens */
	token_sep[2] = '\0';	/* seperators for tokens */

	if ((length = getarecord()) == 0){
		return(length);
	}
	strncpy(org_message, arg, BUFSIZ);

	if ((token[0] = strtok(arg, token_sep)) == NULL){
		if (arg[0] == '\n'){
			return(TYPE98);
		}else{
			return(TYPE99);
		}
	}
	for (index=1; index < 32; index++){
		if ((token[index] = strtok(NULL, token_sep)) == NULL){
			break;
		}
	}
	index--;
	if (index < 1){
		return(TYPE99);
	}
	token_ptr = token[0];
	token_length = strlen(token_ptr);
	end_token_ptr = token_ptr + token_length - 1;
	
	if ((*end_token_ptr == ':') && (!strcmp(token[2], "dequeued"))){
		token_ptr = token[1];
		if (*token_ptr != 'c'){
			return(TYPE2);
		}
		end_token_ptr = token_ptr + 6;
		*end_token_ptr = '\0';
		token_ptr++;
		token_ptr++;
		if (!isdigit(*token_ptr)){
			token_ptr++;
		}

		if (sscanf(token_ptr, "%d", seqno) == 1){
				return(TYPE1);
		}
	}
	return(TYPE99);
}
int
putcancelline(seqno)
int	seqno;		/* holds sequence number */
{
	printf("request \"%s-%d\" cancelled\n", printer, seqno);
}
int
getarecord()
{
	int	i;	/* length of a record */
	int	c;	/* holds a character */
	char	*arg_ptr;

	i = 0;
	arg_ptr = arg;
	while ((c = getc(sfd)) != EOF){
		i++;
		if ((*arg_ptr++ = c) == '\n'){
			break;
		} 
	}
	*arg_ptr = '\0';
	return(i);
}
