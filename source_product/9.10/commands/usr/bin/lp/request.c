/* $Revision: 66.3.1.1 $ */
/* routines for writing and reading request files under request directory */

#include	"lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

/* putrent -- add a request to the specified request file.
	       a request consists of a single character request code
	       followed by a space, followed by text	*/

putrent(request, text, file)
char request;
char *text;
FILE *file;
{
#ifdef REMOTE
	fprintf(file, "%c%s\n", request, text);
#else
	fprintf(file, "%c %s\n", request, text);
#endif REMOTE
}

/* rmreq -- remove a request file and associated data files */

rmreq(dest, seqno, host, ob3)
char	*dest;
int	seqno;
char	*host;
short	ob3;
{
	char cmd[sizeof(REQUEST)+DESTMAX+SEQLEN+SP_MAXHOSTNAMELEN+10];
	char arg[FILEMAX], type, rname[RNAMEMAX];
	char unlink_file[FILEMAX];
	char *unlink_ptr;
	FILE *rfile;
	extern FILE *lockf_open();

#ifdef REMOTE
	if ((ob3 & O_OB3) != 0){
		sprintf(rname, "%s/%s/cfA%03d%s", REQUEST, dest, seqno, host);
	}else{
		sprintf(rname, "%s/%s/cA%04d%s", REQUEST, dest, seqno, host);
	}
#else
	sprintf(rname, "%s/%s/r-%d", REQUEST, dest, seqno);
#endif REMOTE

#ifdef SecureWare
	if(ISSECURE)
	    lp_change_mode(SPOOL, rname, 0640, ADMIN,
				"request file before removal");
	else
	    chmod(rname, 0640);
#else
	chmod(rname, 0640);
#endif

	if((rfile = lockf_open(rname, "r", FALSE)) == NULL){
#ifdef REMOTE
		if ((ob3 & O_OB3) != 0){
			sprintf(cmd, "/bin/rm -f %s/%s/*%03d%s", REQUEST, dest, seqno, host);
		}else{
			sprintf(cmd, "/bin/rm -f %s/%s/*%04d%s*", REQUEST, dest, seqno, host);
		}
#else
		sprintf(cmd, "/bin/rm -f %s/%s/*-%d", REQUEST, dest, seqno);
#endif REMOTE
	}else{
	while(getrent(&type, arg, rfile) != EOF) {
		switch(type){
		    default:
			break;
#ifdef REMOTE
		    case R_UNLINKFILE :
#else REMOTE
		    case R_FILE :
#endif REMOTE
			strcpy(unlink_file, arg);
			break;
		}
	}

	fclose(rfile);
	unlink(rname);
	unlink_ptr = unlink_file;

#ifdef REMOTE
	if ((ob3 & O_OB3) != 0){
		sprintf(cmd, "/bin/rm -f %s/%s/*%s", REQUEST, dest, unlink_ptr+3);
	}else{
		sprintf(cmd, "/bin/rm -f %s/%s/*%s*", REQUEST, dest, unlink_ptr+2);
	}
#else
	sprintf(cmd, "/bin/rm -f %s/%s/*%s", REQUEST, dest, unlink_ptr+2);
#endif REMOTE
	}

#if defined(SecureWare) && defined(B1)
	if(ISSECURE)
	    lp_rm_req(cmd, REQUEST, dest, seqno);
	else
	    system(cmd);
#else
	system(cmd);
#endif
}
/* getrent(request, text, file) -- gets the next request file entry from file.
	'request' is the type of request entry
	'text' is the associated text

	returns: EOF on end of file
		 0 otherwise
*/

int
getrent(request, text, file)
char *request;
char *text;
FILE *file;
{
	char c, *t;

	t = text;
	*request = fgetc(file);
	if(feof(file))
		return(EOF);
	if (*request == '\n'){
		*t = '\0';
		return(0);
	}

	while((c = fgetc(file)) != '\n')
		*(t++) = c;
	*t = '\0';
	return(0);
}
