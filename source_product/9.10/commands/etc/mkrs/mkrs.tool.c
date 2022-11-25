#include <stdio.h>
#include <sys/reboot.h>
#include <sys/sysmacros.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <nlist.h>
#include <machine/param.h>
#include <sys/mknod.h>

#define max_length 258

/*The following are used as constants for numbered choices in the main menu. */
#define RESET_PASSWD   1
#define SHELL_ESCAPE   2
#define AUTO_RECOVERY  3
#define EXIT           4
#define HELP           5

/*The following are used as constants for numbered choices in super block menu */
#define CONTINUE_RECOVERY  1
#define REBOOT_HALT        2

/* the following are used by the mkrs_swap function */
#define SWAP_DEV	0
#define SWAP_START	1
#define SWAP_SIZE	2

/**************************Global Vars************************************/

struct log_entry {
	char *s;
	struct log_entry *next;
};
struct log_entry *log=0;
struct log_entry *tail;
int no_mem=0;
char dummy[max_length];		/* used as a dummy var when the screen
				   needs to stop and wait for a return */
extern int errno;
extern char *sys_errlist[];
int small=0;			/* non-zero if "big" recovery system */
char recdirname[max_length];	/* name of "old files" directory */

/*These are special escape sequences */

char *clr_screen =    "\033H\033J";
char *INV =           "\033&dB"; /*inverse video*/
char *ENDH =          "\033&d@"; /*end enhancement*/

/*************************Table of Messages*******************************/
/*
This is a table of messages for the log file and messages appearing on
the screen
*/

char *AUTO_REC = "AUTOMATIC RECOVERY\n\n";
char *AUTOREC_DONE = "AUTOMATIC RECOVERY HAS FINISHED.\n\n\
The root file system may now be rebooted successfully.\n\
By selecting the option to exit the recovery system and reboot\n\
the root file system, you will be rebooted in the root file\n\
system in single user state with a working file system.\n\
Minimal fixes have been made.  They are logged in /tmp/recovery.log\n";

char *AUTOREC_ENTER = "\
The automatic recovery option will create new versions of all files\n\
necessary to let you reboot in single user state in the root file\n\
system.  A list of these files can be seen in the help option.\n\n\
All old versions of files will be saved in the directory /tmp/recovery.xxxx\n\
(where xxxx is the month and day) on the root file system.  This\n\
is available so that an old file can easily be recovered.\n\n\
All actions taken during the automatic recovery will be printed on\n\
the screen but there is no user interaction once it is started.\n\
All actions will also be logged in /tmp/recovery.log and can be\n\
viewed after the system reboots.\n\n\
Once automatic recovery is finished, you will be prompted to continue,\n\
and then put in the recovery system main menu.\n\n";

char *BAD_SB = "THE SUPER BLOCK IS BAD.\n\n ";
char *CLEAR_SUPASS =  "\nThe root password is being cleared.  Root will be\n\
forced to set a new passwd at the next login.\n";
char *CONT_RECOVERY = "\nCONTINUING RECOVERY...\n\n";
char *COPY1 = "Copied old ";
char *COPY2 = "to ";
char *CTRL_D = "\nWHEN THE FSCK IS COMPLETE,  PLEASE HIT  CTRL-D  TO CONTINUE RECOVERY.\n";
char *ERRORNO_MSG = "The error number is: ";
char *ERROR_SB = "/dev/rdsk/realroot: BAD SUPER BLOCK: MAGIC NUMBER WRONG\012";
char *EXECUTE_FSCK = "\nPLEASE EXECUTE   fsck /dev/rdsk/realroot   AND ANSWER THE FSCK\nQUESTIONS ACCORDINGLY.\n";
char *FAIL_FSCK = "\nSINCE FSCK WAS UNSUCCESSFUL,  YOU MUST REINSTALL OR REBOOT FROM BACKUP.\n";
char *FSCKB_EXIT = "\nWHEN THE FSCK IS SUCCESSFULLY OR UNSUCCESSFULLY COMPLETED,\nPLEASE HIT  CTRL-D  TO CONTINUE.\n";
char *FSCK_B = "\nPLEASE EXECUTE  fsck -b superblock /dev/rdsk/realroot  AND ANSWER\nTHE FSCK QUESTIONS ACCORDINGLY.\n";
char *FSCK_COM = "\nFSCK COMPLETED\n";
char *FSCK_FAIL = "\nIF NONE OF THE ALTERNATE SUPERBLOCKS WORK WITH THE FSCK, YOU MUST EXIT AND REINSTALL\n\
OR REBOOT FROM BACKUP.\n";
char *HELP_OPT = "\nHELP\n\n\
1) Remove the root passwd\n\n\
This option will clear root's password in the password file so that the\n\
super user can log in as root and set a new password after rebooting the file\n\
system.  This option is offered in case the root password is forgotten\n\
or unknown.\n\n\
NOTE:  The password file stays intact.  Only the root password is cleared.\n\n";

char *HELP_2 = "2) Work in a shell to perform recovery manually\n\n\
This option is offered so that an experienced user can look at the root file\n\
system and repair files by executing unix commands.  Manual recovery can be\n\
exited by pressing CTRL-D.  The user will be put back in the recovery main menu.\n\n\
The user SHOULD NOT unmount the root file system.  The root file system cannot\n\
be accessed after an unmount; therefore, an automatic recovery would also fail\n\
after this action.\n\n";

char *HELP_3 = "3) Perform an automatic recovery\n\n\
This option performs recovery actions without user interaction.  Automatic\n\
recovery does minimal fixes to the root file system so that the user can\n\
reboot (reboot root file system option) in single user state in the root file\n\
system with all important files working and intact.  At this point, this system\n\
is in good enough working condition for the user to make other fixes necessary.\n\
All recovery steps taken are displayed on the screen and also logged in the\n\
/tmp/recovery.log file on the root file system.  \n\n\
The following files are replaced:\n";

#ifdef __hp9000s300
char *HELP_3_FILES = "  /etc/fsck\t\t/etc/update\n\
  /usr/bin/tcio\t\t/bin/cpio\n\
  /bin/tar\t\t/etc/init\n\
  /etc/inittab\t\t/etc/passwd\n\
  /bin/sh\t\t/lib/dld.sl\n\
  /lib/libc.sl\n";
char *HELP_3_FILES_SMALL = "  /etc/fsck\t\t/bin/cpio\n\
  /etc/init\t\t/etc/inittab\n\
  /etc/passwd\t\t/bin/sh\n\
  /lib/dld.sl\t\t/lib/libc.sl\n";
#else
char *HELP_3_FILES = "  /etc/fsck\t\t/etc/update\n\
  /usr/bin/tcio\t\t/bin/cpio\n\
  /bin/tar\t\t/etc/init\n\
  /etc/inittab\t\t/etc/passwd\n\
  /etc/mkboot\t\t/usr/lib/uxbootlf.700\n\
  /bin/sh\t\t/lib/dld.sl\n\
  /lib/libc.sl\n";
char *HELP_3_FILES_SMALL = "  /etc/fsck\t\t/bin/cpio\n\
  /etc/init\t\t/etc/inittab\n\
  /etc/passwd\t\t/bin/sh\n\
  /etc/mkboot\t\t/usr/lib/uxbootlf.700\n\
  /lib/dld.sl\t\t/lib/libc.sl\n";
#endif

char *HELP_3b = "The following device files are remade:\n\
  /dev/console\n\
  /dev/kmem\n\
  /dev/mem\n\
  /dev/null\n\
  /dev/swap\n\
  /dev/syscon\n\
  /dev/systty\n\
  /dev/tty\n\
  /dev/dsk/0s0\n\n\
Also:\n\
The boot area is replaced\n\
The hp-ux file is replaced \n\n";

char *HELP_3c = "\
NOTE:  If your recovery system is not up to date, there is a chance that\n\
your hp-ux (copied from the recovery system) will not boot.  If that is\n\
the case, there are a couple options.\n\n\
1.  Boot using SYSBCKUP \n\
2.  If you beleive that your hp-ux on the root file system was not\n\
    corrupt, you may copy it back by choosing the manual recovery \n\
    option and executing  cp /disc/tmp/recovery.xxxx/hp-ux /disc/hp-ux.\n\
    (xxxx is the numbers of the month and day.)\n\
3.  If neither of these work, you must reinstall.\n\n\
NOTE:  All original files that existed before automatic recovery\n\
are copied under their original names in the /tmp/recovery.xxxx\n\
directory (xxxx is the month and day).  If a file is replaced by\n\
the automatic recovery, and the original copy is still needed,\n\
it can be restored from that directory.\n";

#ifdef __hp9000s300
char *HELP_4 = "4) Exit recovery system and reboot the root file system\n\n\
This option should be used once the user knows the root file\n\
system is fixed enough to reboot properly.  This may occur after\n\
the completion of removing the root password, successfully completing\n\
a manual recovery, or doing an automatic recovery (options 1,2, and 3).\n";
#else
char *HELP_4 = "4) Exit recovery system and halt\n\n\
This option should be used once the user knows the root file\n\
system is fixed enough to reboot properly.  This may occur after\n\
the completion of removing the root password, successfully completing\n\
a manual recovery, or doing an automatic recovery (options 1,2, and 3).\n";
#endif

char *HELP_RETURN = "PRESS RETURN TO CONTINUE HELP\n\n";
char *FSCK_INTER = "\nFSCK INTERRUPTED\n";
char *FSCK_MNT_COMPLETE = "The fsck and mount have completed successfully.";
char *FS_MOD = "\nTHE ROOT FILE SYSTEM IS MODIFIED\n";
char *INITTAB_LINE = "is:s:initdefault:";
char *LINE = "\n-----------------------------------------------------------------------\n\n";
char *LOG_TITLE = "\nLOG FILE FOR RECOVERY SYSTEM -- CONTAINS RECORD OF RECOVERY ACTIONS\n\n";
char *MAN_HEADER = "MANUAL RECOVERY\n\n";
char *MAN_REC = "Refer to the Creating and Using a Recovery System\n\
section in your System Administrator Manual to perform a manual recovery.\n\
\nWhen you are finished with the manual recovery,  press CTRL-D to return\n\
to the Main Menu of the recovery system.\n\n\
The root file system has already been checked (fsck) and is mounted on /disc.\n\
Do not unmount the root file system or you will not be able to access it\n\
for recovery.\n\n";
char *MOUNT_FAIL = "The root file system could not be mounted.  The user\n\
will be put in a shell to perform a manual recovery.  Press CTRL-D when\n\
the file system is mounted, otherwise the recovery cannot be completed.\n\n";
char *NOFIX_FS = "\nCOULD NOT AUTOMATICALLY FIX FILE SYSTEM\n";
char *NO_RETURN = "\nDo you want to continue the automatic recovery? (yes or no) ";
char *PASSWD_LINE = "root:,..:0:1::/:/bin/sh\n";
char *PASSWD_NOT_THERE = "The /etc/passwd file does not exist on the root file system.\n\
A new password file has been made with the root entry having no password.\n";
char *PREPARE = "\nThe root file system is being checked (fsck -p) and the\n\
root disk will be mounted on /disc.\n\n";
char *PREPARE2 = "\nSTANDBY FOR POSSIBLE USER INTERACTION.\n\n";
char *REBOOT_FAIL = "The root file system is unable to reboot.\n";
char *REC_HEADER = "HP-UX RECOVERY SYSTEM   2.0       ";
char *RETURN = "\nPRESS RETURN TO CONTINUE.\n";
char *RSHELL_ST = "\nA SHELL IS BEING STARTED\n";
char *SBTAB_EMPTY = "\nTHE /etc/sbtab FILE IS EMPTY\n";
char *SBTAB_LOOK = "\nPLEASE LOOK AT THE /etc/sbtab FILE LISTED AND EXECUTE\n\
fsck -b superblock /dev/rdsk/realroot\n\
WHERE THE superblock OPTION IS A NUMBER\n\
TAKEN FROM THE /etc/sbtab FILE\n\n";
char *SBTAB_NOT_THERE = "\nTHE /etc/sbtab FILE DOES NOT EXIST.\n";
char *SETPASS = "\nAt next reboot of the system,  IMMEDIATELY LOGIN AS ROOT AND SET THE PASSWD\n";
char *SUPASS_HEADER = "REMOVE ROOT PASSWORD\n\n";
char *SUPERBLOCK_ALTERN = "\nTHE /etc/sbtab CANNOT BE OPENED, SUPERBLOCK 16 IS COMMON AND MAY BE TRIED\n\
To do this type   fsck -b 16 /dev/rdsk/realroot\n";
char *TIME_FSCK = "\nThis process will take several minutes.\n\n";
char *UNEXP_ERROR = "\nUNEXPECTED ERROR DURING FSCK\n";
char *UNLINK_FAIL = "The unlink failed and the file was not removed.\nThis could cause an old file to not be replaced.\n";
char *WHERE_TO_FIND_STUFF = "All recovery steps taken are displayed on the screen and also logged in the\n\
/tmp/recovery.log file on the root file system.  The file can be read after\n\
rebooting the root file system.\n\n";


/****************This is a table of files referenced**********************/


char *BOOT= "/etc/boot";
char *BOOT_MSG = "/disc/tmp/boot.msg";
char *DISC = "/disc";

char *DEV_ROOT = "/dev/dsk/realroot";
char *DEV_RROOT = "/dev/rdsk/realroot";

char *FSCK_EMSG = "/tmp/fsck.emsg";
char *FSCK_RC = "/tmp/ret.code";

char *CONSOLE = "/disc/dev/console"; 
char *DNULL = "/disc/dev/null"; 
char *DSK = "/disc/dev/dsk/0s0"; 
char *DSWAP = "/disc/dev/dswap"; 
char *INITTAB = "/disc/etc/inittab";
char *KMEM = "/disc/dev/kmem"; 
char *LOG_FILE = "/disc/tmp/recovery.log";
char *MEM = "/disc/dev/mem"; 
char *PASSWD = "/disc/etc/passwd";
char *RECOVERY_DIRECTORY = "/disc/tmp/recovery.";
char *SBTAB = "/etc/sbtab";
char *SYSCON = "/disc/dev/syscon"; 
char *SYSTTY = "/disc/dev/systty"; 
char *TMP_PASSWD = "/disc/tmp/passwd.old";
char *TTY = "/disc/dev/tty"; 

char *RHP_UX = "/hp-ux"; 
char *RCPIO = "/bin/cpio";
char *RDLD_SL = "/lib/dld.sl";
char *RFSCK = "/etc/fsck";
char *RINIT = "/etc/init";
char *RLIBC_SL = "/lib/libc.sl";
char *RPASSWD = "/etc/passwd";
char *RSHELL = "/bin/sh";
char *RTAR = "/bin/tar";
char *RTCIO = "/usr/bin/tcio";
char *RUPDATE = "/etc/update";
char *RMKBOOT = "/etc/mkboot";
char *RUXBOOTLF_700 = "/usr/lib/uxbootlf.700";

/****************************Main*****************************************/

main(argc,argv)
int argc;
char *argv[];
{
	char *s;

	if (argc>1) {
		if (!strcmp(argv[1],"dev"))
			mkrs_swap(SWAP_DEV);
		else if (!strcmp(argv[1],"start"))
			mkrs_swap(SWAP_START);
		else if (!strcmp(argv[1],"size"))
			mkrs_swap(SWAP_SIZE);
		else {
			fprintf(stderr,"illegal option: %s\n",argv[1]);
			exit(1);
		}
		exit(0);
	}
	s=getenv("PATH");
	if (!s || !*s)
		putenv("PATH=/bin:/etc");
	mkrs_system("/bin/stty erase \\^h intr \\^c kill \\^z eof \\^d");
	prep_rootfs();
	printf("%s",WHERE_TO_FIND_STUFF);
	printf("\n%s",RETURN);
	gets(dummy);
	small=access(RUPDATE,F_OK);	/* is this a -s recovery system? */
	main_menu();
}

/****************************Main_menu************************************/

/* Main_menu displays the first user interface menu giving the user
the option of automatic recovery,  escaping into a shell to do 
manual recovery processes, clearing the root passwd if it was
forgotten, looking at help or rebooting the root file system. */

main_menu()
{
char selection[max_length];
int option;
int t;

while (1) {
t=time(0);
printf("%s\n%s%s\n",clr_screen,REC_HEADER,asctime(localtime(&t)));
printf ("  Select one of the following options by number: \n\n");
printf ("    %d) Remove the root password\n", RESET_PASSWD);
printf ("    %d) Work in a shell to perform recovery manually\n", SHELL_ESCAPE);
printf ("    %d) Perform an automatic recovery\n", AUTO_RECOVERY);
#ifdef __hp9000s300
printf ("    %d) Exit recovery system and reboot root file system\n", EXIT);
#else
printf ("    %d) Exit recovery system and halt\n", EXIT);
#endif
printf ("    %d) Help\n", HELP);
printf (" \nSelection >> ");
gets (selection);
option = atoi(selection);
switch(option) {

case RESET_PASSWD:
	{
	printf("%s", clr_screen);
        log_file(SUPASS_HEADER);
	supass_fix();
        break;
	}

case SHELL_ESCAPE:
	{
	printf ("%s", clr_screen); 
        printf("%s", MAN_HEADER);
	log_file(MAN_REC);
	run_shell();
        break;
	}

case AUTO_RECOVERY:
	{
	printf("%s", clr_screen); 
	printf ("%s", AUTO_REC);
        printf("%s", AUTOREC_ENTER);
        printf("%s", NO_RETURN);
        gets(dummy);
        if (strcmp(dummy, "y") == 0)
	  auto_rec();
        else if (strcmp(dummy, "yes") == 0)
          auto_rec();
        else if (strcmp(dummy, "Y") == 0)
          auto_rec();
        else if (strcmp(dummy, "YES") == 0)
          auto_rec();
	break;
	}

case HELP:
     	{
	printf("%s", clr_screen); 
    	 printf("%s", HELP_OPT);
    	 printf("\n%s",HELP_RETURN);
	 gets(dummy);
	printf("%s", clr_screen); 
 	 printf("%s", HELP_2);
    	 printf("\n%s",HELP_RETURN);
	 gets(dummy);
	printf("%s", clr_screen); 
	 printf("%s", HELP_3);
	 if (small)
	 	printf("%s", HELP_3_FILES_SMALL);
	 else
	 	printf("%s", HELP_3_FILES);
    	 printf("\n%s",HELP_RETURN);
	 gets(dummy);
	printf("%s", clr_screen); 
	 printf("%s", HELP_3b);
    	 printf("\n%s",HELP_RETURN);
	 gets(dummy);
	printf("%s", clr_screen); 
	 printf("%s", HELP_3c);
    	 printf("\n%s",HELP_RETURN);
	 gets(dummy);
	printf("%s", clr_screen); 
         printf("%s", HELP_4);
    	 printf("\n%s",RETURN);
	 gets(dummy);
         break; 
        }

case EXIT:
	{
   	rebootfs();
        break;
	}

default:
	{
	printf (" \n");
	printf (" \n");
	printf("INVALID selection\n");
	printf("\n%s",RETURN);
	gets(dummy);
        break;
	}


                  } /*End of switch*/
                 } /*End of while*/
}

/************************Run_command***************************************/

run_command(format,arg1,arg2,arg3,arg4,arg5,arg6)
char *format;
char *arg1,*arg2,*arg3,*arg4,*arg5,*arg6;
{
	char cmd[max_length];

	sprintf(cmd,format,arg1,arg2,arg3,arg4,arg5,arg6);
	return mkrs_system(cmd);
}

/************************Run_shell*****************************************/

run_shell()
{
	if (mkrs_system(RSHELL)) {
		log_file("Shell failure: %s\n",sys_errlist[errno]);
	}
}

/************************Prepare root file system**************************/
 
/*
The system must be "prepared" by first doing the fsck, mounting the file
system and creating the recovery directory.  The fsck is needed to mount
and the mount is needed to perform any other recovery that accesses files
on the root file system.  After this, the main menu is executed.
*/

prep_rootfs()
{
int t;

	t=time(0);
	printf("%s\n%s%s\n",clr_screen,REC_HEADER,asctime(localtime(&t)));
	printf("%s", PREPARE);
	printf("%s", TIME_FSCK);
	printf("%s", PREPARE2);
	run_fsck();
	mount_fs();
	log_file(FSCK_MNT_COMPLETE);
	printf("%s", LINE);
	create_recovery_directory();
}

/****************************Auto_rec**************************************/

/* Auto_rec calls all routines needed for automatic recovery.  Automatic
recovery repairs the system enough to put the user in a shell in single user
state with workable commands and scripts to make other system repairs. */

auto_rec()

{
	struct stat sbuf;

	printf("%s", clr_screen);
	log_file(AUTO_REC);
	mkdirp("/disc/dev/");		/* make sure /dev directory exists */
	replace_file(RHP_UX);
	copy_bootarea();
	replace_file(RDLD_SL);
	replace_file(RLIBC_SL);
	replace_file(RFSCK);
	replace_file(RCPIO);
#ifdef __hp9000s700
	replace_file(RMKBOOT);
	replace_file(RUXBOOTLF_700);
#endif
	if (!small) {
		replace_file(RUPDATE);
		replace_file(RTCIO);
		replace_file(RTAR);
	}
	replace_file(RINIT);
	replace_file(RSHELL);
	replace_file(RPASSWD);
	inittab_fix();
	make_device(CONSOLE,020622,0,0);
	link_files(CONSOLE,SYSCON);
	link_files(CONSOLE,SYSTTY);
	make_device(TTY,020666,2,0);
	make_device(DNULL,020666,3,2);
	make_device(MEM,020600,3,0);
	make_device(KMEM,020640,3,1);
	if (chown(KMEM, 0, 3)) {
  		log_file("Could not change owner/group for %s\n",KMEM);
	}
	make_device(DSWAP,020600,8,0);
	if (!stat(DEV_ROOT,&sbuf)) {
		make_device(DSK,060644,major(sbuf.st_rdev),minor(sbuf.st_rdev));
	} else {
		make_device(DSK,060644,255,0xffffff);
	}
	printf("%s", LINE);
	printf("%s", AUTOREC_DONE);
	printf("%s", RETURN);
	gets(dummy);
}


/***********************Create_recovery_directory****************************/

/*
Creates /disc/tmp/recovery.xxxx directory where xxxx represents the month
and day.  This directory is used to store all the old versions of files
or commands that are changed in the recovery process.
*/ 

create_recovery_directory()
{
	int t;
	char dbuf[5];

	t=time(0);
	strftime(dbuf,5,"%m%d",localtime(&t));
	dbuf[4]=0;
	strcpy(recdirname,RECOVERY_DIRECTORY);
	strcat(recdirname,dbuf);
	strcat(recdirname,"/");
	if (mkdirp(recdirname) == -1)
		log_file("Could not create recovery directory.\n");
	recdirname[strlen(recdirname)-1]='\0';
}


/****************************Tmp_Copy***************************************/

/* Tmp_copy copies old versions of files and commands to the
/tmp/recovery.xxxx directory. */ 

tmp_copy(file)	/* copy <file> to temp directory */
char *file; 
{
	char *tmp;
	char dest[max_length];

	/* append filename to end of recovery directory path */
	tmp=file;
	tmp += (strlen(tmp) - 1);
	while (*tmp != '/')
  		tmp --;
	strcpy(dest,recdirname);
	strcat(dest, tmp);
  	if (copy_file(file, dest)==0)
		log_file("%s %s %s %s\n", COPY1, file, COPY2, dest);
}


/****************************Copy_file*************************************/

/*
Copy_file is passed a source and destination file name.
It copies the former to the latter.
*/

copy_file(source, dest)
char *source, *dest;

{
	int rc;

	if (mkdirp(dest) == -1) {
		log_file("Mkdir failed: %s\n",sys_errlist[errno]);
	}
	if (rc=run_command("cp %s %s 2>/dev/null", source, dest)) {
		log_file("Copy of %s to %s failed: %s\n",
			source,dest,sys_errlist[errno]);
  	}
	return rc;
}


/****************************Log_file**************************************/

/*
Log_file is passed a string containing important information for the
user to view after the automatic recovery is finished.  The messages
are stored in a global two dimensional array and is then written to
/tmp/recovery.log after it is complete.
*/

log_file(format,arg1,arg2,arg3,arg4,arg5,arg6)
char *format,*arg1,*arg2,*arg3,*arg4,*arg5,*arg6;
{
	char tmp[1024];
	struct log_entry *node;

	sprintf(tmp,format,arg1,arg2,arg3,arg4,arg5,arg6);
	printf("%s",tmp);
	if (no_mem)
		return;
	node=(struct log_entry *)malloc(sizeof(struct log_entry));
	if (!node) {
		no_mem=1;
		return;
	}
	node->s=strdup(tmp);
	if (!node->s) {
		no_mem=1;
		return;
	}
	node->next=0;
	if (log) {
		tail->next=node;
		tail=node;
	} else {
		log=tail=node;
	}
}


/****************************write_log_file*************************************/

/*
Write_log_file writes all the strings stored in the global two dimensional
array (log), to the /tmp/recovery.log file
*/

write_log_file()
{
	FILE *fp;

	if (fp=fopen(LOG_FILE,  "a")) {
		fprintf(fp, "%s", LOG_TITLE);
		while (log) {
  			fputs(log->s,fp);
			log=log->next;
		}
		fclose(fp);
	} else
		printf("Could not write log file!\n");
}

/****************************Run_fsck**************************************/

/*
Run_fsck executes fsck -p to repair the root file system.  It checks
for errors in fsck -p and may force the user to manually do a fsck by
calling other routines.
*/

run_fsck()
{
	FILE *fopen(),  *fp;
	char rv[max_length];

	if (run_command("fsck -p %s >%s; echo $? >%s",
		DEV_RROOT,FSCK_EMSG,FSCK_RC)) {
  		log_file("fsck failed: %s\n",sys_errlist[errno]);
	}
	fp = fopen(FSCK_RC,"r");
	fgets(rv,10,fp);
/*
WARNING - THERE IS A BUG IN FSCK.  When the fsck -b option is executed with a
number that does not exist as a super block, the fsck immediately finishes and 
returns a 0 instead of an 8.  This causes one to believe that fsck completed 
successfully when it really didn't.
*/
	if (strcmp(rv, "0\n") == 0) {
		return;
	} else if (strcmp(rv, "4\n") == 0) {
		fsck_file_system_modified();
	} else if (strcmp(rv, "8\n") == 0) {
		fsck_could_not_complete();
	} else if (strcmp(rv, "12\n") == 0) {
		fsck_file_system_interrupted();
	} else {
		fsck_file_system_error();
	}
	fclose(fp);
}


/****************************Fsck_file_system_modified********************/

/* File system has been modified.  Notify user and log action */     

fsck_file_system_modified()

{
log_file(FSCK_COM);
log_file(FS_MOD);

log_file(CONT_RECOVERY);
return(0);
}


/****************************Fsck_could_not_complete***********************/

/*Fsck -p could not finish.  User will see error messages.  User is put in
a shell and asked to execute fsck interactively.  If the message returned
is bad superblock, other action is taken. */

fsck_could_not_complete()

{
FILE *fopen(),  *fp;
char line[max_length];
char *rv; 
char flag_sb[max_length];
fp = fopen(FSCK_EMSG,  "r");


rv = fgets(line,  max_length, fp);
while (rv != NULL)
{
if (strcmp(ERROR_SB, line) == 0)
  {
  try_fsck_with_alternate_superblocks();
  strcpy(flag_sb, "true");
  }
rv = fgets(line,  max_length, fp);
}
fclose(fp);

if (strcmp(flag_sb, "true") != 0)
  {
  log_file(NOFIX_FS);
  printf("%s", RSHELL_ST);
  log_file(EXECUTE_FSCK);
  log_file(CTRL_D);
  
  run_shell();
  
  log_file(CONT_RECOVERY);
  }
}

/***************Try_fsck_with_alternate_superblocks************************/

/* If the superblock is bad, the /etc/sbtab file is catted and the user
is put in a shell to execute fsck -b with a different superblock.  If this
fails, the user is given the option to exit the recovery system and reboot
from backup or reinstall.  The sbtab file used is from the recovery system. */

/**** WARNING - THERE IS A BUG IN FSCK.  When the fsck -b option is executed with a
 number that does not exist as a super block, the fsck immediately finishes and 
 returns a 0 instead of an 8.  This causes one to believe that fsck completed 
 successfully when it really didn't. *****/


try_fsck_with_alternate_superblocks()

{
char selection[max_length];
int option;
struct stat buf;
int rc;

log_file(BAD_SB);
rc = stat(SBTAB, &buf);
if (rc == -1)
  {
  log_file(SBTAB_NOT_THERE);
  log_file(SUPERBLOCK_ALTERN);
  }

else if (buf.st_size == 0)
  {
  log_file(SBTAB_EMPTY);
  log_file(SUPERBLOCK_ALTERN);
  }

else
  {
  if (run_command("cat %s",SBTAB)) {
    log_file("cat failed: %s\n",sys_errlist[errno]);
  }
  log_file(SBTAB_LOOK);
  printf("%s", LINE);
  }

printf("\n%s",RETURN);
gets(dummy);
log_file(RSHELL_ST);
log_file(FSCK_B);
log_file(FSCK_FAIL);
log_file(FSCKB_EXIT);

run_shell();

while (1) {
printf (" \n\n");
printf ("  Select one of the following options by number: \n\n");
printf ("    %d) The fsck was completed successfully - continue RECOVERY\n", CONTINUE_RECOVERY);
printf ("    %d) The fsck was not completed successfully - system will halt\n", REBOOT_HALT);
printf ("       Recovery is possible by reinstall or booting from backup.\n");
printf ("\nSelection >>");
gets(selection);
option = atoi(selection);
switch (option) {

case CONTINUE_RECOVERY:
	{
	log_file(CONT_RECOVERY);
        return(0);
        break;
	}

case REBOOT_HALT:
        {
 	log_file(FAIL_FSCK);
        rc = reboot(RB_HALT, DEV_RROOT);  /* on s700, DEV_RROOT is ignored */
        if (rc == -1) {
          printf("%s", REBOOT_FAIL);
          printf("%s %d\n", ERRORNO_MSG, errno);
        }
   	exit (0);
        break;
        }

default:	
	{
	printf (" \n\n");
	printf("INVALID selection\n");
	}

                  }/*end of switch*/
             } /*end of while*/

}




/****************************Fsck_file_system_interrupted********************/

/* Fsck was interrupted.  A shell will be started for manual fsck and actions
logged*/

fsck_file_system_interrupted()

{

log_file(FSCK_INTER);
log_file(RSHELL_ST);
log_file(EXECUTE_FSCK);
log_file(CTRL_D);

run_shell();

log_file(CONT_RECOVERY);
}


/****************************Fsck_file_system_error**************************/

/* Unexpected error occurred.  A shell will be started for manual fsck and
actions logged */

fsck_file_system_error()

{

log_file(UNEXP_ERROR);
log_file(RSHELL_ST);
log_file(EXECUTE_FSCK);
log_file(CTRL_D);

run_shell();

log_file(CONT_RECOVERY);
}

/***************************Replace_file*******************************/

/*
Replace a file on the root file system with a good
version from the recovery file system.
*/

replace_file(file)
char *file;
{
	char old[max_length];

	strcpy(old,DISC);
	strcat(old,file);
	if (access(old,F_OK)==0)
		tmp_copy(old);		/* save old version of file */
	log_file("Creating new %s on the root file system\n",file);
	copy_file(file,old);	/* copy new file */
}


/***************************Mount_fs***********************************/

/* Mount the root file system. */ 

mount_fs()
{
	struct ufs_args foo;

	foo.fspec=DEV_ROOT;
	if (vfsmount(MOUNT_UFS,DISC,0,&foo)) {
		log_file("Mount of root file system: %s\n",sys_errlist[errno]);
		log_file(MOUNT_FAIL);
		run_shell();
	}
}


/***************************Copy_bootarea********************************/

/*Copy_bootarea copies the boot area from recovery system to the root disk */

copy_bootarea()
{
#ifdef __hp9000s300
	log_file("Copying /etc/boot to boot area on root file system.\n");
	/*
	The block in and block out messages are written to a file so the
	user doesn't have to see them during the automatic recovery.
	*/
	if (run_command("dd if=%s of=%s count=1 bs=8k >%s 2>&1",
		BOOT,DEV_ROOT,BOOT_MSG)) {
		log_file("Boot area repair failed: %s\n",sys_errlist[errno]);
	}
#endif
#ifdef __hp9000s700
	log_file("Updating secondary loader on root file system.\n");
	/*
	The block in and block out messages are written to a file so the
	user doesn't have to see them during the automatic recovery.
	*/
	if (run_command("/etc/mkboot -F -b /usr/lib/uxbootlf.700 -s 700 -u -v %s 2>&1",DEV_ROOT)) {
		log_file("Secondary loader repair failed\n");
	}
#endif
}

/***************************Inittab_fix**********************************/

/* Re-create /etc/inittab on root file system */

inittab_fix()
{
	FILE *fp;

	tmp_copy(INITTAB);
	mkdirp(INITTAB);
	fp=fopen(INITTAB,"w");
	if (!fp) {
		log_file("fopen %s: %s\n",INITTAB,sys_errlist[errno]);
		return -1;
	}
	if (fputs(INITTAB_LINE,fp)==EOF) {
		log_file("fputs %s: %s\n",INITTAB,sys_errlist[errno]);
		return -1;
	}
	if (fclose(fp)) {
		log_file("fclose %s: %s\n",INITTAB,sys_errlist[errno]);
		return -1;
	}
	log_file("Created new /etc/inittab on root file system\n");
	return 0;
}

/***************************Link_files***********************************/

/* Link files together */

link_files(source,dest)
char *source,*dest;
{
	int rc;

	rc = unlink(dest);
	if (rc && errno!=ENOENT) {
		log_file("unlink %s: %s\n",dest,sys_errlist[errno]);
		return -1;
	}
	else if (!rc)
		log_file("Existing %s removed.\n",dest);

	if (link(source,dest)) {
		log_file("link %s %s: %s\n",source,dest,sys_errlist[errno]);
	} else
		log_file("Successfully created %s on root file system.\n",dest);
	return 0;
}

/***************************Make_device**********************************/

/* Make device files */

make_device(device,mode,maj,min)
char *device;
mode_t mode;
int maj,min;
{
	int rc;

	rc = unlink(device);
	if (rc && errno!=ENOENT) {
		log_file("unlink %s: %s\n",device,sys_errlist[errno]);
		return -1;
	}
	else if (!rc)
		log_file("Existing %s removed.\n",device);
	mkdirp(device);
	if (mknod(device, mode, makedev(maj, min))) {
		log_file("mknod %s: %s\n",device,sys_errlist[errno]);
		return -1;
	} else
		log_file("Successfully created %s on root file system.\n",device);
	return 0;
}


/*************************Rebootfs************************************/

/*
Rebootfs reboots the root file system when the user chooses the main
menu option.  This most likely will occur after automatic recovery
completes, manual fixes complete or root passwd fix completes
*/

rebootfs()
{
	write_log_file();
	if (umount(DEV_ROOT) == -1) {
		printf("Unmount of the root file system failed.\n");
		perror("umount");
	}
#ifdef __hp9000s300
	if (reboot(RB_NEWDEVICE, DEV_RROOT) == -1) {
		printf("Unable to reboot root file system.\n");
		perror("reboot");
		exit(0);
	}
#else
	if (reboot(RB_HALT) == -1) {
		printf("Unable to halt system.\n");
		perror("reboot");
		exit(0);
	}
#endif
}


/*************************Supass_fix**********************************/

/*
This procedure assumes that the only problem the user has is not knowing
the su passwd.  In this case, the root file system /etc/passwd file
remains the same except for the root entry.  The root entry is changed
so that the user must immediately set a new passwd at the next login.
*/ 

supass_fix()
{
	FILE *old_passwd,*new_passwd;
	char line[max_length],*rv;
	struct stat buf;
	int rc;

	/*
	 * Check to see if the /etc/passwd file exists on the root file
	 * system.  If it doesn't, create it with one line (root).
	*/

	rc = stat(PASSWD, &buf);
	if (rc == -1) { 
		mkdirp(PASSWD);
		new_passwd = fopen(PASSWD,  "w");
		fputs(PASSWD_LINE, new_passwd);
		fclose(new_passwd);
		log_file(PASSWD_NOT_THERE);
	} else  {
		/*
		 * If /etc/passwd does exist, copy the whole file with the
		 * new root entry.
		 *
		 * Create a copy of the old /etc/passwd, but make the first
		 * line a fixed "root" entry.  Any old root entry will be
		 * preceeded by "OLD", thus the previous "root" login will
		 * be called "OLDroot" login instead.  This is so that the
		 * previous information from the "root" entry isn't lost
		 * (like the comment field, etc).
		 */
		tmp_copy(PASSWD);
		new_passwd = fopen(TMP_PASSWD,  "w");
		old_passwd = fopen(PASSWD,  "r");
		fputs(PASSWD_LINE, new_passwd);
		do {
			if ((rv = fgets(line,max_length,old_passwd)) != NULL) {
				if (strncmp(line, "root:", 5) == 0)
					fputs("OLD", new_passwd);
				fputs(rv, new_passwd);
			}
		} while (rv != NULL);
		fclose(old_passwd);
		fclose(new_passwd);
		rc = unlink(PASSWD);
		if (rc == -1) {
			log_file(UNLINK_FAIL);
			log_file("%s %d\n", ERRORNO_MSG, errno);
		}
		copy_file(TMP_PASSWD, PASSWD);
		log_file(CLEAR_SUPASS);
		log_file(SETPASS);
	}
	/*
	 * The user is put in the main menu when
	 * finished and can reboot there.
	 */
	printf("\n%s", RETURN);
	gets(dummy);
}


/*************************Mkdirp**************************************/

/*
mkdirp creates intermediate path cpomponents if they don't exist.
All components are created by mkdir(2) using a mode of 0777.
If the umask is set so as to prevent the user wx bits from being
set, chmod(2) is called to ensure that at least those mode bits
are set so that following path components can be created.
If any directory already exists, it is silently ignored.
*/

mkdirp(dir)
char *dir;
{
    char *dirp;

    /* Skip any leading '/' characters */
    for (dirp = dir; *dirp == '/'; dirp++);

    /* For each component of the path, make sure the component
     * exists.  If it doesn't exist, create it.  */
    while ((dirp = strchr(dirp, '/')) != NULL) {
	*dirp = '\0';
	if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
	    log_file("mkdir: %s",sys_errlist[errno]);
	    return -1;
	}
	/*  If this directory did not already exist AND
	 *  the umask prevented the user wx bits from being set,
	 *  then chmod it to set at least u=wx so the next one can be
	 *  created.  */
	if(errno != EEXIST)
	    chmod(dir,0777);

	for (*dirp++ = '/'; *dirp == '/'; dirp++);
    }
    return 0;
}

/*************************mkrs_swap**************************************/

/*
This routine was added so that the functionality in /etc/mkrs.swap
could be combined with mkrs.tool.  This saves some disk space.
*/

struct nlist nl[] = {
#ifdef __hp9000s300
	{ "_swdevt" },
#else
	{ "swdevt" },	/* PA compilers don't add underscores */
#endif
	{ 0 },
};

struct swdt {
	dev_t	sw_dev;
	int	sw_freed;
	int	sw_start;
	int	sw_nblks;
} swdt;

mkrs_swap(what)
int what;
{
	int  kmem;
	char *kmemf, *nlistf;

	kmemf = "/dev/kmem";
	kmem = open (kmemf,0);	/* open the kernel memory device */
	if (kmem < 0)
		exit (1);
	nlistf = "/hp-ux";
	nlist (nlistf,nl);	/* fetch kernel variable addresses */
	if (nl[0].n_type == 0) 
		exit (-1);
        if (lseek (kmem, (long)nl[0].n_value, 0) == -1) 
		exit (-1);
	if (read (kmem,(char *)&swdt,sizeof(struct swdt))!=sizeof(struct swdt)) 
		exit (-1);
	switch (what) {
	    case SWAP_DEV:
		printf("%X", minor(swdt.sw_dev)); break;
	    case SWAP_START:
		printf("%d", swdt.sw_start); break;
	    case SWAP_SIZE:
		printf("%d", swdt.sw_nblks); break;
	}
}

/*************************mkrs_system**************************************/

/*
This routine was added so that the functionality of system(3S)
could be combined with mkrs.tool.  This saves some disk space, since
the current system(3S) calls /bin/posix/sh.  
The following code is revision 64.4 of system(3S).
*/

#ifdef _NAMESPACE_CLEAN
#define execl _execl
#define waitpid _waitpid
#define vfork _vfork
#define sigvector __sigvector
#define system _system
#endif /* _NAMESPACE_CLEAN */

#include <signal.h>

extern int execl(), waitpid();
extern int vfork();
static struct sigvec ignvec = { SIG_IGN, 0, 0 };

#ifdef hp9000s500
#define	OLD_BELL_SIG -1
#endif /* hp9000s500 */

int
mkrs_system(s)
char	*s;
{
	int	pid, status;
#ifndef	hpux
	register int (*istat)(), (*qstat)();
#else	hpux
	struct sigvec istat, qstat;
#endif	/* hpux */

	if (s == (char *)0)
		return(1);

	if((pid = vfork()) == 0) {	/* fork successful, in child process */
		(void) execl("/bin/sh", "sh", "-c", s, 0);
		_exit(127);
	}

	/* else parent process */
	/* check that the vfork succeeded */
	if (pid == -1 ) {
		/* fork failed: return -1
		 *		errno should have been set by vfork()
		 */
		return(-1);
		}
#ifndef	hpux
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
#else	hpux
	(void) sigvector(SIGINT, &ignvec, &istat);
	(void) sigvector(SIGQUIT, &ignvec, &qstat);
#endif	/* hpux */
	if(waitpid(pid, &status, 0) == -1)
		status = -1;		/* EINTR */
#ifndef	hpux
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
#else	hpux
# ifndef hp9000s500
	(void) sigvector(SIGINT, &istat, (struct sigvec *)0);
	(void) sigvector(SIGQUIT, &qstat, (struct sigvec *)0);
# else	hp9000s500
	if	(istat.sv_mask == OLD_BELL_SIG)
		(void) signal(SIGINT, istat.sv_handler);
	else	(void) sigvector(SIGINT, &istat, (struct sigvec *)0);
	if	(qstat.sv_mask == OLD_BELL_SIG)
		(void) signal(SIGQUIT, qstat.sv_handler);
	else	(void) sigvector(SIGQUIT, &qstat, (struct sigvec *)0);
# endif	/* hp9000s500 */
#endif	/* hpux */
	return(status);
}
