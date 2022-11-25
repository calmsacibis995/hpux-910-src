/* $Revision: 72.2 $ */
/*
 * Routines to alter attributes mentioned within request file
 */

#include "lp.h"

extern char work[BUFSIZ];

extern char dst[DESTMAX+1];
extern int seqno;

extern char title[TITLEMAX+1];
extern char opts[OPTMAX];
extern int copies;
extern short priority;
extern short mail;
extern short wrt;
extern char user_name[LOGMAX+1];

#ifdef REMOTE
extern	char	*RM;	/* pointer to the remote host name strings */
extern	char	*RP;	/* pointer to the remote printer name strings */
extern char *from;
extern char host[SP_MAXHOSTNAMELEN];
#endif REMOTE

/* set_user_id() -- set the effective user id			        */
/*	if argument is equal to 0, set the euid to 0 (root)		*/
/*			     else, set the euid to LP administrator	*/

int
set_user_id(id)
int	id;
{
	int	uid, gid;
	struct	passwd	*adm, *getpwnam();
	
	uid = (int)getuid();
	gid = (int)getgid();

	if((adm = getpwnam(ADMIN)) == NULL)
	    fatal("LP administrator not in password file\n", 1);
		
	if(id == 0){
		if( setresuid(uid, 0, -1) == -1
			|| setresgid(gid, adm->pw_gid, -1) == -1 ){
			sprintf(work,
			    "unable to set the effective user id to \"%d\"", 0);
			fatal(work,1);
		}
	}else{
		if( setresuid(uid, adm->pw_uid, -1) ==  -1
			|| setresgid(gid, adm->pw_gid, -1) == -1 ){
			sprintf(work,
			    "unable to set the effective user id to \"%d\"", adm->pw_uid);
			fatal(work,1);
		}
	}
	return(0);
}


#ifdef REMOTE

/* no_response -- check the remote rlpdaemon supports this function */

int
no_response(fd)
int	fd;
{
	char	resp;

	if( read(fd, &resp, 1) != 1 )
		fatal("Lost connection");

	if (resp == '\0')
		return(0);
	else
		return(1);
}


/* remotealt -- send alteration command to the remote rlpdaemon */

void
remotealt(sending_now)
short sending_now;
{
	int fd;
	FILE *fp;
	char	remotesend[FILEMAX];
	char	cmd[BUFSIZ];
	extern FILE *lockf_open();
	
/* waiting for completion of sending to the remote */

	if(sending_now){
		sprintf(remotesend, "%s/%s/.remotesending", REQUEST, dst);
		fp = lockf_open(remotesend, "w", FALSE);
		fclose(fp);
	}

/* open connection with remote host */

	set_user_id(0);			/* set user id to root */

	if( (fd = getport(RM)) < 0 ){
		if(from != host)
		    printf("%s: ", host);
		printf("connection to %s is down\n", RM);
		exit(-1);
	} else {

		/* write the alteration command to remote */

		sprintf(cmd, "%c%s\n", '\6', RP);
		write(fd, cmd, strlen(cmd));

		/* check if the remote rlpdaemon supports alteration */

		if(no_response(fd))
			fatal("connection was down", 1);

		sprintf(cmd, "%c%s %d %s\n", '\2', user_name, seqno, host);
		write(fd, cmd, strlen(cmd));

		if(copies > 0){			/* number of copies */
			sprintf(work, "%c%d\n", R_COPIES, copies);
			write(fd, work, strlen(work));
		}

		if(*title != NULL){		/* title of the request */
			sprintf(work, "%c%s\n", R_HEADERTITLE, title);
			write(fd, work, strlen(work));
		}

		if(*opts != NULL){		/* interface option */
			sprintf(work, "%c%s\n", R_OPTIONS, opts);
			write(fd, work, strlen(work));
		}

		if(mail == TRUE){		/* mail option */
			sprintf(work, "%c\n", R_MAIL);
			write(fd, work, strlen(work));
		}

		if(wrt == TRUE){		/* write option */
			sprintf(work, "%c\n", R_WRITE);
			write(fd, work, strlen(work));
		}


		/* check if the connection still lives */
		
		sprintf(work, "%c\n", '\1');	/* all were sent */
		write(fd, work, strlen(work));

		if(no_response(fd)){
			fatal("connection was down", 1);
		}
	}
}
#endif REMOTE


/* altother -- alteration other option of the request */

altother(iflag)
int	iflag;
{
	FILE	*rfile, *tfile;
	char	rname[RNAMEMAX], tname[RNAMEMAX];
	char	logname[LOGMAX+1];
	char	type, arg[FILEMAX];
#ifdef REMOTE
	char	RM_buf[SP_MAXHOSTNAMELEN];
	char	RP_buf[DESTMAX+1];
	struct qstat q;
#endif REMOTE
	extern FILE *lockf_open();
	char	*strncpy();

	struct outq o;

#ifdef REMOTE
	struct pstat p;

	/* check to see if the destination is remote or not */

	if (getpdest(&p, dst) == EOF) {
		RM = '\0';
		RP = '\0';
		p.p_rflags = 0;
	}else{
		RM = RM_buf;
		RP = RP_buf;
		strcpy(RM, p.p_remotedest);
		strcpy(RP, p.p_remoteprinter);
	}
	endpent();
#endif REMOTE

	/* check to see if the request was almost processed */

	if(getoid(&o, dst, seqno) == EOF)
	    o.o_flags |= O_DEL;
	endoent();

	if(o.o_flags & O_PROCESSED){
#ifdef REMOTE
		if(*RP != '\0' && !iflag){
			remotealt(TRUE);
			return(0);
		}else{
			sprintf(work,
			    "request has been already processed \"%s-%d\"", dst, seqno);
			fatal(work, 1);
		}
#else REMOTE
			sprintf(work,
			    "request has been already processed \"%s-%d\"", dst, seqno);
			fatal(work, 1);
#endif REMOTE
	}

	if(o.o_flags & O_DEL){
#ifdef REMOTE
		if(*RP != '\0' && !iflag){
			remotealt(FALSE);
			return(0);
		}else{
			sprintf(work, "no such request \"%s-%d\"", dst, seqno);
			fatal(work, 1);
		}
#else REMOTE
			sprintf(work, "no such request \"%s-%d\"", dst, seqno);
			fatal(work, 1);
#endif REMOTE
	}


/* alter the local request */

#ifdef REMOTE
	getqdest(&q, dst);

	if(q.q_ob3){
		sprintf(rname, "%s/%s/cfA%03d%s", REQUEST, dst, seqno, o.o_host);
		sprintf(tname, "%s/%s/tfA%03d%s", REQUEST, dst, seqno, o.o_host);
	}else{
		sprintf(rname, "%s/%s/cA%04d%s", REQUEST, dst, seqno, o.o_host);
		sprintf(tname, "%s/%s/tA%04d%s", REQUEST, dst, seqno, o.o_host);
	}

	endqent();
#else REMOTE
	sprintf(rname, "%s/%s/r-%d", REQUEST, dst, seqno);
	sprintf(tname, "%s/%s/t-%d", REQUEST, dst, seqno);
#endif REMOTE

	chmod(rname, 0640);

	if( !strcmp(user_name, o.o_logname) || ISADMIN ){

		if((rfile = lockf_open(rname, "r+", FALSE)) == NULL){
			sprintf(work, "can't open request file \"%s\"", rname);
			fatal(work, 1);
		}

		if(GET_ACCESS(tname, 0) == -1 && (tfile = fopen(tname, "w")) != NULL)
			chmod(tname, 0640);
		else{
			sprintf(work,
			    "can't open temporary request file \"%s\"", tname);
			fatal(work, 1);
		}

		/* rewrite the target control file */

		while( getrent(&type, arg, rfile) != EOF){
			switch(type){

			    case R_COPIES :
				if(copies > 0){
					sprintf(work, "%d", copies);
					putrent(type, work, tfile);
				}else{
					putrent(type, arg, tfile);
				}
				break;
#ifdef REMOTE
			    case R_HEADERTITLE :
#else REMOTE
			    case R_TITLE :
#endif REMOTE
				if(*title != NULL){
					putrent(type, title, tfile);
				}else{
					putrent(type, arg, tfile);
				}
				break;

			    case R_OPTIONS :
				if(*opts != NULL){
					putrent(type, opts, tfile);
				}else{
					putrent(type, arg, tfile);
				}
				break;

			    case R_PERSON :
				strncpy(logname, arg, LOGMAX);
				logname[LOGMAX] = NULL;
				putrent(type, arg, tfile);
				break;

			    default:
				putrent(type, arg, tfile);
				break;
			}
		}

		if(mail)
		    putrent(R_MAIL, logname, tfile);
		if(wrt)
		    putrent(R_WRITE, logname, tfile);

		fclose(rfile);
		fclose(tfile);
		unlink(rname);		/* remove the old request file */

		/* link the request filename to the new request file */

		if(link(tname, rname) == -1){
			sprintf(work,
			    "can't create new request file \"%s\"", rname);
			fatal(work, 1);
		}

		unlink(tname);		/* remove the temporary file */

		chmod(rname, 0440);
	}else{
		fprintf(stderr,
		    "request \"%s-%d\" not altered: not owner\n", o.o_dest, o.o_seqno);
	}
}
