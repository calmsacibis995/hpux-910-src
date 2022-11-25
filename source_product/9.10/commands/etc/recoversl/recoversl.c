static char *HPUX_ID = "@(#) $Revision: 66.10 $";
/* Shared Library Recovery */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <sys/sysmacros.h>
#include <sys/mount.h>
#include <signal.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef NLS
#   define catgets(i, sn,mn,s) (s)
#else /* NLS */
#   define NL_SETN 1    /* set number */
#   include <nl_types.h>
#   include <locale.h>
#   include <langinfo.h>
nl_catd nlmsg_fd;
#endif /* NLS */

/* Fileset containing critical shared libraries */
static char *sh_fileset = "CORE-SHLIBS";

/* List of critical shared libraries */
static char *shared_lib[] = {
	"/lib/dld.sl",			/* dynamic loader */
	"/lib/libc.sl",			/* duh? */
	"",				/* must terminate with empty string */
};

/* messages */
static char *msg[] = {
	"",					/* empty for 0 */
	"\n%s: Checking shared libraries\n",			/* catgets 1 */
	"",							/* catgets 2 */
	"NOT OK\n\n",						/* catgets 3 */
	"One or more critical shared libraries cannot be found.\nRecovery of shared libraries requires update media and\na device capable of reading the media.  If you want to\nrecover these shared libraries, this program will assist\nyou in doing so.\n\nContinue (yes/no)? ",				/* catgets 4 */
	"You have been put into /bin/sh.  When you exit,\nthe system will continue to boot\n\nTo get back to this program, type /etc/recoversl\n",/* catgets 5 */
	"What kind of update media do you have:\n1. Cartridge, Nine-track, or DAT tape\n2. CD-ROM\n\nPlease choose 1 or 2: ",			/* catgets 6 */
	"If you have more than one update tape, use the first tape.\nPlease load tape and press <return> when ready\n",				/* catgets 7 */
	"Recovery was unsuccessful.  Check the following:\n - Correct tape\n - Tape drive on and ready\n - Tape loaded and online\n - Device file correct\n", /* catgets 8 */
	"\nRecovery was successful.  The system will now reboot\n", /* catgets 9 */
	"If you have more than one update disk, use the first disk.\nPlease insert disk and press <return> when ready\n",			/* catgets 10 */
	"Mount of CD-ROM file system failed.  Check the following:\n - Disk drive on and ready\n - Disk inserted\n - Device file correct\n",	/* catgets 11 */
	"%d errors occurred during recovery.  Check the following:\n - Correct CD-ROM\n", /* catgets 12 */
	"\nRecovery was successful.  It is suggested that you now reboot the system\n", /* catgets 13 */
	"\nA device file is required for the %s.\nIf one does not exist, it will be created for you.\n\nDoes a suitable device file already exist\non this system (yes/no/quit)? ",						/* catgets 14 */
	"Enter full pathname of device file: ",			/* catgets 15 */
	"Please enter the major and minor numbers\nfor the block special file: ", /* catgets 16 */
	"device file: %s",					/* catgets 17 */
	": not a block special file\n",				/* catgets 18 */
	": not a character special file\n",			/* catgets 19 */
	"\nIs this correct (yes/no/quit)? ",			/* catgets 20 */
	"rebooting system...\n",				/* catgets 21 */
	"\tfixed permissions on %s\n",				/* catgets 22 */
	"y",							/* catgets 23 */
	"n",							/* catgets 24 */
	"q",							/* catgets 25 */
	"You must be super-user to use this program\n", 	/* catgets 26 */
	"tape",							/* catgets 27 */
	"CD-ROM",						/* catgets 28 */
	"Please enter the major and minor numbers\nfor the character special file: ", /* catgets 29 */
	"Could not find tar in either /bin or /usr/bin!\n",	/* catgets 30 */
};

#define FILEBUFSIZE 4096

char devfile[1024];
char mntpnt[1024];
char s[1024],s2[1024];
char *yes;
char *no;
char *quit;
char filebuf[FILEBUFSIZE];
ushort mode=S_IRUSR|S_IRGRP|S_IROTH|S_IXUSR|S_IXGRP|S_IXOTH;

main(argc,argv,envp)
int argc;
char *argv[];
char *envp[];
{
	int i,n;
	int force=0;
	int boot_time=0;
	char c;
	sigset_t set,oset;
	struct cdfs_args cdfs_data;
	int uid,gid;
	struct passwd *pw;
	int err,good;
	int sfd,dfd;
	struct stat buf;

	/* block signals */
	sigfillset(&set);
	sigprocmask(SIG_SETMASK,&set,&oset);

	yes=msg[23];
	no=msg[24];
	quit=msg[25];

#if defined NLS || defined NLS16        /* initialize to the right language */
        if (!setlocale(LC_ALL,"")) {
                fputs(_errlocale(),stderr);
                nlmsg_fd = (nl_catd)-1;
        }
        else
                nlmsg_fd = catopen("recoversl",0);
#endif

/*
	printf("Bypass recoversl (y/n)? ");
	n=getresp(2,yes,no,"");
	if (n==1) exit(0);
*/

	if (getuid() != 0) {
		fputs(catgets(nlmsg_fd,NL_SETN,26,msg[26]),stderr);
		exit(1);
	}

	while ((c=getopt(argc,argv,"f"))!=EOF) {
		switch(c) {
			case 'f':
				force=1;
				break;
			case '?':
				exit(1);
		}
	}

	/* if this is spawned by init, DO NOT print "checking shared ..." */
	if (getppid()==1) 
		boot_time=1;
	else
		printf(catgets(nlmsg_fd,NL_SETN,1,msg[1]),argv[0]);
	if (libs_ok() && !force) {
		printf("\n");
		exit(0);
	}
	if (boot_time)
		printf(catgets(nlmsg_fd,NL_SETN,3,msg[3]));

startover:
	printf(catgets(nlmsg_fd,NL_SETN,4,msg[4]));
	n=getresp(1,yes,no,"");
	if (n==2) {
		if (boot_time) {
			printf(catgets(nlmsg_fd,NL_SETN,5,msg[5]));
			sigprocmask(SIG_SETMASK,&oset,&set);
			execle("/bin/sh","/bin/sh",0,envp);
			sigprocmask(SIG_SETMASK,&set,&oset);
			perror("exec");
			goto startover;
		} else {
			exit(1);
		}
	}
	printf(catgets(nlmsg_fd,NL_SETN,6,msg[6]));
	n=getresp(0,"1","2","");
	switch(n) {
		case 1: /* Tape */
			if (get_dev_file(catgets(nlmsg_fd,NL_SETN,27,msg[27]),0))
				goto startover;
			printf(catgets(nlmsg_fd,NL_SETN,7,msg[7]));
			gets(s);
			chdir("/");
			/*
			 * In 8.0, tar is in /bin, but in previous releases,
			 * it was in /usr/bin.  In case some bozo decided
			 * to move it around, we check both places.
			 */
			if (access("/bin/tar",X_OK)==0)
				strcpy(s,"/bin/tar -xvf ");
			else if (access("/usr/bin/tar",X_OK)==0)
				strcpy(s,"/usr/bin/tar -xvf ");
			else {
				printf(catgets(nlmsg_fd,NL_SETN,30,msg[30]));
				goto startover;
			}
			strcat(s,devfile);
			strcat(s," ");
			strcat(s,sh_fileset);
			strcat(s," < /dev/null");
			i=system(s);
			if (i) {
				printf(catgets(nlmsg_fd,NL_SETN,8,msg[8]));
				goto startover;
			} else {
				do_reboot(boot_time);
			}
			break;
		case 2:	/* CD-ROM */
cdrom:
			if (get_dev_file(catgets(nlmsg_fd,NL_SETN,28,msg[28]),1))
				goto startover;
			printf(catgets(nlmsg_fd,NL_SETN,10,msg[10]));
			gets(s);
			i=0;
			do {
				tmpnam(mntpnt);
				n=mkdir(mntpnt,S_IRUSR|S_IWUSR);
				if (i++ >= 10) break;
			} while (n);
			if (n) {
				perror("mkdir");
				goto startover;
			}
			cdfs_data.fspec=devfile;
#ifndef DEBUG
			i=vfsmount(MOUNT_CDFS,mntpnt,M_RDONLY,(caddr_t)(&cdfs_data));
#else
			i=vfsmount(MOUNT_UFS,mntpnt,M_RDONLY,(caddr_t)(&cdfs_data));
#endif
			if (i == -1) {
				printf(catgets(nlmsg_fd,NL_SETN,11,msg[11]));
				goto startover;
			}
			err=0;
			good=0;
			pw=getpwnam("bin");
			uid=pw->pw_uid;
			gid=pw->pw_gid;
			for (i=0;*shared_lib[i];i++) {
				printf(" - %s\n",shared_lib[i]);
				strcpy(s2,s);
				strcat(s2,shared_lib[i]);
				sfd=open(s2,O_RDONLY);
				if (sfd == -1) {
					perror(s2);
					err++;
					continue;
				}
				dfd=open(shared_lib[i],O_CREAT|O_TRUNC|O_WRONLY);
				if (dfd == -1) {
					perror(shared_lib[i]);
					err++;
					close(sfd);
					continue;
				}
				while ((n=read(sfd,filebuf,FILEBUFSIZE))>0) {
					n=write(dfd,filebuf,FILEBUFSIZE);
					if (n == -1) {
						perror(shared_lib[i]);
						err++;
						n=0;
						break;
					}
				}
				if (n == -1) {
					perror(s2);
					err++;
				} else
					good++;
				close(sfd);
				close(dfd);
				chmod(shared_lib[i],mode);
				chown(shared_lib[i],uid,gid);
			}
			umount(devfile);
			unlink(mntpnt);	/* unlink mount point */
			if (err) {
				printf(catgets(nlmsg_fd,NL_SETN,12,msg[12]));
				goto startover;
			} else {
				do_reboot(boot_time);
			}
		default:
			goto startover;
	}
}


get_dev_file(device,block)
char *device;
int block;
{
	int maj,min;
	int i,n,m;
	dev_t dev;
	struct stat buf;

	do {
again:
		printf(catgets(nlmsg_fd,NL_SETN,14,msg[14]),device);
		n=getresp(0,yes,no,quit);
		switch (n) {
			case 1:	/* yes */
				printf(catgets(nlmsg_fd,NL_SETN,15,msg[15]));
				gets(devfile);
				break;
			case 2: /* no */
				if (block)
					printf(catgets(nlmsg_fd,NL_SETN,16,msg[16]));
				else
					printf(catgets(nlmsg_fd,NL_SETN,29,msg[29]));
				scanf("%s %s",s,s2);
				if ((maj=number(s))== -1) goto again;
				if ((min=number(s2))== -1) goto again;
				dev=makedev(maj,min);
				if (block)
					m = S_IFBLK|0666;
				else
					m = S_IFCHR|0666;
				i=0;
				do {
					tmpnam(devfile);
					n=mknod(devfile,m,dev);
					if (i++ >= 10) break;
				} while (n);
				if (n) {
					perror("mknod");
					goto again;
				}
				break;
			case 3:	/* start over */
				return(1);
		}
		i=stat(devfile,&buf);
		if (i==0) {
			printf(catgets(nlmsg_fd,NL_SETN,17,msg[17]),devfile);
			if (block) {
				if (buf.st_mode&S_IFBLK)
					printf(" b");
				else {
					printf(catgets(nlmsg_fd,NL_SETN,18,msg[18]));
					goto again;
				}
			} else {
				if (buf.st_mode&S_IFCHR)
					printf(" c");
				else {
					printf(catgets(nlmsg_fd,NL_SETN,19,msg[19]));
					goto again;
				}
			}
			printf(" %d",(buf.st_rdev)>>24);
			printf(" %#06x\n",(buf.st_rdev)&0xffffff);
		} else {
			perror("stat");
			goto again;
		}
		printf(catgets(nlmsg_fd,NL_SETN,20,msg[20]));
		n=getresp(1,yes,no,quit);
		if (n==3)
			return(1);
	} while (n==2);
	return(0);
}

do_reboot(do_it)
int do_it;
{
	if (do_it) {
		printf(catgets(nlmsg_fd,NL_SETN,9,msg[9]));
		printf(catgets(nlmsg_fd,NL_SETN,21,msg[21]));
		fflush(stdout);
		sleep(5);
		reboot(RB_AUTOBOOT);
	} else {
		printf(catgets(nlmsg_fd,NL_SETN,13,msg[13]));
		exit(0);
	}
}

libs_ok()	/* check shared library and permissions */
{
	int i=0,uid,gid;
	struct passwd *pw;
	struct stat buf;

	
	pw=getpwnam("bin");
	uid=pw->pw_uid;
	gid=pw->pw_gid;
	while (*shared_lib[i] != '\0') {
		/* check to see if shared library exists */
		if (stat(shared_lib[i],&buf)==0) {
			/* check to see if it has correct permissions */
			if ((buf.st_mode&0777) != mode) {
				/* fix permissions */
				chmod(shared_lib[i],mode);
				printf(catgets(nlmsg_fd,NL_SETN,22,msg[22]),shared_lib[i]);
				if (stat(shared_lib[i],&buf)!=0 || (buf.st_mode&mode) != mode) {
					return(0);	/* something went wrong */
				}
			}
			/* check owner and group */
			if (buf.st_uid!=uid || buf.st_gid!=gid) {
				/* fix owner and group */
				chown(shared_lib[i],uid,gid);
				printf("\tfixed owner/group on %s\n",shared_lib[i]);
				if (stat(shared_lib[i],&buf)!=0 || buf.st_uid!=uid || buf.st_gid!=gid) {
					return(0);	/* something went wrong */
				}
			}
		} else {	/* file doesn't exist */
			return(0);
		}
		i++;
	}
	return(1);
}

number(arg)
register char *arg;
{
        int     base = 10;              /* selected base        */
        long    num  =  0;              /* function result      */
        int     digit;                  /* current digit        */

        if (*arg == '0')                /* determine base */
                if ((*(++arg) != 'x') && (*arg != 'X'))
                        base = 8;
                else {
                        base = 16;
                        ++arg;
                }

        while (digit = *arg++) {
                if (base == 16) {       /* convert hex a-f or A-F */
                        if ((digit >= 'a') && (digit <= 'f'))
                                digit += '0' + 10 - 'a';
                        else
                        if ((digit >= 'A') && (digit <= 'F'))
                                digit += '0' + 10 - 'A';
                }
                digit -= '0';

                if ((digit < 0) || (digit >= base)) {   /* out of range */
                        fputs("illegal number\n", stderr);
                        return(-1);
                }
                num = num*base + digit;
        }
        return (num);
}

getresp(dflt,a,b,c)
int dflt;
char *a,*b,*c;
{
	int i;
	char *rsp[3];

	rsp[0]=a;
	rsp[1]=b;
	rsp[2]=c;
	while (1) {
		fflush(stdin);
		if (dflt) printf("(%c) ",*rsp[dflt-1]);
		gets(s);
		if (*s=='\0' && dflt) {
			printf("\n");
			return(dflt);
		}
		for (i=0;i<3;i++) {
			if (rsp[i] && *rsp[i] == *s) {
				printf("\n");
				return(i+1);
			}
		}
		printf("? ");
	}
}
