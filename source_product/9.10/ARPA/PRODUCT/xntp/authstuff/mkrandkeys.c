/* $Header: mkrandkeys.c,v 1.2.109.5 94/12/12 16:06:47 mike Exp $
 * mkrandkeys - make a key file for xntpd with some quite random keys
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ntp_stdlib.h"

#define	STREQ(a, b)	(*(a) == *(b) && strcmp((a), (b)) == 0)

char   *progname;
int     debug;

U_LONG  keydata[2];

int     std = 1;		/* DES standard key format 
				*/
u_char  dokey[16] =
{
    0
};

static void     rand_data P ((U_LONG *));

/*
 * main - parse arguments and handle options
 */
void
main (argc, argv)
int     argc;
char   *argv[];
{
    int     c;
    int     i;
    int     j;
    int     errflg = 0;
    int     numkeys;
    U_LONG  tmp;
    char   *passwd;
    extern int  optind;
    extern char    *optarg;
    extern char    *getpass ();

    numkeys = 0;
    progname = argv[0];
    passwd = NULL;
    while ((c = getopt (argc, argv, "dnp:s")) != EOF)
	switch (c)
	    {
	    case 'd':
		++debug;
		break;
	    case 'n':
		std = 0;
		break;
	    case 'p':
		passwd = optarg;
		break;
	    case 's':
		std = 1;
		break;
	    default:
		errflg++;
		break;
	    }

    numkeys = 0;
    for (; !errflg && optind < argc; optind++)
        {
	c = atoi (argv[optind]);
	if (c <= 0 || c > 15)
	    {
	    (void)fprintf ("%s: invalid key number `%s'\n",
		    progname, argv[optind]);
	    exit (2);
	    }
	dokey[c] = 1;
	numkeys++;
        }

    if (errflg || numkeys == 0)
        {
	(void)fprintf (stderr,
		"usage: %s [-ns] [-p seed] key# [key# ...]\n",
		progname);
	exit (2);
        }

    while (passwd == 0 || *passwd == '\0')
        {
	passwd = getpass ("Seed: ");
	if (*passwd == '\0')
	    {
	    (void)fprintf (stderr,
		    "better use a better seed than that\n");
	    }
        }

    keydata[0] = keydata[1] = 0;
    for (i = 0; i < 8 && *passwd != '\0'; i++)
        {
	keydata[i / 4] |= ((((U_LONG)(*passwd)) & 0xff) << (1 + ((3 - (i % 4)) * 8)));
	passwd++;
        }

    for (i = 1; i <= 15; i++)
        {
	if (dokey[i])
	    {
	    for (c = 0, tmp = 0; c < 32; c += 4)
		tmp |= (i << c);
	    keydata[0] ^= tmp;
	    keydata[1] ^= tmp;
	    rand_data (keydata);
	    DESauth_parity (keydata);

	    if (std)
	        {
		(void)printf ("%-2d S\t%08x%08x\n",
			i, keydata[0], keydata[1]);
	        }
	    else
	        {
		for (j = 0; j < 2; j++)
		    {
		    keydata[j]
			= ((keydata[j] & 0xfefefefe) >> 1)
			| ((keydata[j] & 0x01010101) << 7);
		    }
		(void)printf ("%-2d N\t%08x%08x\n",
			i, keydata[0], keydata[1]);
	        }
	    }
        }
    exit (0);
}

char   *volatile_file[] =
{
#ifdef hpux
    "/usr/bin/echo",
    "/usr/bin/sh",
    "/usr/bin/cat",
    "/usr/bin/ls",
    "/usr/bin/stty",
    "/usr/bin/date",
    "/usr/bin/cat",
#else				/* ! hpux */
    "/bin/echo",
    "/bin/sh",
    "/bin/cat",
    "/bin/ls",
    "/bin/stty",
    "/bin/date",
    "/bin/cat",
#endif				/* hpux */
    "/bin/cc",
    "/etc/motd",
    "/etc/utmp",
    "/dev/kmem",
    "/dev/null",
    "",
};

#define NEXT(X) (0x1e1f2f2d*(X) + 0x361962e9)

static void
rand_data (data)
U_LONG *data;
{
    register i;
    struct stat     buf;
    extern LONG     time ();
    char    ekeys[128],
            dkeys[128];

    *data ^= 0x9662f394;
    *(data + 1) ^= 0x9f17c55f;
    DESauth_subkeys (data, ekeys, dkeys);
    *data ^= NEXT (getpid () + (getuid () << 16));
    *(data + 1) ^= NEXT (time ((LONG *)0));
    DESauth_des (data, ekeys);
    for (i = 0; strlen (volatile_file[i]); i++)
        {
	if (stat (volatile_file[i], &buf) == -1)
	    continue;
	if (i & 1)
	    {
	    *data ^= NEXT (buf.st_atime);
	    *(data + 1) ^= NEXT (buf.st_mtime);
	    }
	else
	    {
	    *data ^= NEXT (buf.st_mtime);
	    *(data + 1) ^= NEXT (buf.st_atime);
	    }
	DESauth_des (data, ekeys);
        }
}
