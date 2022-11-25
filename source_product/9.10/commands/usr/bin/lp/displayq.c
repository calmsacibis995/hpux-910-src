/* $Revision: 66.4 $ */

/*
 * Routines to display the state of the queue.
 */

#include "lp.h"

static	FILE	*sfd;	/* Read from remote system stream descriptor */
static	int	fd;	/* Read from remote system file descriptor */

#define JOBCOL	40		/* column for job # in -l format */
#define OWNCOL	7		/* start of Owner column in normal */
#define SIZCOL	62		/* start of Size column in normal */
#define	TYPE1	1
#define	TYPE2	2
#define	TYPE98	98
#define	TYPE99	99

/*
 * Stuff for handling job specifications
 */
extern int	users;		/* # of users in user array */
extern int	requ[];		/* job number of spool entries */
extern int	requests;	/* # of spool requests */

extern	char	host[SP_MAXHOSTNAMELEN];
extern	char	work[BUFSIZ];

extern	char	*from;
extern	char	*printer;
extern	char	*user[];	/* users to process */
extern	char	*RM;
extern	char	*RP;

extern	struct outq o;
extern	struct pstat p;
extern	struct qstat q;


static	long	totsize;	/* total print job size in bytes */

static	int	lflag;		/* long output option */
static	int	rank;		/* order to be printed (-1=none, 0=active) */
static	int	first;		/* first file in ``files'' column? */
static	int	col;		/* column on screen */
static	int	sendtorem;	/* are we sending to a remote? */

static	char	arg[BUFSIZ];
static	char	current[FILEMAX];/* current file being printed */

static	char	*head0 = "Rank   Owner      Job  Files";
static	char	*head1 = "Total Size\n";

/*
 * Display the current state of the queue. Format = 1 if long format.
 */
int
displayq(format,remote_only)
int	format;
int	remote_only;
{
	int	i;
	int	first_status;
	char	RM_buf[SP_MAXHOSTNAMELEN];
	char	RP_buf[DESTMAX+1];
	char	*cp;
	FILE	*sfp;
	FILE	*lockf_open();

	lflag = format;
	rank = -1;

/*	Get the printer description. */


	if (!isdest(printer)){
		fatal("cannot examine spooling area",0);
		sprintf(work,"cannot find the printer %s",printer);
		fatal(work,1);
	}
	
	if (getpdest(&p,printer) == EOF)
		endpent();
	else {
	    endpent();
	    RM = RM_buf;
	    RP = RP_buf;
	    strncpy (RM, p.p_remotedest, SP_MAXHOSTNAMELEN);
	    strncpy (RP, p.p_remoteprinter, DESTMAX+1);
	    
	    /*
	     * If there is no local printer, then print the queue on
	     * the remote machine and then what's in the queue here.
	     * Note that a file in transit may not show up in either queue.
	     */
	    sendtorem = 0;
	    if (*RP != '\0') {
		fflush(stdout);
		
		sendtorem++;
		(void) sprintf(arg, "%c%s", format + '\3', RP);
		cp = arg;
		for (i = 0; i < requests; i++) {
		    cp += strlen(cp);
		    (void) sprintf(cp, " %d", requ[i]);
		}
		for (i = 0; i < users; i++) {
		    cp += strlen(cp);
		    *cp++ = ' ';
		    strcpy(cp, user[i]);
		}
		strcat(arg, "\n");
		fd = getport(RM);
		if (fd < 0) {
		    if (from != host)
			printf("%s: ", host);
		    printf("connection to %s is down\n", RM);
		} else {
		    i = strlen(arg);
		    if (write(fd, arg, i) != i)
			fatal("Lost connection");
		    if (remote_only){
			if(lflag)
			    printf("\nprinter queue for %s\n", printer);
			disp_trans();
		    }else{
			while ((i = read(fd, arg, sizeof(arg))) > 0)
			    (void) fwrite(arg, 1, i, stdout);
		    }
		    (void) close(fd);
 		}
	    }
	}

	if ((getqdest(&q,printer) != EOF) && (!q.q_accept)){
		printf("%s: Warning: %s queue is turned off\n", host, printer);
	}

	if (getpdest(&p,printer) != EOF){
		if ((p.p_flags & P_ENAB) == 0){
			printf("%s: Warning: %s is down\n", host, printer);
		}
		if (p.p_flags & P_BUSY){
			if ((p.p_remob3 & O_OB3) != 0){
				sprintf(current, "%s/%s/cfA%03d%s", REQUEST, printer, p.p_seqno, p.p_host);
			}else{
				sprintf(current, "%s/%s/cA%04d%s", REQUEST, printer, p.p_seqno, p.p_host);
			}
		}
	}else{
		printf("\n");
	}
	endpent();

	/*
	 * Print the status file.
	 */
	first_status = TRUE;
	sprintf(work, "%s/%s/%s", REQUEST, printer, SENDINGSTATUS);
	sfp = lockf_open(work, "r", FALSE);
	if (sfp != NULL) {
		while ((i = fread(work, 1, sizeof(work),sfp)) > 0){
			if (sendtorem && first_status){
				printf("\n%s: ", host);
				first_status = FALSE;
			}
			(void) fwrite(work, 1, i, stdout);
		}
		(void) fclose(sfp);	/* unlocks as well */
	} else{
		putchar('\n');
	}

	if (remote_only){
		return(0);
	}else{
		local_displayq();
	}
}

local_displayq()
{
	int	nitems;
	int	first_header;
	char	rname[FILEMAX];
/*
 * Now, examine the control files and print out the jobs to
 * be done for each user.
 */
	nitems = 0;
	first_header = TRUE;
	while (getoent(&o) != EOF){
		if (strcmp(printer, o.o_dest))	/* only process requests */
			continue;		/* for "printer" */
	if ((o.o_rflags & O_OB3) != 0){
		sprintf(rname, "%s/%s/cfA%03d%s", REQUEST, o.o_dest, o.o_seqno, o.o_host);
	}else{
		sprintf(rname, "%s/%s/cA%04d%s", REQUEST, o.o_dest, o.o_seqno, o.o_host);
	}
		if (!lflag && (first_header)){
			first_header = FALSE;
			header();
		}
		inform(rname);
		nitems++;
	}
	if (nitems == 0) {
		if (!sendtorem)
			printf("no entries\n");
	}
	endoent();
	return(0);
}
/*
 * Print the header for the short listing format
 */
int
header()
{
	printf(head0);
	col = strlen(head0)+1;
	blankfill(SIZCOL);
	printf(head1);
}
int
inform(cf)
char	*cf;
{
	char	*cf_file;
	int	j;
	int	index;
	char	type;
	FILE	*cfp;
	extern	char	*strrchr();
	char	file[MAXNAMLEN];	/* print file name */
	int	copies;
 
	/*
	 * There's a chance the control file has gone away
	 * in the meantime; if this is the case just keep going
	 */
#ifdef SecureWare
	if(ISSECURE)
	    lp_change_mode(SPOOL,cf,0640,ADMIN,"control file for inform()");
	else
	    chmod(cf, 0640);
#else
	chmod(cf, 0640);
#endif
	if ((cfp = lockf_open(cf, "r", FALSE)) == NULL){
		return(0);
	}

	if (rank < 0)
		rank = 0;
	
	if (sendtorem || strcmp(cf, current)){
		rank++;
	}

	j = 0;
	copies = 1;
	while (getrent(&type,arg,cfp) != EOF){
		switch (type) {
                case 'K':
			copies = atoi(arg);
			break;

		case 'P': /* Was this file specified in the user's list? */
			if (!inlist(arg, cf)) {
				fclose(cfp);
				return(0);
			}

		if (lflag) {
			printf("\n%s: ", arg);
			col = strlen(arg) + 2;
			prank(rank);
			blankfill(JOBCOL);
			index = '/';
			if ((cf_file = strrchr(cf,index)) == NULL){
				cf_file = cf;
			}else{
				cf_file++;
			}
			if ((o.o_rflags & O_OB3) != 0){
				cf_file += 3;
			}else{
				cf_file += 2;
			}
			printf(" [job %s]\n", cf_file);
		} else {
			col = 0;
			prank(rank);
			blankfill(OWNCOL);
			if ((o.o_rflags & O_OB3) != 0){
				printf("%-10s %-3d  ", o.o_logname, o.o_seqno);
			}else{
				printf("%-10s %-4d  ", o.o_logname, o.o_seqno);
			}
			col += 16;
			first = 1;
		}
			continue;
		default: /* some format specifer and file name? */
			if (type < 'a' || type > 'z')
				continue;
			if (j == 0 || strcmp(file, arg) != 0)
				strcpy(file, arg);
			j++;
			continue;
		case 'N':
			show(arg, file, copies);
			file[0] = '\0';
			j = 0;
		}
	}
	fclose(cfp);
	if (!lflag) {
		blankfill(SIZCOL);
		printf("%d bytes\n", o.o_size);
		totsize = 0;
	}
}
int
inlist(name, file)
char	*name;
char	*file;
{
	int *r, n;
	int	index;
	char **u, *cp;

	if (users == 0 && requests == 0)
		return(1);
	/*
	 * Check to see if it's in the user list
	 */
	for (u = user; u < &user[users]; u++)
		if (!strcmp(*u, name))
			return(1);
	/*
	 * Check the request list
	 */
	index = '/';
	if ((cp = strrchr(file,index)) == NULL){
		cp = file;
	}else{
		cp++;
	}
	if ((o.o_rflags & O_OB3) != 0){
		cp += 3;
	}else{
		cp += 2;
	}
	for (n = 0 ; isdigit(*cp); )
		n = n * 10 + (*cp++ - '0');
	for (r = requ; r < &requ[requests]; r++)
		if (*r == n && !strcmp(cp, from))
			return(1);
	return(0);
}
int
show(nfile, file, copies)
	char *nfile, *file;
{
	if (strcmp(nfile, " ") == 0)
		nfile = "(standard input)";
	if (lflag)
		ldump(nfile, file, copies);
	else
		dump(nfile, file, copies);
}

/*
 * Fill the line with blanks to the specified column
 */
int
blankfill(n)
	int n;
{
	while (col++ < n)
		putchar(' ');
}

/*
 * Give the abbreviated dump of the file names
 */
int
dump(nfile, file, copies)
char	*nfile;
char	*file;
int	copies;
{
	short n, fill;
	struct stat lbuf;

	/*
	 * Print as many files as will fit
	 *  (leaving room for the total size)
	 */
	 fill = first ? 0 : 2;	/* fill space for ``, '' */
	 if (((n = strlen(nfile)) + col + fill) >= SIZCOL-4) {
		if (col < SIZCOL) {
			printf(" ..."), col += 4;
			blankfill(SIZCOL);
		}
	} else {
		if (first)
			first = 0;
		else
			printf(", ");
		printf("%s", nfile);
		col += n+fill;
	}
	if (*file && !stat(file, &lbuf))
		totsize += copies * lbuf.st_size;
}

/*
 * Print the long info about the file
 */
int
ldump(nfile, file, copies)
char	*nfile;
char	*file;
int	copies;
{
	struct stat lbuf;
	char	file_path[FILEMAX];

	putchar('\t');
	if (copies > 1)
		printf("%-2d copies of %-19s", copies, nfile);
	else
		printf("%-32s", nfile);

	sprintf(file_path,"%s/%s/%s",REQUEST,printer,file);
	if (*file && !stat(file_path, &lbuf))
		printf(" %ld bytes", lbuf.st_size);
	else
		printf(" ??? bytes");
	putchar('\n');
}

/*
 * Print the job's rank in the queue,
 *   update col for screen management
 */
int
prank(n)
{
	char line[100];
	static char *r[] = {
		"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"
	};

	if (n == 0) {
		printf("active");
		col += 6;
		return(0);
	}
	if ((n/10) == 1)
		(void) sprintf(line, "%dth", n);
	else
		(void) sprintf(line, "%d%s", n, r[n%10]);
	col += strlen(line);
	printf("%s", line);
}
int
disp_trans()
{
	char	org_host[SP_MAXHOSTNAMELEN];	/* holds system name */
	char	org_host_parm[SP_MAXHOSTNAMELEN]; /* holds system name */
	char	org_message[BUFSIZ + 1];/* original message */
	char	user[LOGMAX + 1];	/* holds user name */
	char	user_parm[LOGMAX + 1];	/* holds user name */
	char	realname[DESTMAX+1+3];	/* holds real file name */

	int	justdidnl;		/* indicates a nl was done */
					/* by putstatusline */
	int	mtype;			/* holds type of request */
	int	putitback;		/* indicates to put back */
					/* the last record read */
	int	seqno_parm;		/* holds sequence number */
	int	seqno;			/* holds sequence number */

	long	copies;			/* holds the number of */
					/* copies of the request */
	long	size;			/* holds the number of bytes */
					/* in each file */
	int	temp;			/* temporary storage */
	long	total_size;		/* holds the number of bytes */
					/* in the request */
	FILE	*fdopen();

	sfd	= fdopen(fd,"r");

	putitback = 0;
	init1(&total_size, &copies, user, org_host);

	while ((mtype = disp_getamessage(org_host, user, realname, &copies,
			&seqno, &size, org_message, &putitback)) != 0){
		if (mtype == TYPE1){
			putstatusline1(seqno, user, org_host);
			do{
				mtype = disp_getamessage(org_host_parm,
					user_parm, realname, &copies, &seqno_parm,
					&size, org_message, &putitback);
				if (mtype == TYPE2){
					putstatusline2(realname, copies, size);
				}else{
					putitback = mtype;
					justdidnl = TRUE;
					init1(&total_size, &copies, user, org_host);
					break;
				}
			}while (mtype != 0);
			if (mtype == 0){
				break;
			}
		}else{
			if (mtype == TYPE2){
				printf("status being received is corrupt\n");
			}
			if (!((justdidnl) && (mtype == TYPE98))){
				justdidnl = FALSE;
				temp = strlen(org_message);
				(void) fwrite(org_message, 1, temp, stdout);
			}
		}
	}
}

static char *
basename(str)
char	*str;
{
	char	*ptr;

	if ( ( ptr = strrchr(str, '/') ) == NULL )
	    return(str);
	else
	    return(ptr+1);
}

int
disp_getamessage(org_host, user, realname, copies, seqno, size, org_message,
	putitback)
char	*org_host;	/* holds system name */
char	*user;		/* holds user name */
char	*realname;	/* holds file name */
long	*copies;	/* holds the number of */
			/* copies of the request */
int	*seqno;		/* holds sequence number */
long	*size;		/* holds the number of bytes */
			/* in the request */
char	*org_message;	/* original message */
int	*putitback;	/* indicates to put back */
			/* the last record read */
{
	int	index;		/* used as index to token pointers */
	int	length;		/* holds the length of the record */
	int	mtype;		/* holds type of request */
	int	token_length;	/* holds the length of a token */
	int	token_ctr;	/* holds the number of token */
	char	*token[32];	/* pointers to tokens */
	char	*token_ptr;	/* pointers to a token */
	char	*end_token_ptr;	/* pointers to end of a token */
	char	token_sep[3];	/* seperators for tokens */
	extern	char	*strtok();

	token_sep[0] = ' ';	/* seperators for tokens */
	token_sep[1] = '\n';	/* seperators for tokens */
	token_sep[2] = '\0';	/* seperators for tokens */

	mtype = *putitback;
	*putitback = 0;
	if (mtype !=0){
		return(mtype);
	}
	if ((length = disp_getarecord()) == 0){
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
	if (index < 2){
		return(TYPE99);
	}
	token_ptr = token[0];
	token_length = strlen(token_ptr);
	end_token_ptr = token_ptr + token_length - 1;
	
	if ((*token_ptr != '\t') && (*end_token_ptr == ':')){
		*end_token_ptr = '\0';
		strncpy(user, token_ptr, LOGMAX + 1);

		token_ptr = token[index];
		token_length = strlen(token_ptr);
		end_token_ptr = token_ptr + token_length - 1;
		if ((*end_token_ptr == ']') && (!strcmp(token[index - 1],"[job"))){
			*end_token_ptr = '\0';
			if (sscanf(token_ptr, "%d%s", seqno, org_host) == 2){
				return(TYPE1);
			}
		}
	}
	token_ptr = token[index];
	token_length = strlen(token_ptr);
	end_token_ptr = token_ptr + token_length - 1;
	
	if ((*token[0] == '\t') && (!strcmp(token_ptr, "bytes"))){
		token_ptr = token[index - 1];
		if (!strcmp(token_ptr, "???")){
			*size = 0;
		}else{
			sscanf(token_ptr,"%d",size);
		}
		token_ptr = token[0];
		token_ptr++;
		if (!strcmp(token[1], "copies") && (!strcmp(token[2], "of"))){
			sscanf(token_ptr, "%d",copies);
			token_ctr = 4;
			token_ptr = token[3];
		}else{
			*copies = 1;
			token_ctr = 1;
		}

		token_ptr = basename(token_ptr);

		strncpy(realname, token_ptr, DESTMAX);
		*(realname+DESTMAX) = NULL;
 
		if(strlen(token_ptr) > DESTMAX)
			strcpy(realname+DESTMAX, "...");

		for(; token_ctr < index-1; token_ctr++){
			strcat(realname, " ");
			strcat(realname, token[token_ctr]);
		}

		for(;strlen(realname) < DESTMAX+strlen("...");)
		    strcat(realname, " ");

		return(TYPE2);
	}
	return(TYPE99);
}
int
putstatusline1(seqno, user, org_host)
int	seqno;		/* holds sequence number */
char	*user;		/* holds user name */
char	*org_host;	/* holds system name */
{
	char tmp2[FILEMAX];
	char reqid[IDSIZE + 1];

	sprintf(reqid, "%s-%d", printer, seqno);
	sprintf(tmp2, "");
	sprintf(tmp2, " from %s", org_host);

	printf("%-*s %-*s priority ?  %s\n", IDSIZE, reqid, LOGMAX-1,
		       user, tmp2);
}

putstatusline2(realname, copies, size)
char	*realname;
long	copies;
long	size;
{
	char tmp1[4+1+6+1];

	if(copies < 2){
		strcpy(tmp1, "           ");
	}else{
		sprintf(tmp1, "%-4d copies", copies);
	}

	printf("\t%-*s          %s %*ld bytes\n"
	       , DESTMAX, realname, tmp1, OSIZE, size);
}

int
disp_getarecord()
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

int
init1(total_size, copies, user, org_host)
long	*total_size;	/* holds the number of bytes */
			/* in the request */
long	*copies;	/* holds the number of */
			/* copies of the request */
char	*user;		/* holds user name */
char	*org_host;	/* holds system name */
{
	*total_size = 0;
	*copies     = 0;
	strcpy(user    , "unknown");
	strcpy(org_host, "unknown");
}
