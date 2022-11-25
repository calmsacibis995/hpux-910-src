/* HPUX_ID: @(#)sys.c	27.3     85/04/24  */

#include <sys/param.h>
#include <a.out.h>
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/fs.h>
#include <sys/sysmacros.h>
#include <sys/dir.h>

#include "saio.h"

#define	min(a,b)	(((a) < (b)) ? (a) : (b))

extern boot_errno;

ino_t	dlook();

struct dirstuff {
	int loc;
	struct iob *io;
};

/*
 * Concatenate s2 on the end of s1.  S1's space must be large enough.
 * Return s1.
 */

char *
strcat(s1, s2)
register char *s1, *s2;
{
	register char *os1;

	if (s2 == (char *)0)
		return(s1);
	os1 = s1;
	while(*s1++)
		;
	--s1;
	while(*s1++ = *s2++)
		;
	return(os1);
}
/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

int
strcmp(s1, s2)
register unsigned char *s1, *s2;
{

	if(s1 == s2)
		return(0);
	if (s1 == (unsigned char *)0)
	{
		if (s2 == (unsigned char *)0)
			return(0);
		else
			return(-*s2);
	}
	else if (s2 == (unsigned char *)0)
		return(*s1);

	while(*s1 == *s2++)
		if(*s1++ == '\0')
			return(0);
	return(*s1 - *--s2);
}
/*
 * Copy string s2 to s1.  s1 must be large enough.
 * return s1
 */

char *
strcpy(s1, s2)
register char *s1, *s2;
{
	register char *os1;

	if (s2 == (char *)0)
	{
		*s1 = '\0';
		return(s1);
	}
	os1 = s1;
	while(*s1++ = *s2++)
		;
	return(os1);
}
/*
 * Returns the number of
 * non-NULL bytes in string argument.
 */

int
strlen(s)
register char *s;
{
	register char *s0 = s + 1;

	if (s == (char *)0)
		return(0);

	while (*s++ != '\0')
		;
	return (s - s0);
}



_stop(s)
	char *s;
{
	register int i;

	for (i = 0; i < NFILES; i++)
		if (iob[i].i_flgs != 0)
			close(i);
	crtmsg(3, s);
	_rtt();
}

openi(n, io)
	register struct iob *io;
{
	register struct dinode *dp;
	register int cc;

	io->i_offset = 0;
	io->i_bn = fsbtodb(&io->i_fs, itod(&io->i_fs, n)) + io->i_boff;
	io->i_cc = io->i_fs.fs_bsize;
	io->i_ma = io->i_buf;
	cc = devread(io);
	dp = (struct dinode *)io->i_buf;
#ifdef ACLS
	io->i_ino.i_icun.i_ic = dp[itoo(&io->i_fs, n)].di_ic;
#else
	io->i_ino.i_ic = dp[itoo(&io->i_fs, n)].di_ic;
#endif /* ACLS */
	return (cc);
}

find(path, file)
	register char *path;
	register struct iob *file;
{
	register char *q;
	register char c;
	register int n;

	if (path==NULL || *path=='\0')
		return (0);

	if (openi((ino_t) ROOTINO, file) < 0)
		return (0);

	while (*path) {
		while (*path == '/')
			path++;
		q = path;
		while(*q != '/' && *q != '\0')
			q++;
		c = *q;
		*q = '\0';

		if ((n = dlook(path, file)) != 0) {
			if (c == '\0')
				break;
			if (openi(n, file) < 0)
				return (0);
			*q = c;
			path = q;
			continue;
		} else 
			return (0);
	}
	return (n);
}

daddr_t sbmap(io, bn)
	register struct iob *io;
	register daddr_t bn;
{
	register struct inode *ip;
	register int i, j, sh;
	register daddr_t nb, *bap;

	ip = &io->i_ino;
	if (bn < 0)
		return ((daddr_t)0);

	/*
	 * blocks 0..NDADDR are direct blocks
	 */
	if(bn < NDADDR) {
		nb = ip->i_db[bn];
		return (nb);
	}

	/*
	 * addresses NIADDR have single and double indirect blocks.
	 * the first step is to determine how many levels of indirection.
	 */
	sh = 1;
	bn -= NDADDR;
	for (j = NIADDR; j > 0; j--) {
		sh *= NINDIR(&io->i_fs);
		if (bn < sh)
			break;
		bn -= sh;
	}
	if (j == 0)
		return ((daddr_t)0);

	/*
	 * fetch the first indirect block address from the inode
	 */
	nb = ip->i_ib[NIADDR - j];
	if (nb == 0) 
		return ((daddr_t)0);

	/*
	 * fetch through the indirect blocks
	 */
	for (; j <= NIADDR; j++) {
		if (blknos[j] != nb) {
			io->i_bn = fsbtodb(&io->i_fs, nb) + io->i_boff;
			io->i_ma = b[j];
			io->i_cc = io->i_fs.fs_bsize;
			if (devread(io) != io->i_fs.fs_bsize) {
				if (io->i_error)
					errno = io->i_error;
				return ((daddr_t)0);
			}
			blknos[j] = nb;
		}
		bap = (daddr_t *)b[j];
		sh /= NINDIR(&io->i_fs);
		i = (bn / sh) % NINDIR(&io->i_fs);
		nb = bap[i];
		if(nb == 0)
			return ((daddr_t)0);
	}
	return (nb);
}

struct direct *readdir();

ino_t dlook(s, io)
	register char *s;
	register struct iob *io;
{
	register struct direct *dp;
	register struct inode *ip;
	struct dirstuff dirp;
	register int len;

	if (s == NULL || *s == '\0')
		return (0);
	ip = &io->i_ino;
	if ((ip->i_mode&IFMT) != IFDIR) 
		return (0);
	if (ip->i_size == 0)
		return (0);

	len = strlen(s);
	dirp.loc = 0;
	dirp.io = io;
	for (dp = readdir(&dirp); dp != NULL; dp = readdir(&dirp)) {
		if(dp->d_ino == 0)
			continue;
		if (dp->d_namlen == len && !strcmp(s, dp->d_name))
			return (dp->d_ino);
	}
	return (0);
}

/*
 * get next entry in a directory.
 */
struct direct *
readdir(dirp)
	register struct dirstuff *dirp;
{
	register struct direct *dp;
	register struct iob *io;
	register daddr_t lbn, d;
	register int off;

	io = dirp->io;
	for(;;) {
		if (dirp->loc >= io->i_ino.i_size)
			return (NULL);
		off = blkoff(&io->i_fs, dirp->loc);
		if (off == 0) {
			lbn = lblkno(&io->i_fs, dirp->loc);
			d = sbmap(io, lbn);
			if(d == 0)
				return NULL;
			io->i_bn = fsbtodb(&io->i_fs, d) + io->i_boff;
			io->i_ma = io->i_buf;
			io->i_cc = blksize(&io->i_fs, &io->i_ino, lbn);
			if (devread(io) < 0) {
				errno = io->i_error;
				return (NULL);
			}
		}
		dp = (struct direct *)(io->i_buf + off);
		dirp->loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return (dp);
	}
}

get_how()
{
	register how;

	how = *(int *) 0xfffffedc;	 /* MSUS */
	how >>= 24; how &= 0xff;
	if ((how == 0xe1)	/* SRM */
	 || (how == 0xe2))	/* LAN */
		return(F_REMOTE);
	else
		return(0);
}

bad_fd(fdesc)
{
	errno = 0;
	if (fdesc < 0 || fdesc >= NFILES ||
	    (iob[fdesc].i_flgs & F_ALLOC) == 0) {
		errno = EBADF;
		return (-1);
	}
	return(0);
}

lseek(fdesc, addr, ptr)
	register int fdesc, ptr;
	register off_t addr;
{
	register struct iob *io;

	if (bad_fd(fdesc))
		return(-1);
	io = &iob[fdesc];
	if (ptr != 0) {
		errno = EOFFSET;
		return (-1);
	}
	io->i_offset = addr;
	io->i_bn = addr / DEV_BSIZE;
	io->i_cc = 0;
	return (0);
}

/* only works for files -- NOT devices */
/* works for remote file systems (like devices ) 12/15/86 PS */

# define MREAD_BSIZE	256
copyin(fdesc, buf, count)
int fdesc;
register char *buf;
register int count;
{
	register struct iob *io;
	register struct fs *fs;
	register char *p;
	register int c, lbn, off, size, diff;
	char *oldbuf = buf;

	io = &iob[fdesc];
	if (io->i_flgs & F_REMOTE) {	/* not a blocked device */
		off = io->i_offset; off &= (MREAD_BSIZE-1);
		if (off) {		/* read partial buffer */
			io->i_bn = io->i_offset >> 8;
			io->i_ma = io->i_buf;
			io->i_cc = MREAD_BSIZE;
			if (devread(io) < 0) {
				errno = io->i_error;
				return (-1);
			}
			size = MREAD_BSIZE - off;
			count -= size;
			io->i_offset += size;
			p = &(io->i_buf[off]);
			while(size--) *buf++ = *p++;
		}
		io->i_bn = io->i_offset >> 8;
		io->i_ma = buf;
		io->i_cc = count;
		if (devread(io) < 0) {
			errno = io->i_error;
			return (-1);
		}
		io->i_offset += count;
	} else do {
		int old_bn;

		diff = io->i_ino.i_size - io->i_offset;
		if (diff <= 0) return (-1);

		fs = &io->i_fs;
		lbn = lblkno(fs, io->i_offset);
		old_bn = io->i_bn;
		io->i_bn = fsbtodb(fs, sbmap(io, lbn)) + io->i_boff;
		off = blkoff(fs, io->i_offset);
		size = blksize(fs, &io->i_ino, lbn);

		if (old_bn != io->i_bn) {
			/* different block: must devread */
			io->i_cc = size;
			io->i_ma = io->i_buf;
			if (devread(io) < 0) {
				errno = io->i_error;
				return (-1);
			}
		}

		size -= off;
		io->i_cc = size;

		if (io->i_offset - off + size >= io->i_ino.i_size)
			io->i_cc = diff + off;
		p = &io->i_buf[off];

		size = min(size, count);
		count -= size;
		io->i_offset += size;
		while(size--) *buf++ = *p++;
		io->i_ma = p;

	} while(count != 0); 
}

int	errno;

read(fdesc, buf, count)
	register int fdesc, count;
	register char *buf;
{
	register i;
	register struct iob *file;

	if (bad_fd(fdesc))
		return(-1);
	file = &iob[fdesc];
	if ((file->i_flgs&F_READ) == 0) {
		errno = EBADF;
		return (-1);
	}
	if (file->i_offset+count > file->i_ino.i_size)
		count = file->i_ino.i_size - file->i_offset;
	if ((i = count) <= 0)
		return (0);
	if (copyin(fdesc, buf, i) == -1)
		return(-1);
	else
		return (count);
}

int	openfirst = 1;

open(str)
	register char *str;
{
	register char *cp;
	register int i;
	register struct iob *file;
	register struct devtype *dp;
	register int fdesc;
	register int how;

	if (openfirst) {
		for (i = 0; i < NFILES; i++)
			iob[i].i_flgs = 0;
		openfirst = 0;
	}

	for (fdesc = 0; fdesc < NFILES; fdesc++)
		if (iob[fdesc].i_flgs == 0)
			goto gotfile;
	_stop("No more file slots");
gotfile:
	{
		register char *cp;

		file = &iob[fdesc];
		cp = ((char *) file);
		for (i=0; i < sizeof(struct iob); i++)
			*(cp++) = '\0';
	}
	(file = &iob[fdesc])->i_flgs |= F_ALLOC;

/*	devopen(file);		/* null function */

	file->i_ma = (char *)(&file->i_fs);
	file->i_cc = SBSIZE;
	file->i_bn = SBLOCK + file->i_boff;
	file->i_offset = 0;

	how = get_how();
#ifdef DEBUG
	dump_value("how",how);
#endif
	if (how & F_REMOTE) {
		int junk;
		short m1 = -1;
		char string255[80];

		strcpy(string255+1,str);
		string255[0] = strlen(str);
		i = call_bootrom(M_FOPEN, string255, &junk,
			&file->i_ino.i_size, &m1);
		if (file->i_ino.i_size == -1)	/* LAN !@$$ */
			file->i_ino.i_size = 0x7fffffff;
		if (i != 0) {
			return(-1);
		}
		goto file_opened;
	}
	if (devread(file) < 0) {
		errno = file->i_error;
		return (-1);
	}

	if ((i = find(str, file)) == 0) {
		file->i_flgs = 0;
		errno = ESRCH;
		return (-1);
	}
	if (openi(i, file) < 0) {
		errno = file->i_error;
		return (-1);
	}

file_opened:
	file->i_flgs |= F_FILE | F_READ | how;
	file->i_offset = 0;
	file->i_cc = 0;
#ifdef DEBUG
	{
		char msg[80];

		strcpy(msg,str);
		puts(msg);
		dump_value("fd opened",fdesc);
	}
#endif
	return (fdesc);
}


close(fdesc)
	register int fdesc;
{
	register struct iob *file;

	if (bad_fd(fdesc))
		return(-1);
	file = &iob[fdesc];
/*	if ((file->i_flgs&F_FILE) == 0)
/*		devclose(file);			/* null function */
	if (file->i_flgs & F_REMOTE)
		(void) call_bootrom(M_FCLOSE);
	file->i_flgs = 0;
#ifdef DEBUG
	dump_value("fd closed",fdesc);
#endif
	return (0);
}


#define	CLUSTFILE	"/etc/clusterconf"
/* this macro copied from .../dux/duxparam.h */
#define ISSDO(ip) (((ip)->i_mode&(IFMT|ISUID)) == (IFDIR|ISUID))
isCDF(fd)
{
	return (ISSDO(&(iob[fd].i_ino)));
}

/* ================== cut here for open_CDF test suite ================== */
#define MAXCONTEXT 4

open_CDF(path)
char *path;
{
	char prefix[256];	/* the path so far */
	char filename[256];	/* what we're trying now */
	char cnode_name[80];
#ifdef DEBUG
	char msg[80];
#endif
	struct exec x;	/* performance kludge: stop when an a.out is found */
	register char *s;
	register fd, context, ft;

	strcpy(prefix,path);
	if (get_how() == F_REMOTE)
		return(open(path));	/* open_CDF used only for local disk */
	cnode_name[0] = '/';
	get_cnode_name(cnode_name+1);
#ifdef DEBUG
	strcpy(msg,"   path: '");	strcat(msg,path);
	strcat(msg,"'  prefix: '");	strcat(msg,prefix);
	strcat(msg,"'  cnode name: '");	strcat(msg,cnode_name+1);
	strcat(msg,"'");
	puts(msg);
#endif
	context = 0;	/* automagically try base pathname first */
	do {
		strcpy(filename,prefix);
		switch (context) {
			case 1:	s = cnode_name;		break;
			case 2: s = "/HP-MC68010";	break;
			case 3: s = "/localroot";	break;
			case 4: s = "/default";		break;
			default: s = "";		break;
		}
		strcat(filename,s);	/* add CDF component */
#ifdef DEBUG
		strcpy(msg,"trying ");
		strcat(msg,filename);
		puts(msg);
#endif
		if ((fd = open(filename)) >= 0) {
#ifdef DEBUG
			strcpy(msg,"opened ");
			strcat(msg,filename);
			puts(msg);
			dump_value("inode i_mode", iob[fd].i_ino.i_mode);
#endif
			if (isCDF(fd)) {
				/* inode is hidden directory */
				context = 0;		/* re-init context */
				strcpy(prefix,filename);/* last file opened */
				close(fd);		/* give back fd */
			} else {
				lseek(fd,0,0);		/* reset file */
				return(fd);		/* bail out */
			}
		}
	} while (++context <= MAXCONTEXT);
	return(-1);
}

#define NOTFIRST	1
get_cnode_name(name)
char *name;
{
	char buf[120];
	register char *cp, *cname;
	register fd, err, flag = 0;

	fd = open(CLUSTFILE,0);
	strcpy(name,"standalone");	/* in case of premature exit */
	do {
		/* 'gets' a line from /etc/clusterconf into 'buf' */
		cp = buf;
		while (((err = read(fd, cp, 1)) == 1) && (*cp != '\n'))
			cp++;
		*cp = '\0';			/* make it a string */

		if (buf[0] == '#')		/* check for comments */
			continue;
		if (flag & NOTFIRST) {		/* skip first line */
			flag |= NOTFIRST;
			continue;
		}

		/* now, we change all ':'s to '\0' */
		for (cp = buf; *cp; cp++)
			if (*cp == ':')
				*cp = '\0';
		/* ...and make sure we have at least 3+1 fields */
		*(cp++) = '\0';
		*(cp++) = '\0';
		*(cp++) = '\0';

		cp = buf;
		while (*cp++);			/* cp now at cnode id */
		while (*cp++);			/* cp now at name id */
		cname = cp;
		while (*cp++);			/* cp now at cnode type */
		if (*cp == 'r') {
			strcpy(name, cname);
			break;
		}
	} while (err == 1);
	close(fd);
}
/* ================== cut here for open_CDF test suite ================== */
