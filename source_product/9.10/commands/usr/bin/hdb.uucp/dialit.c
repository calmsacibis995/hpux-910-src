static char *HPUX_ID = "@(#) $Revision: 66.2 $";
/*****************************************************************
 *    (c) Copyright 1984 Hewlett Packard Co.
 *        ALL RIGHTS RESERVED
 *****************************************************************/


/***********************************************************************
 *            * * * * D I S C L A I M E R * * * *
 *  The programs contained with in this module are examples of autodial-
 *  ing routines for selected modems currently on the market. H.P.
 *  makes no claim as to the validity or reliability of the code in this
 *  module. These programs are not supported products, but simply examples
 *  for our customers. Their compatibility with future products is not
 *  guaranteed.
 ************************************************************************/



/**************************************************************************
 *  This module consists of: 
 *     main routine - this routine is the main entry point into the module.
 *     The usage of this routine is:
 *               dialit <modemtype> <cua> <phone> <speed>
 *     Where:
 *        modemtype - is the name of a modem know in the Modem structure
 *                    along with a user supplied routine to do the 
 *                    autodialing. The standard is for the modemtype to
 *                    be a name of the form:
 *                             ACUmodemname
 *                    such as:
 *                             ACUVENTEL212
 *                    This convention is followed since this is the form
 *                    expected by both uucp and cu which utilize this program
 *                    to perform their autodialing.
 *
 *        cua - This must be the full path name of the /dev entry over which
 *              the auto dial sequence is to be sent to the modem. In the 
 *              case of uucp and cu this entry is pulled from the L-devices
 *              file. NOTE: that in the L-devices file the full pathname is
 *              not given. But uucp and cu do expand it before calling this
 *              module.
 *
 *        phone - The phone number to be called by the autodial modem. The
 *                phone number may consist of digits, '=' and '-' only.
 *                The special characters are mapped to wait for secondary
 *                dialtone(if implemented on the modemtype) or 5 second 
 *                pause respectively.
 *
 *        speed - This argument is the speed desired for transmission,
 *                i.e. 1200,300,etc. The inclusion of this parameter 
 *                allows you to configure the cua line. If the dial 
 *                routine is called from cu or uucp the line has already
 *                been configured.
 *     
 *     sendstring routine - writes the designated string to the device
 *          whose descriptor was sent it.
 *
 *     await routine - will read from the designated device a sequence of
 *          characters until a certian string is recognized or a specific
 *          number of characters is read.
 *
 *     awaitg routine - like await; imbedded garbage chars allowed
 *
 *     ckoutphone routine - scans the phone string and checks for invalid
 *          characters and determins a delay time used for alarm timeout
 *          purposes when calling the remote machine.
 *
 *     map_phone routine - map the characters '=' and '-' which mean wait
 *          for a secondary dial tone and pause respectfully to their
 *          actual character representation for given modems.
 *
 *     log_entry - make an entry into the DIALLOG which resides in /usr/spool
 *          /uucp.
 *
 *     make_entry - called by log_entry. makes the actual entry in the logfile.
 * 
 *     prefix - tests a string to determine if it begins with a given prefix.
 *
 *     mlock - lock the logfile so only one process may write to it at a time.
 *
 *     remove_lock - remove the logfile lock and allows another process to 
 *          access the log.
 *
 *     close_log - cleans up any temporary log files created and closes the
 *          log file.
 *
 *   USERSPECIFIED ROUTINES:
 *    these routines are supplied by the users of the uucp package. Each
 *    routine is written for a specific type of autodialer modem and must
 *    have an entry in the Modems structure.
 *
 *********************************************************************/
#include <sys/param.h>
#include <stdio.h>
#include <termio.h>
#include <setjmp.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ndir.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

#define FILENAMESIZE (MAXNAMLEN+1)
#define MAXFULLNAME MAXPATHLEN
#define MAXMSGSIZE 256
#define SAME     0  
#define FALSE    0
#define TRUE	(-1)
#define FAIL	(-1)
#define SUCCESS  0
#define MAXRETRIES  3
#define PREFIX "DIAL."

#define LOG_LOCK  "/usr/spool/uucp/LCK..DIAL"

jmp_buf Sjbuf;
char *modemtype;                /* modem name as entered in the L-devices file
				and L.sys file. */
int got_sig();
int alarmtout();

/******************************************************************
 *  The following structure Modems is used in determining which user
 *  supplied routine is to be used for autodialing given a specific
 *  modem type.  Each user specified routine must have at least one entry
 *  in this structure and each modem type used for autodialing must have
 *  only one entry in the structure.
 *
 *  To add additional modem types and routines simply add them to the 
 *  initialization of modem[].
 *****************************************************************/
int vad3450(),                        /* function name for vadic3450*/
    ventel212(),                      /* ventel 212+3 function */
    hp35141_autodial(),               /* hp support link modem */
    hp37212_autodial(),               /* hp smart modem */
    hp37212_error_correct(),          /* HP37212B */
    hayes_smart(),		      /* hayes smartmodem1200 function */
    x25telnet();                      /* dial-up to GTE/TELNET pad       */

struct Modems{
      char *name;                     /* modem name */
      int (*modem_fn)();              /* function to call */
} modem[] = {
     "ACUVADIC3450",    vad3450,
     "ACUHP35141A",     hp35141_autodial,
     "ACUHP37212A",     hp37212_autodial,
     "ACUHP37212B",     hp37212_error_correct,
     "ACUVENTEL212",    ventel212,
     "ACUHAYESSMART",   hayes_smart,
     "ACUHP92205A",     hayes_smart,
     "ACUX25TELNET",    x25telnet,
      0,
};
char script[100];

char *allow_protos;


/********************************************************************
 *  main routine - do any conversions necessary and determine the routine
 *  needed for autodialing and call it.
 ********************************************************************/

main(argc,argv)
int argc; 
char *argv[];
{
   struct Modems *ap;        /* pointer to table of modems*/
   int i;
   int found,                /* flag */
       retval;               /* this is the exit code sent back to uucp module.
				It should be either FAIL or SUCCESS */
   char *phone,              /* expanded phone passed from uucp */
        *speed,              /* char rep of line speed */
	*cua;                /* pathname of the call device line*/

   for (i=0; i < NSIG; i++)
	signal(i, got_sig);             /* catch all signals & clean up */
   signal(SIGCLD, SIG_DFL);
   modemtype = argv[1];                      /* modem name */
   cua = argv[2];                            /* pathname call line */
   phone = argv[3];                          /* phone # to call */
   speed = argv[4];                          /* char rep of line speed */
   allow_protos = argv[5];		     /* string of allowable protos */

   umask(0177);                              /* set file creation mask */
   chdir("/usr/spool/uucp");                 /* change to pwd */

   found = FALSE; 
   for (ap = modem; ap->name;ap++){
       if (strcmp(ap->name,modemtype) == SAME){
	  found = TRUE;
	  retval = (*ap->modem_fn)(phone,cua,speed);
	  break;
       }
   }
   if (!found){
      struct stat statb;
      sprintf(script, "/usr/lib/uucp/X25/%s.out", modemtype+3);
      if (stat(script, &statb) != -1)
	return x25box(modemtype+3, phone, cua, speed);
      log_entry("modem type specified","NOT KNOWN");
      log_entry("autodial","FAILED");
      retval = FAIL;
   }
   close_log();
   exit(retval);
}

got_sig(signo)
int signo;
{
    char s[100];

    sprintf(s, "Got Signal #%d", signo);
    if (signo == SIGKILL)
	strcat(s, "!!!!!!");
    log_entry(s, "FAILED");
    close_log();
    exit(signo);
}


/***************************************************************************
 *   await(string,file,str_length,timeout)
 *
 *   await reads from the line associated with file until the "string"
 *   has been read or the timeout period has been reached
 *   or the total characters read is equal to strlen(string);
 *   
 *   return values:
 *     FAIL - string did not appear within timeout period.
 *     SUCCESS - string did appear within the timeout period.
 ****************************************************************************/

await(file,string,str_length,timeout)
char *string;                          /* str expecting on line */
int file,                              /* descriptor of line to read from */
    str_length,                        /* length of string that is received*/
    timeout;                           /* maxtime to wait for input from line*/
{
  register char_count;                 /* number of characters read from line*/
  int ret;                             /* return value form read*/
  char char_read,                      /* current character read from line*/
       reply[MAXMSGSIZE];              /* buffer to hold incoming message*/

  /*
   *  set up return and fail for the timeout on reading the wanted string
   */

  if (setjmp(Sjbuf))
     return(FAIL);


  char_count = 0;

  signal(SIGALRM,alarmtout);           /* set up for possible alarm timeout*/
  alarm(timeout);

  /*
   *  read in characters until the string is found or timeout occurs or
   *  the number of characters read exceeds the str_length
   */


  while (char_count < str_length ||
         strncmp(string,&reply[char_count - str_length],str_length) != 0){

	ret = read(file,&char_read,1);    /*read one character at a time*/

     	if (ret <= 0){                    /*if read failed - abort*/
		alarm(0);
		return(FAIL);
     	}

	/*
	 *  if the numbers of characters read is greater than the size
	 *  of the desired reply and the desired reply has not been
	 *  seen, then terminate with failure.
	 */

     	if (char_count >= sizeof(reply)){
		alarm(0);
		return(FAIL);
     	}

	/*
	 * add the character just read onto the reply string
	 */

     	reply[char_count++] = char_read;
  }

  /*
   * add trailing null to delimit the string received from the modem.
   */

  reply[char_count] = '\0';
  alarm(0);
  return(SUCCESS);
}

/***************************************************************************
 *   awaitg(string,file,timeout)
 *
 *   awaitg reads from the line associated with file until the "string"
 *   has been read of the total characters read is equal to strlen(string);
 *
 *   imbedded garbage characters ignored
 *   
 *   return values:
 *     FAIL - string did not appear within timeout period.
 *     SUCCESS - string did appear within the timeout period.
 ****************************************************************************/

awaitg(file,string,timeout)
char *string;                          /* str expecting on line */
int file,                              /* descriptor of line to read from */
    timeout;                           /* maxtime to wait for input from line*/
{
  register char_count;                 /* number of characters read from line*/
  int str_length;                      /* lenght of string looking for */
  int ret;                             /* return value form read*/
  char char_read,                      /* current character read from line*/
       reply[MAXMSGSIZE];              /* buffer to hold incoming message*/

  if (setjmp(Sjbuf))
     return(FAIL);


  char_count = 0;
  str_length = strlen(string); 

  /***********************************************************************
   *  read in characters until the string is found or timeout occurs or
   *  the number of characters read exceeds the str_length
   ***********************************************************************/


  signal(SIGALRM,alarmtout);           /* set up for possible alarm timeout*/
  alarm(timeout);

  while (char_count < str_length)
  {
     ret = read(file,&char_read,1);
     if (ret <= 0){
	alarm(0);
	return(FAIL);
     }
     if (char_count >= sizeof(reply)){
	alarm(0);
	return(FAIL);
     }
     if (char_read == string[char_count])
	reply[char_count++] = char_read;
  }
  reply[char_count] = '\0';
  alarm(0);
  return(SUCCESS);
}

/**************************************************************************
 *   alarmtout()
 *
 *   alarm timeout routine
 ***************************************************************************/

alarmtout()
{
   longjmp(Sjbuf,1);
}




/******************************************************************************
 *  sendstring(string,file)
 *
 *  write string to the communications line associated with the descriptor
 *  passed in file.
 ******************************************************************************/

sendstring(file,string,size)
char *string;                             /* string to be sent to modem */
int file,                                 /* descriptor of communications line*/
    size;                                 /* number of chars to send */
{
   int  sent;                             /* number of characters written*/

   if (setjmp(Sjbuf))
      return;				  /* let await kill it */

   signal(SIGALRM,alarmtout);

   alarm(15);
   sent = write(file,string,size);
   alarm(0);
}

/****************************************************************************
 *  ckoutphone(phone,badphone)
 *  
 *  ckoutphone examins the characters in the string phone and calculates 
 *  a delay time for the actual dialing.  If a bad character is found the
 *  badphone flag is set as an error condition.
 *
 *  RETURN VALUE:
 *     the amount of delay needed to dial the phone number.
 *****************************************************************************/

ckoutphone(phone,badphone)
char *phone;                           /* string containing phone number */
int  *badphone;                        /* bad number flag */
{
   char *ptr_phone,                     /* ptr to characters in phone # */
	 msg[50];                       /* text of log message*/
   int  delay = 24;                     /* delay for dialing to be returned*/

   *badphone = FALSE;                   /* initial setting for flag */
   for ( ptr_phone = phone; *ptr_phone != '\0'; ptr_phone++){
       switch (*ptr_phone) {
	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
	       delay += 2;             /* 2 second delay per digit */
	       break;
	  case '=':
	  case '-':
		delay += 5;            /* five second pause */
		break;
          default:
		*badphone = TRUE;
		sprintf(msg,"bad character in phone - %c",*ptr_phone);
		log_entry(msg,"ERROR");
		break;
       }
    if (*badphone)
       break;
    }
    sprintf(msg,"phone # %s, delay %d secs",phone,delay);
    log_entry(msg,"PHONE OK");
    return(delay);
}

/************************************************************************
 *   map_phone(phone,type)
 *     Map the characters '=' & '-' in the phone number to their respective
 *   representations for the modem being used. The parameter 'type' is
 *   an integer denoting which type of mapping is necessary.
 *
 *   RETURN VALUE
 *     none.
 *************************************************************************/

map_phone(phone,type)
char *phone;                                 /*address of phone # string */
int  type;                                   /* type of modem being used */
{
	char *ptr_phone;                     /* *char in the phone number */

	switch (type){                /* determine type of modem */

		/*
		 * mapping for the VADIC3450 
		 */

		case 1:               
			for (ptr_phone = phone; *ptr_phone != '\0';ptr_phone++){
				switch (*ptr_phone){
					case '=':
					case '-':
						*ptr_phone = 'K';
						break;
					default:
						break;
				}
			}
			break;

		/*
		 * mappings for the VENTEL212 modem
		 */

		case 2:
			for (ptr_phone = phone; *ptr_phone != '\0';ptr_phone++){
				switch (*ptr_phone){
					case '=':
						*ptr_phone = '&';
						break;
					case '-':
						*ptr_phone = '%';
						break;
					default:
						break;
				}
			}
			break;
		
		/*
		 * mapping for the HAYES_SMARTMODEM and the HP92205A modems
		 */

		case 3:
			for (ptr_phone = phone; *ptr_phone != '\0';ptr_phone++){
				switch (*ptr_phone){
					case '=':
						*ptr_phone = ',';
						break;
					case '-':
						*ptr_phone = ',';
						break;
					default:
						break;
				}
			}
			break;
		
		/*
		 * mapping for the HP35141A modem 
		 */

		case 4:               
			for (ptr_phone = phone; *ptr_phone != '\0';ptr_phone++){
				switch (*ptr_phone){
					case '=':
						*ptr_phone = 'K';
						break;
					case '-':
						break;
					default:
						break;
				}
			}
			break;

                /*
                 * mapping for the HP37212A modem
                 */

                case 5:
                        for(ptr_phone = phone; *ptr_phone != '\0';ptr_phone++){
                               switch (*ptr_phone){
                                       case '=':
                                               *ptr_phone = '%';
                                               break;
                                       case '-':
                                               *ptr_phone = '"';
                                               break;
                                       default:
                                               break;
                                }
                        }
                        break;

		default:
			log_entry("mapping of phone number","FAILED");
			exit(-1);
	}
}

FILE *fp_tmp_log;
char Temporary_log[MAXFULLNAME] = "";

/**************************************************************************
 * log_entry(text,status)
 *   make an entry in the DIALLOG. If the DIALLOG is locked then use a 
 * temporary log to make an entry to.
 *
 * RETURN VALUES: none
 **************************************************************************/

log_entry(text,status)
char *text,                            /*text of msg to put into the log*/
     *status;                          /*relavent status of message*/
{
	int count;                     /*counter */
	FILE *fp_log;                  /*file pointer to logfile*/

	if (fp_tmp_log != NULL){       /*see if a templog exists*/
				       /*tmp log exist - make an entry*/
		make_entry(fp_tmp_log,status,text);
		return;
  	}

	/*
	 * temporary log does not exist. Make a lock on the main
	 * log file and enter the text and status.
	 */

	if (mlock(LOG_LOCK,101) == 0){
		
		/*
		 * lock was created successfully. Open & make an entry
		 */
		
		if ((fp_log = fopen("/usr/spool/uucp/DIALLOG","a")) == NULL){
			
			/*
			 * open failed remove lock and make temp entry
			 */
			
			remove_lock(LOG_LOCK);
		}
		else {                        /* make a log entry */
			make_entry(fp_log,status,text);
			fclose(fp_log);
			remove_lock(LOG_LOCK);
			return;
		}
	}

	/*
	 * A temporary file does not exist and  for some reason an entry
	 * into the main logfile is not possible. Make a temp log file
	 * for temporary entries.
	 */
	
	for (count = 0; count < 10; count++){
		sprintf(Temporary_log,"%s/DIAL.%05d.%1d","/usr/spool/uucp",
			getpid(),count);
		if (access(Temporary_log,0) == -1)   /*see if tmp exists */
			break;                       /*doesnt exist - so use*/
	}

	/*
	 * open the temporary logfile and make an entry
	 */
	
	if ((fp_tmp_log = fopen(Temporary_log,"w")) == NULL)
		return;                   /*can't open temporary */
	chmod(Temporary_log,0622);
	chown(Temporary_log, geteuid());        /* should be 'uucp' */
	setbuf(fp_tmp_log,0);
	make_entry(fp_tmp_log,status,text);
	return;
}

/*************************************************************************
 *  make_entry(fp_log,status,text)
 *    make an entry into the logfile pointed to by the file pointer 
 *    fp_log.
 *
 *  RETURN VALUES: none
 *************************************************************************/

make_entry(fp_log,status,text)
FILE *fp_log;                        /*pointer to appropriate log file */
char *status,                        /*status associated with entry*/
     *text;                          /*text of the entry*/
{
	static proc_id = 0;          /* process id */
	struct tm *time_ptr;         /*pointer to time structure*/
	static char user[10];        /*login user name*/
	extern struct tm *localtime();
	time_t clock;

	if (!proc_id){               /*if proc id not set get it*/
		proc_id = getpid();
		get_user_name(user);
	}

	/*
	 * get the time and convert to local time. make the necessary
	 * entries in the log file.
	 */

	time(&clock);
	time_ptr = localtime(&clock);
	fprintf(fp_log,"%s %s ",user,modemtype);
	fprintf(fp_log,"(%d/%d-%d:%2.2d-%d) ",time_ptr->tm_mon+1,
		time_ptr->tm_mday,time_ptr->tm_hour,time_ptr->tm_min,
		proc_id);
	fprintf(fp_log,"%s (%s)\n",status,text);
	return;
}

/**********************************************************************
 * get_user_name(user)
 *
 *  get user login name
 *
 *  RETURN VALUES: none
 **********************************************************************/

get_user_name(user)
char *user;                                /* location for user name*/
{
	struct passwd *pwd;
	struct passwd *getpwuid();

	if ((pwd = getpwuid(getuid())) == NULL){
		/*did not find user*/
		strcpy(user,"unkown");
		return;
	}
	strcpy(user,pwd->pw_name);
	return;
}

/*********************************************************************
 *   close_log()
 *   
 *   check to see if a temporary log exists. if so attempt to append
 *   it to the main logfile and remove it from the directory if 
 *   successful
 *
 *   RETURN VALUES: none
 *********************************************************************/

 close_log()
 {
	FILE *fp_log;                      /*pointer to the log file*/
	DIR  *fp_dir;                      /*pointer to the directory*/
	char file_name[FILENAMESIZE],      /*file names from directory*/
             msg[50];                      /*text buffer for log entries*/

	if (fp_tmp_log != NULL){           /* need to close temp log*/
		fclose(fp_tmp_log);
		chmod(Temporary_log,0444);
	}

	/*
	 * clean up any temp logs
	 */

	if (mlock(LOG_LOCK,101) == 0){     /*lock logfile*/
		if((fp_log=fopen("/usr/spool/uucp/DIALLOG","a")) != NULL){
			
			/*
			 * got logfile now open directory
			 */

			fp_dir = opendir("/usr/spool/uucp");
			if (fp_dir == (DIR *)NULL){        /*dir did not open*/
				remove_lock(LOG_LOCK);
				make_entry(fp_log,"FAILED",
					     "OPEN OF /usr/spool/uucp");
				make_entry(fp_log,"NOT DONE","CLEANUP");
				return;
			}
			
			/*
			 *get file names and see if they are temps
			 */
			
			while (get_file_name(fp_dir,file_name) != 0){
				if (prefix(PREFIX,file_name)){
					if (appendit(fp_log, file_name)){
						sprintf(msg,
						   "append of tmp file %s",
						   file_name);
						make_entry(fp_log,"SUCCESSFUL",
 						   msg);
						unlink(file_name);
					}
					else{
						sprintf(msg,"append of file %s",
 						   file_name);
						make_entry(fp_log,"FAILED",msg);
					}
				}
			}
			closedir(fp_dir);
		}
 		remove_lock(LOG_LOCK);
	}
}

/************************************************************************
 * prefix(s1,s2)
 *
 * test to see if s1 is a prefix of s2.
 *
 * RETURN VALUES:
 *     0 - not a prefix
 *     1 - is a prefix
 *************************************************************************/

prefix(s1,s2)
char *s1,                               /* prefix to look for */
     *s2;                               /* string to test for prefix */

{
	char item;

	while ((item = *s1++) == *s2++)
		if (item == '\0')
			return(1);
	return(item == '\0');
}

/***********************************************************************
 *  appendit(fp,file_name)
 *
 *  append the contents of 'file_name' to the file pointed to by
 *  fp.
 *
 *  RETURN VALUES:
 *      1 - successful append
 *      0 - not successful
 ************************************************************************/

appendit(fp,file_name)
FILE *fp;                           /* file pointer to receiving file*/
char *file_name;                    /* file to read data from*/
{
	FILE *fp_read_file;         /* pointer to read file */
	char file_text[512];        /* text from read file */

	chmod(file_name, 0600);    /* make sure we can read the file */
				   /* but nobody else can */

	if ((fp_read_file = fopen(file_name,"r")) == NULL)   /*did not open*/
		return(0);
	
	/*
	 * file opened ok - read contents and append until exhausted.
	 */
	
	while (fgets(file_text,512,fp_read_file))
		fputs(file_text,fp);
	fclose(fp_read_file);
	return(1);
}

/**************************************************************************
 *   get_file_name(fp_dir,file_name)
 *
 *   read file names from directory pointed to and store in file_name.
 *
 *   RETURN VALUES:
 *       0 - no more entries in dir
 *       1 - file name found
 *************************************************************************/

get_file_name(fp_dir,file_name)
DIR *fp_dir;                              /*file pointer to directory*/
char *file_name;                          /*location to store file name*/
{
	static struct direct *dir_entry;  /*hold directory entries*/
	int count;                        /*counter*/
	char *item;                       /*char pointer*/

	while (1){                      /* get entries in directory */
		if ((dir_entry=readdir(fp_dir))==(struct direct *)NULL)
			return(0);
		if (dir_entry->d_ino != 0)
			break;
	}

	for (count = 0,item = dir_entry->d_name; count < (FILENAMESIZE - 1);
		 count++)
		 if ((file_name[count] = *item++) == '\0')
			break;

	file_name[FILENAMESIZE-1] = '\0';
	return(1);
}

/*************************************************************************
 * mlock(lock_name,life_span);
 *
 *  make a lock file to act as a semaphore for log entries.
 *
 *  RETURN VALUES:
 *     0 - success
 *     1 - fail
 *************************************************************************/

mlock(lock_name,life_span)
char *lock_name;                            /* name of lock file to create */
int  life_span;                             /* time of allowable life*/

{
	struct stat stat_buf;               /* buffer for file status*/
	time_t present_time;                /* system present time*/
	int ret,                            /* return indicator */
	    fd;                             /* file descriptor*/
	static int pid = -1;                /* process id */
	static char tmp_file[FILENAMESIZE]; /* temporary file name*/

	if ( pid < 0){                      /* set pid */
		pid = getpid();
		sprintf(tmp_file,"LTMP.%d",pid);
	}

	/*
	 * see if lock file already exists. if not then create it
	 */

	fd = creat(tmp_file,0666);          /*create temp file*/
	if (fd < 0)                         /*create failed abort*/
		return(1);
	write(fd,(char *)&pid,sizeof(int)); /*write out pid to file*/
	close(fd);

	/*
	 * attempt to link temp file to lock file - if fails it already
	 * exits.
	 */
	
	if (link(tmp_file,lock_name) < 0){
		
		/*
		 * file exists. see if it is too old
		 */

		ret = stat(lock_name,&stat_buf);
		if ( ret != -1){                  /*got status*/
			time(&present_time);
			if ((present_time - stat_buf.st_ctime) < life_span){
				/*file still has life left*/
		                unlink(tmp_file);
				return(1);
			}
		}
		ret = unlink(lock_name);

	        /*
	         * attempt to link temp file to lock file - if fails it already
	         * exits.
	         */
        	
	        if (link(tmp_file,lock_name) < 0){
			unlink(tmp_file);
			return(1);
		}
	}
	unlink(tmp_file);
	return(0);
}

/*************************************************************************
 * remove_lock(lock_name)
 *
 *   remove the file named 'lock_name' which is associated with a lock on
 *   the file.
 *
 *   RETURN VALUES: none
 *************************************************************************/

remove_lock(lock_name)
char *lock_name;                            /*name of lock to be removed*/
{
	int ret;                            /*return value indicator*/
	
	ret = unlink(lock_name);           
	if ( ret != 0){
		fprintf(stderr,"LOCK FILE CANNOT BE REMOVED\n");
		return;
	}
	return;
}

/*****************************************************************************
 *  USER-SUPPLIED-ROUTINES
 * 
 *  The following routines are supplied by the user to implement hardware
 *  dependent dialing sequences based on the modem being used.  Each 
 *  routine that follows which is associated with a dialing sequence for
 *  a specific type of modem should have at least one entry in the Modems
 *  structure declared above.
 *  If no entry is made for the routine, it will not be accessible from the
 *  main entry routine.
 *
 *  RETURN VALUES:
 *     These routines should return one of two values only. The possible 
 *  choices are:
 *     SUCCESS - if the dialing process was successful and the remote
 *               system is now online.
 *     FAIL - if the dialing process was unsuccessful and/or the remote
 *            system did not come online.
 ***************************************************************************/

/****************************************************************************
 * message array for the HP37212A smart modem
 ****************************************************************************/
 
struct hp_smart{
       
       char *string;     /* char string message */
       int   length;     /* length of string message */

} hp37212a[] ={

/* 0 */       "\r",                       1,
/* 1 */       "Z\r",                      2,
/* 2 */       "?\r",                      2,
/* 3 */       "H@@@@@",                   6,
/* 3          "@DH@@@@@",                 8,     1200 Baud only */
/* 3          "@@H@@@@@",                 8,      300 Baud only */
/* 4 */       "T",                        1,
/* 5 */       "K\r",                      2,
/* 6          "ABH@@@BC",                 8,         300 Baud only */
/* 6          "AFH@@@BC",                 8,      1200 Baud only */
/* 6 */       "H@@@BC",                   6,  
/* 7 */       "T\r",                      2,
/* 8 */       "<\r",                      2,

};

/***************************************************************************
 * HP37212A(phone,cua,speed)
 *
 * This routine implements the autodial sequence necessary for the
 * HP37212A modem.  Note, to use this routine the modem must be configured
 * as follows prior to attempting operation.
 *
 * All internal switchs S9 through S16 must be open, giving the following
 * configuration.
 *
 * Computer mode, smart mode, Bell 212A mode, asynchronous, 10 bit data,
 * enable auto answer, DSR/CTS/CD under RS232C interface lines control,
 * DTR line monitored by modem.
 *
 * RETURN VALUES:
 *    SUCCESS - autodial successful
 *    FAIL    - autodial failed
 ****************************************************************************/

hp37212_autodial(phone,cua,speed)

char *phone,                     /* string containing phone number */
     *cua,                       /* call line used for dialing */
     *speed;                     /* speed of both lines */

{
   int retval = FAIL;            /* value to be returned by routine */
   int delay,                    /* delay to dial number */
       badphone,                 /* flag for bad phone number */
       fd_cua;                   /* descriptor for call line */
   
   long time_now,                /* time when checking modem status */
        time_stop;               /* time to stop checking modem status */

   int normal_delay = 2;         /* delay for normal message */
   char msg[50];                 /* text for log entry */

   /*
    * make sure that the null device was not sent as the call line
    */

   if (strcmp(cua,"/dev/null") != SAME){

      /*
       * attempt to open the autodial line for sending and reading the
       * autodial sequence.
       */

      if ((fd_cua = open(cua,O_RDWR)) >= 0){
         sprintf(msg,"open of cua device %s",cua);
         log_entry(msg,"SUCCESSFUL");
         
         /*
          * calculate the approximate time that is necessary for dialing
          * the desired phone number.
          */

         delay = ckoutphone(phone,&badphone);

         /*
          * make sure all characters in the telephone number are valid
          */
   
         if (!badphone){
    
            /*
             * map the delay characters to the proper representation for the
             * specific modem being used.  Second parameter of the following
             * routine call specifies which mapping is to be done.
             */

            map_phone(phone,5);   /* map special characters */
            sprintf(msg,"%s",phone);
            log_entry(msg,"MAPPED PHONE - SUCCESS");

            /*
             * send the command string to activate the modem  <cr>
             */

            sendstring(fd_cua,hp37212a[0].string,hp37212a[0].length);
         
            /*
             * sleep for 1 second
             */

            sleep(1);

            /*
             * send the command string to reset the modem to powerup state
             */

            sendstring(fd_cua,hp37212a[1].string,hp37212a[1].length);

            /*
             * sleep for 2 seconds
             */

            sleep(2);

            /*
             * send <cr> to reactivate modem after reset
             */

            sendstring(fd_cua,hp37212a[0].string,hp37212a[0].length);

            /*
             * sleep for 1 second
             */

            sleep(1);

	    /*
	     * send < to put it into computer mode if it isn't already
	     */

            sendstring(fd_cua,hp37212a[8].string,hp37212a[8].length);

            /*
             * sleep for 1 second
             */

            sleep(1);

            /*
             * send the command string requesting the modem status
             */
 
            sendstring(fd_cua,hp37212a[2].string,hp37212a[2].length);
            if (await(fd_cua,hp37212a[3].string,hp37212a[3].length,
                      normal_delay)){

               /*
                * modem failed to return status.  Abort the proceedure
                */

               log_entry("no response from 1st status request","FAILED");
               log_entry("autodial","FAILED");
               return(retval);
            }

            sprintf(msg,"dial phone - %s",phone);  /* prepare log entry */
            log_entry(msg,"REQUESTED");

            /*
             * first send an uppercase S, this tell the modem to use smart
             * dialing where the modem will figure out if pulse or tone
             * dialling is to be used.  Next send the phone number to be
             * dialed followed by uppercase K <cr> to allow us to check the
             * modem status
             * 
             * NOTE: Change uppercase S to T for our site.  Works better for us.
             */

            sendstring(fd_cua,hp37212a[4].string,hp37212a[4].length);
            sendstring(fd_cua,phone,strlen(phone));
            sendstring(fd_cua,hp37212a[5].string,hp37212a[5].length);
            
            /*
             * request modem status from the modem, need to check modem
             * several time/second so we catch the online status as soon
             * as it occurs
             */

             sleep(10);

             time_stop = time(0) + 30L;

            /*
             * NOTE:  HP37212A has a 40 sec timeout when originating calls.
             * now loop until 30 seconds has passed checking the modem status
             */

            while (((time_now = time(0)) <= time_stop) && (retval == FAIL))
            {
                 sendstring(fd_cua,hp37212a[2].string,hp37212a[2].length);
                 if (!await(fd_cua,hp37212a[6].string,hp37212a[6].length,
                            normal_delay)){

                            retval = SUCCESS;
                 }
            }

            /*
             * put the modem in transparent mode so we can use is now
             */

            if (retval == FAIL)
            {
                 log_entry(msg,"FAILED");
                 log_entry("autodial","FAILED");
                 return(retval);
            }

            sendstring(fd_cua,hp37212a[7].string,hp37212a[7].length);

            log_entry(msg,"SUCCESSFUL");
            log_entry("remote system","ONLINE");
            log_entry("autodial","SUCCESSFUL");
         }

         else{

            /*
             * phone number contains bad characters
             */
 
            sprintf(msg,"phone number - %s",phone);
            log_entry(msg,"BAD");
            log_entry("autodial","FAILED");

         }
      }
   
      else{

          /*
           * autodial line did not open successfully
           */

          sprintf(msg,"openof cua %s",cua);
          log_entry(msg,"FAILED");
          log_entry("autodial","FAILED");
      }
   }
   
   else{

       /*
        * call line given was the /dev/null device.
        */

       log_entry("invalid cua device","FAILED");
       log_entry("autodial","FAILED");
   }
   return(retval);
}


/****************************************************************************
 * message array for the HP37212B smart modem
 ****************************************************************************/
 
struct hp_smartB{
       
       char *string;     /* char string message */
       int   length;     /* length of string message */

} hp37212B[] ={

/* 0 */       "\r",                       1,
/* 1 */       "Z\r",                      2,
/* 2 */       "37212B",                   6,
/* 3 */       "P",                        1,
/* 4 */       "ON LINE\r\n\0",            9,
};

hp37212_error_correct(phone,cua,speed)

char *phone,                     /* string containing phone number */
     *cua,                       /* call line used for dialing */
     *speed;                     /* speed of both lines */

{
   int retval = FAIL;            /* value to be returned by routine */
   int delay,                    /* delay to dial number */
       badphone,                 /* flag for bad phone number */
       fd_cua;                   /* descriptor for call line */
   
   long time_now,                /* time when checking modem status */
        time_stop;               /* time to stop checking modem status */

   int normal_delay = 2;         /* delay for normal message */
   char msg[50];                 /* text for log entry */

   /*
    * make sure that the null device was not sent as the call line
    */

   if (strcmp(cua,"/dev/null") != SAME){

      /*
       * attempt to open the autodial line for sending and reading the
       * autodial sequence.
       */

      if ((fd_cua = open(cua,O_RDWR)) >= 0){
         sprintf(msg,"open of cua device %s",cua);
         log_entry(msg,"SUCCESSFUL");
         
         /*
          * calculate the approximate time that is necessary for dialing
          * the desired phone number.
          */

         delay = ckoutphone(phone,&badphone);

         /*
          * make sure all characters in the telephone number are valid
          */
   
         if (!badphone){
    
            /*
             * map the delay characters to the proper representation for the
             * specific modem being used.  Second parameter of the following
             * routine call specifies which mapping is to be done.
             */

            map_phone(phone,5);   /* map special characters */
            sprintf(msg,"%s",phone);
            log_entry(msg,"MAPPED PHONE - SUCCESS");

            /*
             * send the command string to activate the modem  <cr>
             */

            sendstring(fd_cua,hp37212B[0].string,hp37212B[0].length);
         
            /*
             * sleep for 1 second
             */

            sleep(1);

            /*
             * send the command string to reset the modem to powerup state
             */

            sendstring(fd_cua,hp37212B[1].string,hp37212B[1].length);

            /*
             * sleep for 5 seconds.  37212B takes more time to complete
             * self test.!!
             */

            sleep(5);

            /*
             * send <cr> to reactivate modem after reset
             */

            sendstring(fd_cua,hp37212B[0].string,hp37212B[0].length);

            /*
             * sleep for 1 second
             */

            sleep(1);

            /*
             * send the command string requesting the modem status
             */
 
            if (await(fd_cua,hp37212B[2].string,hp37212B[2].length,
                      normal_delay)){

               /*
                * modem failed to return status.  Abort the proceedure
                */

               log_entry("37212B: After reset, incorrect 1st string (37212B) ","FAILED");
               log_entry("37212B: autodial","FAILED");
               return(retval);
            }

            sprintf(msg,"dial phone - %s",phone);  /* prepare log entry */
            log_entry(msg,"REQUESTED");

            /*
             * first send an uppercase P, this tell the modem to use pulse
             * dialing where the modem will figure out if pulse or tone
             * dialling is to be used.  Next send the phone number to be
             * dialed followed by uppercase K <cr> to allow us to check the
             * modem status
             * 
             * NOTE: Change uppercase S to T for our site.  Works better for us.
             */

            sendstring(fd_cua,hp37212B[3].string,hp37212B[3].length);
            sendstring(fd_cua,phone,strlen(phone));
            sendstring(fd_cua,hp37212B[0].string,hp37212B[0].length);
            
            /*
             * request modem status from the modem, need to check modem
             * several time/second so we catch the online status as soon
             * as it occurs
             */

             sleep(10);

             time_stop = time(0) + 50L;

            /*
             * NOTE:  HP37212B has a 60 sec timeout when originating calls.
             * now loop until 50 seconds has passed checking the modem status
             */

            while (((time_now = time(0)) <= time_stop) && (retval == FAIL))
            {
                 if (!awaitg(fd_cua,hp37212B[4].string,
                            normal_delay)){

                            retval = SUCCESS;
                 }
            }

            /*
             * put the modem in transparent mode so we can use is now
             */

            if (retval == FAIL)
            {
                 log_entry(msg,"FAILED");
                 log_entry("After dialing out. Did not get 2nd status (ON LINE CR LF)","FAILED");
                 return(retval);
            }

            log_entry(msg,"SUCCESSFUL");
            log_entry("Got 2nd status OK (ON LINE CR LF)","ONLINE");
            log_entry("autodial","SUCCESSFUL");
         }

         else{

            /*
             * phone number contains bad characters
             */
 
            sprintf(msg,"phone number - %s",phone);
            log_entry(msg,"BAD");
            log_entry("autodial","FAILED");

         }
      }
   
      else{

          /*
           * autodial line did not open successfully
           */

          sprintf(msg,"openof cua %s",cua);
          log_entry(msg,"FAILED");
          log_entry("autodial","FAILED");
      }
   }
   
   else{

       /*
        * call line given was the /dev/null device.
        */

       log_entry("Cannot use /dev/null","FAILED");
       log_entry("autodial","FAILED");
   }
   return(retval);
}


/****************************************************************************
 * message array for the HP35141A support link modem
 ****************************************************************************/

struct hp_support_link{
		char *string;     /*char string message*/
		int  length;      /*length of string message*/
} hp35141a[] ={
/* 0*/        "I\r",                              2,
/* 1*/        "\5\r",                             2,
/* 2*/        "\r\nHELLO:I'M READY\r\0\0\n*",     22,
/* 3*/        "D\r",                              2,
/* 4*/        "\r\0\0\nNUMBER? \r\0\0\n",         16,
/* 5*/        "\r\0\0\n",                         4,
/* 6*/        "\r",                               1,
/* 7*/        "\r\0\0\nDIALING...\r\0\0\n",       18,
/* 8*/        "ANSWER TONE\r\0\0\nON LINE",       22,
/* 9*/        "FAILED CALL\r\n",                  13,
/*10*/        "IDLE\r\n",                         6,
};


 /*******************************************************************
  *  HP35141A(phone,cua,speed)
  *
  * This routine implements the autodial sequence necessary for the
  * HP35141A modem. Note this modem is similar to but not compatible
  * with the Racal Vadic 3450. To utilize this routine the modem 
  * MUST be configured as follows prior to attempting operation.
  *
  * CONFIGURATION OF THE MODEM (use t command to see current config)
  *
  *   01*2   STANDARD OPTN               02*1   ASYNCH/SYNCH
  *   03*2   DATA RATE SEL      	 04*1   103 OPERATION
  *   05*3   CHARACTER LEN               06*1   ORIG/ANS MODE
  *   07*2   SLAVE CLOCK                 08*2   DTR CONTROL
  *   09*2   ATT/UNATT DISC		 10*1   LOSS CXR DISC
  *   11*1   REC SPACE DISC              12*2   SEND SPACE DIS
  *   13*1   ABORT DIAC			 14*1   REMOT TST RESP
  *   15*3   DSR CONTROL		 16*2   CXR CONTROL
  *   17*1   AUTO LINKING		 18*3   ALB CONTROL
  *   19*1   AUTO ANSWER          	 20*2   TERMINAL BELL
  *   21*2   LOCAL COPY                  22*3   DIAL MODE
  *   23*2   BLIND DIAL                  24*1   CALL PROGRESS
  *   24*1   FAIL CALL SEL		 25*1   AUTO REDIAL
  *
  * 
  *  RETURN VALUES:
  *     SUCCESS - autodial successful.
  *     FAIL - autodial failed.
  ******************************************************************/

hp35141_autodial(phone,cua,speed)
char *phone,                     /* string containing phone number */
     *cua,                       /* call line used for dialing */
     *speed;                     /* speed of both lines */
{
   int retval = FAIL;            /* value to be returned by routine */
   int delay,                    /* delay to dial number */
       badphone,                 /* flag for bad phone number */
       fd_cua;                   /* descriptor for call line */
   int normal_delay = 3;         /* delay for normal message */
   char msg[50];                 /* text for log entry*/

   /*
    * make sure that the null device was not sent as the call line
    */

   if (strcmp(cua,"/dev/null") != SAME){

      /*
       * attempt to open the autodial line for sending and reading the
       * autodial sequence.
       */

      if ((fd_cua = open(cua,O_RDWR)) >= 0){
	 sprintf(msg,"open of cua device %s",cua);
	 log_entry(msg,"SUCCESSFUL");

	 /*
	  * calculate the approximate time that is necessary for dialing
	  * the desired phone number.
	  */
         
	 delay = ckoutphone(phone,&badphone);

	 /*
	  * make sure all characters in telephone number are valid
	  */

         if (!badphone){ 
	    /*
	     * map the delay characters to the proper representation for the
	     * specific modem being used. Second parameter of the following
	     * routine call specifies which mapping is to be done.
	     */

	    map_phone(phone,4);   /* map special characters*/
	    sprintf(msg,"%s",phone);
	    log_entry(msg,"MAPPED PHONE - SUCCESS");
            
	    /*
	     * send the command string to make sure the modem is in an idle 
	     * state.
	     */

	    sendstring(fd_cua,hp35141a[0].string,hp35141a[0].length);
            if (!await(fd_cua,hp35141a[10].string,hp35141a[10].length,
							     normal_delay))
	       
	       /*
		* if modem was active and command forced it into an idle state,
		* pause to allow for modem reset.
		*/

	       sleep(2); 
					       
           /*
	    * send the modem wake up signal and await the wake up reply
	    */

	    sendstring(fd_cua,hp35141a[1].string,hp35141a[1].length);
            if (await(fd_cua,hp35141a[2].string,hp35141a[2].length,
							   normal_delay)){
	       
	       /*
		* if the proper wake up response "HELLO: I'M READY" is not
		* seen, end routine without a second attempt at wake up
		*/
	       
	       log_entry("modem wake up","FAILED");
	       log_entry("second wake up attempt","NOT ATTEMPTING");
	       log_entry("autodial","FAILED");
	       return(retval);
            }

	    /*
	     * send the command "D" to request the dialing of a number and
	     * await the reply "NUMBER?" to request number to be dialed.  
	     */

	    sendstring(fd_cua,hp35141a[3].string,hp35141a[3].length);
	    if (await(fd_cua,hp35141a[4].string,hp35141a[4].length,
							normal_delay)){
	       
	       /*
		* "NUMBER?" prompt was not received. Abort the procedure
		*/

	       log_entry("no response from dial prompt","FAILED");
	       log_entry("autodial","FAILED");
	       return(retval);
            }

	    sprintf(msg,"dial phone - %s",phone);  /* prepare log entry */
	    log_entry(msg,"REQUESTED");

	    /*
	     * send the phone number to be dialed followed by an <cr> and 
	     * await the modem to echo the phone number.
             */

            sendstring(fd_cua,phone,strlen(phone));
	    sendstring(fd_cua,hp35141a[6].string,hp35141a[6].length);

	    /*
	     * check for the proper phone string to be echoed
	     */

            if (await(fd_cua,hp35141a[5].string,hp35141a[5].length,
		normal_delay) || await(fd_cua,phone,strlen(phone),
		normal_delay) || await(fd_cua,hp35141a[5].string,
		hp35141a[5].length,normal_delay)){
	       /*
		* modem did not echo phone number properly. abort routine
		*/

	       log_entry(msg,"FAILED");
	       log_entry("autodial","FAILED");
	       return(retval);
            }
        
	   /*
	    * send a <cr> to activate the actual dialing of the phone number
	    * and await for the "DIALING" message from modem.
	    */

	    sendstring(fd_cua,hp35141a[6].string,hp35141a[6].length);
            if (await(fd_cua,hp35141a[7].string,hp35141a[7].length,5)){

               /*
		* "DIALING" message not received. The autodial has failed
		*/

	       log_entry(msg,"FAILED");
	       log_entry("autodial","FAILED");
	       return(retval);
            }

	    /*
	     * wait for the modem to give the "ON LINE" message. If it does
	     * not, then autodial has failed so return. Note: modem is 
	     * configured to automatically retry number so delay is doubled.
	     */

            if (!await(fd_cua,hp35141a[8].string,hp35141a[8].length,
					                         2*delay)){

	       /*
		* "ON LINE" message was received. Autodial was successful.
		*/

	       log_entry(msg,"SUCCESSFUL");
	       log_entry("remote system","ONLINE");
	       log_entry("autodial","SUCCESSFUL");
	       retval = SUCCESS;
	    }
            else{

	       /*
		* "ON LINE" message NOT received. Autodial failed.
		*/

	       log_entry("connection with remote system","FAILED");
	       log_entry("autodial","FAILED");
	    }
         }

	 else{

	    /*
	     * phone number contains bad characters
	     */

	    sprintf(msg,"phone number - %s",phone);
	    log_entry(msg,"BAD");
	    log_entry("autodial","FAILED");
	 }
      }

      else {

	 /*
	  * autodial line did not open successfully
	  */

	 sprintf(msg,"open of cua %s",cua);
	 log_entry(msg,"FAILED");
	 log_entry("autodial","FAILED");
      }
   }

   else{

       /*
	* call line given was the /dev/null device.
	*/

       log_entry("invalid cua device","FAILED");
       log_entry("autodial","FAILED");
   }
   return(retval);
}

/**********************************************************************
 *   The structure below is used by the ventel212 program to issue    *
 *  the proper sequence of autodial commands.                         *
 **********************************************************************/


struct Ventel_sequence{
			 char *string;   /* the char string message */
			 int length;     /* total chars in string   */
} ventel[] ={
/* 0*/       "Q",                        1,
/* 1*/       "\r",                       1,
/* 2*/       "$",                        1,
/* 3*/       "UIT",                      3,
/* 4*/       "VENTEL",                   6,
/* 5*/       "k\0\0\0",                  4,
/* 6*/       "DIAL: ",                   6,
/* 7*/       "R",                        1,
/* 8*/       "\0\0\0",                   3,
/* 9*/       "ONLINE!",                  7,
	     };

/*********************************************************************
 *  ventel212()                                                      *
 *    This submodule implements the autodialing sequence for a       *
 *    ventel modem model 212+3 with the weco EPROM. The entry for    *
 *    this modem should be                                           *
 *                ACUVENTEL212                                       *
 *    in the L.sys and L-devices file.                               *
 *                                                                   *
 *    return codes are:                                              *
 *         FAIL - the autodial was  not successful                   *
 *         SUCCESS - The autodial was successful                     *
 *********************************************************************/

#define MAXQUITS 4                      /* max # of quit messages sent*/

ventel212(phone,cua,speed)
char *phone,                             /* string containing the ph # */
     *cua,                               /* dial line pathname */
     speed;                              /* char rep of speed desired */
{
  int  retval = FAIL,                    /* initial return value */
       delay,                            /* delay for dialing # */
       badphone,                         /* flag to indicate bad ph */
       fd_cua,                           /* descriptor for dial line */
       i,                                /* loop parameter */
       pause_time = 1,                   /* pause between spec. chars*/
       normal_delay = 3;                 /* delay for normal messages */
  char *p,                               /* pntr to phone digits */
       msg[50];                          /* message string*/

  /*
   * make sure the line specified for the autodial sequence is not the
   * /dev/null device.
   */

  if (strcmp(cua,"dev/null") != SAME){   /* cua was specified - go on */

      /*
       * attempt opening of the cua line to be used for the autodial
       * sequence.
       */

      if ((fd_cua = open(cua,O_RDWR)) >= 0){ /*cua open ok - continue*/
	  sprintf(msg,"opening of %s",cua);
	  log_entry(msg,"SUCCESSFUL");

	  /*
	   * calculate the approx. time necessary to dial the number
	   */

	  delay = ckoutphone(phone,&badphone);

	  /*
	   * make sure the phone number contains only valid characters
	   */

	  if (!badphone){                /* ph # ok then continue*/

	      /*
	       * map the special pause characters to the correct representation
	       * for the ventel 212 modem.
	       */

              map_phone(phone,2);        /*map special chars */
	      sprintf(msg,"%s",phone);
	      log_entry(msg,"MAPPED PHONE - SUCCESS");

	      /*
	       * send the "Q" command to make sure the modem is in the correct
	       * idle state.
	       */

              sendstring(fd_cua,ventel[0].string,ventel[0].length);
	      await(fd_cua,ventel[3].string,ventel[3].length,normal_delay);
	      sleep(2);            /* wait for modem reset */

	      /*
	       * send a <cr><cr> to wake up the modem. There must be a pause be-
	       * tween the sending of each <cr> or the modem will not see the
	       * second one.
	       */

              sendstring(fd_cua,ventel[1].string,ventel[1].length);
	      for (i = 0; i < 11000; i++);   /*wait to send next cr*/
	      sendstring(fd_cua,ventel[1].string,ventel[1].length);
	      if (await(fd_cua,ventel[2].string,ventel[2].length,normal_delay)){
		  
		  /*
		   * initial wake up failed. Try to recover by sending string
		   * <cr><cr> again.
		   */

		  log_entry("modem wake up","FAILED");
		  log_entry("second modem wakeup","ATTEMPTING");

		  sendstring(fd_cua,ventel[1].string,ventel[1].length);
	          for (i = 0; i < 11000; i++);   /*wait to send next cr*/
	          sendstring(fd_cua,ventel[1].string,ventel[1].length);

		  if (await(fd_cua,ventel[2].string,ventel[2].length,
							     normal_delay)){
                      
		      /*
		       * wake up message prompt "$" not seen. Second attempt
		       * failed also
		       */

		      log_entry("second wake up","FAILED");
		      log_entry("autodial","FAILED");
		      return(retval);
                  }

		  /*
		   * wake up prompt "$" received. second attempt successful
		   */

		  log_entry("modem wake up - second attempt","SUCCESS");

		  /*
		   * make sure modem is set for correct parity by sending the
		   * string "VENTEL".
		   */

                  sendstring(fd_cua,ventel[4].string,ventel[4].length);
		  if (await(fd_cua,ventel[2].string,ventel[2].length,
								normal_delay)){


                      /*
		       * "$" prompt not received. modem in unknown state.
		       */

		      log_entry("modem parity message","NOT RECEIVED");
		      log_entry("autodial","FAILED");
		      return(retval);
                  }
              }
	      else {

		  /*
		   * initial wake up was successful. make sure mode is using the
		   * correct parity by sending the "VENTEL" string.
		   */

                  log_entry("modem wake up","SUCCESS");
                  sendstring(fd_cua,ventel[4].string,ventel[4].length);
		  if (await(fd_cua,ventel[2].string,ventel[2].length,
							      normal_delay)){

		      /*
		       * "$" prompt was not received. modem in unknown state.
		       */

		      log_entry("modem parity message","NOT RECEIVED");
		      log_entry("autodial","FAILED");
		      return(retval);
                  }
              }

	      /*
	       * send the dial command "k" 
	       */

              sendstring(fd_cua,ventel[5].string,ventel[5].length);
	      if (await(fd_cua,ventel[6].string,ventel[6].length,
							   normal_delay)){

		  /*
		   * the "DIAL: " string was not received from the modem.
		   * modem is in an unknown state at this point.
		   */

		  log_entry("no response from dial prompt","FAILED");

		  /*
		   * if dial message not received abort and make   
		   * sure the modem is back in an idle state by    
		   * issuing a sequence of QUIT messages.          
		   */

                  for (i = 0; i < MAXQUITS; i++)
		      sendstring(fd_cua,ventel[0].string,ventel[0].length);
  		  log_entry("autodial","FAILED");
		  return(retval);
              }

	      /*
	       * send phone number and initiate the dialing          
	       */

	      sprintf(msg,"dial number - %s",phone);
	      log_entry(msg,"REQUESTED");
	      sendstring(fd_cua,phone,strlen(phone));

	      /*
	       * initiate the dialing and wait for "ONLINE!" message.
	       * if not received, retry the number once.            
	       */

	      sendstring(fd_cua,ventel[1].string,ventel[1].length);
	      if (await(fd_cua,ventel[9].string,ventel[9].length,delay)){

		  /*
		   * "ONLINE!" message not received. send the "R" retry
		   * command.
		   */

		  log_entry("dialing of phone number","FAILED");
		  log_entry(msg,"RETRYING");

		  sendstring(fd_cua,ventel[7].string,ventel[7].length);
		  if (!await(fd_cua,ventel[9].string,ventel[9].length,delay)){

		      /*
		       * "ONLINE!" message was received. autodial successful
		       */

		      log_entry("remote system","ONLINE");
		      log_entry("autodial","SUCCESSFUL");
		      retval = SUCCESS;
		  }
		  else{

                      /*
		       * retry was not successful. autodial failed.
		       */

		      log_entry("connection with remote system","FAILED");
		      log_entry("autodial","FAILED");
		  }
              }
	      else{
		  
		  /*
		   * first attempt at dialing successful. the "ONLINE!"
		   * message was received. autodial successful
		   */

		  log_entry("remote system","ONLINE");
		  log_entry("autodial","SUCCESSFUL");
		  retval = SUCCESS;
	      }
          }
	  else{

	      /*
	       * phone number contains bad characters.
	       */

	      sprintf(msg,"phone number - %s",phone);
	      log_entry(msg,"BAD");
	      log_entry("autodial","FAILED");
	  }
      }
      else{
	 
	  /*
	   * autodial line did not open successfully.
	   */

	  sprintf(msg,"open of cua %s",cua);
	  log_entry(msg,"FAILED");
	  log_entry("autodial","FAILED");
      }
  }
  else{
      
      /*
       * /dev/null was specified as the call line device.
       */

      log_entry("invalid cua device","FAILED");
      log_entry("autodial","FAILED");
  }
  return(retval);
}

/*********************************************************************
 * The structure below is used to configure the hayes smartmodem1200.*
 * If the user needs a different configuration, this structure must  *
 * be changed. The configuration parameters used have the following  *
 * meaning:							     *
 *	E0 - do not echo characters.				     *
 *	C1 - carrier is automatic.				     *
 *	S8=2 - secs to wait for a secondary dial tone.               *
 *	F1 - full duplex mode.					     *
 *	H0 - standard disconnect on hangup                           *
 *  	M0 - turn speaker off.					     *
 *	Q0 - send result code.					     *
 *	V1 - send results in english words.			     *
 *	X0 - send only basic result codes.			     *
 *      S6=2 - pause time in secs.				     *
 *	S7=15 - wait time to detect carrier after connect to remote  *
 *		modem.						     *
 * For further explanation of these codes consult the Hayes Documen- *
 * tation.
 *********************************************************************/

struct hayes_config {
			char *string;	/* the configuration string */
			int length;	/* length of string */
} hayesconfig[] ={
/* 0*/			"ATE0\r",	5, /*turn off echo of cmds*/
			                /* main register and parameter config */
/* 1*/                  "ATC1 S8=2 F1 H0 M0 Q0 V1 X0 S6=5 S7=30\r",   39,
		 };

/***********************************************************************
 * struct hayes_messages gives the possible dial message and return    *
 * code messages utilized in the await routine.                        *
 ***********************************************************************/

struct hayes_messages{
			char *string;		/*string of message*/
			int length;		/*length of message*/
}hayes_msg[] ={
/* 0*/		"ATDT",		4,		/*prefix of dial cmd
						  NOTE: THIS PREFIX ASSUMES
						  TONE DIAL. REPLACE TRAILING
						  "T" WITH "P" FOR PULSE 
						  DIAL              */
/* 1*/		"CONNECT\r\n",	9,		/*connection message*/
/* 2*/		"OK\r\n",	4,		/*normal return code*/
};


/*********************************************************************
 * hayes_smart()                                                     *
 *	This routine implements the autodial sequence for the Hays   *
 * Smartmodem1200. The routine attempts to configure the modem for   *
 * the autodial sequence. The configuration is given in the          *
 * hayes_config string above. The dialing assumes that tone (T) dial *
 * is to be used. If other than tone dial is to be used, the user    *
 * will have to modify this routine to handle the special cases.     *
 *                                                                   *
 * NOTE: the Hayes Smartmodem1200 is the save as the HP9205A         *
 *       modem                                                       *
 * 								     *
 * RETURN CODES:						     *
 *    SUCCESS - the autodial was successful			     *
 *    FAIL - connection not made				     *
 *********************************************************************/

hayes_smart(phone,cua,speed)
char *phone,				/*string containing phone #*/
     *cua,				/*dial line pathname */
     speed;				/*char rep of speed */
{
	int retval = FAIL,		/* initial return value*/
	    delay,			/* delay for dialing number*/
	    badphone,			/* flag to indicate bad ph #*/
	    fd_cua,			/* descriptor for dial line */
	    normal_delay = 3;		/* delay for normal msg return */

	char *p,			/* pntr to phone digits*/
	msg[50];			/* message string */

	/*
	 * make sure the line specified as the autodial line is not the  
	 * /dev/null file                                    
	 */

	if (strcmp(cua,"/dev/null") != SAME){	/* cua specification good */
		
		/*
		 * attempt the opening of the line used for autodialing
		 */

		if ((fd_cua=open(cua,O_RDWR)) >= 0){ /* cua open good*/
			sprintf(msg,"opening of %s",cua);
			log_entry(msg,"SUCCESSFUL");

			/*
			 * calculate the approx. time necessary to dial the
			 * telephone number.
			 */

			delay = ckoutphone(phone,&badphone);

			/*
			 * make sure the phone number does not contain any
			 * bad characters.
			 */

			if (!badphone){		/*ph # ok - send init
						  configuration message*/

                                /*
				 * map the special pause characters to the 
				 * correct representation for the Hayes.
				 */

				map_phone(phone,3);
				sprintf(msg,"%s",phone);
				log_entry(msg,"MAPPED PHONE - SUCCESS");

				/*
				 * send configuration - 1st turn off echo *
				 * then configure the parameters and regs *
				 */
				
				sendstring(fd_cua,"ATZ\r",4);
				await(fd_cua,hayes_msg[2].string,
				              hayes_msg[2].length,normal_delay);

				/*
				 *  turn off echo and await the "OK" message.
				 */

				sendstring(fd_cua,hayesconfig[0].string,
					   hayesconfig[0].length);
				await(fd_cua,hayes_msg[2].string,
					      hayes_msg[2].length,normal_delay);

				/*
				 * send configuration string and await "OK"
				 * message
				 */

				sendstring(fd_cua,hayesconfig[1].string,
					   hayesconfig[1].length);
				if (await(fd_cua,hayes_msg[2].string,
					     hayes_msg[2].length,normal_delay)){

					/*
					 * "OK" not seen. configuration failed
					 */

					log_entry("modem configuration",
						   "FAILED");
					log_entry("autodial","FAILED");
					return(retval);
				}

				/*
				 *modem configured. fix telephone up for  
				 *dial sequence.			   
				 */
				
				log_entry("modem configuration","SUCCESSFUL");
				sprintf(msg,"dial number - %s",phone);
				log_entry(msg,"REQUESTED");
				strcpy(msg,hayes_msg[0].string);
				strcat(msg,phone);
				strcat(msg,"\r");

				/*
				 * phone number now in proper form of:
				 * "ATDT<number><cr>" for dialing. send to
				 * modem.
				 */

				sendstring(fd_cua,msg,strlen(msg));
				if (await(fd_cua,hayes_msg[1].string,
						hayes_msg[1].length,delay)){

                                        /*
					 * "CONNECT" message not seen. dialing
					 * failed.
					 */

					log_entry("dialing of phone number",
						  "FAILED");
					log_entry("dialing of phone","RETRYING");
				        /*
					 * retry the dialing of the number
					 */

					sendstring(fd_cua,msg,strlen(msg));
					if (await(fd_cua,hayes_msg[1].string,
						   hayes_msg[1].length,delay)){


						/*
						 * retry unsuccessful. abort
						 */

						log_entry("connection with remote system",
							  "FAILED");
						log_entry("autodial","FAILED");
						return(retval);
					}
				}

				/*
				 * dial successful. system is online
				 */

				log_entry("remote system","ONLINE");
				log_entry("autodial","SUCCESSFUL");
				retval=SUCCESS;
			}
			else{
				/*
				 * telephone number contains improper characters
				 */

				sprintf(msg,"phone # - %s",phone);
				log_entry(msg,"BAD");
				log_entry("autodial","FAILED");
			}
		}
		else{
			/*
			 * cannot open the autodial line
			 */

			sprintf(msg,"open of cua %s",cua);
			log_entry(msg,"FAILED");
			log_entry("autodial","FAILED");
		}
	}
	else {

		/*
		 * the autodial line specified was /dev/null
		 */

		sprintf(msg,"invalid cua %s device",cua);
		log_entry(msg,"FAILED");
		log_entry("autodial","FAILED");
	}
	return(retval);
} 

/****************************************************************************
 * message array for the racal vadic 3450P/S/G series
 ****************************************************************************/

char *vadic3450[] ={
/* 0*/        "I\r",
/* 1*/        "\5\r",
/* 2*/        "HELLO: I'M READY\r\n*",
/* 3*/        "D\r",
/* 4*/        "NUMBER?\r\n",
/* 5*/        "\r\n",
/* 6*/        "\r",
/* 7*/        "DIALING:  ",
/* 8*/        "ON LINE\r\n",
/* 9*/        "FAILED CALL\r\n",
/*10*/        "R\r3\n",
/*11*/        "IDLE\r\n",
};


 /*******************************************************************
  *  vad3450(phone,dev)
  *
  *  This routine is designed to implement autodialing for a racal 
  *  vadic 3450P/S/G-series modem.
  * 
  *  RETURN VALUES:
  *     SUCCESS - autodial successful.
  *     FAIL - autodial failed.
  ******************************************************************/

vad3450(phone,cua,speed)
char *phone,                     /* string containing phone number */
     *cua,                       /* call line used for dialing */
     *speed;                     /* speed of both lines */
{
   int retval = FAIL;            /* value to be returned by routine */
   int delay,                    /* delay to dial number */
       badphone,                 /* flag for bad phone number */
       fd_cua;                   /* descriptor for call line */
   int normal_delay = 3;         /* delay for normal message */
   char msg[50];                 /* text for log entry*/

   /*
    * make sure that the null device was not sent as the call line
    */

   if (strcmp(cua,"/dev/null") != SAME){

      /*
       * attempt to open the autodial line for sending and reading the
       * autodial sequence.
       */

      if ((fd_cua = open(cua,O_RDWR)) >= 0){
	 sprintf(msg,"open of cua device %s",cua);
	 log_entry(msg,"SUCCESSFUL");

	 /*
	  * calculate the approximate time that is necessary for dialing
	  * the desired phone number.
	  */
         
	 delay = ckoutphone(phone,&badphone);

	 /*
	  * make sure all characters in telephone number are valid
	  */

         if (!badphone){ 
	    
	    /*
	     * map the delay characters to the proper representation for the
	     * specific modem being used. Second parameter of the following
	     * routine call specifies which mapping is to be done.
	     */

	    map_phone(phone,1);   /* map special characters*/
	    sprintf(msg,"%s",phone);
	    log_entry(msg,"MAPPED PHONE - SUCCESS");
            
	    /*
	     * send the command string to make sure the modem is in an idle 
	     * state.
	     */

	    sendstring(fd_cua,vadic3450[0],strlen(vadic3450[0]));
            if (!await(fd_cua,vadic3450[11],strlen(vadic3450[11]),normal_delay))
	       
	       /*
		* if modem was active and command forced it into an idle state,
		* pause to allow for modem reset.
		*/

	       sleep(2); 
					       
           /*
	    * send the modem wake up signal and await the wake up reply
	    */

	    sendstring(fd_cua,vadic3450[1],strlen(vadic3450[1]));
            if (await(fd_cua,vadic3450[2],strlen(vadic3450[2]),normal_delay)){
	       
	       /*
		* if the proper wake up response "HELLO: I'M READY" is not
		* seen, end routine without a second attempt at wake up
		*/
	       
	       log_entry("modem wake up","FAILED");
	       log_entry("second wake up attempt","NOT ATTEMPTING");
	       log_entry("autodial","FAILED");
	       return(retval);
            }

	    /*
	     * send the command "D" to request the dialing of a number and
	     * await the reply "NUMBER?" to request number to be dialed.  
	     */

	    sendstring(fd_cua,vadic3450[3],strlen(vadic3450[3]));
	    if (await(fd_cua,vadic3450[4],strlen(vadic3450[4]),normal_delay)){
	       
	       /*
		* "NUMBER?" prompt was not received. Abort the procedure
		*/

	       log_entry("no response from dial prompt","FAILED");
	       log_entry("autodial","FAILED");
	       return(retval);
            }

	    sprintf(msg,"dial phone - %s",phone);  /* prepare log entry */
	    log_entry(msg,"REQUESTED");

	    /*
	     * send the phone number to be dialed followed by an <cr> and 
	     * await the modem to echo the phone number.
	     */

            sendstring(fd_cua,phone,strlen(phone));
	    sendstring(fd_cua,vadic3450[6],strlen(vadic3450[6]));
            strcat(phone,vadic3450[5]);
            if (await(fd_cua,phone,strlen(phone),normal_delay)){
	       
	       /*
		* modem did not echo phone number properly. abort routine
		*/

	       log_entry(msg,"FAILED");
	       log_entry("autodial","FAILED");
	       return(retval);
            }
         
	   /*
	    * send a <cr> to activate the actual dialing of the phone number
	    * and await for the "DIALING" message from modem.
	    */

	    sendstring(fd_cua,vadic3450[6],strlen(vadic3450[6]));
            if (await(fd_cua,vadic3450[7],strlen(vadic3450[7]),normal_delay)){

               /*
		* "DIALING" message not received. The autodial has failed
		*/

	       log_entry(msg,"FAILED");
	       log_entry("autodial","FAILED");
	       return(retval);
            }

	    /*
	     * wait for the modem to give the "ON LINE" message. If it does
	     * not, then autodial has failed so return.
	     */

            if (!await(fd_cua,vadic3450[8],strlen(vadic3450[8]),delay)){

	       /*
		* "ON LINE" message was received. Autodial was successful.
		*/

	       log_entry(msg,"SUCCESSFUL");
	       log_entry("remote system","ONLINE");
	       log_entry("autodial","SUCCESSFUL");
	       retval = SUCCESS;
	    }
            else{

	       /*
		* "ON LINE" message NOT received. Autodial failed.
		*/

	       log_entry("connection with remote system","FAILED");
	       log_entry("autodial","FAILED");
	    }
         }

	 else{

	    /*
	     * phone number contains bad characters
	     */

	    sprintf(msg,"phone number - %s",phone);
	    log_entry(msg,"BAD");
	    log_entry("autodial","FAILED");
	 }
      }

      else {

	 /*
	  * autodial line did not open successfully
	  */

	 sprintf(msg,"open of cua %s",cua);
	 log_entry(msg,"FAILED");
	 log_entry("autodial","FAILED");
      }
   }

   else{

       /*
	* call line given was the /dev/null device.
	*/

       log_entry("invalid cua device","FAILED");
       log_entry("autodial","FAILED");
   }
   return(retval);
}



/**********************************************************************
 *   The structure below is used by the bridge_cs100 program to issue *
 *  the proper sequence of port connection commands.                  *
 **********************************************************************/


struct Bridge_sequence{
			 char *string;   /* the char string message */
			 int length;     /* total chars in string   */
} bridge[] ={
/* 0*/       "cs/100",                   6,
/* 1*/       "\r",                       1,
/* 2*/       "pause",                    5,
/* 3*/       "connected to ",           13,
/* 4*/       "\r\n",			 2,
/* 5*/       "resume\r",		 7,
/* 6*/       "resumed",		 	 7,
	     };

/*********************************************************************
 *  bridge_cs100()                                                   *
 *    This submodule implements port configuration sequence for a    *
 *    Bridge Communications, Inc., CS/100 communications server.     *
 *    The CS/100 is not really a modem, but rather a "port selector" *
 *    that can be treated like a modem in that the equivalent of a   *
 *    dial sequence is used to establish a connection with another   *
 *    port.							     * 
 *								     *
 *    The "phone number" parameter, which can contain only digits    *
 *    and '-' (no white space) is of the form:		             *
 *								     *
 *		<port#> [ - [<macro1#>] [ - <macro2#> ] ]	     *
 *    where:							     *
 *       	<port#> is the number of the port with which         *
 *			connection is desired.			     *
 *  		<macro1#> is an optional digit sequence, which if    *
 *			supplied will cause dialit to execute the    *
 *			macro of the name "dial<macro1#>" before     *
 *			establishing the connection.		     *
 *  		<macro2#> is an optional digit sequence, which if    *
 *			supplied will cause dialit to execute the    *
 *			macro of the name "dial<macro2#>" after      *
 *			establishing the connection.		     *
 *								     *
 *    The "speed" parameter is ignored.				     *
 *								     *
 *    The entry for this modem in L.sys and L-devices files should be*
 *                                                                   *
 *                ACUBRIDGECS100                                     *
 *                                                                   *
 *    Return codes are:                                              *
 *         FAIL - the autodial was  not successful                   *
 *         SUCCESS - The autodial was successful                     *
 *********************************************************************/


bridge_cs100(phone,cua,speed)
char *phone,                             /* string containing the port addr */
     *cua,                               /* dial line pathname */
     speed;                              /* (parameter ignored) */
{
  int  retval = FAIL,                    /* initial return value */
       delay = 5,                        /* delay for messages */
       con_delay = 20,                   /* delay for connection */
       fd_cua;                           /* descriptor for dial line */
  char msg[50],                          /* message string*/
       *mp1,*mp2;			 /* macro pointers */
  /*
   * make sure the line specified for the autodial sequence is not the
   * /dev/null device.
   */

  if (strcmp(cua,"/dev/null") != SAME){   /* cua was specified - go on */

      /*
       * attempt opening of the cua line to be used for the autodial
       * sequence.
       */

      sprintf(msg,"opening of %s",cua);
      log_entry(msg,"REQUESTED");
      if ((fd_cua = open(cua,O_RDWR)) >= 0){ /*cua open ok - continue*/
	  sprintf(msg,"opening of %s",cua);
	  log_entry(msg,"SUCCESSFUL");

	  /*
	   * Inspect phone number for macros. Leave mp1 and mp2
	   * pointing to first and second macros or to \0.
	   */
	  
	  for (mp1=phone; *mp1!='-' && *mp1!='\0'; mp1++);
	  mp2 = mp1;
	  if (*mp1!='\0')
	  {
	      *mp1++ = '\0';
	      for (mp2=mp1; *mp2!='-' && *mp2!='\0'; mp2++);
	      if (*mp2!='\0') *mp2++ = '\0';

	  }
	  /*
	   * send a leading cr and then look for a "pause"
	   * command executed by CS/100's init macro
	   */

	  log_entry("init macro","REQUESTED");
	  sendstring(fd_cua,bridge[1].string,bridge[1].length);
	  if (!await(fd_cua,bridge[2].string,bridge[2].length,delay))
	  {
	      log_entry("init macro","FOUND");

              /*
               * send a break to abort the init macro
               */

	      log_entry("break","REQUESTED");
	      if (ioctl(fd_cua,TCSBRK,0))
	          log_entry("break","FAILED");
	      else
	      {
		  /*
		   * Break was ok, demand a prompt
		   */

		  log_entry("break prompt","REQUESTED");
	          if (await(fd_cua,bridge[0].string,bridge[0].length,delay))
	          {
	              log_entry("break prompt","FAILED");
	              return(retval);
	          }
	      }
	  }
          

	  /*
  	   * There might be another prompt, for wait for it.
 	   */

	  await(fd_cua,bridge[0].string,bridge[0].length,delay);


	  /*
	   * Send another cr and then demand a prompt
	   */

	  log_entry("prompt","REQUESTED");
	  sendstring(fd_cua,bridge[1].string,bridge[1].length);
	  if (await(fd_cua,bridge[0].string,bridge[0].length,delay))
	  {
	      log_entry("initial prompt","FAILED");
	      return(retval);
	  }

	  /*
	   * Now that we have control, see if we should execute a macro
	   */

	  if (*mp1!='\0') 
	  {
	      sprintf(msg,"do dial%s\r",mp1);
	      log_entry(msg,"REQUESTED");
	      sendstring(fd_cua,msg,strlen(msg));
	      if (await(fd_cua,bridge[0].string,bridge[0].length,delay))
	      {
	          log_entry("dial macro1","FAILED");
	   	  return(retval);
	      }
	  }

	  /*
	   * Now try for a connection. Append "ecm" to the connection
	   * command if there is another macro to execute.
	   */

	  if (*mp2=='\0') sprintf(msg,"co !%s\r",phone);
	  	     else sprintf(msg,"co !%s ecm\r",phone);
	  log_entry(msg,"REQUESTED");
	  sendstring(fd_cua,msg,strlen(msg));
	  if (await(fd_cua,bridge[3].string,bridge[3].length,con_delay))
	  {
	      log_entry("connection message","FAILED");
	      return(retval);
	  }

	  /*
	   *  If requested, wait for prompt and execute the second macro
	   */

	  if (*mp2!='\0') 
	  {
	      if (await(fd_cua,bridge[0].string,bridge[0].length,delay))
	      {
	          log_entry("connect prompt","FAILED");
	   	  return(retval);
	      }
	      sprintf(msg,"do dial%s\r",mp2);
	      log_entry(msg,"REQUESTED");
	      sendstring(fd_cua,msg,strlen(msg));
	      if (await(fd_cua,bridge[0].string,bridge[0].length,delay))
	      {
	          log_entry("dial macro2","FAILED");
	   	  return(retval);
	      }
	      log_entry("resume","REQUESTED");
	      sendstring(fd_cua,bridge[5].string,bridge[5].length);
	      if (await(fd_cua,bridge[6].string,bridge[6].length,delay))
	      {
	          log_entry("resume","FAILED");
	   	  return(retval);
	      }
	  }

	  /*
	   * Wait for final \r\n and connection is successful
	   */ 

	  log_entry("final cr/lf","REQUESTED");
	  await(fd_cua,bridge[4].string,bridge[4].length,delay);
	  log_entry("connection","SUCCESSFUL");
	  retval = SUCCESS;

      }
      else{
	 
	  /*
	   * autodial line did not open successfully.
	   */

	  sprintf(msg,"open of cua %s",cua);
	  log_entry(msg,"FAILED");
	  log_entry("connection","FAILED");
      }
  }
  else{
      
      /*
       * /dev/null was specified as the call line device.
       */

      log_entry("invalid cua device","FAILED");
      log_entry("connection","FAILED");
  }
  return(retval);
}



/****************************************************************/
/*
 *
 *  START OF X25 ROUTINES NEEDED FOR TALKING TO GTE/TELNET AND
 *  THE HP2334A
 *
 */
/****************************************************************/
/***************************************************************************
*   awaitr(string,file,timeout)
*
*   await reads from the line associated with file until the "string"
*   has been read or the total characters read.   Chars are only matched 
*   at the end of the buffer read.  Nulls are not skipped.
*   
*   return values:
*     FAIL - string did not appear within timeout period.
*     SUCCESS - string did appear within the timeout period.
****************************************************************************/

awaitr(file,string,timeout)
char	*string;	/* str expecting on line */
int	file,		/* descriptor of line to read from */
	timeout;	/* maxtime to wait for input from line*/
{
register	char_count;	/* number of characters read from line*/
int	str_length;		/* length of string looking for */
int	ret;			/* return value form read*/
int	x,found;			
char	char_read,		/* current character read from line*/
	reply[1024];	        /* buffer to hold incoming message*/

	/* always return on timeout problem with failure */
	if (setjmp(Sjbuf)) return(FAIL);

	char_count = 0;
	found      = 0;
	str_length = strlen(string); 

	signal(SIGALRM,alarmtout);	/* set up for possible alarm timeout*/
	alarm(timeout);

	while (found == 0) {
		ret = read(file,&char_read,1);
		if (ret <= 0){
			alarm(0);
			return(FAIL);
		}
		if (char_count > sizeof(reply)){
			alarm(0);
			return(FAIL);
		}
		reply[char_count++] = toascii(char_read);
		if ((x=char_count-str_length) > 0) {
                	if (strncmp(string,&reply[x],str_length) == 0) {
				found = 1;
			}
		}
	}
	alarm(0);
	return(SUCCESS);
}

/* 
 * x25access - performs a user check on the calling of the id of the 
 *             process.  root, and uucp are bypassed and any gid in
 *             uucp.  If not the above, the name list in the X25SYSFILE
 *             will be consulted for a match.
 *
 *             Takes calling network address as the arguments to locate
 *             and entry in the table.
 *
 *	       Returns the dialer name, default_phone number,
 *             telnet id, telnet pwd as the 
 *             arguments.  Returns true if user ok'd and the network
 *	       address was found, false otherwise.
 *
 */

int x25access( netcall, dialer, default_phone, telid, telpwd )
char *netcall,          /* network call address users wishes to connect to */
     *dialer,           /* name of returned dialer to use for access       */
     *default_phone,    /* preferred phone number for GTE/Telnet pad       */
     *telid,            /* gte/telnet login id  */
     *telpwd;           /* gte/telnet login pwd */
{
FILE *fp_x25;	/* file pointer for X25 file */
char buf[256], name[30];
struct passwd *pwd, *getpwuid(), *getpwnam();
struct group  *grp, *getgrnam();
int uucpuid = 0,
    uucpgid = 0;
int g, uid, getuid(), getgid(), geteuid(), getegid();
int retval = FAIL;

	/* open L.X25 file for connection information */
	if((fp_x25 = fopen("/usr/lib/uucp/L.X25", "r")) == NULL) {
		log_entry("X25 read failed on sysfile","FAILED");
		return(retval);
	}

	while(fgets(buf, 256, fp_x25) != NULL) {
		if ( strcmp(netcall,strtok(buf," \t")) != 0 )
			continue;
		strcpy( dialer, strtok((char*)NULL," \t"));
		if (strlen(dialer) == 0 || strncmp(dialer,"ACU",3) != 0 ) {
			log_entry("X25 sysfile - bad dialer","FAILED");
			return(retval);
		}
		strcpy( default_phone, strtok((char*)NULL," \t"));
		if (strlen(default_phone) == 0 ) {
			log_entry("X25 sysfile - bad phone#","FAILED");
			return(retval);
		}
		strcpy(  telid, strtok((char*)NULL," \t"));
		if (strlen(telid) == 0) {
			log_entry("X25 sysfile - bad telid","FAILED");
			return(retval);
		}
		strcpy( telpwd, strtok((char*)NULL," \t"));
		if (strlen(telpwd) == 0) {
			log_entry("X25 sysfile - bad telpwd","FAILED");
			return(retval);
		}
	}
	printf("dialit: x25access %s %s %s\n",default_phone,telid,telpwd);

	/* ok, table entries found, now check user's access root first */
	if ((uid = getuid()) == 0 || geteuid() == 0 ) {
		printf("dialit: x25 root access\n");
		return(SUCCESS); 
	}

	/* nope, now check for real uucp login uid */
	pwd = getpwnam("uucp");  if (pwd != NULL) uucpuid = pwd->pw_uid;
	if ( uucpuid == getuid() ) {
		  printf("dialit: x25 uucp access\n");
		  return(SUCCESS); 
	}

	/* nope, also allow if real uucp group id */
	grp = getgrnam("uucp");  if (grp != NULL) uucpgid = grp->gr_gid;
	if ( uucpgid == getgid() ) {
		printf("dialit: x25 uucp group access\n");
		return(SUCCESS); 
	}

	/* nope, now see if user's name is in the access list */
	pwd = getpwuid( uid );
	printf("dialit: x25 checking access for %s uid=%d gid=%d\n",
                       pwd->pw_name,uid,getgid());
	while(strcpy(name,strtok((char*)NULL," \t")) != NULL) {
		printf("dialitx: scan %s\n",name);
		if (strncmp(name,"*",1) == 0)         /* wild card for alles */
			return(SUCCESS);
		if (strcmp(name,pwd->pw_name) == 0)   /* real name */ 
			return(SUCCESS);
		if (sscanf(name,"%d",&g) != 0)        /* check group too */
			if (g==getgid())
				return(SUCCESS);
	}
	sprintf(buf,"X25 bad access uid=%d gid=%d\n",uid,getgid());
	printf("%s\n",buf);
	log_entry(buf,"FAILED");
	return(retval);
}

/*
 * various command prompts and feedback encountered when dealing
 * with a GTE/Telnet dial-up pad.
 */
char *gte_telnet[] ={
/* 0*/        "TELNET\r",            
/* 1*/        "303 12D\r",          
/* 2*/        "TERMINAL=",         
/* 3*/        "d1\r",             
/* 4*/        "@",               
/* 5*/        "ID HPFSDLAB\r",  
/* 6*/        "PASSWORD =",    
/* 7*/        "071667\r",          
/* 8*/        "CONNECTED\r\n",      
/* 9*/        "\r",
/*10*/	      "\r@\r",
/*11*/        "CONT\r",
/*12*/        "TELENET\r\n\r\n@",
/*13*/        "CONT",
};


/*
 *  x25telnet(phone,dev)
 *
 *  This routine is designed to implement autodialing for a racal 
 *  vadic 212P/S/G-series modem and login to an X25 network address.
 * 
 *  RETURN VALUES:
 *     SUCCESS - autodial successful.
 *     FAIL - autodial failed.
 */

x25telnet(phone,cua,speed)
char 	*phone,                     /* string containing phone number */
	*cua,                       /* call line used for dialing */
	*speed;                     /* speed of both lines */
{
int	retval = FAIL;            /* value to be returned by routine */
int	fd_cua;			  /* cua file pointer */
int	uucpmode = 0;		  /* flag for uucico activity */
int	normal_delay = 5;         /* delay for normal message */
struct	Modems *ap;               /* pointer to table of modems*/
char	msg[50];                  /* text for log entry*/
char    call_request[50];         /* formatted text for connect message */
char    realphone[50],               /* real phone number */
	netaddress[50],              /* telnet network address */
	*netcall,
	parstring[40],
	parstring2[40],
	dialer[30],		/* auto dialer to use, from X25SYSFILE */
	def_phone[30],		/* default phone number for Telnet Pad */
	telid[16],		/* telnet login id                     */
	telpwd[16];		/* telnet login password	       */
char	*strrchr();
int	par02 = 1,		/* echo */
	par03 = 2,		/* data forwarding signal to cr */
	par04 = 10,  		/* idle timer delay 1/2 second */
	par05 = 1,		/* flow control by pad */
	par12 = 1,		/* flow control by device */
	par13 = 4,		/* local echo line feed */
	par15 = 1,		/* editing */
	par16 = 8,		/* delete character ascii decimal 8 */
	par17 = 24, 		/* unknown */
	par00 = 33,		/* GTE-TELNET national escape for ext */
	par57 = 1,		/* GTE-TELENT extended, set xparent */
	par63 = 0;		/*    "       8-bit no parity       */

	/* now see if can open cu line, if not, fail */
	if ((fd_cua = open(cua,O_RDWR)) >= 0){
		sprintf(msg,"open of cua device %s",cua);
		log_entry(msg,"SUCCESSFUL");
	} else {
		sprintf(msg,"open of cua %s",cua);
		log_entry(msg,"FAILED");
		log_entry("autodial","FAILED");
		return(retval);
	}

	/* realphone is delimited from netaddress by an 'X' in phone */
        /* set up a pointer realphone to beginning of phone then     */
        /* set net pointer to first char after X                     */

	/* taken out for now, asssume phone is the netaddress
	  if (strlen(phone) != 0 && phone[0] == 'X') {
		realphone[1] = '\0';
	  	(void) strcpy(netaddress,strtok(phone,"X"));
	  } else {
	  	(void) strcpy(realphone,strtok(phone,"X"));
         	(void) strcpy(netaddress,strtok((char*)NULL,"X"));
	  }
	*/
	  (void) strcpy(netaddress,phone);

	  if (strlen(netaddress) == 0 ) {
		  log_entry("X25 null netaddress","FAILED");
		  return(retval);
	  }

	/* if uucico is driving, the first char of netaddr is '-' */
	  netcall = &netaddress[0];
	  if (netaddress[0] == '-') {
		  printf("dialitx: uucico mode\n");
		  uucpmode = 1;
	          netcall++;
	  }

	/* pass the call address to the lookup function         */
        /* now check for user's access and set up dialer et. al */
	  if ( x25access(netcall,dialer,def_phone,telid,telpwd) == FAIL )
	  	  return(FAIL);

	/* the real phone number to dial is the default from L.X25 */ 
	  strcpy(realphone,def_phone);

	/* call the appropriate modem routine */ 
	  for (ap = modem; ap->name;ap++) {
		  if (strcmp(ap->name,dialer) == SAME) {
			  retval = (*ap->modem_fn)(realphone,cua,speed);
			  if (retval==FAIL) return(retval);
		  }
	  }

	/* set up gte x.3 x.28 parameters                         */
	  if (uucpmode == 1) { 
		par02 = par03 = par04 = par05 = 0;
		par12 = par13 = par15 = par16 = par17 = 0;
		par04 = 5; 
	  }
	sprintf(parstring,"set 1:0,2:%d,3:%d,4:%d,5:%d,6:1,7:8,8:0,9:0,",
			par02,par03,par04,par05); 
	sprintf(&parstring[strlen(parstring)],
               "12:%d,13:%d,14:0,15:%d,16:%d,17:%d\r",
                    par12,par13,par15,par16,par17);
	sprintf(parstring2,"set 0:%d,57:%d,63:%d\r",par00,par57,par63);

	/* ok, now we should be connected to the GTE/TELNET network */
	/* allow things to stablize */
	  sleep(2);

	/* then send two carriage returns */
	  sendstring(fd_cua,gte_telnet[9],1);
	  sleep(1);
	  sendstring(fd_cua,gte_telnet[9],1);

	/* await 'TERMINAL=' */ 
    	if (awaitr(fd_cua,gte_telnet[2],normal_delay)){	
		log_entry("GTE/TELNET Wake up","FAILED");
		return(retval);
	}

	/* now send terminal type of null, and awaitr '@' */
	sendstring(fd_cua,gte_telnet[9],1);
	if (awaitr(fd_cua,gte_telnet[4],normal_delay)){
		log_entry("TELNET prompt not found","FAILED");
		return(retval);
	}

	/* after prompt, send login id and wait for 'PASSWORD =' */
	sendstring(fd_cua,gte_telnet[5],strlen(gte_telnet[5]));
	if (awaitr(fd_cua,gte_telnet[6],normal_delay)){
		log_entry("TELNET no PASSWORD =","FAILED");
		return(retval);
	}

	/* now send password and wait for '@' */
	sendstring(fd_cua,gte_telnet[7],strlen(gte_telnet[7]));
	if (awaitr(fd_cua,gte_telnet[4],normal_delay)){
		log_entry("TELNET no prompt after passwd","FAILED");
		return(retval);
	}


	sprintf(call_request,"C %s\r",netcall);
	sprintf(msg,"TELNET %s",call_request);
	log_entry(msg,"REQUESTED");
	sendstring(fd_cua,call_request,strlen(call_request));
	if (awaitr(fd_cua,gte_telnet[8],25)){
		log_entry(msg,"FAILED");
		return(retval);
	}
	log_entry(msg,"SUCCESS");
	retval = SUCCESS;

	/* now escape back to command mode in TELNET */
	sendstring(fd_cua,gte_telnet[10],strlen(gte_telnet[10]));
	if (awaitr(fd_cua,gte_telnet[12],normal_delay)) {
		log_entry("TELNET command escape","FAILED");
		return(retval);
	}

	/* now send parameter settings */
	sendstring(fd_cua,parstring,strlen(parstring));
	if (awaitr(fd_cua,gte_telnet[4],normal_delay)){
		log_entry("TELNET set failed","FAILED");
		return(retval);
	}
	sendstring(fd_cua,parstring2,strlen(parstring2));
	if (awaitr(fd_cua,gte_telnet[4],normal_delay)){
		log_entry("TELNET set failed","FAILED");
		return(retval);
	}

	/* now send CONT */
	sendstring(fd_cua,gte_telnet[11],strlen(gte_telnet[11]));
	if (awaitr(fd_cua,gte_telnet[13],normal_delay)){
		log_entry("TELNET CONT failed","FAILED");
		return(retval);
	}

return(retval);
}


/* perform clear, dial, and set for generic X.25 PAD */

x25box (pad, phone,cua,speed)
char *pad, *phone,*cua,*speed;
{
    int fd_cua;
    int clrsvc_pid = 0;
    int opx25_pid = 0;
    int retval=FAIL;
    int normal_delay = 10;
    int status;
    int i,j;

    char dial[21];
    char c;
    char scriptflag[50], inflag[10], outflag[10], numberflag[100];

    if (strlen(allow_protos) > 1) {
	    log_entry(pad, "TOO MANY PROTOS");
	    return( retval );
    }

    log_entry (pad,"INIT LINE");

    if ( *allow_protos == '\0' )
	strcpy(allow_protos, "i");
    switch (*allow_protos) {
	case 'g':  /* X.3 param supporting uucp g-protocol */
	    log_entry(pad, "CHOSE g-PROTOCOL");
	    break;
	case 'f':  /* X.3 param supporting uucp f-protocol */
	    log_entry(pad, "CHOSE f-PROTOCOL");
	    break;
	case 'i': /* interactive session */
	    log_entry(pad, "INTERACTIVE SESSION");
	    break;
	default:
	    log_entry(pad,"UNKNOWN PROTOCOL");
	    return(retval);
    }

    for (i=j=0; i < strlen (phone); i++)
	if (isdigit (c = phone [i]))
	    dial [j++] = c;

    dial[j] = '\0';

    if (strcmp (cua,"/dev/null") == SAME)
	return(retval);

    if ((fd_cua=open (cua,O_RDWR)) < 0) {
	log_entry (pad, "open FAILED");
	return(retval);
    }

    if (setjmp(Sjbuf)) {
	log_entry(pad,"CLRSVC TIMED OUT");
	if (clrsvc_pid)
		kill(clrsvc_pid, SIGKILL);
	return(retval);
    }
    signal(SIGALRM, alarmtout);
    alarm(40);
    signal(SIGHUP, SIG_IGN);     /* could get hangup here */

    if ( (clrsvc_pid = fork()) )
	wait(&status);
    else {
	execl("/usr/lib/uucp/X25/clrsvc", "clrsvc", cua, pad, 0);
	log_entry(pad, "CLRSVC MISSING!");
	exit(69);
    }
    alarm(0);
    if (status & 0xffff) {
	log_entry (pad,"clrsvc FAILED");
	return (retval);
    }
    signal(SIGHUP, got_sig);

    sprintf(scriptflag, "-f/usr/lib/uucp/X25/%s.out", pad);
    sprintf(inflag, "-i%d", fd_cua);
    sprintf(outflag, "-o%d", fd_cua);
    sprintf(numberflag, "-n%s", dial);

    if (setjmp(Sjbuf)) {
	log_entry(pad,"OPX25(dial) TIMED OUT");
	if (opx25_pid)
		kill(opx25_pid, SIGKILL);
	return(retval);
    }

    signal(SIGALRM, alarmtout);
    alarm(30);
    if (opx25_pid=fork())
	wait(&status);
    else {
	execl("/usr/lib/uucp/X25/opx25", "opx25",
			scriptflag,
			numberflag,
			inflag,
			outflag,
			0);
	log_entry(pad, "OPX25 MISSING!");
	exit(1);
    }

    alarm(0);
    status &= 0xffff;
    if ((status >> 8) == 1) {
	log_entry (dial,"dial FAILED");
	return (retval);
    }
    if ((status >> 8) == 2) {
	log_entry (pad, "wait for '@' FAILED");
	return (retval);
    }

    if (status != 0) {
	log_entry(pad, "error in script");
	return (retval);
    }
    log_entry(pad,"DIAL SUCCEEDED");

/* set up PAD for requested protocol */

    sprintf(scriptflag, "-f/usr/lib/uucp/X25/%s.out", pad);
    strcat(scriptflag, allow_protos);
    sprintf(inflag, "-i%d", fd_cua);
    sprintf(outflag, "-o%d", fd_cua);

    if (setjmp(Sjbuf)) {
	log_entry(pad,"OPX25(set) TIMED OUT");
	if (opx25_pid)
	    kill(opx25_pid, SIGKILL);
	return(retval);
    }

    signal(SIGALRM, alarmtout);
    alarm(20);
    if (opx25_pid=fork())
	wait(&status);
    else
	execl("/usr/lib/uucp/X25/opx25","opx25",scriptflag,inflag,outflag,0);
    alarm(0);
    if (status != 0) {
	log_entry(pad,"OPX25(set) failed - don't know why");
	return(retval);
    }

    retval = SUCCESS;
    log_entry (pad, "dial & set SUCCESS");

    return (retval);
}

