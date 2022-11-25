static char *HPUX_ID = "@(#) $Revision: 66.8 $";

/*
 * 4.1(Berkeley) code.  However, the algorithm implemented here is S5.2-like
 * clri filsys inumber ...
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#ifdef HP_NFS
#include <time.h>
#include <sys/vnode.h>
#endif HP_NFS
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/fs.h>
#ifdef SecureWare
#include <sys/security.h>
#include <sys/audit.h>
#endif

#define ISIZE	(sizeof(struct dinode))
#define	NI	(MAXBSIZE/ISIZE)

struct	ino {
	char	junk[ISIZE];
};

#if defined(SecureWare) && defined(B1)
char auditbuf[80];
#endif

struct	ino	buf[NI];

union {
	struct fs  sblk;
	char	   pad[SBSIZE];
} sb_un;
#define sblock sb_un.sblk

int	status;

main(argc, argv)
	int argc;
	char *argv[];
{
	register i, f;
	unsigned long n;
	int j, k;
	long  dblk;    
	

#ifdef SecureWare
        struct dinode *dp;

	if( ISSECURE ){
            set_auth_parameters(argc, argv);
            if (!authorized_user("sysadmin")) {
                fprintf(stderr,
			"clri: you must have the 'sysadmin' authorization\n");
                exit(4);
            }
#ifdef B1
	    if(ISB1){
	        initprivs();
	        (void) forcepriv(SEC_ALLOWDACACCESS);
	        (void) forcepriv(SEC_ALLOWMACACCESS);
	    }
#endif
	}
#endif
	if (argc < 3) {
		fputs("usage: clri filsys inumber ...\n", stdout);
		exit(4);
	}

	/* since inodes are no longer written out on file close, we
	   must sync here to avoid having effects of clri overwritten
	   by later sync. */
	sync();
	f = open(argv[1], 2);
	if (f < 0) {
		fputs("cannot open ", stdout);
		fputs(argv[1], stdout);
		fputc('\n', stdout);
		exit(4);
	}
	lseek(f, (long) dbtob(SBLOCK), 0);  

	/* read in super block  */
	if (read(f, &sblock, SBSIZE) != SBSIZE) {
		fputs("cannot read ", stdout);
		fputs(argv[1], stdout);
		fputc('\n', stdout);
		exit(4);
	}
#if defined(FD_FSMAGIC)
	if (sblock.fs_magic != FS_MAGIC && 
	    sblock.fs_magic != FS_MAGIC_LFN &&
	    sblock.fs_magic != FD_FSMAGIC) {
#else /* not new magic number */
#ifdef LONGFILENAMES
	if (sblock.fs_magic != FS_MAGIC && 
	    sblock.fs_magic != FS_MAGIC_LFN) {
#else
	if (sblock.fs_magic != FS_MAGIC) {
#endif
#endif /* new magic number */
		fputs("bad super block magic number\n", stdout);
		exit(4);
	}
	
#if defined(SecureWare) && defined(B1)
	if( ISB1 )
        	disk_set_file_system(&sblock, sblock.fs_bsize);
#endif	
	/* check given inode number: inodes 0 and 1 are reserved  */
	for (i = 2; i < argc; i++) {
		if (!isnumber(argv[i])) {
			fputs(argv[i], stdout);
			fputs(": is not a number\n", stdout);
			status = 1;
			continue;
		}
		n = atol(argv[i]);
		if (n == 0) {
			fputs(argv[i], stdout);
			fputs(": is zero\n", stdout);
			status = 1;
			continue;
		}

		/* convert inode number to disc block address */
		dblk = fsbtodb(&sblock, itod(&sblock, n));  

		/* seek to that disc block */
		lseek(f, (long)dbtob(dblk), 0);

		/* read  n basic block size of bytes */
		if (read(f, (char *)buf, sblock.fs_bsize) != sblock.fs_bsize) {
			fputs(argv[i], stdout);
			fputs(": read error\n", stdout);
			status = 1;
		}
	}
	if (status)
		exit(status);

	/* do actual work here and zero out content of inode in inode list */
	for (i = 2; i < argc; i++) {
		extern char *ultoa();

		n = atol(argv[i]);
		fputs("clearing ", stdout);
		fputs(ultoa(n), stdout);
		fputc('\n', stdout);
		dblk = fsbtodb(&sblock, itod(&sblock, n));

		/* read inode block containing inode info to be cleared */
		lseek(f, (long)dbtob(dblk), 0);
		read(f, (char *)buf, sblock.fs_bsize);

		/* get offset of inode number in the inode block */
#if defined(SecureWare) && defined(B1)
		if(ISB1){
                    disk_inode_in_block (&sblock, (char *)buf, &dp, n);
                    (void) memset ((char *) dp, '\0', sizeof(*dp));
                    clri_extended_clear(dp);
		    sprintf(auditbuf, "clear inode %d of device %s",n,argv[1]);
		    audit_subsystem(auditbuf, "requested inode cleared",
			ET_SUBSYSTEM);
		}
		else{
		    j = itoo(&sblock, n);
		    for (k = 0; k < ISIZE; k++)
			buf[j].junk[k] = 0;
		}
#else 	
		j = itoo(&sblock, n);
		for (k = 0; k < ISIZE; k++)
			buf[j].junk[k] = 0;
#endif

		lseek(f, (long)dbtob(dblk), 0);
		write(f, (char *)buf, sblock.fs_bsize);
	}
	exit(status);
}

isnumber(s)
	char *s;
{
	register c;

	while(c = *s++)
		if (c < '0' || c > '9')
			return(0);
	return(1);
}
