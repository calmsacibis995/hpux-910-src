static char *HPUX_ID = "@(#) $Revision: 64.1 $";
/* HPUX_ID: @(#) $Revision: 64.1 $  */
#include <stdio.h>
#include <fcntl.h>
#include <varargs.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <cluster.h>
#include <stdlib.h>

/* clustersh -- This command is used in place of cdf's that were used */
/*    to run commands on the root server. It should exist in /etc     */
/*    for the remote side of the execution, and it should be linked   */
/*    (symbolically or hard) to the "remoteroot" cdf of the commands  */
/*    in question. i.e. this command would replace a remoteroot cdf   */
/*    of the form:                                                    */
/*                                                                    */
/*          #!/bin/sh                                                 */
/*                                                                    */
/*          remsh `cnodes -r` /usr/bin/<cmd> $*                       */
/*                                                                    */
/*    clustersh is needed because remsh does not return the exit      */
/*    code of the remote command. It also has the advantage of        */
/*    passing the complete environment to the remote side.            */

#define REMOTE_NAME      "/etc/clustersh"
#define MAX_NAME_SIZE     32                  /* size of name arrays */
#define TEMP_SIZE         32                  /* temp file name size */
#define TEMP_FILE_PREFIX "c_s"
#define REMSH            "/usr/bin/remsh"

char tempfile[TEMP_SIZE] = "\0";

/* External declarations */

extern char **environ;
extern char *gethcwd();
extern cnode_t cnodeid();

/* Forward declarations */

int cleanup();
char *skip();
void errexit(), getnames(), do_local(), do_remote(), writearg();
void waitfor();

main(argc,argv)
    int argc;
    char **argv;
{
    char myname[MAX_NAME_SIZE];
    char servername[MAX_NAME_SIZE];

    signal(SIGHUP,cleanup);
    signal(SIGINT,cleanup);
    signal(SIGQUIT,cleanup);
    signal(SIGTERM,cleanup);

    getnames(myname,servername);

    if (strcmp(argv[0],REMOTE_NAME) == 0) {

	/* We are remote server, make sure we are on the root server */

	if (strcmp(myname,servername) != 0)
	    errexit("This command should only be run on the root server.\n");

	if (argc < 2)
	    errexit("Missing argument file name on command line.\n");

	strncpy(tempfile,argv[1],TEMP_SIZE);
	tempfile[TEMP_SIZE - 1] = '\0';
	do_remote();
    }
    else {

	/* We are client, make sure we are not on the root server */

	if (strcmp(myname,servername) == 0)
	    errexit("This command should not be run on the root server.\n");

	do_local(argc,argv,servername);
    }

    exit(0);
}

void
getnames(myname,servername)
    char *myname;
    char *servername;
{
    int cnode;
    char *s;
    struct cct_entry *cctptr;

    cnode = (int) cnodeid();

    /* Get my name */

    setccent();
    if ((cctptr = getcccid(cnode)) == (struct cct_entry *)0)
	errexit("Could not get cnode name\n");

    strcpy(myname,cctptr->cnode_name);

    /* Get root server name */

    setccent();
    while ((cctptr = getccent()) != (struct cct_entry *)0) {
	if (cctptr->cnode_type == 'r') {

	    strcpy(servername,cctptr->cnode_name);
	    endccent();
	    return;
	}
    }

    errexit("Could not get root server name.\n");
}

void
do_local(argc,argv,servername)
    int argc;
    char **argv;
    char *servername;
{
    register int i;
    int tempfd;
    int curumask;
    int envcount;
    char **envp;
    char cwd[MAXPATHLEN];
    char obuf[64];
    int childpid;
    char exitval;
    struct stat sbuf;

    /* Get current working directory */

    if (gethcwd(cwd,MAXPATHLEN) == (char *)0)
	errexit("Could not get current working directory.\n");

    /* Get umask */

    curumask = umask(022);

    /* Create argument file */

    sprintf(tempfile,"/tmp/%s.%d",TEMP_FILE_PREFIX,getpid());

    if ((tempfd = open(tempfile,(O_RDWR | O_CREAT | O_TRUNC), 0600)) < 0)
	errexit("Could not create argument file (%s).\n",tempfile);

    /* Count number of environment variables */

    i = 0;
    envp = environ;
    while (*envp != (char *)0) {
	i++;
	envp++;
    }
    envcount = i;

    /* Write argument file. file will contain nulls to separate  */
    /* arguments so that newlines can be contained in a argument */

    /* write out # of environment variables */

    sprintf(obuf,"%d",envcount);
    writearg(tempfd,obuf,strlen(obuf)+1);

    /* write out argc */

    sprintf(obuf,"%d",argc);
    writearg(tempfd,obuf,strlen(obuf)+1);

    /* write out umask */

    sprintf(obuf,"%d",curumask);
    writearg(tempfd,obuf,strlen(obuf)+1);

    /* write out current working directory */

    writearg(tempfd,cwd,strlen(cwd)+1);

    /* write out environment variables */

    for (i = 0; i < envcount; i++)
	writearg(tempfd,environ[i],strlen(environ[i])+1);

    /* write out command line arguments */

    for (i = 0; i < argc; i++)
	writearg(tempfd,argv[i],strlen(argv[i])+1);

    /* Fork and let child run remsh to start execution on server */

    if ((childpid = fork()) < 0)
	errexit("Could not fork (local).\n");

    if (childpid == 0) {

	/* Child -- run remsh */

	(void) execl(REMSH,"remsh",servername,REMOTE_NAME,tempfile,(char *)0);

	errexit("Exec of %s failed.\n",REMSH);
    }

    /* Parent -- wait for child to finish */

    waitfor(childpid,(int *)0);

    /* Check to see if argument file size is 1 byte. If not then exit */
    /* with a 1, otherwise read byte and exit with that value.        */

    if (fstat(tempfd,&sbuf) != 0)
	cleanup();

    if (sbuf.st_size != 1)
	cleanup();

    if (lseek(tempfd,0L,0) != 0)
	cleanup();

    if (read(tempfd,&exitval,1) != 1)
	cleanup();

    (void) unlink(tempfile);
    exit(exitval);
}

void
do_remote()
{
    register int i;
    register char *s;
    struct stat sbuf;
    int tempfd;
    char *argbuf;
    char *endbuf;
    int argbufsize;
    int argc;
    char **argv;
    int envcount;
    char **envp;
    int newumask;
    int childpid;
    int childstatus;
    char exitbyte;

    /* Open argument file */

    if ((tempfd = open(tempfile,O_RDWR)) < 0)
	errexit("Could not open argument file (%s).\n",tempfile);

    /* Get size, allocate memory and read entire contents of file */

    if (fstat(tempfd,&sbuf) != 0)
	errexit("Could not stat %s.\n",tempfile);

    if ((argbufsize = sbuf.st_size) < 2)
	errexit("Bad format in argument file (%s).\n",tempfile);

    if ((argbuf = malloc(argbufsize)) == (char *)0)
	errexit("Malloc failure.\n");

    if (read(tempfd,argbuf,argbufsize) != argbufsize)
	errexit("Read error on %s\n",tempfile);

    endbuf = argbuf + argbufsize;

    /* Get number of environment variables */

    s = argbuf;
    envcount = atoi(s);

    /* Get argc */

    s = skip(s,endbuf);
    argc = atoi(s);

    /* get umask and set it */

    s = skip(s,endbuf);
    newumask = atoi(s);
    (void) umask(newumask);

    /* get working directory and set it */

    s = skip(s,endbuf);
    if (chdir(s) != 0)
	errexit("Could not set working directory on remote.\n");

    /* Malloc space for environment pointers and argv pointers */

    envp = (char **)malloc((envcount + 1) * sizeof(char **));
    argv = (char **)malloc((argc + 1) * sizeof(char **));
    if (envp == (char **)0 || argv == (char **)0)
	errexit("Malloc failure.\n");

    /* Set up environment */

    envp[envcount] = (char *)0;
    for (i=0; i < envcount; i++) {
	s = skip(s,endbuf);
	envp[i] = s;
    }
    environ = envp;

    /* Set up argv */

    argv[argc] = (char *)0;
    for (i=0; i < argc; i++) {
	s = skip(s,endbuf);
	argv[i] = s;
    }

    /* Fork and child executes requested command. Parent waits for  */
    /* child and then returns proper exit code by writing tempfile. */

    if ((childpid = fork()) < 0)
	errexit("Fork failed.\n");

    if (childpid == 0) {

	/* Child -- execute command using execvp */

	execvp(argv[0],argv);
	errexit("Exec failed on remote.\n");
    }

    /* Parent -- wait for child to complete and then write exit status */

    waitfor(childpid,&childstatus);

    if ((childstatus & 0xff) != 0)
	exitbyte = 1;
    else
	exitbyte = ((childstatus >> 8) & 0xff);

    if (lseek(tempfd,0L,0) != 0)
	errexit("Lseek error on remote argument file.\n");

    if (ftruncate(tempfd,0) != 0)
	errexit("Could not truncate argument file.\n");

    if (write(tempfd,&exitbyte,1) != 1)
	errexit("Error in writing exit status.\n");

    close(tempfd);

    exit(0);
}

char *
skip(s,ends)
    register char *s;
    char *ends;
{
    while (*s++ != '\0')
	;

    if (s >= ends)
	errexit("Bad format in argument file (%s).\n",tempfile);

    return(s);
}

void
waitfor(pid,statusptr)
    int pid;
    int *statusptr;
{
    int wpid;

    for (;;) {

	if((wpid = wait(statusptr)) < 0)
	    errexit("Wait failed.\n");

	if (wpid == pid)
	    return;
    }
}

void
writearg(fd,data,size)
    int fd;
    char *data;
    int size;
{
    if (write(fd,data,size) != size)
	errexit("Write error on argument file.\n");
}

cleanup()
{
    /* Unlink temporary file (if it exists) */

    (void) unlink(tempfile);
    exit(1);
}

void
errexit(va_alist)
    va_dcl
{
    va_list args;
    char *fmt;

    /* print error message */

    va_start(args);
    fmt = va_arg(args,char *);
    fprintf(stderr,"clustersh: ");
    (void) vfprintf(stderr, fmt, args);
    va_end(args);
    (void) unlink(tempfile);
    exit(1);
}
