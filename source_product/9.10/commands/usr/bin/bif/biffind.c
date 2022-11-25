static char *HPUX_ID = "@(#) $Revision: 27.1 $";
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dir.h"

typedef int boolean;
#define false (0)
#define true (1)
#define eq(a,b) (strcmp((a),(b))==0)

char *strcpy(), *strncpy(), *strcat(), *strchr(), *strrchr();
char *bfstail();

extern char * normalize();

char *pname;
boolean debug = false;

main(argc, argv)
char **argv;
{
	char **arg_ptr;

	pname = *argv++;

	if (*argv!=NULL && strcmp(*argv, "-x")==0)
		debug=true, argv++, argc--;

	if (argc<=2)
		usage();

	for (arg_ptr=argv; *arg_ptr && **arg_ptr!='-'; arg_ptr++)
		;

	for (; *argv && **argv!='-'; argv++)
		process(*argv, arg_ptr);

	exit(0);
}


process(path, expr)
char *path;
char **expr;
{
	int dp;
	struct stat statbuf;
	char buf[256], new_name[15];
	struct direct direct;

	path = normalize(path);
	dp = bfsopen(path, 0);				/* open file/dir */
	if (dp == -1) {
		fprintf(stderr, "%s: can't open %s\n", pname, path);
		return;
	}


	process1(path, dp, expr);		/* process both files & dirs */

	bfsfstat(dp, &statbuf);

	if ((statbuf.st_mode & 0170000) != 0040000) {
		bfsclose(dp);
		return;
	}

	while (bfsread(dp, (char *) &direct, sizeof(direct))==sizeof(direct)) {
		if (direct.d_ino==0)		/* deleted entry */
			continue;

		if (eq(direct.d_name,".") || eq(direct.d_name,".."))
			continue;

		strncpy(new_name, direct.d_name, 14);
		new_name[14] = '\0';		/* seal off long name */
		strcpy(buf, path);		/* directory name */
		if (buf[strlen(buf)-1]!='/')	/* if no seperator: */
			strcat(buf, "/");	/* add / seperator */
		strcat(buf, new_name);		/* complete the name */
		process(buf, expr);
	}
	bfsclose(dp);
}

/*
 * -name name
 * -perm onum
 * -type b|c|d|p|f
 * -links n
 * -exec <command> ;
 * -ok <command> ;
 * -user <username>|<uid>
 * -group <groupname>|<gid>
 * -size n
 * -print
 * -inum n
 */

process1(path, dp, expr)
char *path;
int dp;
char **expr;
{
	struct stat statbuf;

	bfsfstat(dp, &statbuf);

	while (*expr) {
		char *op;

		op = *expr++;
		if (eq(op, "-name")) {
		/*	if (!eq(bfstail(path), *expr++)) */
			if (!gmatch(path, *expr++))
				return;
		}
		else if (eq(op, "-perm")) {
			char *s = *expr++;		/* get target string */
			int perm;

			if (*s!='-') {
				sscanf(s, "%o",  &perm);
				if ((statbuf.st_mode & 0777) != perm)
					return;
			} else {
				sscanf(s+1, "%o",  &perm);
				if (((statbuf.st_mode & 017777) & perm) != perm)
					return;
			}
		}
		else if (eq(op, "-type")) {
			char mode;

			switch(statbuf.st_mode & 0170000) {
			case 0010000: mode = 'p'; break;
			case 0020000: mode = 'c'; break;
			case 0040000: mode = 'd'; break;
			case 0060000: mode = 'b'; break;
			case 0000000:
			case 0100000: mode = 'f'; break;
			default:      mode = '?'; break;
			}

			if (*expr==NULL || *(*expr++)!=mode)
				return;
		}
		else if (eq(op, "-links")) {
			if (statbuf.st_nlink != atoi(*expr++))
				return;
		}
		else if (eq(op, "-print")) {
			printf("%s\n", path);
		}
		else if (eq(op, "-inum")) {
			if (statbuf.st_ino != atoi(*expr++))
				return;
		}
		else if (eq(op, "-exec") || eq(op, "-ok")) {
			char *args[50], **p;

			for (p=args; *expr && !eq(*expr, ";"); expr++) {
				if (eq(*expr, "{}"))
					*p++ = path;
				else
					*p++ = *expr;
			}
			if (*expr==NULL)
				usage();
			expr++;				/* skip the ; */
			*p++ = NULL;			/* seal off arg list */

			if (eq(op, "-ok")) {
				char answer[3];
				printf("< %s ... %s >?", *args, path);
				fflush(stdout);
				answer[0]='n';
				fgets(answer, sizeof(answer), stdin);
				if (answer[0]!='y')
					return;
			}

			if (execute(args)!=0)
				return;

		}
		else if (eq(op, "-user")) {
			if (statbuf.st_uid != tran_uid(*expr++))
				return;
		}
		else if (eq(op, "-group")) {
			if (statbuf.st_gid != tran_gid(*expr++))
				return;
		}
		else if (eq(op, "-size")) {
			if (statbuf.st_size != atoi(*expr++))
				return;
		}
		else
			usage();
	}
}

execute(args)
char **args;
{
	int ret, pid;

	pid = fork();

	switch (pid) {
	case -1:						/* error */
		fprintf(stderr, "%s: can't fork\n", pname);
		return -1;
	case 0:							/* child */
		execvp(args[0], args);
		fprintf(stderr, "%s: can't exec %s\n", pname, args[0]);
		return -1;
	default:						/* parent */
		while (wait(&ret)!=pid);	/* wait for kid */
		return (ret>>8);
	}
}


#include <pwd.h>
tran_uid(s)
char *s;
{
	struct passwd *pw,*getpwnam();

	if (isnumber(s))
		return atoi(s);

	if((pw=getpwnam(s)) == NULL) {
		fprintf(stderr,"%s: unknown user: %s\n", pname, s);
		exit(4);
	}
	return pw->pw_uid;
}


#include <grp.h>
tran_gid(s)
char *s;
{
	struct group *gr,*getgrnam();

	if (isnumber(s))
		return atoi(s);

	if((gr=getgrnam(s)) == NULL) {
		fprintf(stderr,"%s: unknown group: %s\n", pname, s);
		exit(4);
	}
	return gr->gr_gid;
}


#include <ctype.h>
isnumber(s)
register char *s;
{
	for (; *s; s++)
		if (!isdigit(*s))
			return false;
	return true;
}



usage()
{
	fprintf(stderr, "usage: %s directories expression\n",
		pname);
	fprintf(stderr, "directories: device:path\n");
	fprintf(stderr, "expression:\n");
	fprintf(stderr, "	-name <pattern>\n");
	fprintf(stderr, "	-perm onum\n");
	fprintf(stderr, "	-type b|c|d|p|f\n");
	fprintf(stderr, "	-links n\n");
	fprintf(stderr, "	-user <username>|<uid>\n");
	fprintf(stderr, "	-group <groupname>|<gid>\n");
	fprintf(stderr, "	-size n\n");
	fprintf(stderr, "	-exec <command> ;\n");
	fprintf(stderr, "	-ok <command> ;\n");
	fprintf(stderr, "	-print\n");
	fprintf(stderr, "	-inum n\n");
	exit(1);
}

gmatch(s, p)
register char	*s, *p;
{
	register int	scc;
	char		c;

	if(scc = *s++) {
		if((scc &= 0177) == 0) {
			scc = 0200;
		}
	}
	switch(c = *p++) {

	case '[':
		{
			int ok; 
			int lc; 
			int notflag = 0;

			ok = 0; 
			lc = 077777;
			if( *p == '!' ) {
				notflag = 1; 
				p++; 
			}
			while( c = *p++ ) {
				if(c == ']') {
					return(ok?gmatch(s,p):0);
				} else if (c == '-') {
					if(notflag) {
						if(lc > scc || scc > *(p++)) {
							ok++;
						} else { 
							return(0);
						}
					} else { 
						if( lc <= scc && scc <= (*p++)) {
							ok++;
						}
					}
				} else {
					if(notflag) {
						if(scc != (lc = (c&0177))) {
							ok++;
						} else {
							return(0);
						}
					} else { 
						if(scc == (lc = (c&0177))) { 
							ok++;
						}
					}
				}
			}
			return(0);
		}
	case '?':
		return(scc?gmatch(s,p):0);

	case '*':
		if(*p == 0) {
			return(1);
		}
		--s;
		while(*s) {
			if(gmatch(s++,p)) {
				return(1);
			} 
		}
		return(0);

	case 0:
		return(scc == 0);

	default:
		if((c&0177) != scc) {
			return(0);
		}

	}
	return(gmatch(s,p)?1:0);
}

