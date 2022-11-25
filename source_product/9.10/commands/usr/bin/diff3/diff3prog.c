static char *HPUX_ID = "@(#) $Revision: 66.4 $";

/********************************************
 * NOTE:  this source is used both for diff3(1) and for rcsmerge(1).
 * The options that are not documented for diff3 are used by 
 * rcsmerge.  Do not remove them.
 *********************************************/

#include <stdio.h>
#include <limits.h>	/* for LINE_MAX */

/* diff3 - 3-way differential file comparison*/

/* 
 * diff3 [-ex3EX] d13 d23 f1 f2 f3 [m1 m3]
 *
 * d13 = diff report on f1 vs f3
 * d23 = diff report on f2 vs f3
 * f1, f2, f3 the 3 files
 * if changes in f1 overlap with changes in f3, m1 and m3 are used
 * to mark the overlaps; otherwise, the file names f1 and f3 are used
 * (only for options E and X).
 */

struct  range {int from,to; };

/* 
 * from is first in range of changed lines
 * to is last+1
 * from=to=line after point of insertion
 * for added lines
 */
struct diff {struct range old, new;};

#define NC 512
struct diff d13[NC];
struct diff d23[NC];

/* 
 * de is used to gather editing scripts,
 * that are later spewed out in reverse order.
 * its first element must be all zero
 * the "new" component of de contains line positions
 * or byte positions depending on when you look(!?)
 * (After calls to edscript(), "new" positions are
 * byte offsets.)
 */
struct diff de[NC];

/*
 * Array "overlap" indicates which sections in de correspond to
 * lines that are different in all three files.
 */
char overlap[NC];
int  overlapcnt =0;

char line[LINE_MAX];     /*  buffer used by getline */

/*
 *  fp[0] = file1
 *  fp[1] = file2
 *  fp[2] = file3
 */
FILE *fp[3];

/*	the number of the last-read line in each file
 *	is kept in cline[0-2]
 */
int cline[3];

/*	
 * 	the latest known correspondence between line
 *	numbers of the 3 files is stored in last[1-3]
 */
int last[4];
int eflag;
int oflag;      /* indicates whether to mark overlaps (-E or -X)*/
int debug  = 0;
char f1mark[40], f3mark[40]; /*markers for -E and -X*/


main(argc,argv)
char **argv;
{
	register i,m,n;
        eflag=0; oflag=0;

	/*
	 * Parse options
	 */
	if(*argv[1]=='-') {
		switch(argv[1][1]) {
		default:
			eflag = 3;
			break;
		
		case '3':	/* Incorporate only changes marked "====3" */
			eflag = 2;
			break;
		
		case 'x':	/*  Incorporate only changes marked "====" */
			eflag = 1;
                        break;
		
                case 'E':	/*  Build an ed script. */
                        eflag = 3;
                        oflag = 1;
                        break;
                case 'X':	/* Does ???? (seems to suppress all output */
                        oflag = eflag = 1;
                        break;
		}
		argv++;
		argc--;
	}
	if(argc<6) {
		fprintf(stderr,"diff3: arg count\n");
		exit(1);
	}

	/*
	 *  If extra arguments are given following the filenames,
	 *  they are used to mark the overlaps instead of the 
	 *  filenames.
	 */
        if (oflag) { 
                sprintf(f1mark,"<<<<<<< %s",argc>=7?argv[6]:argv[3]);
                sprintf(f3mark,">>>>>>> %s",argc>=8?argv[7]:argv[5]);
        }

	/*
	 *  Read the two diff files, transform the diffs into the 
	 *  arrays d13 and d23, and return the number of diffs found
	 *  in each file.
	 */
	m = readin(argv[1],d13);
	n = readin(argv[2],d23);

	/*
	 *  Open the source files (file1, file2 and file3) and keep
	 *  file descriptors in the global array fd[].
	 */
	for(i=0;i<=2;i++)
		if((fp[i] = fopen(argv[i+3],"r")) == NULL) {
			printf("diff3: can't open %s\n",argv[i+3]);
			exit(1);
		}

	/*
	 *  Interpret the ranges recorded in d13 and d23 with help
	 *  from the source files, and output either diff3-style output
	 *  or an ed script.
	 */
	merge(m,n);
	exit(0);
}

/* 
 * pick up the line numbers of all changes from
 * one change file
 *
 *  Readin() stores the ranges output by diff as follows:
 *	
 *	diff line        interpretation
 *      ---------	 --------------
 *
 *	a,b'C'c,d	form used below:  a,b may be equal, in which case
 *			only a is shown; c,d ditto.  'C' = 'a'|'c'|'d'.
 *
 *	a,b'd'c      old.from = start line of range to be deleted from
 *			        "old" file; = a.
 *		     old.to   = line after end of "old" range; = b + 1.
 *		     new.from = next line after the deletion position in
 *			        "new" file; = c + 1.
 *
 *      a'a'c,d     old.from = line after add position in "new" file
 *                              (insert *before* this line); = a + 1.
 *		    old.to   = same as old.from
 *		    new.from = starting line of range to insert from
 *			   	"old" file; = c.
 *		    new.to   = line after range being added from
 *				"old" file; = d + 1.
 *	a,b'c'c,d   old.from = starting line of range in "old" file to
 *				be changed; = a.
 *		    old.to   = line following range to change in "old"
 *				file; = b + 1.
 *		    new.from = starting line of range in "new" file to
 *				use as replacement in "old"; = c.
 *		    new.to   = line following replacement range from "new"
 *				file; = d + 1.
 *
 *  These interpreted ranges are stored in the global structures d13 and
 *  d23.  Interestingly, after the type of change ('a','c' or 'd') is 
 *  used to adjust range endpoints, it is not saved.
 *
 * Returns the number of change lines found in each diff file.
 */
readin(name,dd)
char *name;
struct diff *dd;
{
	register i;
	int a,b,c,d;
	char kind;
	char *p;
	fp[0] = fopen(name,"r");

	/*
	 *  Find each change line.  The text line is stored temporarily
	 *  in the global buffer, "line".
	 */
	for(i=0;getchange(fp[0]);i++) {
		if(i>=NC) {
			fprintf(stderr,"diff3: too many changes\n");
			exit(1);
		}
		p = line;

		/*
		 * get the beginning of the source range
		 */
		a = b = number(&p);

		/*
		 * if it is a range, get the end line address.
		 */
		if(*p==',') {
			p++;
			b = number(&p);
		}

		/*
		 * "kind" can be one of 'a', 'c', or 'd'
		 */
		kind = *p++;

		/*
		 *  Get beginning address of destination
		 */
		c = d = number(&p);

		/*
		 * if it is a range, get end address
		 */
		if(*p==',') {
			p++;
			d = number(&p);
		}

		/*
		 *  If the kind of diff is an append, increment the 
		 *  beginning of the source range.
		 */
		if(kind=='a')
			a++;
		/*
		 *  If diff is a delete, increment beginning of destination.
		 */
		if(kind=='d')
			c++;
		/*
		 *  Bump ends of both source and destination ranges to
		 *  one more than what "diff" reported.
		 */
		b++;
		d++;

		/*
		 *  Assign all to global d13 or d23, on which diff file
		 *  this is.
		 */
		dd[i].old.from = a;
		dd[i].old.to = b;
		dd[i].new.from = c;
		dd[i].new.to = d;
	}
	dd[i].old.from = dd[i-1].old.to;
	dd[i].new.from = dd[i-1].new.to;
	fclose(fp[0]);
	return(i);
}

/*
 *  Turn string of digits into decimal number.
 */
number(lc)
char **lc;
{
	register nn;
	nn = 0;
	while(digit(**lc))
		nn = nn*10 + *(*lc)++ - '0';
	return(nn);
}

/*
 *  Tell if this character is a digit
 */
digit(c)
{
	return(c>='0'&&c<='9');
}

/*
 *  find and read next line containing a change instruction.
 *  All diff change lines are of the form <old range>'[acd]'<new range>,
 *  where the ranges always start with integers, so this looks for next
 *  line that starts with a digit.
 */
getchange(b)
FILE *b;
{
	/*
	 *  Getline reads line from file "b" and puts it in global "line".
	 */
	while(getline(b))
		if(digit(line[0]))
			return(1);
	return(0);
}

/*
 *  Read line from diff file into global, "line".  
 *  Return number of characters read.
 */
getline(b)
FILE *b;
{
	register i, c;
	for(i=0;i<sizeof(line)-1;i++) {
		c = getc(b);
		if(c==EOF)
			break;
		line[i] = c;
		if(c=='\n') {
			line[++i] = 0;
			return(i);
		}
	}
	return(0);
}

/*
 *  Where the real work is done...
 *  Merge interprets the change information encoded in d13 and d23, 
 *  sometimes using information from the source files.
 *  Output is either in diff3(1) form, or an ed script for use by
 *  merge(1).
 */
merge(m1,m2)
{
	register struct diff *d1, *d2, *d3;
	int dup;
	int j;
	int t1,t2;
	d1 = d13;	/*  pointers to global arrays */
	d2 = d23;
	j = 0;

	/*
	 *  Loop until all diffs from both diff files have been
	 *  looked at.
	 */
	for(;(t1 = d1<d13+m1) | (t2 = d2<d23+m2);) {
		if(debug) {
			printf("%d,%d=%d,%d %d,%d=%d,%d\n",
			d1->old.from,d1->old.to,
			d1->new.from,d1->new.to,
			d2->old.from,d2->old.to,
			d2->new.from,d2->new.to);
		}

		/*
		 *  If at the end of the d23 array, OR
		 *  the end of the "new file" range of the current 1-3
		 *  diff is earlier than the start of the "new" range
		 *  of the current 2-3 diff ...
		 *  Note, if doing ed script, nothing is written, meaning
		 *  no diff will be applied to file1.
		 */
		if(!t2||t1&&d1->new.to < d2->new.from) {
			/* stuff peculiar to 1st file */
			if(eflag==0) {
				separate("1");
				change(1,&d1->old,0);
				keep(2,&d1->old,&d1->new);
				change(3,&d1->new,0);
			}
			d1++;
			continue;
		}

		/*
		 *  If at end of the d13 array , OR
		 *  the end of the "new" range of the current 2-3 diff
		 *  is earlier than the beginning of the "new" range of
		 *  the current 1-3 diff...
		 *  Again, if doing ed script, nothing is written, no
		 *  diff; is applied to file1.
		 */
		if(!t1||t2&&d2->new.to < d1->new.from) {
			if(eflag==0) {
				separate("2");
				keep(1,&d2->old,&d2->new);
				change(2,&d2->old,0);
				change(3,&d2->new,0);
			}
			d2++;
			continue;
		}

		/* 
		 * merge overlapping changes in first file
 		 * this happens after extension see below
		 */
		if(d1+1<d13+m1 &&
		   d1->new.to>=d1[1].new.from) {
			d1[1].old.from = d1->old.from;
			d1[1].new.from = d1->new.from;
			d1++;
			continue;
		}

		/* 
		 *  merge overlapping changes in second
		 */
		if(d2+1<d23+m2 &&
		   d2->new.to>=d2[1].new.from) {
			d2[1].old.from = d2->old.from;
			d2[1].new.from = d2->new.from;
			d2++;
			continue;
		}

		/* 
		 *  The "new file" range of the current 1-3 diff is the
		 *  same as the "new" range of the current 2-3 diff.
		 */
		if(d1->new.from==d2->new.from&&
		   d1->new.to==d2->new.to) {

			/*
			 * See if the text in the range is identical in
			 * file1 and file2.
			 * dup = 0: all file differ,
			 * dup = 1: files 1 & 2 same.
			 */
			dup = duplicate(&d1->old,&d2->old);
			if(eflag==0) {
				separate(dup?"3":"");
				change(1,&d1->old,dup);
				change(2,&d2->old,0);
				d3 = d1->old.to>d1->old.from?d1:d2;
				change(3,&d3->new,0);
			} else

				/*
				 * for ed scripting, call edit.
				 * Adds an element to array, "de" 
				 * similar to d13, d23
				 */
				j = edit(d1,dup,j);
			d1++;
			d2++;
			continue;
		}

		/*
		 * overlapping changes from file1 & 2
		 * extend changes appropriately to
		 * make them coincide
		 */
		 if(d1->new.from<d2->new.from) {
			d2->old.from -= d2->new.from-d1->new.from;
			d2->new.from = d1->new.from;
		}
		else if(d2->new.from<d1->new.from) {
			d1->old.from -= d1->new.from-d2->new.from;
			d1->new.from = d2->new.from;
		}
		if(d1->new.to >d2->new.to) {
			d2->old.to += d1->new.to - d2->new.to;
			d2->new.to = d1->new.to;
		}
		else if(d2->new.to >d1->new.to) {
			d1->old.to += d2->new.to - d1->new.to;
			d1->new.to = d2->new.to;
		}
	}
        if(eflag) {
		edscript(j);
		if(j && !oflag)  /* oflag not set if E option not given */
			printf("w\nq\n");
		else		  /* E option used, expect different behavior */
       		   	exit(overlapcnt);
	}

}

separate(s)
char *s;
{
	printf("====%s\n",s);
}

/*	
 * 	the range of lines rold.from thru rold.to in file i
 *	is to be changed. it is to be printed only if
 *	it does not duplicate something to be printed later
 */
change(i,rold,dup)
struct range *rold;
{
	printf("%d:",i);
	last[i] = rold->to;
	prange(rold);
	if(dup)
		return;
	if(debug)
		return;
	i--;
	skip(i,rold->from,(char *)0);
	skip(i,rold->to,"  ");
}

/*	
 *	print the range of line numbers, rold.from  thru rold.to
 *	as n1,n2 or n1
 */
prange(rold)
struct range *rold;
{
	/*
	 *  If the end of the range is less than the beginning,
	 *  it must be an "append".  rold->from-1 is the line after
	 *  which to apply the append.
	 */
	if(rold->to<=rold->from)
		printf("%da\n",rold->from-1);
	else {

		/*
		 *  Must be a "change"..
		 *  Print beginning of range, ",<end>" only if the end
		 *  addr is equal to or greater than the start.
		 *  (I think "deletes" are taken care of by giving a
		 *   "change" with no replacement text. RC)
		 */
		printf("%d",rold->from);
		if(rold->to > rold->from+1)
			printf(",%d",rold->to-1);
		printf("c\n");
	}
}

/*	
 *	no difference was reported by diff between file 1(or 2)
 *	and file 3, and an artificial dummy difference (trange)
 *	must be ginned up to correspond to the change reported
 *	in the other file
 */
keep(i,rold,rnew)
struct range *rold, *rnew;
{
	register delta;
	struct range trange;
	delta = last[3] - last[i];
	trange.from = rnew->from - delta;
	trange.to = rnew->to - delta;
	change(i,&trange,1);
}

/*
 * 	skip to just befor line number "from" in file i
 *	if "pr" is nonzero, print all skipped stuff
 * 	with string pr as a prefix
 *	Returns number of characters read.
 */
skip(i,from,pr)
char *pr;
{
	register j,n;
	for(n=0;cline[i]<from-1;n+=j) {
		if((j=getline(fp[i]))==0)
			trouble();
		if(pr)
			printf("%s%s",pr,line);
		cline[i]++;
	}
	return(n);
}

/*	
 *	return 1 or 0 according as the old range
 *	(in file 1) contains exactly the same data
 *	as the new range (in file 2)
 */
duplicate(r1,r2)
struct range *r1, *r2;
{
	register c,d;
	register nchar;
	int nline;
	if(r1->to-r1->from != r2->to-r2->from)
		return(0);

	/*
	 *  Move file pointer to just before the target ranges in files 
	 *  1 & 2.
	 */
	skip(0,r1->from,(char *)0);
	skip(1,r2->from,(char *)0);
	nchar = 0;

	/*
	 * Read lines and compare character-by-character, keeping a count
	 * of both characters and lines.
	 */
	for(nline=0;nline<r1->to-r1->from;nline++) {
		do {
			c = getc(fp[0]);
			d = getc(fp[1]);
			if(c== -1||d== -1)
				trouble();
			nchar++;

			/*
			 *  The text in the ranges is not identical.
			 */
			if(c!=d) {
				repos(nchar);
				return(0);
			}
		} while(c!= '\n');
	}

	/*
	 * Return file pointers to where they were before this.
	 */
	repos(nchar);
	return(1);
}

/*
 *  Back up the file pointer in all files by "nchar" characters.
 */
repos(nchar)
{
	register i;
	for(i=0;i<2;i++) 
		fseek(fp[i], (long)-nchar, 1);
}

trouble()
{
	fprintf(stderr,"diff3: logic error\n");
	abort();
}

/*	collect an editing script for later regurgitation
 * 	Gets called once for every diff that is to be applied.
 */
edit(diff,dup,j)
struct diff *diff;
{
	if(((dup+1)&eflag)==0)
		return(j);
	j++;
        overlap[j] = !dup;
        if (!dup) overlapcnt++;

	/*
	 *  Give source range values in diff file
	 */
	de[j].old.from = diff->old.from;
	de[j].old.to = diff->old.to;

	/*
	 *  Calculate destination range based on ending address of
	 *  last diff to be applied.  Use that address + the offset 
	 *  to the beginning of the dest range in file 3.
	 *  Note that the "new" addresses are file offsets in bytes.
	 */
	de[j].new.from = de[j-1].new.to
	    +skip(2,diff->new.from,(char *)0);
	de[j].new.to = de[j].new.from
	    +skip(2,diff->new.to,(char *)0);
	return(j);
}

/*  
 *  Regurgitate information saved in global "de" by edit().
 *  "n" as passed in here, is "j" the global counter of the number of
 *  diffs to be applied.
 */
edscript(n)
{
	register j,k;
	char block[BUFSIZ];
	/*  for each diff ... */
	for(n=n;n>0;n--) {

		/*
		 * If doing ed script and there are no overlaps
		 * write an ed instruction for each diff.
		 */
                if (!oflag || !overlap[n]) 
                        prange(&de[n].old);

		/*
		 *  Otherwise write diff3-style output.
		 */
                else
                        printf("%da\n=======\n", de[n].old.to -1);

		/*
		 *  Get to the starting point for this diff in file3.
		 */
		fseek(fp[2], (long)de[n].new.from, 0);

		/*
		 *  Get and write replacement text for the range.
		 *  If the diff is a "delete", no text is output.
		 */
		for(k=de[n].new.to-de[n].new.from;k>0;k-= j) {
			j = k>BUFSIZ?BUFSIZ:k;
			if (fread(block, 1, (unsigned) j, fp[2])!=j)
				trouble();
			fwrite(block, 1, (unsigned) j, stdout);
		}

		/* 
		 *  For ed script, output a '.', to be left at last 
		 *  inserted line or at the line after the last
		 *  deleted line.
		 */
                if (!oflag || !overlap[n]) 
                        printf(".\n");
                else {
                        printf("%s\n.\n",f3mark);
                        printf("%da\n%s\n.\n",de[n].old.from-1,f1mark);
                }
	}
}
