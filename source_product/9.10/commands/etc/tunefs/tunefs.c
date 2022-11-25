static char *HPUX_ID = "@(#) $Revision: 70.1 $";

/*
 * tunefs: change layout parameters to an existing file system.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <stdio.h>
#ifdef SecureWare
#include <sys/security.h>
#endif

union {
	struct	fs sb;
	char pad[MAXBSIZE];
} sbun;
#define	sblock sbun.sb

int fi;

main(argc, argv)
	int argc;
	char *argv[];
{
	char *cp, *special, *name;
	struct stat st;
	int i;
	int Aflag = 0;
	int vflag = 0;
	char device[MAXPATHLEN];

	argc--, argv++; 
	if (argc < 2)
		goto usage;
	special = argv[argc - 1];

#ifdef SecureWare
	if (ISSECURE)  {
		set_auth_parameters(argc, argv);

#ifdef B1
		if (ISB1) {
			initprivs();
			(void) forcepriv(SEC_ALLOWDACACCESS);
			(void) forcepriv(SEC_ALLOWMACACCESS);
		} 
#endif

		if (!authorized_user("backup"))  {
			fprintf(stderr, "tunefs: no authorization to use tunefs\n");
			exit (1);
		}
   }
#endif
	if (stat(special, &st) < 0) {
		fputs("tunefs: ", stderr);
		perror(special);
		exit(1);
	}
	if ((st.st_mode & S_IFMT) != S_IFBLK &&
	    (st.st_mode & S_IFMT) != S_IFCHR)
		fatal("%s: not a block or character device", special);

	getsb(&sblock, special);
	for (; argc > 0 && argv[0][0] == '-'; argc--, argv++) {
		for (cp = &argv[0][1]; *cp; cp++)
			switch (*cp) {

			case 'A':
				Aflag++;
				continue;

			case 'a':
				name = "maximum contiguous block count";
				if (argc < 1)
					fatal("-a: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 1)
					fatal("%s: %s must be >= 1",
						*argv, name);
				fprintf(stdout, "%s changes from %d to %d\n",
					name, sblock.fs_maxcontig, i);
				sblock.fs_maxcontig = i;
				continue;

			case 'd':
				name =
				   "rotational delay between contiguous blocks";
				if (argc < 1)
					fatal("-d: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 0)
					fatal("%s: bad %s", *argv, name);
				fprintf(stdout,
					"%s changes from %dms to %dms\n",
					name, sblock.fs_rotdelay, i);
				sblock.fs_rotdelay = i;
				continue;

			case 'e':
				name =
				  "maximum blocks per file in a cylinder group";
				if (argc < 1)
					fatal("-e: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 1)
					fatal("%s: %s must be >= 1",
						*argv, name);
				fprintf(stdout, "%s changes from %d to %d\n",
					name, sblock.fs_maxbpg, i);
				sblock.fs_maxbpg = i;
				continue;

			case 'm':
				name = "minimum percentage of free space";
				if (argc < 1)
					fatal("-m: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 0 || i > 99)
					fatal("%s: bad %s", *argv, name);
				fprintf(stdout,
					"%s changes from %d%% to %d%%\n",
					name, sblock.fs_minfree, i);
				sblock.fs_minfree = i;
				continue;

			case 'v':
				vflag++;
				continue;
			default:
				fatal("-%c: unknown flag", *cp);
			}
	}
	if (argc != 1)
		goto usage;
	bwrite(SBLOCK, (char *)&sblock, SBSIZE);
	if(vflag)
	        print_sb(&sblock, stdout);
	if (Aflag)
		for (i = 0; i < sblock.fs_ncg; i++)
			bwrite(fsbtodb(&sblock, cgsblock(&sblock, i)),
			    (char *)&sblock, SBSIZE);
	close(fi);
	exit(0);
usage:
	fputs("Usage: tunefs tuneup-options special-device\n", stderr);
	fputs("where tuneup-options are:\n", stderr);
	fputs("\t-a maximum contiguous blocks\n", stderr);
	fputs("\t-d rotational delay between contiguous blocks\n", stderr);
	fputs("\t-e maximum blocks per file in a cylinder group\n", stderr);
	fputs("\t-m minimum percentage of free space\n", stderr);
	fputs("\t-v display primary super block\n", stderr);
	exit(2);
}

getsb(fs, file)
	register struct fs *fs;
	char *file;
{

	fi = open(file, 2);
	if (fi < 0) {
		fputs("cannot open", stderr);
		perror(file);
		exit(3);
	}
	if (bread(SBLOCK, (char *)fs, SBSIZE)) {
		fputs("bad super block", stderr);
		perror(file);
		exit(4);
	}
#if defined(FD_FSMAGIC)
	if ((fs->fs_magic != FS_MAGIC) && (fs->fs_magic != FS_MAGIC_LFN)
		&& (fs->fs_magic != FD_FSMAGIC))
#else /* not NEW_MAGIC */
#ifdef LONGFILENAMES
	if ((fs->fs_magic != FS_MAGIC) && (fs->fs_magic != FS_MAGIC_LFN))
#else /* not LONGFILESNAMES */
	if (fs->fs_magic != FS_MAGIC)
#endif /* not LONGFILESNAMES */
#endif /* NEW_MAGIC */
	{
		fputs(file, stderr);
		fputs(": bad magic number\n", stderr);
		exit(5);
	}
}

bwrite(blk, buf, size)
	char *buf;
	daddr_t blk;
	register size;
{
	if (lseek(fi, blk * DEV_BSIZE, 0) == -1) {
		perror("FS SEEK");
		exit(6);
	}
	if (write(fi, buf, size) != size) {
		perror("FS WRITE");
		exit(7);
	}
}

bread(bno, buf, cnt)
	daddr_t bno;
	char *buf;
{
	register i;

	if (lseek(fi, bno * DEV_BSIZE, 0) == -1)
		return(1);
	if ((i = read(fi, buf, cnt)) != cnt) {
		for(i=0; i<sblock.fs_bsize; i++)
			buf[i] = 0;
		return (1);
	}
	return (0);
}

print_sb(afs, outf)
    struct fs *afs;
    FILE *outf;
{
    int i;

    fprintf(outf, "super block last mounted on: %s\n", afs->fs_fsmnt);
    fprintf(outf, "magic\t%x", afs->fs_magic);
#ifdef hpux
    print_sb_clean(afs->fs_clean, outf);
#endif
    fprintf(outf, "\ttime\t%s", ctime((time_t *)&(afs->fs_time)));
    fprintf(outf, "sblkno\t%d\tcblkno\t%d\tiblkno\t%d\tdblkno\t%d\n",
	afs->fs_sblkno, afs->fs_cblkno, afs->fs_iblkno, afs->fs_dblkno);
    fprintf(outf, "sbsize\t%d\tcgsize\t%d\tcgoffset %d\tcgmask\t0x%08x\n",
	afs->fs_sbsize, afs->fs_cgsize, afs->fs_cgoffset, afs->fs_cgmask);
    fprintf(outf, "ncg\t%d\tsize\t%d\tblocks\t%d\n",
	afs->fs_ncg, afs->fs_size, afs->fs_dsize);
    fprintf(outf, "bsize\t%d\tbshift\t%d\tbmask\t0x%08x\n",
	afs->fs_bsize, afs->fs_bshift, afs->fs_bmask);
    fprintf(outf, "fsize\t%d\tfshift\t%d\tfmask\t0x%08x\n",
	afs->fs_fsize, afs->fs_fshift, afs->fs_fmask);
    fprintf(outf, "frag\t%d\tfragshift\t%d\tfsbtodb\t%d\n",
	afs->fs_frag, afs->fs_fragshift, afs->fs_fsbtodb);
    fprintf(outf, "minfree\t%d%%\tmaxbpg\t%d\n",
	afs->fs_minfree, afs->fs_maxbpg);
    fprintf(outf, "maxcontig %d\trotdelay %dms\trps\t%d\n",
	afs->fs_maxcontig, afs->fs_rotdelay, afs->fs_rps);
    fprintf(outf, "csaddr\t%d\tcssize\t%d\tcsshift\t%d\tcsmask\t0x%08x\n",
	afs->fs_csaddr, afs->fs_cssize, afs->fs_csshift, afs->fs_csmask);
    fprintf(outf, "ntrak\t%d\tnsect\t%d\tspc\t%d\tncyl\t%d\n",
	afs->fs_ntrak, afs->fs_nsect, afs->fs_spc, afs->fs_ncyl);
    fprintf(outf, "cpg\t%d", afs->fs_cpg);
    if (afs->fs_frag == 0)
    	fprintf(outf, "\tbpg\t**\n");
    else
    	fprintf(outf, "\tbpg\t%d", afs->fs_fpg / afs->fs_frag);
    fprintf(outf, "\tfpg\t%d\tipg\t%d\n", afs->fs_fpg, afs->fs_ipg);
    fprintf(outf, "nindir\t%d\tinopb\t%d\tnspf\t%d\n",
	afs->fs_nindir, afs->fs_inopb, afs->fs_nspf);
    fprintf(outf, "nbfree\t%d\tndir\t%d\tnifree\t%d\tnffree\t%d\n",
	afs->fs_cstotal.cs_nbfree, afs->fs_cstotal.cs_ndir,
	afs->fs_cstotal.cs_nifree, afs->fs_cstotal.cs_nffree);
    fprintf(outf, "cgrotor\t%d\tfmod\t%d\tronly\t%d\n",
	afs->fs_cgrotor, afs->fs_fmod, afs->fs_ronly);
#ifdef hpux
    fprintf(outf, "fname %6s\tfpack %6s\n", afs->fs_fname, afs->fs_fpack);
#endif
    if (afs->fs_ncyl % afs->fs_cpg) {
	fprintf(outf, "\ncylinders in last group %d\n",
	    i = afs->fs_ncyl % afs->fs_cpg);
	fprintf(outf, "blocks in last group %d\n",
	    i * afs->fs_spc / NSPB(afs));
    }
    fprintf(outf, "\n");
}

#ifdef hpux
print_sb_clean(clean, outf)
     char clean;
     FILE *outf;
{
	fprintf(outf, "\tclean   ");
	switch( clean ) {
	      case FS_OK    : fprintf(outf, "FS_OK");
			      break;
	      case FS_NOTOK : fprintf(outf, "FS_NOTOK");
			      break;
	      case FS_CLEAN : fprintf(outf, "FS_CLEAN");
			      break;
	      default       : fprintf(outf, "0x%x", (int)clean);
	}
}
#endif



/* VARARGS1 */
fatal(fmt, arg1, arg2)
	char *fmt, *arg1, *arg2;
{

	fputs("tunefs: ", stderr);
	fprintf(stderr, fmt, arg1, arg2);
	fputc('\n', stderr);
	exit(10);
}
