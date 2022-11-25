
static char *HPUX_ID = "@(#) $Revision: 70.1 $";

/*
 *   Modified:  25 June 1992 fenwick (John Fenwick)
 *	- fixed defect that read of passwd entries with fgets(BUF_SIZ)
 *	  would read first 1023 chars of 1024-char entry, then would
 *	  read 1024th char and \012 and treat as another passwd entry
 *	  which would then be entered with uid=0 (a root login
 *	  without a password, which is a grave security hole)
 *	  This defect also exists in existing 7.0 amd 8.0 code!
 *   Modified:  25 May 1989 - mhn (Mark Notess)
 *	- created local getpwent, getpwuid, and getpwnam routines
 *	  (they appear with an _nyp suffix) to solve the problem of
 *	  escaping to Yellow Pages.  getpwent_nyp does _not_ escape
 *	  to YP upon encountering a + in the first column of /etc/passwd.
 *	  Neither does it strip +; hence +user1 and user1 will not match.
 *	  Note:  the code for this was hacked from reconfig (reconfig/user.c).
 *	- solving the YP problem also solved the following auditing/security
 *	  problem:  chfn copied the passwords from the trusted system's
 *	  /.secure/etc/passwd file into the regular /etc/passwd (because
 *	  getpwent is .secure-smart, but putpwent isn't).  Since chfn
 *	  doesn't need the password field, it doesn't touch the secure
 *	  password file.
 *	- fixed an error message spelling error.
 *	- un-ifdef'd SIGTSTP since it is now defined for all HP-UX.
 *	- added ulimit call to plug security hole.  If user has set
 *        a very small ulimit, the passwd file could get corrupted.
 *
 * merged in the following revisions on top of the local versions
 * (which should have been here in the first place).
 * revision 56.2        
 * date: 88/02/25 11:26:14;  author: lois;  state: Exp;  lines added/del: 1/2
 * removed declaration of int  endpwent() and removed (void) cast from
 * invocation of endpwent.  endpwent is defined in pwd.h to return void
 * ----------------------------
 * revision 56.1        
 * date: 88/02/24 18:01:59;  author: bes;  state: Exp;  lines added/del: 1/1
 * Changed include <sys/time.h> to include <time.h>
 *
 * Modified: 8 July 1986 - lkc
 * chfn of uid root accounts now only change the requested account
 * instead of changing all uid 0 accounts.
 * 
 * Modified: 11 December 1986 - lkc
 * does not modify a pri=n item in the GCOS field
 */

/*
 *	 chfn - change finger entries
 */
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <pwd.h>
#include <time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <sys/file.h>
#include <ctype.h>
#include <sys/param.h>
#ifdef AUDIT
#include <sys/audit.h>
#include <sys/errno.h>
#include <sys/stat.h>
#endif

struct default_values {
	char *name;
	char *office_num;
	char *office_phone;
	char *home_phone;
};

char	passwd[] = "/etc/passwd";
char	temp[]	 = "/etc/ptmp";
struct	passwd *pwd;
static struct passwd *getpwent_nyp(), *getpwnam_nyp(), *getpwuid_nyp();
char	*crypt();
char	*getpass();
char	*malloc(), *strcpy(), *strchr(), *strcat();
#ifdef AUDIT
char	savedname[9];
struct stat sbuf;
#endif

extern void rewind(), perror(), exit();
extern long strtol(), ulimit();
extern FILE *fopen();
extern int strcmp(), strncmp(), strlen(), fclose();
extern char *fgets(), *strncpy();
extern unsigned short getuid();

main(argc, argv, environ)
int argc;
char *argv[];
char **environ;
{
	int user_uid;
	char replacement[4*BUFSIZ];
	char loginname[4*BUFSIZ];
	int fd;
	FILE *tf;

	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH", 0);

	if (argc > 2) {
		printf("Usage: chfn [user]\n");
		exit(1);
	}

#ifdef AUDIT
	/* Do self-auditing */
	if (audswitch(AUD_SUSPEND) == -1) {
		fprintf(stderr, "chfn: "); perror("audswitch");
		exit(1);
	}

	/* Stat password file to save group/owner/mode */
	if (stat(passwd, &sbuf) != 0) {
		fprintf(stderr, "chfn: ");
		perror(passwd);
		exit(1);
	}
#endif
	/*
	 * Error check to make sure the user (foolishly) typed their own name.
	 */
	user_uid = getuid();
	if (argc==2) {
		pwd = getpwnam_nyp(argv[1]);
		if (pwd == NULL) {
			printf("There is no account for %s on this machine.\n", 
				argv[1]);
			if (user_uid!=0)
				printf("%s%s%s%s%s",
				   "You probably misspelled your login name;\n",
				   "only root is allowed to change another",
				   " person's finger entry.\n",
				   "Note:  You do not need to type your login",
				   " name as an argument.\n");
#ifdef AUDIT
			/* Construct a failed audit record */
			audit(argv[1], " No account for user", 1);
#endif
			exit(1);
		}
		if (user_uid != 0 && pwd->pw_uid != user_uid) {
			printf("%s%s",
				"You are not allowed to change another",
				" person's finger entry.\n");
#ifdef AUDIT
			/* Construct a failed audit record */
			audit (argv[1], " Permission denied", 1);
#endif
			exit(1);
		}
	}
	else /* (argc == 1) */ {
		pwd = getpwuid_nyp(user_uid);
		if (pwd == NULL) {
		    fprintf(stderr, "You have no /etc/passwd file entry.\n");
#ifdef AUDIT
		    /* Construct a failed audit record */
		    audit("Current user", " No passwd file entry", 1);
#endif
		    exit(1);
		}
	}
	user_uid = pwd->pw_uid;
	strcpy(loginname, pwd->pw_name);
#ifdef AUDIT
	strcpy(savedname, pwd->pw_name);
#endif
	/*
	 * Collect name, room number, school phone, and home phone.
	 */
#ifdef AUDIT
	get_info(pwd->pw_gecos, replacement, savedname);  /* info for audit rec */
#else
	get_info(pwd->pw_gecos, replacement);
#endif

	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTSTP, SIG_IGN);

	(void) umask(0);
	if ((fd = open(temp, O_CREAT|O_EXCL|O_RDWR, 0644)) < 0) {
		printf("Temporary file busy -- try again\n");
#ifdef AUDIT
		/* Construct a failed audit record */
		audit(savedname, " Temporary file busy", 1);
#endif
		exit(1);
	}
	if ((tf = fdopen(fd, "w")) == NULL) {
		printf("Absurd fdopen failure - seek help\n");
		goto out;
	}
#ifndef HP
	unlimit(RLIMIT_CPU);
	unlimit(RLIMIT_FSIZE);
#endif HP
	/*
	 * Copy passwd to temp, replacing matching lines
	 * with new gecos field.
	 */

	ulimit(2,20000); 	/* plug security hole */

	while ((pwd = getpwent_nyp()) != NULL) {
		if ( (pwd->pw_uid == user_uid) && 
		     (strcmp(loginname, pwd->pw_name) == 0) ) {
		        int len;
			char t[BUFSIZ];
			len = strlen(pwd->pw_name);
			len += strlen(pwd->pw_passwd);
			len += strlen(pwd->pw_age) <= 0 ? 0 :
				strlen(pwd->pw_age) + 1; /* + 1 for comma */
			len += strlen(replacement);
			len += strlen(pwd->pw_dir);
			len += strlen(pwd->pw_shell);
			sprintf(t,"%d%d",pwd->pw_uid, pwd->pw_gid);
			len += strlen(t);
			len += 6;	/* separators ':' */
			if(len < 1024)
			    pwd->pw_gecos = replacement;
		}
		putpwent(pwd,tf);              
	}
	endpwent();

#ifdef AUDIT
	/* Restore owner/group/mode of password file */
	if (chown(temp, sbuf.st_uid, sbuf.st_gid) != 0) {
		fprintf(stderr, "chfn: cannot chown %s\n", passwd);
		(void) unlink(temp);
		/* Construct a failed audit record */
		audit(savedname," Error in chown", 1);
		exit(1);
	}

	if (chmod(temp, sbuf.st_mode) != 0) {
		fprintf(stderr, "chfn: cannot chmod %s\n", passwd);
		(void) unlink(temp);
		/* Construct a failed audit record */
		audit(savedname," Error in chmod", 1);
		exit(1);
	}
#endif
	if (rename(temp, passwd) < 0) {
		fprintf(stderr, "chfn: "); perror("rename");
  out:
		(void) unlink(temp);
#ifdef AUDIT
		/* Construct a failed audit record */
		audit(savedname," Error in rename", 1);
#endif
		exit(1);
	}
	(void) fclose(tf);
#ifdef AUDIT
	/* Construct a successful audit record */
	audit(savedname, " Successful chfn", 0);
#endif
	exit(0);
}

#ifndef HP
unlimit(lim)
{
	struct rlimit rlim;

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
/*	(void) setrlimit(lim, &rlim);*/
}
#endif HP

/*
 * Get name, room number, school phone, and home phone.
 */
#ifdef AUDIT
get_info(gecos_field, answer, name)
#else
get_info(gecos_field, answer)
#endif
	char *gecos_field;
	char *answer;
#ifdef AUDIT
	char *name;
#endif
{
	char in_str[BUFSIZ];
	struct default_values *defaults, *get_defaults();

	answer[0] = '\0';
	if(strncmp("pri=", gecos_field, 4) == 0) {
	    int i;
	    
	    i = 4;
	    if(gecos_field[i] == '-')
		i++;
	    while(gecos_field[i] >= '0' && gecos_field[i] <= '9')
		i++;
	    strncpy(answer, gecos_field, i);
#ifdef AUDIT
	    defaults = get_defaults(&gecos_field[i], name);
#else
	    defaults = get_defaults(&gecos_field[i]);
#endif
	}
	else {
#ifdef AUDIT
	    defaults = get_defaults(gecos_field, name);
#else
	    defaults = get_defaults(gecos_field);
#endif
	}
	printf("Default values are printed inside of of '[]'.\n");
	printf("To accept the default, type <return>.\n");
	printf("To have a blank entry, type the word 'none'.\n");
	/*
	 * Get name.
	 */
	do {
		printf("\nName [%s]: ", defaults->name);
		(void) fgets(in_str, BUFSIZ, stdin);
		if (special_case(in_str, defaults->name)) 
			break;
	} while (illegal_input(in_str));
	(void) strcat(answer, in_str);
	/*
	 * Get room number.
	 */
	do {
#ifdef HP
		printf("Location (Ex: 42U-J4) [%s]: ",
			defaults->office_num);
#else
		printf("Room number (Exs: 597E or 197C) [%s]: ",
			defaults->office_num);
#endif HP
		(void) fgets(in_str, BUFSIZ, stdin);
		if (special_case(in_str, defaults->office_num))
			break;
	} while (illegal_input(in_str));
	(void) strcat(strcat(answer, ","), in_str);
	/*
	 * Get office phone number.
#ifdef	HP
	 * Removes hyphens only; requires four digits
#else	HP
	 * Remove hyphens and 642, x2, or 2 prefixes if present.
#endif	HP
	 */
	do {
		printf("Office Phone (Ex: 1632) [%s]: ",
			defaults->office_phone);
		(void) fgets(in_str, BUFSIZ, stdin);
		if (special_case(in_str, defaults->office_phone))
			break;
		remove_hyphens(in_str);
	} while (illegal_input(in_str) || not_all_digits(in_str)
		 || wrong_length(in_str, 4));
	(void) strcat(strcat(answer, ","), in_str);
	/*
	 * Get home phone number.
	 * Remove hyphens if present.
	 */
	do {
		printf("Home Phone (Ex: 9875432) [%s]: ", defaults->home_phone);
		(void) fgets(in_str, BUFSIZ, stdin);
		if (special_case(in_str, defaults->home_phone))
			break;
		remove_hyphens(in_str);
	} while (illegal_input(in_str) || not_all_digits(in_str));
	(void) strcat(strcat(answer, ","), in_str);
}

/*
 * Prints an error message if a ':' or a newline is found in the string.
 * A message is also printed if the input string is too long.
 * The password file uses :'s as seperators, and are not allowed in the "gcos"
 * field.  Newlines serve as delimiters between users in the password file,
 * and so, those too, are checked for.  (I don't think that it is possible to
 * type them in, but better safe than sorry)
 *
 * Returns '1' if a colon or newline is found or the input line is too long.
 */
illegal_input(input_str)
	char *input_str;
{
	char *ptr;
	int error_flag = 0;
	int length = strlen(input_str);

	if (strchr(input_str, ':')) {
		printf("':' is not allowed.\n");
		error_flag = 1;
	}
	if (input_str[length-1] != '\n') {
		/* the newline and the '\0' eat up two characters */
		printf("Maximum number of characters allowed is %d\n",
			BUFSIZ-2);
		/* flush the rest of the input line */
		while (getchar() != '\n')
			/* void */;
		error_flag = 1;
	}
	/*
	 * Delete newline by shortening string by 1.
	 */
	input_str[length-1] = '\0';
	/*
	 * Don't allow control characters, etc in input string.
	 */
	for (ptr=input_str; *ptr != '\0'; ptr++) {
		if ((int) *ptr < 040) {
			printf("Control characters are not allowed.\n");
			error_flag = 1;
			break;
		}
	}
	return(error_flag);
}

/*
 * Removes '-'s from the input string.
 */
remove_hyphens(str)
	char *str;
{
	char *hyphen;

	while ((hyphen=strchr(str, '-')) != NULL) {
		(void) strcpy(hyphen, hyphen+1);
	}
}

/*
 *  Checks to see if 'str' contains only digits (0-9).  If not, then
 *  an error message is printed and '1' is returned.
 */
not_all_digits(str)
	char *str;
{
	char *ptr;

	for (ptr=str; *ptr != '\0'; ++ptr) {
		if (!isdigit(*ptr)) {
			printf("Phone numbers can only contain digits.\n");
			return(1);
		}
	}
	return(0);
}

/*
 * Returns 1 when the length of the input string is not zero or equal to n.
 * Prints an error message in this case.
 */
wrong_length(str, n)
	char *str;
	int n;
{
	if ((strlen(str) == 5) && (str[0] == '7'))
		return(0);
	if ((strlen(str) != 0) && (strlen(str) != n)) {
		printf("The phone number should be %d digits long.\n", n);
		return(1);
	}
	return(0);
}


/* get_defaults picks apart "str" and returns a structure points.
 * "str" contains up to 4 fields separated by commas.
 * Any field that is missing is set to blank.
 */
#ifdef AUDIT
struct default_values 
*get_defaults(str, name)
#else
struct default_values 
*get_defaults(str)
#endif
	char *str;
#ifdef AUDIT
	char *name;
#endif
{
	struct default_values *answer;

	answer = (struct default_values *)
		malloc((unsigned)sizeof(struct default_values));
	if (answer == (struct default_values *) NULL) {
		fprintf(stderr,
			"\nUnable to allocate storage in get_defaults!\n");
#ifdef AUDIT
		/* Construct a failed audit record */
		audit(name, " malloc error", 0);
#endif
		exit(1);
	}
	/*
	 * Values if no corresponding string in "str".
	 */
	answer->name = str;
	answer->office_num = "";
	answer->office_phone = "";
	answer->home_phone = "";
	str = strchr(answer->name, ',');
	if (str == 0) 
		return(answer);
	*str = '\0';
	answer->office_num = str + 1;
	str = strchr(answer->office_num, ',');
	if (str == 0) 
		return(answer);
	*str = '\0';
	answer->office_phone = str + 1;
	str = strchr(answer->office_phone, ',');
	if (str == 0) 
		return(answer);
	*str = '\0';
	answer->home_phone = str + 1;
	return(answer);
}

/*
 *  special_case returns true when either the default is accepted
 *  (str = '\n'), or when 'none' is typed.  'none' is accepted in
 *  either upper or lower case (or any combination).  'str' is modified
 *  in these two cases.
 */
int special_case(str,default_str)
	char *str;
	char *default_str;
{
	static char word[] = "none\n";
	char *ptr, *wordptr;

	/*
	 *  If the default is accepted, then change the old string do the 
	 *  default string.
	 */
	if (*str == '\n') {
		(void) strcpy(str, default_str);
		return(1);
	}
	/*
	 *  Check to see if str is 'none'.  (It is questionable if case
	 *  insensitivity is worth the hair).
	 */
	wordptr = word-1;
	for (ptr=str; *ptr != '\0'; ++ptr) {
		++wordptr;
		if (*wordptr == '\0')	/* then words are different sizes */
			return(0);
		if (*ptr == *wordptr)
			continue;
		if (isupper(*ptr) && (tolower(*ptr) == *wordptr))
			continue;
		/*
		 * At this point we have a mismatch, so we return
		 */
		return(0);
	}
	/*
	 * Make sure that words are the same length.
	 */
	if (*(wordptr+1) != '\0')
		return(0);
	/*
	 * Change 'str' to be the null string
	 */
	*str = '\0';
	return(1);
}

#ifdef AUDIT
/****************************************************************************
 *
 *	audit - Construct a self audit record for the event and write
 *		it to the audit log.  This routine assumes that the
 *		effective uid is currently 0.
 *
 ****************************************************************************/ 

audit(name, msg, errnum)
char *name;
char *msg;
int errnum;

{
	char *txtptr;
	struct self_audit_rec audrec;

	txtptr = (char *)audrec.aud_body.text;
	strcpy (txtptr, "User= ");
	strcat (txtptr, name);
	strcat (txtptr, msg);
	audrec.aud_head.ah_pid = getpid();
	audrec.aud_head.ah_error = errnum;
	audrec.aud_head.ah_event = EN_CHFN;
	audrec.aud_head.ah_len = strlen(txtptr);
	/* Write the audit record */
	audwrite(&audrec);
	
	/* Resume auditing */
	audswitch(AUD_RESUME);
}
#endif AUDIT

/***************************** nyp routines *****************************
 * These ignore yellow pages.  They also ignore /.secure/etc/passwd
 */

#define	COLON	':'
#define	COMMA	','
#define	NEWLINE	'\n'
#define	PLUS	'+'
#define	MINUS	'-'
#define	ATSIGN	'@'
#define	EOS	'\0'

void setpwent_nyp(), endpwent_nyp();

static char PASSWD[]	= "/etc/passwd"; 
static char EMPTY[] = "";
static FILE *pwf = NULL;	/* pointer into /etc/passwd */
static char line[BUFSIZ+1];

static struct passwd *
getpwnam_nyp(name)
	register char *name;
{
	struct passwd *pw;

	setpwent_nyp();
	if (!pwf)
		return NULL;

	while ((pw = getpwent_nyp()) && strncmp(name, pw->pw_name, L_cuserid - 1))
		;
	endpwent_nyp();
	return (pw);
}

static struct passwd *
getpwuid_nyp(uid)
	register uid;
{
	struct passwd *pw;

	setpwent_nyp();
	if (!pwf)
		return NULL;
	while ((pw = getpwent_nyp()) && pw->pw_uid != uid)
		;
	endpwent_nyp();
	return(pw);
}



void
setpwent_nyp()
{
	if (pwf == NULL)
		pwf = fopen(PASSWD, "r");
	else
		rewind(pwf);
}



void
endpwent_nyp()
{
	if (pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
}

static char *
pwskip_nyp(p)
	register char *p;
{
	while(*p && *p != COLON && *p != NEWLINE)
		++p;
	if (*p == NEWLINE)
		*p = EOS;
	else if (*p != EOS)
		*p++ = EOS;
	return(p);
}

static struct passwd *
getpwent_nyp()
{
	register char *p;
	char *end;
	long	x, strtol();
	char *memchr();
	static struct passwd passwd;

	if (pwf == NULL) {
		if( (pwf = fopen( PASSWD, "r" )) == NULL )
			return(NULL);
	}

	/* fix defect of fgets returning 1024 char line in BUFSIZ-1=1023
	    and then 1-char parts, thereby creating a new passwd login of 
	    1 char name which gets created with uid=0(root) and with the 
	    password field blank.  Define a much bigger buffer 8*BUFSIZ so 
	    should never run into field limits. Ref. DTS report DSDe406839 */
	p = fgets(line, 8*BUFSIZ, pwf);
	if(p == NULL)
		return(NULL);
	passwd.pw_name = p;
	p = pwskip_nyp(p);
	passwd.pw_passwd = p;
	p = pwskip_nyp(p);
	x = strtol(p, &end, 10);	
	p = pwskip_nyp(p);
	passwd.pw_uid = (x < -2 || x > MAXUID)? (MAXUID+1): x;
	x = strtol(p, &end, 10);	
	p = pwskip_nyp(p);
	passwd.pw_gid = (x < -2 || x > MAXUID)? (MAXUID+1): x;
	passwd.pw_comment = EMPTY;
	passwd.pw_gecos = p;
	p = pwskip_nyp(p);
	passwd.pw_dir = p;
	p = pwskip_nyp(p);
	passwd.pw_shell = p;
	(void) pwskip_nyp(p);

	p = passwd.pw_passwd;
	while(*p != EOS && *p != COMMA)
		p++;
	if(*p == COMMA)
		*p++ = EOS;

	passwd.pw_age = p;

	return(&passwd);
} /*  getpwent_nyp  */
