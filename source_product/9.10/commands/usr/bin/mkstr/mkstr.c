static char *HPUX_ID = "@(#) $Revision: 66.1 $";

/***************************************************************\
|*       (c) Copyright 1985, Hewlett-Packard Company.          *|
|*                 All rights are reserved.                    *|
|*							       *|
|*  No part of this program may be photocopied, reproduced     *|
|*  or translated to another programming language or natural   *|
|*  language without prior written consent of Hewlett-Packard  *|
|*  Company.                				       *|
\***************************************************************/

/*
 * NAME
 *     mkstr - extract error messages from C source into a file
 *
 * HISTORY
 *     Based on an earlier program conceived by Bill Joy and Chuck Haley
 *
 *     Written by Bill Joy UCB August 1977
 *
 *     Modified March 1978 to hash old messages to be able to recompile
 *     without adding messages to the message file (usually)
 *
 *     Ported to HP-UX and debugged by Dave Decot, HP Data Systems Division:
 *	   No longer searches in nor modifies calls in comments.
 *	   Better command line argument checking.
 *	   Lints better than used to.
 *
 * DESCRIPTION 
 *     Program to create a string error message file
 *     from a group of C programs.  Arguments are the name
 *     of the file where the strings are to be placed, the
 *     prefix of the new files where the processed source text
 *     is to be placed, and the files to be processed.
 *
 *     The program looks for 'error("' in the source stream.
 *     Whenever it finds this, the following characters from the '"'
 *     to a '"' are replaced by 'seekpt' where seekpt is an lseek-style
 *     pointer into the error message file.
 *     If the '(' is not immediately followed by a '"' no change occurs.
 *
 *     The optional '-' causes strings to be added at the end of the
 *     existing error message file for recompilation of single routines.
 */

#include <stdio.h>

#ifdef NLS
#include <locale.h>
#endif NLS

#ifdef NLS16
#include <nl_ctype.h>
#endif NLS16

#define	ungetchar(c)	ungetc(c, stdin)

long	ftell();
char	*calloc(), *strcpy();

FILE	*mesgread, *mesgwrite;
char	*progname;
char	usagestr[] =	"Usage: %s [ - ] mesgfile prefix file ...\n";
char	name[100], *np;

main(argc, argv)
	int argc;
	char *argv[];
{
	short int addon = 0;

#ifdef NLS || NLS16			/* initialize to the correct locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("mkstr"), stderr);
		putenv("LANG=");
	}
#endif NLS || NLS16

	argc--, progname = *argv++;
	if (argc > 1 && argv[0][0] == '-')
	    if (argv[0][1] == '\0')
		addon++, argc--, argv++;
	    else
		fprintf(stderr, usagestr, progname), exit(1);

	if (argc < 3)
		fprintf(stderr, usagestr, progname), exit(1);

	mesgwrite = fopen(argv[0], addon ? "a" : "w");
	if (mesgwrite == NULL)
		perror(argv[0]), exit(1);

	mesgread = fopen(argv[0], "r");
	if (mesgread == NULL)
		perror(argv[0]), exit(1);

	inithash();
	argc--, argv++;
	strcpy(name, argv[0]);
	np = name + strlen(name);
	argc--, argv++;

	do {
		strcpy(np, argv[0]);
		if (freopen(argv[0], "r", stdin) == NULL)
			perror(argv[0]), exit(1);
		if (freopen(name, "w", stdout) == NULL)
			perror(name), exit(1);
		process();
		argc--, argv++;
	} while (argc > 0);

	exit(0);
}

process()
{
    register int c;

    while ((c = getchar()) != EOF) {
	switch (c) {
	case '/':
	    if (match("/*"))	/* handle comments */
	    {
#ifdef NLS16
	    int	secondbyte = 0;
#endif NLS16

		printf("/*");
		while ((c = getchar()) != EOF)
		{
#ifdef NLS16
		    if (secondbyte) {
			secondbyte = 0;
			putchar(c);
		    } else if (FIRSTof2(c)) {
			secondbyte++;
			putchar(c);
		    } else
#endif NLS16
		    if (c == '*')
		    {
			if (match("*/"))
			    break;
		    }
		    else
			putchar(c);
		}

		printf("*/");
		if (c == EOF)
		    return;

	    }
	    break;
	case 'e':
	    if (match("error(")) {
		printf("error(");
		c = getchar();
		if (c != '"')
			putchar(c);
		else
			copystr();
	    }
	    break;
	default:
	    putchar(c);
	}
    }
}

match(ocp)
	char *ocp;
{
	register char *cp;
	register int c;

	for (cp = ocp + 1; *cp; cp++) {
		c = getchar();
		if (c != *cp) {
			while (ocp < cp)
				putchar(*ocp++);
			ungetchar(c);
			return (0);
		}
	}
	return (1);
}


copystr()
{
    char buf[512];
    register int c, ch;
    register char *cp = buf;

#ifdef NLS16
    int secondbyte = 0;
#endif NLS16

	for (;;) {
		c = getchar();
		if (c == EOF)
			break;
	
#ifdef NLS16
		if (secondbyte)
			secondbyte = 0;
		else if (FIRSTof2(c))
			secondbyte++;
		else
#endif NLS16

		switch (c) {

		case '"':
			*cp++ = 0;
			goto out;
		case '\\':
			c = getchar();
			switch (c) {

			case 'b':
				c = '\b';
				break;
			case 't':
				c = '\t';
				break;
			case 'r':
				c = '\r';
				break;
			case 'n':
				c = '\n';
				break;
			case '\n':
				continue;
			case 'f':
				c = '\f';
				break;
			case '0':
				c = 0;
				break;
			case '\\':
				break;
			default:
#ifdef NLS16
				if (FIRSTof2(c)) {
					secondbyte++;
					break;
				} else
#endif NLS16
				if (!octdigit(c))
					break;
				c -= '0';
				ch = getchar();
				if (!octdigit(ch))
					break;
				c <<= 7, c += ch - '0';
				ch = getchar();
				if (!octdigit(ch))
					break;
				c <<= 3, c+= ch - '0', ch = -1;
				break;
			}
		}
		*cp++ = c;
	}
out:
	*cp = 0;
	printf("%d", hashit(buf, 1, NULL));
}


octdigit(c)
char c;
{
    return (c >= '0' && c <= '7');
}


inithash()
{
	char buf[512];
	unsigned int mesgpt = 0;

	rewind(mesgread);
	while (fgetNUL(buf, sizeof buf, mesgread) != NULL) {
		hashit(buf, 0, mesgpt);
		mesgpt += strlen(buf) + 2;
	}
}


#define	NBUCKETS	511

struct	hash {
    long         hval;
    unsigned int hpt;
    struct hash *hnext;
} *bucket[NBUCKETS];

hashit(str, really, fakept)
char *str;
char really;
unsigned int fakept;
{
	int i;
	register struct hash *hp;
	char buf[512];
	long hashval = 0;
	register char *cp;

	if (really)
		fflush(mesgwrite);
	for (cp = str; *cp;)
		hashval = (hashval << 1) + *cp++;
	i = hashval % NBUCKETS;
	if (i < 0)
		i += NBUCKETS;
	if (really != 0)
		for (hp = bucket[i]; hp != 0; hp = hp->hnext)
		if (hp->hval == hashval) {
			fseek(mesgread, (long) hp->hpt, 0);
			fgetNUL(buf, sizeof buf, mesgread);
/*
			fprintf(stderr, "Got (from %d) %s\n", hp->hpt, buf);
*/
			if (strcmp(buf, str) == 0)
			    break;
		}
	if (!really || hp == 0) {
		hp = (struct hash *) calloc(1, sizeof *hp);
		hp->hnext = bucket[i];
		hp->hval = hashval;
		hp->hpt = really ? ftell(mesgwrite) : fakept;
		if (really) {
		    fwrite(str, sizeof (char), strlen(str) + 1, mesgwrite);
		    fwrite("\n", sizeof (char), 1, mesgwrite);
		}
		bucket[i] = hp;
	}
/*
	fprintf(stderr, "%s hashed to %ld at %d\n", str, hp->hval, hp->hpt);
*/
	return (hp->hpt);
}

fgetNUL(obuf, rmdr, file)
char *obuf;
register int rmdr;
FILE *file;
{
    register int c;
    register char *buf = obuf;

	while (--rmdr > 0 && (c = getc(file)) != 0 && c != EOF)
		*buf++ = c;
	*buf++ = 0;
	getc(file);
	return ((feof(file) || ferror(file)) ? NULL : 1);
}
