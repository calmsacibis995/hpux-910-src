#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 66.12 $";
#endif

/**
***    ##    ##    ####    #######   ##    ##   ##   ##    ##  ########    ###
***    ##    ##   ##  ##   ##    ##  ###   ##   ##   ###   ##  ##    ##    ###
***    ##    ##  ##    ##  ##    ##  ####  ##   ##   ####  ##  ##          ###
***    ## ## ##  ########  #######   ## ## ##   ##   ## ## ##  ##  ####    ###
***    ########  ##    ##  ##  ##    ##  ####   ##   ##  ####  ##    ##     #
***    ###  ###  ##    ##  ##   ##   ##   ###   ##   ##   ###  ##    ##
***    ##    ##  ##    ##  ##    ##  ##    ##   ##   ##    ##  ########     #
***
***	The shar program is NOT an ordinary program.  It emits a shell
***	script that must work on ALL Unix systems produced anywhere and
***	at any time.  For example, you might mail shar output to your
***	friend back at school, who is not running an HP-UX system.  The
***	shell script must work on Bell System III.  It must work on
***	Berkeley 4.2.  It must work on Xenix, etc.
***
***	Therefore, when adding features to shar, think:
***
***		Will this feature work on all systems, both old & new?
***
***	If you don't know, then DON'T DO IT!
***
***	Examples of features that have been added to shar that didn't work:
***
***	- Use of "unset LANG" instead of "LANG=''".
***	  "unset" isn't supported by older shells.
***
***	- Use of "whoami" to detect super-user.
***	  No such command in Berkeley.
***
***	- Use of "set --" to set positional parameters.
***	  Doesn't exist in Berkeley shells.
***
***	- Explicit paths like "/usr/ucb/bin" hard-coded in.
***	  Not guaranteed to exist on all systems.
**/

/*
 * shar - shell archive
 *
 * Usage:
 *	shar [-options] <file>|<directory> ...
 *	Files are emitted directly, directories are recursed upon.
 *
 * Options:
 *	-a	 assume all files are mailable, don't encode them
 *	-b	 use basenames instead of full pathnames
 *	-c	 use wc(1) to check integrity of transfer
 *	-C	 include a "--- cut here ---" line
 *	-d	 don't recurse on contents of directories
 *	-D<dir>	 must be in <dir> to unpack
 *	-e	 don't overwrite existing files
 *      -f<file> file containing list of directories and files
 *      -h       follow symbolic links instead of archiving them
 *	-m	 retain modification/access times on files
 *	-o	 retain user/group owner information
 *	-r	 must be "root" to unpack
 *	-s	 use sum(1) to check integrity of transfer
 *	-t	 verbose messages to /dev/tty instead of stderr
 *	-u	 assume remote side has uudecode(1) for unpacking
 *	-v	 verbose mode
 *	-Z	 shrink files using compress(1)
 *
 * Author:
 *	Jack Applin at HP-FSD 3/19/85
 *	(taken from the shell script of the same name).
 *
 * Things to do:
 *	nicer table of contents (trailing slash for directories?)
 *	files containing \n ruin table of contents
 *	touch for -m option doesn't take time zone into account
 *	doesn't re-link linked files
 */

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>			/* for localtime(3) */
#include <unistd.h>
#include <sys/types.h>			/* types for sysmacros.h */
#ifdef hpux
#include <sys/utsname.h>		/* for uname(2) */
#include <sys/sysmacros.h>		/* define major & minor */
#include <dirent.h>			/* directory access routines */
#else
#include <sys/dir.h>
#endif
#include <sys/stat.h>			/* for stat(2) */
#include <pwd.h>			/* for getpwuid(3) */
#include <grp.h>			/* for getgrgid(3) */
#include <string.h>

#ifndef hpux
#   define strchr  index
#   define strrchr rindex
#endif /* not hpux */

extern void perror(), exit(), setgrent();
extern char *getenv(), *ctime();
extern void *malloc(), *realloc();
extern long time();
extern int stat(), lstat();
extern struct tm *localtime();
extern struct passwd *getpwuid(), *getpwnam();
extern struct group *getgrgid();

char *basename(), *quote(), *usr_name(), *grp_name();

#define max(a,b) ((a) > (b) ? (a) : (b))

typedef int boolean;
#define false 0
#define true 1

#define eq(a,b) (strcmp((a),(b))==0)
#define tab(x) (((x)+8) & ~7)
#define is_dir(buf)	((buf.st_mode & S_IFMT) == S_IFDIR)
#define is_char(buf)	((buf.st_mode & S_IFMT) == S_IFCHR)
#define is_block(buf)	((buf.st_mode & S_IFMT) == S_IFBLK)
#define is_file(buf)	((buf.st_mode & S_IFMT) == S_IFREG)
#ifdef S_IFSOCK
#define is_sock(buf)	((buf.st_mode & S_IFMT) == S_IFSOCK)
#endif
#ifdef S_IFIFO
#define is_pipe(buf)	((buf.st_mode & S_IFMT) == S_IFIFO)
#endif
#ifdef RFA
#define is_nwk(buf)	((buf.st_mode & S_IFMT) == S_IFNWK)
#endif
#ifdef SYMLINKS		/* do we have symbolic links? */
#define is_lnk(buf)	((buf.st_mode & S_IFMT) == S_IFLNK)
#define Stat (*(follow_sym ? stat : lstat))	/* use version of stat    */
						/* that understands links */
#else			/* no symbolic links */
#define Stat stat	/* use plain old stat */
#endif

boolean unpacker_exists=false;		/* have we emitted unpacker yet? */

boolean assume_mailable=false;		/* Use sed mode at most, don't encode */
boolean base_option=false;		/* use basenames, not full paths */
boolean check_option=false;		/* check for errors with wc(1) */
boolean cut_here=false;			/* no "cut here" line by default */
boolean sum_option=false;		/* check for errors with sum(1) */
boolean verbose=false;			/* verbose operation */
boolean directory_only=false;		/* don't recurse on directories */
boolean retain_times=false;		/* retain modification/access times */
boolean retain_perm=true;		/* retain file permissions */
boolean must_be_root=false;		/* only "root" can unpack */
boolean retain_owner=false;		/* retain user/group information */
boolean assume_uudecode=false;		/* assume remote site has uudecode */
boolean overwrite_check=false;		/* check if file already exists */
boolean compress_files=false;		/* use compress/uncompress */
boolean follow_sym=false;               /* don't follow sym links by default */
#ifdef ACLS
boolean aclflag=false;			/* print warning messages */
#endif

char *required_dir=NULL;		/* must be unpacking here */
char *filedir_list=NULL;		/* file containing dirs & files list */

char *pname;				/* our argv[0] (e.g., shar) */

main(argc, argv)
int argc;
char *argv[];
{
	extern int optind;
	extern char *optarg;

	char *p;
	int c, i, n, maxlen, len;
	long now;
	struct stat stat_buf;
	struct passwd *pw;

	pname = argv[0];		/* get name  for error messages */

	while ((c=getopt(argc, argv, "aAbcCdD:ef:hmorstuvZ"))!=EOF) {
		switch (c) {
		case 'a': assume_mailable=true;			break;
#ifdef ACLS
		case 'A': aclflag=true;				break;
#endif
		case 'b': base_option=true;			break;
		case 'c': check_option=true;			break;
		case 'C': cut_here=true;			break;
		case 'd': directory_only=true;			break;
		case 'D': required_dir=optarg;			break;
		case 'e': overwrite_check=true;			break;
		case 'f': if (filedir_list) {
				fprintf(stderr,
				   "%s: only one -f allowed per invocation\n",
				   pname);
				exit(1);
			}
			else {
				filedir_list=optarg;
			}
								break;
                case 'h': follow_sym=true;                      break;
		case 'm': retain_times=true;			break;
		case 'o': retain_owner=true;			break;
		case 'r': must_be_root=true;			break;
		case 's': sum_option=true;			break;
		case 't': verbose=true;
			freopen("/dev/tty", "w", stderr);	break;
		case 'u': assume_uudecode=true;			break;
		case 'v': verbose=true;				break;
		case 'Z': compress_files=true;			break;
		default: usage();				break;
		}
	}

	if (required_dir && *required_dir != '/')
		fprintf(stderr, "Warning: \"%s\" is not an absolute path\n",
			required_dir);

	argc -= optind-1;		/* skip arguments */
	argv += optind-1;		/* skip arguments */


	/* if file of filenames given then add filenames */
	/* and directories from file */
	if (filedir_list) adjust_args(&argc, &argv);

	if (argc<=1) {			/* no arguments? complain/give usage */
		fprintf(stderr, "No files or directories were given\n\n");
		usage();
	}


	/* Check all files for validity */
	for (i=1; i<argc; i++) {

		if (Stat(argv[i], &stat_buf) == -1) {
			fprintf(stderr, "%s: %s does not exist\n",
				pname, argv[i]);
			exit(1);
		}

		/* Directories with basenames is kinda funny. */
		if (is_dir(stat_buf) && base_option && !directory_only) {
			fprintf(stderr,
			   "%s: can't recursively archive %s with -b option\n",
				pname, argv[i]);
			exit(3);
		}
	}


	/*
	 * Emit the prologue and list of ingredients:
	 * (The leading newline is for those just execute it with csh).
	 */
	puts("");
	if (cut_here)
		puts("#---------------------------------- cut here ----------------------------------");
	puts("# This is a shell archive.  Remove anything before this line,");
	puts("# then unpack it by saving it in a file and typing \"sh file\".");
	printf("#\n# Wrapped");
	p = getenv("LOGNAME");				/* Try for $LOGNAME */
	pw = p!=NULL					/* If $LOGNAME set */
		? getpwnam(p)				/* use it */
		: getpwuid(getuid());			/* else use uid */

	if (pw!=NULL) {					/* In /etc/passwd? */
		printf(" by ");
		p=pw->pw_gecos;
		if (*p == '*') p++;			/* don't know why */
		if (strncmp(p, "pri=", 4)==0) {		/* login priority? */
			p+=4;				/* skip pri= */
			while (*p=='-' || isdigit(*p))	/* skip digits */
				p++;
		}
		while (*p==' ') p++;			/* leading blanks */

		for (; *p && *p != ',' && *p != ';' && *p != '%'; p++) {
			if (*p == '&') {		/* login name? */
				putchar(toupper(pw->pw_name[0]));
				printf("%s", &pw->pw_name[1]);
			}
			else
				putchar(*p);
		}
		printf(" <%s", pw->pw_name);
#ifdef hpux
		{
			struct utsname uinf;
			uname(&uinf);
			printf("@%s", uinf.nodename);
		}
		putchar('>');
#endif
	}

	time(&now);
	printf(" on %s", ctime(&now));

	printf("#\n# This archive contains:");

	/* figure out longest name */
	for (maxlen=0, i=1; i<argc; i++) {
		p = base_option ? basename(argv[i]) : argv[i];
		maxlen = max(maxlen, strlen(p));
	}

	maxlen = tab(maxlen);			/* add a tab for spacing */

	/* emit arguments in nice columns */
	for (len=999, i=1; i<argc; i++, len+=maxlen) {
		if (len+maxlen>=80) {
			printf("\n#\t");
			len=8;
		}
		p = base_option ? basename(argv[i]) : argv[i];
		printf("%s", p);
		if (len+maxlen<80) {		/* if not last one */
			for (n=strlen(p); n<maxlen; n=tab(n))
				putchar('\t');
		}
	}
	puts("\n#");
	if (required_dir)
		printf("# Archive must be unpacked in directory \"%s\".\n",
			required_dir);
	if (must_be_root)
		puts("# Only user \"root\" may unpack this archive.");
	if (overwrite_check)
		puts("# Existing files will not be overwritten.");
	if (retain_times)
		puts("# Modification/access file times will be preserved.");
	if (retain_owner)
		puts("# User/group owner information will be preserved.");
	if (check_option)
		puts("# Error checking via wc(1) will be performed.");
	if (sum_option)
		puts("# Error checking via sum(1) will be performed.");
	if (compress_files)
		puts("# Files are compressed using compress(1).");

	puts("");

	/* Get non-localized form of output */
	puts("LANG=\"\"; export LANG");
	/* Use standard utilities (e.g., rm, sum) before non-standard ones. */
	puts("PATH=/bin:/usr/bin:$PATH; export PATH");
	puts("");

	if (must_be_root) {
		puts("if test \"$LOGNAME\" != root");
		puts("then");
		puts("	echo 'Must be \"root\" to unpack this archive'");
		puts("	exit 1");
		puts("fi\n");
	}

	/* Advise user to be in given safe directory */
	if (required_dir!=NULL) {
		printf("if test \"`pwd`\" != %s\n", quote(required_dir));
		puts("then");
		printf("	echo You really should be in directory %s\n",
			quote(required_dir));
		puts("	echo 'Do you wish to continue?'; read answer");
		puts("	if test \"$answer\" != 'yes' -a \"$answer\" != 'y'");
		puts("	then");
		puts("		exit 0");
		puts("	fi");
		puts("fi\n");
	}

	if (sum_option) {
		puts("if sum -r </dev/null >/dev/null 2>&1");
		puts("then");
		puts("	sumopt='-r'");
		puts("else");
		puts("	sumopt=''");
		puts("fi\n");
	}

	/* Emit the files. */
	for (i=1; i<argc; i++)
		process_file(argv[i]);

	if (unpacker_exists)
		puts("rm -f /tmp/unpack$$");	/* might not've been compiled */

	/* Finish up */
	puts("exit 0");		/* sharchives unpack even if junk follows. */
	return 0;
}


usage()
{
#ifdef ACLS
	fprintf(stderr, "usage: %s [-AabCcDdefhmorstuvZ] <file|dir> ...",pname);
#else
	fprintf(stderr, "usage: %s [-abCcDdefhmorstuvZ] <file|dir> ...",pname);
#endif

#ifdef ACLS
	fprintf(stderr, "\
\n-A:		supress warning messages for optional acl entries");
#endif

	fprintf(stderr, "\
\n-a:		assume files are shippable, don't uuencode\
\n-b:		use basenames instead of full pathnames\
\n-c:		use wc(1) to check integrity of transfer\
\n-C:		include a \"cut here\" line\
\n-d:		don't recurse on contents of directories\
\n-D <dir>:	must be in <dir> to unpack\
\n-e:		don't overwrite existing files\
\n-f <file>:	file containing list of directories and files\
\n		or - to read filenames from standard input\
\n-h		follow symbolic links instead of archiving them\
\n-m:		retain modification/access times on files\
\n-o:		retain user/group owner information\
\n-r:		must be \"root\" to unpack\
\n-s:		use sum(1) to check integrity of transfer\
\n-t:		verbose messages to /dev/tty instead of stderr\
\n-u:		assume remote site has uudecode(1) for unpacking\
\n-v:		verbose mode\
\n-Z:		shrink files using compress(1)\n");

	exit(4);
}



adjust_args(argcptr, argvptr)
int *argcptr;
char ***argvptr;
{
	FILE	*f;
	int	newargc;
	char	**newargv;
	char	filename[256];
	int	i, argv_size;


	/* open the file containing the file and directory list */
	if (eq(filedir_list, "-"))
		f = stdin;
	else
		f = fopen(filedir_list, "r");
	if (f==NULL) {
		perror(filedir_list);
		fprintf(stderr, "%s: can't read %s\n", pname, filedir_list);
		exit(1);
	}


	/* create an array of 1000 strings for file and directory names */
	argv_size = 1000;
	newargv = (char **)malloc(argv_size * sizeof(char *));
	newargc = 1;

	/* pick up file names letting scan ignore blanks */
	while (fscanf(f, "%s", filename) != EOF) {
		if (newargc>=argv_size) {
			argv_size += 1000;
			newargv = (char **) realloc((void *)newargv, argv_size);
		}
		newargv[newargc++] = strdup(filename);
	}


	/* now load in any left over file or directory names */
	for (i=1; i<*argcptr; i++) {
		if (newargc>=argv_size) {
			argv_size += 1000;
			newargv = (char **) realloc((void *)newargv, argv_size);
		}
		newargv[newargc++] = (*argvptr)[i];
	}

	/* fake out the calling routine's argv and argc */
	*argvptr = newargv;
	*argcptr = newargc;
}


/*
 * Process a file.
 *
 * For ordinary files, emit their contents.
 * For a directory, mkdir it and recurse on the contents (unless -d).
 * In all cases, emit a chmod to get the access right.
 */
process_file(fname)
char *fname;
{
	int c;
	char *unpack_name, quote_name[256];
	struct stat stat_buf;
	FILE *f;

	if (fname[0]=='.' && fname[1]=='/' && fname[2]!='\0')	/* ./file ? */
		fname+=2;					/* skip ./ */

	if (fname[0]=='/' && fname[1]=='/')			/* //file ? */
		fname++;					/* skip / */

	if (base_option) {
		unpack_name=basename(fname);
		if (verbose)
			fprintf(stderr, "a - %s [from %s]\n",
				unpack_name, fname);
	}
	else {
		unpack_name=fname;
		if (verbose)
			fprintf(stderr, "a - %s\n", fname);
	}

	strcpy(quote_name, quote(unpack_name));

	if (Stat(fname, &stat_buf) == -1) {
		perror(fname);
		fprintf(stderr, "%s: can't access file %s\n", pname, fname);
		exit(5);
	}

#ifdef ACLS
	if(!aclflag) {
	    if(stat_buf.st_acl)
	        fprintf(stderr, "optional acl entries for %s not archived\n",base_option?basename(fname):fname);
	}
#endif

	/* For directories, emit a mkdir and recurse */
	if (is_dir(stat_buf)) {
		DIR *dirp;
		struct dirent *dp;

		if (!eq(fname, ".") && !eq(fname, "..")) { /* already exist */
			printf("echo mkdir - %s\n", quote_name);
			printf("mkdir %s\n\n", quote_name);
		}

		/* Recurse if we're allowed to. */
		if (!directory_only) {
			if ((dirp=opendir(fname))==NULL) {
				perror(fname);
				exit(6);
			}

			/* Read each file and recurse on it. */
			while ((dp=readdir(dirp))!=NULL) {
				char buf[1024];
				if (eq(dp->d_name, ".") || eq(dp->d_name, ".."))
					continue;
				sprintf(buf, "%s/%s", fname, dp->d_name);
				process_file(buf);
			}
			closedir(dirp);
		}
	}

#ifndef MINOR_FORMAT
#define MINOR_FORMAT "%d"
#endif

	else if (is_char(stat_buf)) {
		printf("echo mknod - %s\n", quote_name);
		printf("mknod %s c %d ", quote_name, major(stat_buf.st_rdev));
		/* print minor number in sys-dependent fashion from mknod.h */
		printf(MINOR_FORMAT, minor(stat_buf.st_rdev));
		printf("\n");
	}

	else if (is_block(stat_buf)) {
		printf("echo mknod - %s\n", quote_name);
		printf("mknod %s b %d ", quote_name, major(stat_buf.st_rdev));
		/* print minor number in sys-dependent fashion from mknod.h */
		printf(MINOR_FORMAT, minor(stat_buf.st_rdev));
		printf("\n");
	}
#ifdef is_sock
	else if (is_sock(stat_buf)) {
		fprintf(stderr, "%s: %s: Operation not supported on socket\n",
		       pname, fname);
		exit(7);
	}
#endif
#ifdef is_pipe
	else if (is_pipe(stat_buf)) {
		printf("echo mknod - %s\n", quote_name);
		printf("mknod %s p\n", quote_name);
	}
#endif

#ifdef RFA
	else if (is_nwk(stat_buf)) {
		char buf[2048];
		int fd, len;

		fd = open(fname, O_RDONLY);
		if (fd == -1) {
			perror(fname);
			fprintf(stderr, "%s: can't read network file %s\n",
				pname, fname);
			exit(7);
		}
		len = read(fd, buf, sizeof(buf));
		if (len == -1) {
			perror(fname);
			fprintf(stderr, "%s: read of file failed %s\n",
				pname, fname);
			exit(7);
		}
		close(fd);
		buf[len] = '\0';
		printf("echo mknod - %s\n", quote_name);
		printf("mknod %s n %s\n", quote_name, quote(buf));
	}
#endif /* RFA */

#ifdef SYMLINKS
	else if (is_lnk(stat_buf)) {
		char real_name[2048];
		int bufsize;

		bufsize = readlink(fname, real_name, sizeof(real_name));
		real_name[bufsize] = NULL;
		printf("echo linking - %s\n", quote_name);
		printf("ln -s %s %s\n", quote(real_name), quote_name);
	}
#endif /* SYMLINKS */

	/* For normal files, use cat/sed/unpacker to extract the file. */
	else if (is_file(stat_buf)) {
		if (overwrite_check) {
			printf("if test -f %s\n", quote_name);
			puts("then");
			printf("\techo Ok to overwrite existing file %s\\?\n",
				quote_name);
			puts("\tread answer");
			puts("\tcase \"$answer\" in");
			puts("\t[yY]*)	echo Proceeding;;");
			puts("\t*)	echo Aborting; exit 1;;");
			puts("\tesac");
			printf("\trm -f %s\n", quote_name);
			printf("\tif test -f %s\n", quote_name);
			puts("\tthen");
			printf("\t\techo Error: could not remove %s, aborting\n",
				quote_name);
			puts("\t\texit 1");
			puts("\tfi");
			puts("fi");
		}

		f = fopen(fname, "r");
		if (f==NULL) {
			perror(fname);
			fprintf(stderr, "Can't read %s\n", fname);
			exit(1);
		}

		if (linkit(quote_name, &stat_buf))
			/* We linked it inside linkit */;

		else if (stat_buf.st_size==0)	/* empty file */
			printf("echo x - %s\n>%s\n",
				quote_name, quote_name);

		else if (compress_files) {
			FILE *cf;
			char buf[5120];

			sprintf(buf, "compress <%s", quote(fname));
			cf = popen(buf, "r");
			if (cf == NULL) {
				perror("can't execute compress");
				exit(1);
			}
			emit_unpacker(); /* Emit the c program that unpacks */
			printf("echo x - %s '[compressed]'\n", quote_name);
			emit_uuencode(cf, unpack_name, 0600);
			pclose(cf);
			printf("uncompress <%s >/tmp/compress$$\n", quote_name);
			printf("mv /tmp/compress$$ %s\n", quote_name);
		}
		else if (assume_mailable || type_cat(f)) {
			rewind(f);
			printf("echo x - %s\n", quote_name);
			printf("cat >%s <<'@EOF'\n", quote_name);
			while ((c=getc(f)) != EOF)
				putchar(c);
			puts("@EOF");
		}
		else if (type_sed(f))
			emit_sed(f, unpack_name);
		else {
			emit_unpacker(); /* Emit the c program that unpacks */
			printf("echo x - %s '[non-ascii]'\n", quote_name);
			emit_uuencode(f, unpack_name, stat_buf.st_mode);
		}
		fclose(f);


		/* Check for transmission errors. */
		if (check_option || sum_option) {
			unsigned long sum, lines, words, chars;

			stats(fname, &sum, &lines, &words, &chars);

			if (sum_option) {
			    printf("set `sum $sumopt <%s`; ", quote_name);
			    printf("if test $1 -ne %u\n", sum);
			    puts("then");
			    printf("\techo ERROR: %s checksum is $1 should be %u\n",
				quote_name, sum);
			    printf("fi\n");
			}
			if (check_option) {
			    /*
			     * Use this set nonsense to be independent
			     * of the column output of wc.
			     */
			    printf("set `wc -lwc <%s`\n", quote_name);
			    printf("if test $1$2$3 != %d%d%d\n",
				lines, words, chars);
			    puts("then");
			    printf("\techo ERROR: wc results of %s are $* should be %d %d %d\n",
				quote_name, lines, words, chars);
			    printf("fi\n");
			}
		}
		puts("");
	}

	/* It's a truly bizarre file. */
	else {
		fprintf(stderr, "%s: can't archive file %s with mode %o\n",
			pname, fname, stat_buf.st_mode);
		exit(8);
	}

	/*
	 * Must do this stuff *after* we process the files, becuase recursing
	 * on a directory will change its access/mod times.  Also, we might make
	 * the directory non-writable here.
	 */
	if (!eq(fname, ".") && !eq(fname, "..")
#ifdef SYMLINKS
	   && !is_lnk(stat_buf)		/* owner/group/perm have no meaning */
#endif /* SYMLINKS */
	) {
		if (retain_times) {
			struct tm *t;

			t=localtime(&stat_buf.st_mtime);
			printf("touch -m %.2d%.2d%.2d%.2d%.2d %s\n",
				t->tm_mon+1, t->tm_mday, t->tm_hour,
				t->tm_min, t->tm_year, quote_name);
			t=localtime(&stat_buf.st_atime);
			printf("touch -a %.2d%.2d%.2d%.2d%.2d %s\n",
				t->tm_mon+1, t->tm_mday, t->tm_hour,
				t->tm_min, t->tm_year, quote_name);
		}
		if (retain_perm)
			printf("chmod %.3o %s\n",
				stat_buf.st_mode & 07777, quote_name);
		if (retain_owner) {
			printf("chgrp %s %s\n",
				quote(grp_name((int) stat_buf.st_gid)),
				quote_name);
			/* change owner last of all, might give away file */
			printf("chown %s %s\n",
				quote(usr_name((int) stat_buf.st_uid)),
				quote_name);
		}
	}
	puts("");			/* space between files */
}


/*
 * If this file is linked to a file that we've already emitted,
 * then emit a ln statement between the two files.
 * If we emitted a ln statement, return 1, else 0.
 */
int
linkit(fname, sb)
char *fname;
register struct stat *sb;
{
	struct links {
		char *fname;
		dev_t dev;
		ino_t ino;
	} *p;
	static struct links *links=NULL;
	static int links_size=0;
#define links_chunk 32

	if (sb->st_nlink == 1)	/* is this file linked? */
		return 0;		/* forget it */

	/* Is it in the list? */
	for (p=links; p<links+links_size; p++) {
		if (p->ino==sb->st_ino && p->dev==sb->st_dev) {
			printf("echo x - %s \\[linked to %s\\]\n", fname, p->fname);
			printf("ln %s %s\n", p->fname, fname);
			return 1;
		}
	}

	/* Add it to the list */
	if (links_size % links_chunk == 0)
		links = (struct links *)
				realloc((char *) links, sizeof(*links)*(links_size+links_chunk));

	p = &links[links_size++];
	p->fname = strdup(fname);
	p->dev = sb->st_dev;
	p->ino = sb->st_ino;
	return 0;
}



/*
 * Is this character transmittable by mailers?
 */
#define ok_to_send(c) \
	((c)>=32 && (c)<0177 || (c)=='\t' \
	|| (c)=='\b' || (c)=='\n' || (c)=='\f')

type_cat(f)
FILE *f;
{
	int c, nextc, linelen;

	linelen = 0;

	rewind(f);
	while ((c=getc(f))!=EOF) {
		linelen++;

		if (!ok_to_send(c) || linelen>=256)
			return false;

		if (linelen==1) {
			nextc = getc(f); ungetc(nextc, f);
			if (c=='@'			/* our quote char */
			||  c=='~'			/* mailx doesn't like */
			||  c=='.'			/*sendmail dislikes */
			||  c=='F' && nextc=='r')	/* Could be "From" */
				return false;
		}

		if (c=='\n')
			linelen=0;
	}

	if (linelen!=0)			/* Must end with a newline */
		return false;

	return true;
}



type_sed(f)
FILE *f;
{
	int c, linelen;

	linelen = 0;

	rewind(f);
	while ((c=getc(f))!=EOF) {
		linelen++;

		if (!ok_to_send(c) || linelen>=256)
			return false;

		if (c=='\n')
			linelen=0;
	}

	return (linelen==0);		/* Must end with a newline */
}



/*
 * Use sed to process the file.
 * Quote with "@" funny characters
 * and lines (such as "From") that may offend some mailers.
 */
emit_sed(f, where_to)
FILE *f;
char *where_to;
{
	char buf[1024];

	rewind(f);

	printf("echo x - %s\n", where_to);
	printf("sed 's/^@//' >%s <<'@EOF'\n", where_to);

	while (fgets(buf, sizeof(buf), f)) {
		if (buf[0]=='@'			/* our quote character */
		||  buf[0]=='~'			/* Berkeley mailer cmd escape */
		||  buf[0]=='.'			/* sendmail doesn't like */
		||  strncmp(buf, "From", 4)==0)	/* gets converted to >From */
			putchar('@');		/* Insulate this horrid line */
		fputs(buf, stdout);
	}
	puts("@EOF");
}


/*
 * Emit the file in uuencode(1) format which will be unpacked
 * at the remote end.  Emit the unpacker itself if needed.
 */
emit_uuencode(f, where_to, st_mode)
FILE *f;
char *where_to;
{
	char buf[80];
	register char *p;
	register int count, n;

	rewind(f);
	printf("%s <<'@eof'\n", assume_uudecode ? "uudecode" : "$unpacker");

#define ENC(c) (((c) & 077) + ' ')

	printf("begin %o %s\n", st_mode & 0777, where_to);

	do {
		n = fread(buf, sizeof(char), 45, f);	/* up to 45 chars */
		putchar(ENC(n));			/* length of line */

		count=0;
		for (p=buf; p<&buf[n]; p+=3) {
			putchar(ENC(p[0] >> 2));
			putchar(ENC((p[0] << 4) & 060 | (p[1] >> 4) & 017));
			putchar(ENC((p[1] << 2) & 074 | (p[2] >> 6) & 03));
			putchar(ENC(p[2] & 077));
			count+=4;
		}

		/* put X at end to protect from blank stripping */
		printf("%*sX\n", 60-count, "");		/* pad to 60 chars */
	} while (n>0);

	puts("end\n@eof"); /* @EOF could appear in the data, @eof can't */
}


/*
 * Return the last component of a pathname.
 */
char *
basename(fname)
char *fname;
{
	char *p;

	p = strrchr(fname, '/');		/* look for a slash */
	return (p==NULL ? fname : p+1);		/* whole name or after slash */
}


/*
 * Emit uudecode for unpacking on the remote system.
 * It's a shame not to indent the unpacker code.  Problem is, the elm mailer
 * expands tabs into strings of blanks.  If it does this, the archive
 * won't even unpack, because the EOF won't be found.  Also, the #include's
 * won't work, because the # must be in the first column.
 *
 * This doesn't solve the problem of expanding tabs in the user's files,
 * but one thing at a time.
 */
emit_unpacker()
{
	if (assume_uudecode || unpacker_exists)
		return;
	unpacker_exists=true;
	puts("\
\nrm -f /tmp/uud$$\
\n(echo \"begin 666 /tmp/uud$$\\n#;VL*n#6%@x\\n \\nend\" | uudecode) >/dev/null 2>&1\
\nif [ X\"`cat /tmp/uud$$ 2>&1`\" = Xok ]\
\nthen\
\n	unpacker=uudecode\
\nelse\
\n	echo Compiling unpacker for non-ascii files\
\n	pwd=`pwd`; cd /tmp\
\n	cat >unpack$$.c <<'EOF'\
\n#include <stdio.h>\
\n#define C (*p++ - ' ' & 077)\
\nmain()\
\n{\
\n	int n;\
\n	char buf[128], *p, a,b;\
\n\
\n	scanf(\"begin %o \", &n);\
\n	gets(buf);\
\n\
\n	if (freopen(buf, \"w\", stdout) == NULL) {\
\n		perror(buf);\
\n		exit(1);\
\n	}\
\n\
\n	while (gets(p=buf) && (n=C)) {\
\n		while (n>0) {\
\n			a = C;\
\n			if (n-- > 0) putchar(a << 2 | (b=C) >> 4);\
\n			if (n-- > 0) putchar(b << 4 | (a=C) >> 2);\
\n			if (n-- > 0) putchar(a << 6 | C);\
\n		}\
\n	}\
\n	exit(0);\
\n}\
\nEOF\
\n	cc -o unpack$$ unpack$$.c\
\n	rm unpack$$.c\
\n	cd $pwd\
\n	unpacker=/tmp/unpack$$\
\nfi\
\nrm -f /tmp/uud$$\
\n");
}

#define SAFE(c) (isalnum(c)\
	|| (c)=='-' || (c)=='_' || (c)=='=' || (c)=='+'\
	|| (c)==',' || (c)=='.' || (c)=='/')

/*
 * Return a pointer to a quoted version of s.
 * If s needs no quoting, just return s itself.
 */
char *
quote(s)
char *s;
{
	char *p, c;
	static char buf[1024];

	for (p=s; c = *p; p++) {
		if (!SAFE(c))
			goto bad;
	}
	return s;			/* no quoting needed */

bad:
	p=buf;
	for (p=buf; c = *s; s++) {
		if (SAFE(c))		/* A regular character? */
			*p++ = c;	/* just put it in */

		else if (c=='\n') {	/* A newline? */
			*p++ = '\'';	/* It must have single quotes */
			*p++ = '\n';	/* the newline itself */
			*p++ = '\'';
		}
		else {
			*p++ = '\\';	/* quote the next character */
			*p++ = c;	/* the funny character (e.g., *) */
		}
	}
	*p++ = '\0';			/* cork off quoted string */
	return buf;
}

/*
 * Compute a checksum of the file.
 */
stats(fname, sum, lines, words, chars)
char fname[];
register unsigned long *sum, *lines, *words, *chars;
{
	FILE *f;
	register int c, checksum;
	register boolean in_a_word;

	f = fopen(fname, "r");
	if (f==NULL) {
		perror(fname);
		exit(13);
	}

	in_a_word=false;
	checksum = *lines = *words = *chars = 0;
	while ((c = getc(f)) != EOF) {
		/* rotate the sum right with end-around carry */
		checksum = (((checksum >> 1) | (checksum << 15)) + c) & 0xffff;

		++*chars;
		if (c=='\n')
			++*lines;

		if (c==' ' || c=='\t' || c=='\n')	/* separator? */
			in_a_word = false;		/* not in a word */

		else if (c>' ' && c<0177) {		/* in a word */
			if (!in_a_word)			/* if a transition */
				++*words;		/* one more word */
			in_a_word = true;		/* in a word now */
		}
	}

	fclose(f);
	*sum = checksum;
}



char *
usr_name(id)
{
	struct passwd *p;
	static char buf[20];

	setpwent();				/* rewind password file */
	p = getpwuid(id);			/* try to find this user */
	if (p!=NULL)				/* if we found it */
		return p->pw_name;		/* return the user name */
	sprintf(buf, "%d", id);			/* just copy out numeric id */
	return buf;				/* and return that */
}


char *
grp_name(id)
{
	struct group *p;
	static char buf[20];

	setgrent();				/* rewind group file */
	p = getgrgid(id);			/* try to find this group */
	if (p!=NULL)				/* if we found it */
		return p->gr_name;		/* return the group name */
	sprintf(buf, "%d", id);			/* just copy out numeric id */
	return buf;				/* and return that */
}
