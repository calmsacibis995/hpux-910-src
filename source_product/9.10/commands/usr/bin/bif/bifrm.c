static char *HPUX_ID = "@(#) $Revision: 27.1 $";
/*
**	rm [-fir] file ...
**
**	(BELL FILE SYSTEM VERSION)
*/

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	"dir.h"

int	errcode;
int	debug=0;
char	*pname;

main(argc, argv)
char *argv[];
{
	register char *arg;
	int	fflg, iflg, rflg;

	pname = argv[0];
	fflg = 0;
	iflg = 0;
	rflg = 0;
	if(argc>1 && argv[1][0]=='-') {
		arg = *++argv;
		argc--;
		while(*++arg != '\0')
			switch(*arg) {
			case 'f':
				fflg++;
				break;
			case 'i':
				iflg++;
				break;
			case 'r':
				rflg++;
				break;
			default:
				fprintf(stderr, "usage: %s [-fir] file ...\n",
					pname);
				exit(2);
			}
	}
	while(--argc > 0) {
		if(!strcmp(*++argv, "..")) {
			fprintf(stderr, "%s: cannot remove ..\n",
				pname);
			continue;
		}
		rm(*argv, fflg, rflg, iflg);
	}

	exit(errcode?2:0);
}

extern char * normalize();

rm(argg, fflg, rflg, iflg)
char argg[];
{
	struct	stat	buf;
	struct	direct	direct;
	char	name[100];
	int	d;
	register char * arg;

	arg = argg;
	arg = normalize(arg);
	if(bfsstat(arg, &buf)) {
		if (fflg==0) {
			fprintf(stderr, "%s: %s non-existent\n",
				pname, arg);
			++errcode;
		}
		return;
	}
	if ((buf.st_mode&S_IFMT) == S_IFDIR) {
		if(rflg) {
			if(iflg && !dotname(arg)) {
				printf("directory %s: ", arg);
				if(!yes())
					return;
			}
			if((d=bfsopen(arg, 0)) < 0) {
				fprintf(stderr, "%s : cannot read %s\n",
					pname, arg);
				exit(2);
			}
			if (!bfsfile(d)) {
				fprintf(stderr, "%s: use rm to process %s\n",
					pname, arg);
				errcode++;
				return;
			}
			while(bfsread(d, &direct, sizeof(direct)) == sizeof(direct)) {
				if(direct.d_ino != 0 && !dotname(direct.d_name)) {
					sprintf(name, "%s/%.14s", arg, direct.d_name);
					rm(name, fflg, rflg, iflg);
				}
			}
			bfsclose(d);
			errcode += rmdir(arg, iflg);
			return;
		}
		fprintf(stderr, "%s: %s directory\n", pname, arg);
		++errcode;
		return;
	}

	if(iflg) {
		printf("%s: ", arg);
		if(!yes())
			return;
	}
	/** no access for BFS
	else if(!fflg) {
		if(access(arg, 02) < 0 && isatty(fileno(stdin))) {
			printf("%s: %o mode ", arg, buf.st_mode&0777);
			if(!yes())
				return;
		}
	}
	**/
	/* here check for write permision on file */
	if ((buf.st_mode&S_IWRITE) != 0 || (fflg || iflg))
	{
		if(bfsunlink(arg) != 0)
		{
			perror(pname);
			fprintf(stderr, "%s: %s not removed\n", pname, arg);
			++errcode;
		}
	}
	else
	{
		perror(pname);
		fprintf(stderr, "%s: %s not removed\n", pname, arg);
		++errcode;
	}
}

dotname(s)
char *s;
{
	if(s[0] == '.')
		if(s[1] == '.')
			if(s[2] == '\0')
				return(1);
			else
				return(0);
		else if(s[1] == '\0')
			return(1);
	return(0);
}

rmdir(f, iflg)
char *f;
{
	int	status, i;
	char buf[100];

	if(dotname(f))
		return(0);
	if(iflg) {
		printf("%s: ", f);
		if(!yes())
			return(0);
	}
	while((i=fork()) == -1)
		sleep(3);
	if(i) {
		wait(&status);
		return(status);
	}
	strcpy(buf, pname);		/* our name: bfsrm, say */
	strcat(buf, "dir");		/* make it bfsrmdir, say */
	execl(buf, buf, f, 0);		/* try to remove the directory */
	perror(pname);
	sprintf(stderr, "%s: can't execute %s\n", pname, buf);
	exit(2);
}

yes()
{
	int	i, b;

	i = b = getchar();
	while(b != '\n' && b > 0)
		b = getchar();
	return(i == 'y');
}
