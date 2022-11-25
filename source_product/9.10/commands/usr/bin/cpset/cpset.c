static char *HPUX_ID = "@(#) $Revision: 66.4 $";
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <string.h>
extern char *getenv();
extern long strtol();

/* defines values for user and group ids */
#define ROOT 0
#define RESERVED 100	/* Below this is reserved for admin logins */
#define BIN 2

/* mapping file for relocating commands */
#define DEST "/usr/src/destinations"

/* file names as they are constructed */
char *newname;	/* Name of the new file (- directory) */
char *newpath;	/* Full path name of the new file */
char *destdir;	/* Name of the destination directory */
char *Realname;	/* the actual destination name in all it's glory */

extern int errno;

int mvflag;	/* mv the file before trying to install over it */
int notmp;	/* set if no tempoary file was used to do the install */
int modefail;	/* set on mode change failure */
int ownfail;	/* set on user/group change failure */
char *tmpname;	/* Name of temp file created in directory */
char *inname;	/* Name invoked under */

main(argc,argv)
int argc;
char *argv[];
{
	inname = argv[0];
	if(argc < 3) usage();
	if(argv[1][0] == '-') {
		if(strcmp(argv[1],"-o") == NULL)
		  mvflag = 1;
		else usage();
		argv[1]=inname;
		realmain(--argc,&argv[1]);
	}
	else realmain(argc,argv);
	fprintf(stderr,"%s: Shouldn't be here...\n",inname);
}

realmain(argc,argv)
int argc;
char *argv[];
{
	struct stat st_buf;
	struct passwd *pass;
	struct group *grp;
	int len;
	int fdin, fdout;
	char bufr[4096];
	char *end;
	int owner, group, mode;

	mode=0555;	/* Default mode */
	owner=BIN;	/* Default owner */
	group=BIN;	/* Default group */
	if(geteuid() > RESERVED) {
		owner = geteuid();
		group = getegid();
	}
	switch(argc) {
	 case 3:
		break;
	 case 6:
		if((grp=(struct group *)getgrnam(argv[5])) == NULL) {
			fprintf(stderr,"%s: unknown group-id '%s'\n",inname,argv[5]);
			exit(1);
		}
		group=grp->gr_gid;
	  case 5:
		if((pass = (struct passwd *)getpwnam(argv[4])) == NULL) {
			fprintf(stderr,"%s: unknown user-id '%s'\n",inname,argv[4]);
			exit(1);
		}
		owner=pass->pw_uid;
	  case 4:
		mode=strtol(argv[3],&end,8);
		if (end-argv[3] != strlen(argv[3])) {
			fprintf(stderr,"%s: badly formed mode '%s'\n",inname,argv[3]);
			exit(1);
		}
		break;
	  default:
		fprintf(stderr,"%s: incorrect number of arguments\n",argv[0]);
		exit(1);
	}

	destdir =  (char *) malloc(strlen(argv[2]) + 1);
	strcpy(destdir,argv[2]);
	path_clean(destdir);
	path_clean(argv[1]);
	if ((newname=strrchr(argv[1],'/')) == NULL)
		newname = argv[1];
	else
		newname++;

	if(stat(argv[1],&st_buf) == -1) {
		switch(errno) {
		 case ENOTDIR:
		 case EACCES:
			fprintf(stderr,"%s: Can't access '%s'\n",inname,argv[1]);
			exit(1);
		 case ENOENT:
			fprintf(stderr,"%s: File '%s' not found\n",inname,argv[1]);
			exit(1);
		  default:
			fprintf(stderr,"%s: Unexpected error %d\n",inname,errno);
			exit(1);
		}
	}
	else
		if (S_ISDIR(st_buf.st_mode)) {
			fprintf (stderr,"%s: '%s' is a directory\n",inname,argv[1]);
			usage();
		}

	if(stat(destdir,&st_buf) == -1) {
		switch(errno) {
		 case ENOTDIR:
		 case EACCES:
			fprintf(stderr,"%s: Can't access '%s'\n",inname,argv[2]);
			exit(1);
		 case ENOENT:
			newname=strrchr(destdir,'/');
			/* assign the dest file name and adjust the directory
			   name
			*/
			if(newname != NULL) {
				if(destdir == newname) destdir="/";
				*newname++ = NULL;
			} else {
				newname=destdir;
				destdir="./";
			}
			if(stat(destdir,&st_buf) != -1) break;
			fprintf(stderr,"%s: Directory '%s' not found\n",inname,destdir);
			exit(1);
		  default:
			fprintf(stderr,"%s: Unexpected error %d\n",inname,errno);
			exit(1);
		}
	}
	else if ((st_buf.st_mode&S_IFMT) == S_IFREG || (st_buf.st_mode&S_IFMT) == 0) {
		newname=strrchr(destdir,'/');
		/* assign the dest file name and adjust the directory
		   name
		*/
		if(newname != NULL) {
			if(newname == destdir) newname="/";
			*newname++ = NULL;
		} else {
			newname=destdir;
			destdir="./";
		}
	} else if ((st_buf.st_mode&S_IFMT) != S_IFDIR) {
		fprintf(stderr, "%s: '%s' is not a directory\n",inname,destdir);
		exit(1);
	}
	
	/* now that we have a destination path decided on...lets see
	   what the destination file has to say
	*/
	do_dest();
	make_real();

	tmpname = (char *) malloc(strlen(destdir)+17);
	strcpy(tmpname,destdir);
	strcat(tmpname,"/INStmpXXXXXXXX");
	mktemp(tmpname);
	fdin=open(argv[1],O_RDONLY);
	if(fdin == -1) {
		fprintf(stderr,"%s: couldn't open '%s' \n",inname,argv[1]);
		exit(1);
	}

	fdout=open(tmpname,O_WRONLY|O_CREAT);
	if(fdout == -1) {
		if((fdout=open(Realname,O_WRONLY|O_CREAT)) == -1) {
			fprintf(stderr,"%s: couldn't create '%s'\n",inname,Realname);
			exit(1);
		} else notmp++;
	}
	while (len=read(fdin,bufr,4096)) write(fdout,bufr,len);
	close(fdin);
	close(fdout);
	/* FSDlj07589 - If notmp is set, then use "Realname" (instead of */
	/* tmpname) when setting the mode and group/owner. */
	if (notmp) {
	    if (chmod(Realname,mode) == -1) modefail++;
	    if (chown(Realname,owner,group) == -1) ownfail++;
	    if(mode > 0777)
		if (chmod(Realname,mode) == -1) modefail++;
	} else {
	    if (chmod(tmpname,mode) == -1) modefail++;
	    if (chown(tmpname,owner,group) == -1) ownfail++;
	    if(mode > 0777)
		if (chmod(tmpname,mode) == -1) modefail++;
	}
	if(modefail)
	    fprintf(stderr,"%s: warning: couldn't set modes on '%s'\n",
		inname,Realname);
	if(ownfail)
	   fprintf(stderr,"%s: warning: couldn't change user/group-id on '%s'\n",
		inname,Realname);
	if(notmp) 
	    /* FSDlj07541 - If changing the mode or owner/group fails, */
	    /* exit status should not be 0. */
	    if (ownfail || modefail)
		exit(1);
	    else			
		exit(0);
	if (newname == NULL) {
		newname = strrchr(argv[1], '/');
		if (newname == NULL) newname = argv[1];
		 else newname++;
	}
	newpath =  (char *) malloc(strlen(newname)+strlen(destdir)+2);
	strcpy(newpath,destdir);
	strcat(newpath,"/");
	strcat(newpath,newname);
	if (mvflag) {
		char *OLDname;	/* name of the OLDfile */
		OLDname=(char *) malloc(strlen(newname)+strlen(destdir)+sizeof("/OLD")+1);
		strcpy(OLDname,destdir);
		strcat(OLDname,"/OLD");
		strcat(OLDname,newname);
		if(unlink(OLDname) == -1 && errno != ENOENT) {
			fprintf(stderr,"%s: Could not unlink '%s'\n",inname,OLDname);
			exit(1);
		}
		if(link(newpath,OLDname) == -1 && errno != ENOENT) {
			fprintf(stderr,"%s: Could not link in '%s'\n",inname,destdir);
			cleanup();
		}
	}
	if ((unlink(newpath) == -1)  && (errno != ENOENT)) {
		fprintf(stderr,"%s: Couldn't unlink '%s'\n",inname,newpath);
		cleanup();
	}
	if(link(tmpname,newpath) == -1) {
		fprintf(stderr,"%s: Couldn't link in '%s'\n",inname,destdir);
		cleanup();
	}
        if (unlink(tmpname) == -1) {
                fprintf(stderr,"%s: Couldn't unlink tmp '%s'\n",inname,tmpname);
		exit(1);
	}

	/* FSDlj07541 - If changing the mode or owner/group fails, */
	/* exit status should not be 0. */
	if (ownfail || modefail)
	    exit(1);
	else			
	    exit(0);
}


usage()
{
	fprintf(stderr,"%s: Usage: %s [-o] filename destination [mode [owner [group]]]\n",inname,inname);
	exit(1);
}

/* cleans up the temporary filename created */
cleanup()
{
	if(unlink(tmpname) == -1)
	  fprintf(stderr,"%s: Couldn't unlink temporary file '%s'\n",inname,tmpname);
	exit(1);
}


char dbuf[256];
do_dest()
{
	FILE *destfd;
	char *Name;
	char *newpath;
	char *rpath;
	char *dpath;
	struct stat statbuf;

/* locate the destinations files and open it */

	if ((rpath = getenv("ROOT")) != NULL) {
		dpath = (char *) malloc(strlen(rpath) + sizeof(DEST) + 1);
		strcpy(dpath,rpath);
		strcat(dpath,DEST);
	} else {
		dpath = DEST;
	}

	if ((destfd = fopen(dpath,"r")) == NULL) return;
	/* construct the name that we are looking for */
	Name=(char *) malloc(strlen(destdir)+strlen(newname) + 2);
	strcpy(Name,destdir);
	strcat(Name,"/");
	strcat(Name,newname);
	path_clean(Name);

	while(fgets(dbuf,256,destfd) != NULL) {
		char *temp;
		if ((temp = strchr(dbuf,'\n')) == NULL) {
			fprintf(stderr,"%s: error reading '%s', line > 256 characters--ignoring destination file\n",
			   inname,dpath);
			break;
		}
		*temp = NULL;
		newpath = dbuf + strcspn(dbuf," \t");
		if (newpath == dbuf) {
			fprintf(stderr,"%s: bad entry in '%s' -- ignoring destination file\n",inname,dpath);
			break;
		}
		*newpath++ = NULL;
		newpath = newpath+strspn(newpath," \t");
		path_clean(dbuf);
		if(strcmp(Name,dbuf) == 0) {
			if(stat(Name,&statbuf) != -1) {
				fprintf(stderr,"%s: warning: '%s' exists\n",
				  inname,Name);
			}
			if ((temp = strrchr(newpath,'/')) == NULL) {
				fprintf(stderr,"%s: bad entry in '%s' -- ignoring destination file\n",
				  inname,dpath);
				break;
			}
			newname = temp + 1;
			*temp = NULL;
			destdir = newpath;
		}
	}
	fclose(destfd);
}

make_real()
{
	Realname = (char *) malloc(strlen(destdir)+strlen(newname) + 2);
	strcpy(Realname,destdir);
	strcat(Realname,"/");
	strcat(Realname,newname);
}

path_clean(str)
char *str;
{
	register char *temp;
	temp = str + strlen(str)-1;
	while (str != temp && *temp == '/') *temp-- = NULL;
	temp = str;
	while (*temp != NULL) {
		if (*temp == '/' )
			if ( *(temp+1) == '/')
				strcpy(temp+1,temp+2);
			else if (*(temp+1) == '.' && *(temp+2) == '/')
				strcpy(temp+1,temp+3);
			else temp++;
		else temp++;
	}
}
