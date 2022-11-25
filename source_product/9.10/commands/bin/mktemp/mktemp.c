static char *HPUX_ID = "@(#) $Revision: 64.2 $";

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#define	MIN(a,b)	((a)<(b)?(a):(b))

main(argc,argv)
int	argc;
char	*argv[];
{
	int		c,path_max;
	int		create = 0;
	char		*prefix = (char*)NULL;
	char		*dir = (char*)NULL;
	char		*getenv(),*mktmp();
	char		*temp;
	extern char	*optarg;

	while ((c = getopt(argc,argv,"cd:p:")) != EOF)
		switch (c) {
		case 'c': create = 1; break;
		case 'd': dir = optarg; break;
		case 'p': prefix = optarg; break;
		}
	if ((prefix == (char*)NULL) && ((prefix = getenv("LOGNAME")) == NULL))
		prefix = "";
	if (dir != (char*)NULL && (temp = mktmp(dir,prefix)) == (char*)NULL
		|| dir == (char*)NULL
			&& ((temp = mktmp(dir = getenv("TMPDIR"),prefix))
					== (char*)NULL)
			&& ((temp = mktmp(dir = "/tmp",prefix)) == (char*)NULL)
			&& ((temp = mktmp(dir = ".",prefix)) == (char*)NULL)) {
		fprintf(stderr,"couldn't generate a unique pathname\n");
		exit(1);
	}
	if (create) {
		if ((path_max = pathconf(dir,_PC_PATH_MAX)) < 0) {
			perror(dir);
			exit(1);
		}
		if (strlen(temp) > path_max) {
			fprintf(stderr,"length of %s exceeds PATH_MAX (%d)\n",
					temp,path_max);
			exit(1);
		}
		if (creat(temp,0666) < 0) {
			perror(temp);
			exit(1);
		}
	}
	puts(temp);
	exit(0);
}

char
*mktmp(dir,prefix)
char	*dir,*prefix;
{
	int	name_max;
	char	*tempname;
	char	*malloc();

	if ((name_max = pathconf(dir,_PC_NAME_MAX)) < 0)
		return((char*)NULL);
	if ((tempname = malloc(strlen(dir)+strlen(prefix)+7)) == (char*)NULL)
		return((char*)NULL);
	strcpy(tempname,dir);
	strcat(tempname,"/");
	strncat(tempname,prefix,MIN(name_max-6,strlen(prefix)));
	strcat(tempname,"XXXXXX");
	if (strcmp(mktemp(tempname),"") == 0)
		return((char*)NULL);
	return(tempname);
}
