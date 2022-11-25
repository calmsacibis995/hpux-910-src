#ifndef lint
static char *hpux_rel = "@(#) $Revision: 62.1 $";
#endif
/* $Header: nmf.c,v 62.1 88/05/02 11:30:00 runyan Exp $ (Hewlett-Packard) */

#if defined(UNIX5) && !defined(pdp11)
#include "stdio.h"

main(argc, argv)
char	*argv[];
{
	char name[BUFSIZ], buf[BUFSIZ], *fname = NULL, *pty, *strncpy();
	register char *p;
	int nsize, tysize, lineno;

	strcpy(name, argc > 1? argv[1] : "???");
	if (argc > 2)
		fname = argv[2];
	else
		fname = "???";
	while (gets(buf))
	{
		p = buf;
		while (*p != ' ' && *p != '|')
			++p;
		nsize = p - buf;
		do ; while (*p++ != '|');		/* skip rem of name */
		do ; while (*p++ != '|');		/* skip value */
		do ; while (*p++ != '|');		/* skip class */
		while (*p == ' ')
			++p;
		if (*p != '|')
		{
			pty = p++;
			while (*p != '|')
				++p;
			tysize = p - pty;
		}
		else
			pty = (char *) NULL;
		++p;
		do ; while (*p++ != '|');		/* skip size */
		while (*p == ' ')
			++p;
		lineno = atoi(p);
		do ; while (*p++ != '|');		/* and xlated line */
		while (*p == ' ')
			++p;
		if (!strncmp(p, ".text", 5) || !strncmp(p, ".data", 5))
		{					/* it's defined */
			strncpy(name, buf, nsize);
			name[nsize] = '\0';
			printf("%s = ", name);
			if (pty)
				printf("%.*s", tysize, pty);
			else
			{
				fputs("???", stdout);
				if (!strncmp(p, ".text", 5))
					fputs("()", stdout);
			}
			printf(", <%s %d>\n", fname, lineno);
		}
		else
			printf("%s : %.*s\n", name, nsize, buf);
	}
	exit(0);
}
#endif define check
#ifdef hp9000s200
#include "stdio.h"

#define SYMSTART 14
#define SYMCLASS 11

main(argc, argv)
char	*argv[];
{
	char	name[BUFSIZ], buf[BUFSIZ];
	char	*fname = NULL;
	char	*p;

	strncpy(name, argc > 1? argv[1] : "", BUFSIZ);
	if (argc > 2)
		fname = argv[2];
	while (gets(buf)) {
		p = &buf[SYMSTART];
		if (*p == '_')
			++p;
		switch (buf[SYMCLASS]) {
		case 'U':
			printf("%s : %s\n", name, p);
			continue;
		case 'T':
			printf("%s = text", p);
			strcpy(name, p);
			break;
		case 'D':
			printf("%s = data", p);
			if (strcmp(name, "") == 0)
				strcpy(name, p);
			break;
		case 'B':
			printf("%s = bss", p);
			break;
		case 'A':
			printf("%s = abs", p);
			break;
		default:
			continue;
		}
		if (fname != NULL)
			printf(", %s", fname);
		printf("\n");
	}
	exit(0);
}
# endif hp9000s200
#ifdef hp9000s500
#include "stdio.h"
#define SYMSTART 43
#define SYMDEF 5
#define SYMCLASS 7

main(argc, argv)
char	*argv[];
{
	char	name[15], buf[64];
	char	*fname = NULL;
	char	*p;

	strcpy(name, argc > 1? argv[1] : "");
	if (argc > 2)
		fname = argv[2];
	while (gets(buf)) {
		p = &buf[SYMSTART];
		if (*p == '_')
			++p;
		switch (buf[SYMDEF]) {
		case 'U':
			printf("%s : %s\n", name, p);
			continue;
		case '.':
			switch (buf[SYMCLASS]) {
			case 'F': /* FUNC */
			case 'E': /* ENTRY */
			case 'S': /* SYSTEM */
			case 'L': /* LABEL */
				printf("%s = text", p);
				strcpy(name, p);
				break;
			case 'P': /* PTR */
				printf("%s = data", p);
				if (strcmp(name, "") == 0)
					strcpy(name, p);
				break;
			case 'D': /* DDATA, DCOMM */
			case 'I': /* IDATA, ICOMM */
			case 'U': /* UDATA */
				if (strncmp(buf[SYMCLASS+1], "DATA", 4))
					printf("%s = comm", p);
				else
					{
					printf("%s = data", p);
					if (strcmp(name, "") == 0)
						strcpy(name, p);
					}
				break;
			case 'A': /* ABS */
				printf("%s = abs", p);
				break;
			default:
				continue;
			}
		}
		if (fname != NULL)
			printf(", %s", fname);
		printf("\n");
	}
	exit(0);
}
# endif hp9000s500
#ifdef hp9000s800
#include "stdio.h"

main(argc, argv)
char	*argv[];
{
	char name[BUFSIZ], buf[BUFSIZ], *fname = NULL, *strncpy();
	register char *p;
	int nsize;

	strcpy(name, argc > 1? argv[1] : "???");
	if (argc > 2)
		fname = argv[2];
	else
		fname = "???";
	while (gets(buf))
	{
		p = buf;
		while (*p != ' ' && *p != '|')
			++p;
		nsize = p - buf;
		do ; while (*p++ != '|');		/* skip rem of name */
		do ; while (*p++ != '|');		/* skip value */
		do ; while (*p++ != '|');		/* skip scope */
		do ; while (*p++ != '|');		/* skip type */
		strncpy(name, buf, nsize);
		name[nsize] = '\0';
                if (name[0] == '_')
		    printf("%s = ", name+1);
 		else
		    printf("%s = ", name);
		if (!strncmp(p, "$CODE$", 6)) {
		    fputs("$CODE$", stdout);
		    fputs("()", stdout);
		} else if (!strncmp(p, "$DATA$", 6)) {
		    fputs("$DATA$", stdout);
		} else if (!strncmp(p, "$BSS$", 6))
  		    fputs("$BSS$", stdout);
		printf(", <%s>\n", fname);
	}
	exit(0);
}
#endif
