/* @(#) $Revision: 37.2 $ */    
#ifdef NLS
#include	<msgbuf.h>
#endif
# include	"../../hdr/defines.h"
# include	<ndir.h>

int	nfiles;
char	had_dir;
char	had_standinp;


do_file(p,func)
register char *p;
int (*func)();
{
	extern char *Ffile;
	char str[FILESIZE];
	char ibuf[FILESIZE];
	struct direct *dir;
	DIR *dirp;

	if (p[0] == '-') {
		had_standinp = 1;
		while (gets(ibuf) != NULL) {
			if (sccsfile(ibuf)) {
				Ffile = ibuf;
				(*func)(ibuf);
				nfiles++;
			}
		}
	}
	else if (exists(p) && (Statbuf.st_mode & S_IFMT) == S_IFDIR) {
		had_dir = 1;
		Ffile = p;
		if((dirp = opendir(p)) == NULL)
			return;
		while ((dir=readdir(dirp)) != NULL) {
			if(dir->d_ino == 0) continue;
			if ((dir->d_name == ".") || (dir->d_name == ".."))
				continue;
#ifdef hp9000s500
			dir->d_name[16] = '\0';   /*  KAH */
#endif
			sprintf(str,"%s/%s",p,dir->d_name);
			if(sccsfile(str)) {
				Ffile = str;
				(*func)(str);
				nfiles++;
			}
		}
		closedir(dirp);
	}
	else {
		Ffile = p;
		(*func)(p);
		nfiles++;
	}
}
