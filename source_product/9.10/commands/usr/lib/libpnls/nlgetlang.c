


#include <stdio.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>

short nlgetlang(function, err)
short		function;
unsigned short	err[];
{
	char	langName[MAXNAMLEN];
	char	f_langname[MAXNAMLEN];
	int	f_langid, langid;
	FILE	*fp;
	char	buffDir[13 + MAXNAMLEN];
	struct stat statbuf;

	err[0] = err[1] = (unsigned short)0;

	if (function < 1  || function > 3) {
		err[0] = 3;
		return(0);
	}

	langid = -1;

	strcpy(buffDir, "/usr/lib/nls");
	if (stat(buffDir, &statbuf) == -1) {
		err[0] = 1;
		return(0);
	}

	strcat(buffDir, "/config");
	if (stat(buffDir, &statbuf) == -1) {
		err[0] = 1;
		return(0);
	}

	if ((fp = fopen(buffDir, "r")) == NULL) {
		err[0] = 1;
		return(0);
	}

	if (getenv("LANG") == NULL) {
		err[0] = 4;
		strcpy(langName, "n-computer");
	} else
		strcpy(langName, getenv("LANG"));

	while (fscanf(fp, "%d%s", &f_langid, f_langname) != EOF) {
		if (strcmp(langName, f_langname) == 0) {
			langid = f_langid;
			break;
		}
	}

	fclose(fp);

	if (langid == -1) {
		err[0] = 2;
		return(0);
	} else {
		sprintf(buffDir, "/usr/lib/nls/%s", f_langname);
		if (stat(buffDir, &statbuf) == -1) {
			err[0] = 1;
			return(0);
		}

	}

	return((short)langid);
}

