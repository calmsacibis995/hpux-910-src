#ifndef lint
static char *HPUX_ID = "@(#) $Revision 126.1 $";
#endif

/*
 *	Huffman encoding program 
 *	Usage:	pack [[ - ] filename ... ] filename ...
 *		- option: enable/disable listing of statistics
 */


#include  <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define	END	256
#if u370
#define	BLKSIZE	4096
#else
#define	BLKSIZE	512
#endif
#define NAMELEN 80
#define PACKED 017436 /* <US><RS> - Unlikely value */
#define	SUF0	'.'
#define	SUF1	'z'

struct stat status, ostatus;
struct utimbuf utimbuf;

/* union for overlaying a long int with a set of four characters */
union FOUR {
	struct { long int lng; } lint;
	struct { char c0, c1, c2, c3; } chars;
};

/* character counters */
long	count [END+1];		/* hold frequency for each character (weights)*/
union	FOUR insize;		/* input file size stats */
long	outsize;		/* output file size */
long	dictsize;		/* number of bytes dictionary requires in
				   output file (overhead) */
int	diffbytes;		/* number of disctint chars in input file */

/* i/o stuff */
char	vflag = 0;	/* print statistics if set ( '-' on command line) */
int	force = 0;	/* allow forced packing for consistency in directory */
char	filename [NAMELEN];
int	infile;		/* unpacked file */
int	outfile;	/* packed file */
char	inbuff [BLKSIZE];
char	outbuff [BLKSIZE+4];

/* variables associated with the tree */
int	maxlev;			/* depth of Huffman tree */
int	levcount [25];
int	lastnode;		/* demarcate end of node space in parent */
int	parent [2*END+1];	/* tracks parents of count[] */

/* variables associated with the encoding process */
char	length [END+1];
long	bits [END+1];
union	FOUR mask;
long	inc;
#ifdef vax
char	*maskshuff[4]  = {&(mask.chars.c3), &(mask.chars.c2), &(mask.chars.c1), &(mask.chars.c0)};
#else
#ifdef pdp11
char	*maskshuff[4]  = {&(mask.chars.c1), &(mask.chars.c0), &(mask.chars.c3), &(mask.chars.c2)};
#else	/* u370 or 3b20 */
char	*maskshuff[4]  = {&(mask.chars.c0), &(mask.chars.c1), &(mask.chars.c2), &(mask.chars.c3)};
#endif
#endif

/* the heap */
int	n;		/* total number of nodes */
struct	heap {
	long int count;		/* node value (weight) */
	int node;		/* node identifier */
} heap [END+2];
/* macro to move one node to another */
#define hmove(a,b) {(b).count = (a).count; (b).node = (a).node;}

/* gather character frequency statistics */
/* return 1 if successful, 0 otherwise */
input ()
{
	register int i;
	for (i=0; i<END; i++)	/* zero all possible chars */
		count[i] = 0;
	while ((i = read(infile, inbuff, BLKSIZE)) > 0)
		while (i > 0)
			count[inbuff[--i]&0377] += 2; /* inc by two for later
							shift divide in packing
							output algorithm */
	if (i == 0)
		return (1);
	printf (": read error");
	return (0);
}

/* encode the current file */
/* return 1 if successful, 0 otherwise */
output ()
{
	int c, i, inleft;
	char *inp;
	register char **q, *outp;
	register int bitsleft;
	long temp;

	/* output ``PACKED'' header */
	outbuff[0] = 037; 	/* ascii US */
	outbuff[1] = 036; 	/* ascii RS */
	/* output the length and the dictionary */
	temp = insize.lint.lng;
	for (i=5; i>=2; i--) {
		outbuff[i] =  (char) (temp & 0377);
		temp >>= 8;
	}
	outp = &outbuff[6];
	*outp++ = maxlev;
	for (i=1; i<maxlev; i++)
		*outp++ = levcount[i];		/* having both the number
						   of leaves at each level */
	*outp++ = levcount[maxlev]-2;
	for (i=1; i<=maxlev; i++)		/* and the characters in */
		for (c=0; c<END; c++)		/* order of compaction */
			if (length[c] == i)	/* is sufficient for unpack */
				*outp++ = c;
	dictsize = outp-&outbuff[0];

	/* output the text */
	lseek(infile, 0L, 0);
	outsize = 0;
	bitsleft = 8;
	inleft = 0;
	do {
		if (inleft <= 0) {
			inleft = read(infile, inp = &inbuff[0], BLKSIZE);
			if (inleft < 0) {
				printf (": read error");
				return (0);
			}
		}
		c = (--inleft < 0) ? END : (*inp++ & 0377);
		mask.lint.lng = bits[c]<<bitsleft;
		q = &maskshuff[0];
		if (bitsleft == 8)		/* pack the bit codes into */
			*outp = **q++;		/* bytes for output */
		else
			*outp |= **q++;
		bitsleft -= length[c];
		while (bitsleft < 0) {
			*++outp = **q++;
			bitsleft += 8;
		}
		if (outp >= &outbuff[BLKSIZE]) {
			if (write(outfile, outbuff, BLKSIZE) != BLKSIZE) {
wrerr:				printf (".z: write error");
				return (0);
			}
			((union FOUR *) outbuff)->lint.lng = ((union FOUR *) &outbuff[BLKSIZE])->lint.lng;
			outp -= BLKSIZE;	/* reset pointers after a */
			outsize += BLKSIZE;	/* block of text output */
		}
	} while (c != END);
	if (bitsleft < 8)
		outp++;
	c = outp-outbuff;			/* write the last partial
							block */
	if (write(outfile, outbuff, c) != c)
		goto wrerr;
	outsize += c;
	return (1);
}

/* makes a heap out of heap[i],...,heap[n] */
heapify (i)		/* heap ?  sort the nodes so that the
			   low weight node is at the bottom of
			   the heap */
{
	register int k;
	int lastparent;
	struct heap heapsubi;
	hmove (heap[i], heapsubi);  /*  copy passed node to heapsubi */
				    /*  for later restoral to proper sequence*/
	lastparent = n/2;	
	while (i <= lastparent) {
		k = 2*i;	/* look at all nodes in binary progression*/
		if (heap[k].count > heap[k+1].count && k < n)
			k++;
		if (heapsubi.count < heap[k].count)
			break;
		hmove (heap[k], heap[i]);
		i = k;
	}
	hmove (heapsubi, heap[i]);
}

/* return 1 after successful packing, 0 otherwise */
int packfile ()		/* main packing routine */
{
	register int c, i, p;
	long bitsout;

	/* gather frequency statistics */
	if (input() == 0)	/* if some error reading file */
		return (0);

	/* put occurring chars in heap with their counts */
	diffbytes = -1;
	count[END] = 1;
	insize.lint.lng = n = 0;
	for (i=END; i>=0; i--) {	/* run thru all possible chars */
		parent[i] = 0;		/* initialize the possible parent */
		if (count[i] > 0) {	/* for each valid weight */
			diffbytes++;	/* increment the number of different
					   chars in input file */
			insize.lint.lng += count[i]; /* sum for size of input
							file */
			heap[++n].count = count[i];  /* create a node with it's
							weight and identifier */
			heap[n].node = i;
		}
	}
	if (diffbytes == 1) {
		printf (": trivial file");
		return (0);
	}
	insize.lint.lng >>= 1;	/* divide by two to get true size */
	for (i=n/2; i>=1; i--)  /* order the nodes by weight */
		heapify(i);

	/* build Huffman tree */
	lastnode = END;
	while (n > 1) {  	/* for all nodes */
		parent[heap[1].node] = ++lastnode;	/* take lowest weight
							node and assign a new
							node to it as a parent*/
		inc = heap[1].count;
		hmove (heap[n], heap[1]);	/* reduce the number of nodes
						   left to process by one */
		n--;
		heapify(1);	/* Get the next lowest node */
		parent[heap[1].node] = lastnode;  /* assign it to same parent */
		heap[1].node = lastnode;  /* enter the parent node into heap */
		heap[1].count += inc;	  /* with new value and go find the */
		heapify(1);		/* current lowest node for next time */
	}
	parent[lastnode] = 0;	/* set last parent to root */

	/* assign lengths to encoding for each character */
	bitsout = maxlev = 0;
	for (i=1; i<=24; i++)
		levcount[i] = 0;
	for (i=0; i<=END; i++) {
		c = 0;		/* traverse the tree to determine number
				   of bits required to encode this char */
		for (p=parent[i]; p!=0; p=parent[p])
			c++;
		levcount[c]++;	/* how many leaves at this level */
		length[i] = c;
		if (c > maxlev)
			maxlev = c;
		bitsout += c*(count[i]>>1);	/* determine total number
						of bits in output file if
						this encoding used: number of
					this char times its code length in 
					bits.  */
	}
	if (maxlev > 24) {
		/* can't occur unless insize.lint.lng >= 2**24 */
		printf (": Huffman tree has too many levels");
		return(0);
	}

	/* don't bother if no compression results in number of blocks required*/
	outsize = ((bitsout+7)>>3)+6+maxlev+diffbytes;
	if ((insize.lint.lng+BLKSIZE-1)/BLKSIZE <= (outsize+BLKSIZE-1)/BLKSIZE
	    && !force) {			/* unless user wants it anyway*/
		printf (": no saving");
		return(0);
	}

	/* compute bit patterns for each character */
	inc = 1L << 24;
	inc >>= maxlev;
	mask.lint.lng = 0;
	for (i=maxlev; i>0; i--) {			/* it appears the codes
						are in the upper bits of bits[]
						with '1' being assigned to most
						frequent char and '0'... to
						the rest.  */
		for (c=0; c<=END; c++)
			if (length[c] == i) {
				bits[c] = mask.lint.lng;
				mask.lint.lng += inc;
			}
		mask.lint.lng &= ~inc;
		inc <<= 1;
	}	/* there is no direct relation ship between the
		   bit codes generated here and the huffman tree */

	return (output());
}

main(argc, argv)
int argc; char *argv[];
{
	register int i;
	register char *cp;
	int k, sep;
	int fcount =0; /* count failures */

	for (k=1; k<argc; k++) {
		if (argv[k][0] == '-' && argv[k][1] == '\0') {
			vflag = 1 - vflag;	/* toggle statistics flag */
			continue;
		}
		if (argv[k][0] == '-' && argv[k][1] == 'f') {
			force++;	/* force packing even when not
						beneficial */
			continue;
		}
		fcount++; /* increase failure count - expect the worst */
		printf ("%s: %s", argv[0], argv[k]);
		sep = -1;  cp = filename;
		for (i=0; i < (NAMELEN-3) && (*cp = argv[k][i]); i++)
			if (*cp++ == '/') sep = i; /* find basename */
		if (cp[-1]==SUF1 && cp[-2]==SUF0) {	/* if last two
							letters of file name
							are '.z' then assume
							file is:  */
			printf (": already packed\n");
			continue;
		}
		if (i >= (NAMELEN-3) || (i-sep) > 13) { /* 13 or 12? */
			printf (": file name too long\n");
			continue;
		}
		if ((infile = open (filename, 0)) < 0) {
			printf (": cannot open\n");
			continue;
		}
		fstat(infile,&status);	/* get info on open file */
		if (status.st_mode&040000) {
			printf (": cannot pack a directory\n");
			goto closein;
		}
		if( status.st_nlink != 1 ) {
			printf(": has links\n");
			goto closein;
		}
		*cp++ = SUF0;  *cp++ = SUF1;  *cp = '\0';
		if( stat(filename, &ostatus) != -1) {	/* get info on
							   proposed outfile */
			printf(".z: already exists\n");
			goto closein;
		}
		if ((outfile = creat (filename, status.st_mode&07000)) < 0) {
			printf (".z: cannot create\n");
			goto closein;
		}
#ifdef ACLS
		/*
		 * If the unpacked file has optional ACL entries, they should
		 * be copied to the packed file.  Note this does an fstat to 
		 * get the uid and gid of the packed file in case the file 
		 * already existed and has different ownerships (in this 
		 * case the cpacl or chmod may fail).
		 */
		if (status.st_acl) {
			fstat(outfile, &ostatus); 
			if (fcpacl(infile, outfile, status.st_mode, 
			    status.st_uid, status.st_gid, ostatus.st_uid, 
			    ostatus.st_gid) != 0)
				printf("can't copy ACL to %s\n", filename);
		}
		else
		{
		/*
		 * File has no optional ACL entries or is remote.  Chmod is
		 * more efficient to use than cpacl.
		 */
			if (chmod (filename, status.st_mode) != 0)
				printf("can't change mode to %o\n", status.st_mode);
		}
#else /* no ACLS */
		if (chmod (filename, status.st_mode) != 0)
			printf("can't change mode to %o\n", status.st_mode);
#endif /* ACLS */
		if (packfile()) {	/* pack this file */
			if (unlink(argv[k]) != 0)	/* rm input file */
				fprintf(stderr, "%s: can't unlink %s\n",
					argv[0], argv[k]);
			fcount--;  /* success after all */
			if ( insize.lint.lng != 0 )
			    printf (": %.1f%% Compression\n",
				    ((double)(-outsize+(insize.lint.lng))/
				    (double)insize.lint.lng)*100);
			else
			    printf (": 0.0%% Compression (empty file)\n");

			/* output statistics */
			if (vflag) {	/* if user specifies print statistics */
				printf("	from %ld to %ld bytes\n",  insize.lint.lng, outsize);
				printf("	Huffman tree has %d levels below root\n", maxlev);
				printf("	%d distinct bytes in input\n", diffbytes);
				printf("	dictionary overhead = %ld bytes\n", dictsize);
				printf("	effective  entropy  = %.2f bits/byte\n", 
					((double) outsize / (double) insize.lint.lng) * 8 );
				printf("	asymptotic entropy  = %.2f bits/byte\n", 
					((double) (outsize-dictsize) / (double) insize.lint.lng) * 8 );
			}	/* end output statistics */
		}		/* else if packing not done */
		else
		{       printf (" - file unchanged\n");
			unlink(filename);  /* rm output file candidate */
		}

      closein:	close (outfile);
		close (infile);
		utimbuf.actime = status.st_atime;
		utimbuf.modtime = status.st_mtime;
		utime(filename, &utimbuf);	/* preserve acc & mod times */
						  /* of output file if approp */
		chown (filename, status.st_uid, status.st_gid);

	}	/* end for args to pack loop */
	return (fcount);		/* to pack invoker */
}
