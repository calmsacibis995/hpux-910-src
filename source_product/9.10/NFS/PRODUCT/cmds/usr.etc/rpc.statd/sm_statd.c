/* @(#)rpc.statd:	$Revision: 1.18.109.6 $	$Date: 93/12/20 11:11:00 $
*/
/* (#)sm_statd.c	1.1 87/08/05 3.2/4.3NFSSRC */
/* (#)sm_statd.c	1.2 86/12/30 NFSSRC */
#ifndef lint
static char sccsid[] = "(#)sm_statd.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif

#ifdef PATCH_STRING
static char *patch_3453="@(#) PATCH_9.0: sm_statd.o $Revision: 1.18.109.6 $ 93/12/20 PHNE_3453";
#endif
	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

	/* sm_statd.c consists of routines used for the intermediate
	 * statd implementation(3.2 rpc.statd);
	 * it creates an entry in "current" directory for each site that it monitors;
	 * after crash and recovery, it moves all entries in "current" 
	 * to "backup" directory, and notifies the corresponding statd of its recovery.
	 */

/* NOTE: sm_svc.c, sm_proc.c, sm_stad.c, pmap.c, tcp.c, and udp.c       */
/* share a single message catalog (statd.cat).  Buyer Beware! pmap.c,   */
/* tcp.c, and udp.c have messages in BOTH statd.cat and lockd.cat.      */
/* For that reason we have allocated messages in ranges per file: 	*/
/* 1 through 20 for sm_svc.c, 21 through 40 for sm_proc.c and from 41 	*/
/* on for sm_statd.c.  If we need more than 20 messages in this file  	*/
/* we will need to take into account the message numbers that are 	*/
/* already used by the other files.					*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <dirent.h>
#include <rpc/rpc.h>
#include <fcntl.h>
#include <signal.h>
#include <rpcsvc/sm_inter.h>
#include <errno.h>
#include <sys/stat.h>
#include "sm_statd.h"
#include "sm_sec.h"

#define MAXPGSIZE 8192
#define SM_INIT_ALARM 15
extern int debug;
extern int errno;
extern char STATE[20], CURRENT[20], BACKUP[20];

#ifdef NLS
extern nl_catd nlmsg_fd;
#endif NLS

int filename_size = 0;
int LOCAL_STATE;

struct name_entry {
    char *name;
    int count;
    struct name_entry *prev;
    struct name_entry *nxt;
};
typedef struct name_entry name_entry;

name_entry *find_name();
name_entry *insert_name();
name_entry *record_q;
name_entry *recovery_q;

char hostname[MAXNAMLEN];
char *malloc();


sm_try()
{
	name_entry *nl, *next;
	char	name[MAXNAMLEN];

	alarm(0);
	if(debug >= 2)
	    logmsg((catgets(nlmsg_fd,NL_SETN,76, 
			"enter sm_try: recovery_q = %s\n")), recovery_q->name);
	next = recovery_q;
	while((nl = next) != NULL) {
	    next = next->nxt;
	    if (statd_call_statd(nl->name) == 0) {
		/* remove entry from recovery_q */ 
		strcpy(name,nl->name);
		delete_name(&recovery_q, name);
		mk_sm_file(BACKUP, name,recovery_q);
	    }
	}
	if(recovery_q != NULL) {
	    signal(SIGALRM, sm_try);
	    alarm(SM_INIT_ALARM);
        }
}

/*
 * sm_run_queue() -- very similar to sm_try, except we are passed a hostname
 * of the system we should try.  This is used with the asynchronous portmap
 * changes when we finally get a response from the remote system.
 */

sm_run_queue( host )
char *host;
{
	name_entry *nl, *next;
	char	name[MAXNAMLEN];

	next = recovery_q;
	while((nl = next) != NULL) {
	    next = next->nxt;
	    if(!strcmp(nl->name, host) && statd_call_statd(nl->name) == 0){
		/* remove entry from recovery_q */ 
		strcpy(name,nl->name);
		delete_name(&recovery_q, name);
		mk_sm_file(BACKUP,name,recovery_q);
	    }
	}
}


sm_notify(ntfp)
stat_chge *ntfp;
{
	if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,41, 
		"sm_notify: %1$s state =%2$d\n")), ntfp->name, ntfp->state);
	send_notice(ntfp->name, ntfp->state);
}

/*
 * called when statd first comes up; it searches /etc/sm and /etc/sm.bak
 * to gather all entries to notify its own failure
 */
statd_init()
{
	int cc, fd;
	char buf[MAXPGSIZE];
	long base;
	int nbytes;
	DIR	*dirp_new;
	struct dirent	*dir_entry;
	char *bufp;
	int len;
	name_entry *nl, *next;
                        /* tjs 11/93 to to prevent rm and path overflow */
                        /* after strcat                                 */ 
	char rm[2*MAXNAMLEN], path[2*MAXNAMLEN], sm_file_entry[MAXNAMLEN],
		name[MAXNAMLEN], dummy_file[2*MAXNAMLEN];
	FILE *fp, *fopen();
	int err;
	int rec_len;
	int exit_val = 1;

	if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,42, "enter statd_init\n")));
	gethostname(hostname, MAXNAMLEN);

	/*
	 * The ALLOWDACACCESS privilege will be enabled for the whole
	 * routine because there are enough things to do that require
	 * it that raising and lowering it many times would be silly.
	 * It is finally lowered if a normal return is done or if
	 * an error occurs.
	 */

	ENABLEPRIV(SEC_ALLOWDACACCESS);
	if((fp = fopen(STATE, "r+")) == NULL)
	    if((fp = fopen(STATE, "w+")) == NULL) {
	       	logmsg((catgets(nlmsg_fd,NL_SETN,43,
			 "fopen(stat file) error\n")));
	        goto bad;
	    }

	if(fseek(fp, 0, 0) == -1) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,44, "fseek")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,45, "\n")));
	    goto bad;
	}
	if((cc = fscanf(fp, "%d", &LOCAL_STATE)) == EOF) {
	    if(debug >= 2)
		logmsg((catgets(nlmsg_fd,NL_SETN,46, "empty file\n")));
	    LOCAL_STATE = 0;
	}
	if(LOCAL_STATE % 2 == 0) 
	    LOCAL_STATE = LOCAL_STATE +1;
	else
	    LOCAL_STATE = LOCAL_STATE + 2;
	if(fseek(fp, 0, 0) == -1) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,47, "fseek")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,48, "\n")));
	    goto bad;
	}
	fprintf(fp, "%d", LOCAL_STATE);
	fflush(fp);
	if(fsync(fileno(fp)) == -1) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,49, "fsync")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,50, "\n")));
	    goto bad;
	}
	fclose(fp);
	if(debug)
	    logmsg((catgets(nlmsg_fd,NL_SETN,51, "local state = %d\n")), 
			LOCAL_STATE);
	if((mkdir(CURRENT, 00555)) == -1) {
	    if(errno != EEXIST) {
		log_perror((catgets(nlmsg_fd,NL_SETN,52, "mkdir current")));
		logmsg((catgets(nlmsg_fd,NL_SETN,53, "\n")));
		goto bad;
	    }
	}
	/*
	 * Just enable SEC_CHOWN here and disable it if there is an error
	 * or when it is no longer needed.
	 */

	ENABLEPRIV(SEC_CHOWN);
	if ((chown(CURRENT,2,2)) == -1) {
	        DISABLEPRIV(SEC_CHOWN);
		goto bad;
	}
	if((mkdir(BACKUP, 00555)) == -1) {
	    if(errno != EEXIST) {
	        DISABLEPRIV(SEC_CHOWN);
		log_perror((catgets(nlmsg_fd,NL_SETN,54, "mkdir backup")));
		logmsg((catgets(nlmsg_fd,NL_SETN,55, "\n")));
		goto bad;
	    }
	}
	if ((chown(BACKUP,2,2)) == -1) {
	        DISABLEPRIV(SEC_CHOWN);
		goto bad;
	}
	DISABLEPRIV(SEC_CHOWN);
	
	/*
	 * Check to see how big the maximum filename can be in CURRENT's 
	 * file system.
	 */
	strcpy(dummy_file,CURRENT);
	strcat(dummy_file,"/");
                      /* tjs 11/93 x should be value not string */
	memset((char *)((int)dummy_file + strlen(CURRENT) + 1),'x',MAXNAMLEN);
	dummy_file[strlen(CURRENT) + MAXNAMLEN + 1] = '\0';
	if ((fd = creat(dummy_file,0777)) == -1) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,56,"open current directory")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,57,"\n")));
	    goto bad;
	}
	close(fd);
	
	if ((dirp_new = opendir(CURRENT)) == NULL) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,65,"open current directory")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,66, "\n")));
	    goto bad;
	}
	while ((dir_entry = readdir(dirp_new)) != NULL) {
	    if (filename_size < dir_entry->d_namlen)
		filename_size = dir_entry->d_namlen;
	}
	if(unlink(dummy_file) == -1) {
	    log_perror(dummy_file);
	    logmsg((catgets(nlmsg_fd,NL_SETN,71, "\n")));
	    goto bad;
	}
	if (closedir(dirp_new) == -1) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,72, "close current directory\n")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,73, "\n")));
	    goto bad;
	}
	
	/* get all entries in BACKUP into recovery_q */
	if ((dirp_new = opendir(BACKUP)) == NULL) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,65,"open backup directory")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,66, "\n")));
	    goto bad;
	}
	while ((dir_entry = readdir(dirp_new)) != NULL) {
	    if(strcmp(dir_entry->d_name, ".") != 0  && 
	       		strcmp(dir_entry->d_name, "..") != 0) {
		strcpy(path,BACKUP);
		strcat(path,"/");
		strcat(path,dir_entry->d_name);
		if((fp = fopen(path,"r")) == NULL) {
		    logmsg((catgets(nlmsg_fd,NL_SETN,80,"fopen: \n")));
		    log_perror(path);
		    logmsg((catgets(nlmsg_fd,NL_SETN,81, "\n")));
		    if(errno != EACCES)
			goto bad;
		} else {
		    /* 
		     * Read first entry, if it exists, it is entered in 
		     * the recovery_q and and we continue to record the 
		     * file entries in the recovery_q.  If it does not 
		     * exist, we assume old file format.  This means that 
		     * the filename is the name of the host to contact.  
		     */
		    if (fscanf(fp,"%s\n",sm_file_entry) == EOF)
		    	insert_name(&recovery_q, dir_entry->d_name);
		    else {
			insert_name(&recovery_q, sm_file_entry);
			while (fscanf(fp,"%s\n",sm_file_entry) != EOF) 
			    insert_name(&recovery_q, sm_file_entry);
		    }
		    if (fclose(fp) != 0) {
			log_perror((catgets(nlmsg_fd,NL_SETN,83,"fclose")));
			logmsg((catgets(nlmsg_fd, NL_SETN,84, "\n")));
			goto bad;
		    }
		}
	    }
	}
	if(closedir(dirp_new) == -1) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,72, 
			"close backup directory\n")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,73, "\n")));
	    goto bad;
	}

	/* get all entries in CURRENT into recovery_q */
	if((dirp_new = opendir(CURRENT)) == NULL) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,65,"open current directory")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,66, "\n")));
	    goto bad;
	}
	while ((dir_entry = readdir(dirp_new)) != NULL) {
	    if(strcmp(dir_entry->d_name, ".") != 0  && 
		   strcmp(dir_entry->d_name, "..") != 0) {
		strcpy(path,CURRENT);
		strcat(path,"/");
		strcat(path,dir_entry->d_name);
		if((fp = fopen(path,"r")) == NULL) {
		    logmsg((catgets(nlmsg_fd,NL_SETN,80,"fopen: \n")));
		    log_perror(path);
		    logmsg((catgets(nlmsg_fd,NL_SETN,81, "\n")));
		    if(errno != EACCES)
			goto bad;
		} else {
		    /* 
		     * Read first entry, if it exists, it is entered in 
		     * the recovery_q and and we continue to record the 
		     * file entries in the recovery_q.  If it does not 
		     * exist, we assume old file format.  This means that 
		     * the filename is the name of the host to contact.  
		     */
		    if (fscanf(fp,"%s\n",sm_file_entry) == EOF)
		        insert_name(&recovery_q, dir_entry->d_name);
		    else {
			insert_name(&recovery_q, sm_file_entry);
			while (fscanf(fp,"%s\n",sm_file_entry) != EOF) 
			    insert_name(&recovery_q, sm_file_entry);
		    }
		    if (fclose(fp) != 0) {
			log_perror((catgets(nlmsg_fd,NL_SETN,83,"fclose")));
			logmsg((catgets(nlmsg_fd, NL_SETN,84, "\n")));
			goto bad;
		    }
		}
	    }
	}
	if(closedir(dirp_new) == -1) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,72, 
			"close current directory\n")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,73, "\n")));
	    goto bad;
	}

	/*
	 *  Recreate sm.bak files from the recovery_q.  
	 */
	
	next = recovery_q;
	while (next != NULL) {
	    mk_sm_file(BACKUP,next->name,recovery_q);
	    next = next->nxt;
	}
	
	/*
	 * Remove all entries in CURRENT 
	 */

	if((dirp_new = opendir(CURRENT)) == NULL) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,56,"open current directory")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,57, "\n")));
	    goto bad;
	}
	while ((dir_entry = readdir(dirp_new)) != NULL) {
	    if(strcmp(dir_entry->d_name, ".") != 0  &&
		strcmp(dir_entry->d_name, "..") != 0) {
		/* Remove all entries in CURRENT */
		strcpy(rm , CURRENT);
		strcat(rm, "/");
		strcat(rm, dir_entry->d_name);
	    	if(unlink(rm) == -1) {
		    log_perror(rm);
		    logmsg((catgets(nlmsg_fd,NL_SETN,71, "\n")));
		    goto bad;
	    	}
	    }
	}
	if (closedir(dirp_new) == -1) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,63, 
		"close current directory\n")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,64, "\n")));
	    goto bad;
	}

	/*
	 * Notify remote rpc.statd processes of our recovery from crash
	 */
	next = recovery_q;
	while((nl = next) != NULL) {
	    next = next->nxt;
	    if(statd_call_statd(nl->name) == 0) {
	    	/* remove entry from recovery_q */ 
		strcpy(name,nl->name);
		delete_name(&recovery_q, name);
		mk_sm_file(BACKUP,name,recovery_q);
	    }
	}

	/*
	 * notify statd 
	 */
	if(recovery_q != NULL) {
	    signal(SIGALRM, sm_try);
	    alarm(SM_INIT_ALARM);
        }
        DISABLEPRIV(SEC_ALLOWDACACCESS);
	return;
bad:
    /* 
     * An exit point where all of the error cases will
     * go through.  It was added just so I didn't have to have
     * DISABLEPRIV(SEC_ALLOWDACACCESS) all over the place
     * taking up code space.  It isn't structured code, but
     * sometime you break the rules.
     */
     DISABLEPRIV(SEC_ALLOWDACACCESS);
     exit(exit_val);
}

xdr_notify(xdrs, ntfp)
XDR *xdrs;
stat_chge *ntfp;
{
	if(!xdr_string(xdrs, &ntfp->name, MAXNAMLEN+1)) {
	    return(FALSE);
	}
	if(!xdr_int(xdrs, &ntfp->state)) {
	    return(FALSE);
	}
	return(TRUE);
}

statd_call_statd(name)
char *name;
{
	stat_chge ntf;
	int err;

	ntf.name =hostname;
	ntf.state = LOCAL_STATE;
	if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,74, 
		"statd_call_statd at %s\n")), name);
	if((err = call_tcp(name, SM_PROG, SM_VERS, SM_NOTIFY, 
		xdr_notify, &ntf, xdr_void, NULL, 0)) == (int)RPC_SUCCESS
		|| err == (int)RPC_PROGNOTREGISTERED) {
	    return(0);
	}
	else {
	    if ( err != (int) RPC_TIMEDOUT ) {
		logmsg( (catgets(nlmsg_fd,NL_SETN,75, "statd cannot talk to statd at %s")), clnt_spcreateerror(name));
	    }
	    return(-1);
	}
}

char *
xmalloc(len)
unsigned len;
{
	char *new;

	if((new = mem_alloc(len)) == 0) {
	    log_perror((catgets(nlmsg_fd,NL_SETN,77, "malloc")));
	    logmsg((catgets(nlmsg_fd,NL_SETN,78, "\n")));
	    return(NULL);
	}
	else {
	    memset(new, 0, len);
	    if ( debug > 2 )
		logmsg("xmalloc(%d) = %x", len, new);
	    return(new);
	}
}

void
xfree(a)
char **a;
{
        if(*a != NULL) {
            mem_free(*a,0);
	    if ( debug > 2)
		logmsg("xfree(%d) = %x", *a);
            *a = NULL;
        }
}



/*
 * the following two routines are very similar to
 * insert_mon and delete_mon in sm_proc.c, except the structture
 * is different
 */
name_entry *
insert_name(namepp, name)
name_entry **namepp;
char *name;
{
	name_entry *new;

	new = (name_entry *) xmalloc(sizeof(struct name_entry)); 
	new->name = xmalloc(strlen(name) + 1);
	strcpy(new->name, name);
	new->nxt = *namepp;
	if(new->nxt != NULL)
	    new->nxt->prev = new;
	*namepp = new; 
	return(new);
}

delete_name(namepp, name)
name_entry **namepp;
char *name;
{
	name_entry *nl;

	nl = *namepp;
	while(nl != NULL) {
	    if(strcmp(nl->name, name) == 0) {/*found */
		if(nl->prev != NULL)
			nl->prev->nxt = nl->nxt;
		else 
			*namepp = nl->nxt;
		if(nl->nxt != NULL)
		    nl->nxt->prev = nl->prev;
		xfree(&nl->name);
		xfree(&nl);
		return;
	    }
	    nl = nl->nxt;
	}
	return;
}

name_entry *
find_name(namep, name)
name_entry *namep;
char *name;
{
	name_entry *nl;

	nl = namep;
	while(nl != NULL) {
	    if(strcmp(nl->name, name) == 0) {
		return(nl);
	    }
	    nl = nl->nxt;
	}
	return(NULL);
}

record_name(name, op)
char *name;
int op;
{
	name_entry *nl;
	int fd;
	char path[MAXNAMLEN];

	if(op == 1) { /* insert */
	    if((nl = find_name(record_q, name)) == NULL) {
		nl = insert_name(&record_q, name);
		mk_sm_file(CURRENT,name,record_q);
	    }
	    nl->count++;
	}
	else { /* delete */
	    if((nl = find_name(record_q, name)) == NULL) {
		return;
	    }
	    nl->count--;
	    if(nl->count == 0) {
		delete_name(&record_q, name);
		mk_sm_file(CURRENT,name,record_q);
	    }
	}
	
}

sm_crash()
{
	name_entry *nl, *next;

	if(record_q == NULL)
	    return;
	next = record_q;	/* clean up record queue */
	while((nl = next) != NULL) {
	    next = next->nxt;
	    delete_name(&record_q, nl->name);
	}

	if(recovery_q != NULL) { /* clean up all onging recovery act*/
	    if(debug) logmsg((catgets(nlmsg_fd,NL_SETN,87, 
			"sm_crash clean up\n")));
	    alarm(0);
	    next = recovery_q;
	    while( (nl = next) != NULL) {
		next = next ->nxt;
		delete_name(&recovery_q, nl->name);
	    }
	}
	statd_init();
}

/* 
 * mk_sm_file
 * 
 * This routine was added to create files in /etc/sm and /etc/sm.bak for 
 * storage of status information.  This routine was added to fix a problem
 * in storing status information on short filename systems.
 *
 */

mk_sm_file(dir_path,name,queue_ptr)
char	*dir_path;
char	*name;
name_entry *queue_ptr;

{
	char		sm_file_name[MAXNAMLEN];
	char		*temp_file_name;
                        /* tjs 11/93 to prevent strcat overflow */
	char		path[2*MAXNAMLEN];
	int		empty_file = 1, exit_val = 1;
	FILE		*fp, *fopen();
	name_entry	*next;
	char		*mktemp();

	/*
	 * Set umask to 0200
	 */
	umask(S_IRUSR | S_IRWXG | S_IRWXO);

	/* 
	 * Create an /etc/sm or /etc/sm.bak filename of proper length.  
	 * Note: this is different for short and long filename systems.
	 */

        filename_size = strlen(name);
        if (filename_size > MAXNAMLEN-1) {
               filename_size = MAXNAMLEN-1; }
        memcpy(sm_file_name,name,filename_size);
        sm_file_name[filename_size] = '\0';

	/*
	 * Check for entries into the monitor queue that should be placed in
	 * the file that we are creating.  If entries exist, place them in
	 * a temporary file and rename the temporary file to the real file.
	 * If such queue entries do not exist, unlink the real file.
	 */

	next = queue_ptr;
	ENABLEPRIV(SEC_ALLOWDACACCESS); 
	while (next != NULL) { 

		if(  ((filename_size=MAXNAMLEN-1) &&
                     (strncmp(next->name,sm_file_name, filename_size) == 0))
               		|| (strcmp(next->name,sm_file_name)== 0) )  {
		if (empty_file == 1) { 
                              /* tjs 11/93 to prevent temp_file_name */
                              /* buffer overflow after strcat        */  
		    if ((temp_file_name = (char *) xmalloc(2*MAXNAMLEN)) == NULL)
			goto bad;
		    strcpy(temp_file_name,dir_path);
		    strcat(temp_file_name,"/tempXXXXXX");
		    temp_file_name = mktemp(temp_file_name);
		    if (debug >= 2) 
			logmsg((catgets(nlmsg_fd,NL_SETN,79, 
				"create monitor entry %s\n")), temp_file_name);
		    if ((fp = fopen(temp_file_name,"w")) == NULL) {
			logmsg((catgets(nlmsg_fd,NL_SETN,80,"fopen: \n")));
			log_perror(temp_file_name);
			logmsg((catgets(nlmsg_fd,NL_SETN,81, "\n")));
			goto bad;
		    } else {
			empty_file = 0;
		    }
		}
		fprintf(fp,"%s\n",next->name);
	    }
	    next = next->nxt;
	}
	strcpy(path,dir_path);
	strcat(path,"/");
	strcat(path,sm_file_name);
	if (empty_file == 1) {
	    if (unlink(path) == -1) {
		log_perror(path);
		logmsg((catgets(nlmsg_fd,NL_SETN,71, "\n")));
		if (errno != ENOENT)
		    goto bad;
	    }
	} else {
	    if (fclose(fp) != 0) {
		log_perror((catgets(nlmsg_fd,NL_SETN,83, "fclose")));
		logmsg((catgets(nlmsg_fd, NL_SETN,84, "\n")));
		goto bad;
	    }
	    if (rename(temp_file_name,path) == -1){
		logmsg((catgets(nlmsg_fd,NL_SETN,80, "rename: \n")));
		log_perror(path);
		logmsg((catgets(nlmsg_fd,NL_SETN,81, "\n")));
		goto bad;
	    }
	    xfree(&temp_file_name);
	}
	DISABLEPRIV(SEC_ALLOWDACACCESS);
	return;
bad:
    /* 
     * An exit point where all of the error cases will
     * go through.  It was added just so I didn't have to have
     * DISABLEPRIV(SEC_ALLOWDACACCESS) all over the place
     * taking up code space.  It isn't structured code, but
     * sometime you break the rules.
     */
     DISABLEPRIV(SEC_ALLOWDACACCESS);
     exit(exit_val);
}

/* 
 *  semclose() is a routine (found in lockd/sem.c) used by the lock manager 
 *  in order to release a binary semaphore so that another lock manager may 
 *  be started.  We have a dummy one here because udp.c calls semclose before 
 *  it calls exit(1).  In lockd, the call will release the semaphore, here
 *  we do nothing
 */

semclose() { }
