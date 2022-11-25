static char *HPUX_ID = "@(#) $Revision: 66.1 $";

/*
	S5.2 , shared
 */

#include <stdio.h>
#include <mntent.h>
#ifdef SecureWare
#include <sys/security.h>
#ifdef B1
#include <mandatory.h>
#endif
#endif

#define BUFS 256

#ifdef SecureWare
main(argc, argv)
int argc;
char *argv[];
#else
main()
#endif
{

	int	i;
	char	*cp,inbuf[BUFS],fs[BUFS],dir[BUFS];
	FILE	*fp;
	struct mntent mnt;

#ifdef DEBUG
	fp = stdout;
#else
#ifdef SecureWare
	if (ISSECURE)  {
		set_auth_parameters(argc, argv);

#ifdef B1
		if (ISB1) {
			initprivs();
			(void) forcepriv(SEC_OWNER);
			(void) forcepriv(SEC_ALLOWDACACCESS);
			(void) forcepriv(SEC_ALLOWMACACCESS);
			mand_init();
		} 
#endif

		if (!authorized_user("backup"))  {
			fprintf(stderr, "setmnt: no authorization to use setmnt\n");
			exit (1);
		}
		if (ISB1)
			setslabel(mand_syslo);
   }
#endif
	/* create mnttab or rewrite existing one */
	if ((fp = setmntent(MNT_MNTTAB,"w")) == NULL) {
		perror(MNT_MNTTAB);
		exit(1);
	}
	if (chmod(MNT_MNTTAB, 0644) == -1) {
		perror(MNT_MNTTAB);
		exit(1);
	}
#endif DEBUG

	/* Fill in the mntent structure with the default values */
	mnt.mnt_fsname = fs;
	mnt.mnt_dir = dir;
	mnt.mnt_type = MNTTYPE_HFS;
	mnt.mnt_opts = MNTOPT_DEFAULTS;
	mnt.mnt_freq = 0;
	mnt.mnt_passno = 1;
	mnt.mnt_time = time((long *)0);

	while(1) {
		/* read from standard input and save in inbuf */
		for(cp = inbuf;;cp++) {
			if (read(0,cp,1) != 1) {
				exit(1);
			}
			if (*cp == '\n') {
				*cp = '\0';
				break;
			}
		}

		for(cp = inbuf; *cp && *cp != ' '; cp++);
		*cp++ = '\0';

		strcpy(mnt.mnt_fsname, inbuf);
		strcpy(mnt.mnt_dir, cp);
		if ( addmntent(fp, &mnt) == 1 ) {
			perror(MNT_MNTTAB);
			exit(1);
		}
		for(i = 0; i < BUFS ; i++) {
			inbuf[i] = '\0';
		}
	}
}
