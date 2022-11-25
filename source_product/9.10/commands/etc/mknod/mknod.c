static char *HPUX_ID = "@(#) $Revision: 66.9 $";
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h> /* for makedev() macro */
#include <sys/param.h>

#if defined(SecureWare) && defined(B1)
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
#endif

#ifdef CNODE_DEV
#include <ctype.h>
#include <cluster.h>
#endif /* CNODE_DEV */

#if defined(SecureWare) && defined(B1)
char auditbuf[80];
#endif

main(argc, argv)
int argc;
char **argv;
{
	register int m;
	register int a;
	register int b;
#ifdef CNODE_DEV
	register int n;
#endif	/* CNODE_DEV */
#ifdef RFA
	int fd;
	int length;
#endif /* RFA */

#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		set_auth_parameters(argc, argv);
		initprivs();
		(void) enablepriv(SEC_ALLOWDACACCESS);
		(void) enablepriv(SEC_ALLOWMACACCESS);
		(void) enablepriv(SEC_WRITEUPSYSHI);
		(void) enablepriv(SEC_WRITEUPCLEARANCE);
		(void) enablepriv(SEC_OWNER);
		(void) enablepriv(SEC_SETOWNER);
#if SEC_ILB
		(void) enablepriv(SEC_ILNOFLOAT);
#endif
		(void) disablepriv(SEC_SUSPEND_AUDIT);
	}
#endif
	if (argc == 3 && !strcmp(argv[2], "p")) { /* fifo */
		a = mknod(argv[1], S_IFIFO|0666, 0);
		if (a)
			perror("mknod");
		exit(a == 0 ? 0 : 2);
	}
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
            if (!hassysauth(SEC_MKNOD))  {
		sprintf(auditbuf, "does not have %s authorization",
			sys_priv[SEC_MKNOD].name);
		audit_security_failure(OT_PROCESS, auditbuf,
			"abort mknod", ET_SUBSYSTEM);
		fprintf(stderr, "mknod: need `%s' authorization\n",
			sys_priv[SEC_MKNOD].name);
		exit(2);
	    }
	    if (!enablepriv(SEC_MKNOD))  {
		sprintf(auditbuf, "does not have %s potential privilege", sys_priv[SEC_MKNOD].name);
		audit_security_failure(OT_PROCESS, auditbuf, "abort mknod", ET_INSUFF_PRIV);
		fprintf(stderr, "mknod: need `%s' potential privilege\n", sys_priv[SEC_MKNOD].name);
		exit(2);
	    }
	}
	else{
	    if (geteuid() != 0) {
		fputs("mknod: must be super-user\n", stderr);
		exit(2);
	    }
	}
#else
	if (geteuid() != 0) {
		fputs("mknod: must be super-user\n", stderr);
		exit(2);
	}
#endif

#ifdef RFA
	/* network special */
	if (argc == 4 && strcmp(argv[2], "n") == 0) {
		if (mknod(argv[1], (S_IFNWK | 0666), 0)) {
			perror("mknod");
			exit(2);
		}
		if ((fd = open(argv[1], 1)) < 0) {
			perror("mknod");
			exit(2);
		}
		length = strlen(argv[3]);
		if (write(fd, argv[3], length+1) != length+1) {
			perror("mknod");
			exit(2);
		}
		chmod(argv[1], 0444);		/* may quietly fail */
		exit(0);                        /* all done!        */
	}
#endif /* RFA */

#ifdef  CNODE_DEV
	if (argc != 5 && argc != 6)
#else   /* no CNODE_DEV */
	if (argc != 5)
#endif  /* CNODE_DEV */
	{
		fputs("mknod: arg count\n", stderr);
		usage();
	}
	if (*argv[2] == 'b')
		m = S_IFBLK|0666;
	else if (*argv[2] == 'c')
		m = S_IFCHR|0666;
	else
		usage();
	a = number(argv[3]);
	if (a < 0)
		usage();
	b = number(argv[4]);
	if (b < 0)
		usage();
#ifdef  CNODE_DEV
	if (argc == 6)
	{
	    if ((n = cnodenametoid(argv[5])) == -1)
	    {
		fputs("Invalid cnode ", stderr);
		fputs(argv[5], stderr);
		fputc('\n', stderr);
		exit(2);
	    }
	    if (mkrnod(argv[1], m, makedev(a,b), n) < 0)
	    {
		perror("mknod");
		exit(2);
	    }
	}
	else if (mknod(argv[1], m, makedev(a,b)) < 0)
	{
	    perror("mknod");
	    exit(2);
	}
#else   /* no CNODE_DEV */
	if (mknod(argv[1], m, makedev(a,b)) < 0) {
		perror("mknod");
		exit(2);
	}
#endif  /* CNODE_DEV */
	exit(0);
}

number(arg)
register char *arg;
{
	int	base = 10;		/* selected base	*/
	long	num  =  0;		/* function result	*/
	int	digit;			/* current digit	*/

	if (*arg == '0')		/* determine base */
		if ((*(++arg) != 'x') && (*arg != 'X'))
			base = 8;
		else {
			base = 16;
			++arg;
		}

	while (digit = *arg++) {
		if (base == 16) {	/* convert hex a-f or A-F */
			if ((digit >= 'a') && (digit <= 'f'))
				digit += '0' + 10 - 'a';
			else
			if ((digit >= 'A') && (digit <= 'F'))
				digit += '0' + 10 - 'A';
		}
		digit -= '0';

		if ((digit < 0) || (digit >= base)) {	/* out of range */
			fputs("mknod: illegal number\n", stderr);
			usage ();
		}
		num = num*base + digit;
	}
	return (num);
}

usage()
{
#ifdef  CNODE_DEV
	fputs("usage: mknod name b|c major minor [ cnode ]\n", stderr);
#else   /* no CNODE_DEV */
	fputs("usage: mknod name b|c major minor\n", stderr);
#endif  /* CNODE_DEV */
	fputs("       mknod name p\n", stderr);
#ifdef RFA
        fputs("       mknod name n nodename\n", stderr);
#endif
	exit(2);
}

#ifdef  CNODE_DEV
int
cnodenametoid(name)
char *name;
{
    struct cct_entry *cnode_cct_entry;

    /*
     * If name is NULL, return 0
     */
    if (name[0] == '\0')
    {
	return 0;
    }

    /*
     * If name is a number, return the number and treat as the cnode ID
     */
    if (isdigit(name[0]))
    {
	char *endp = NULL;
	long val = strtol(name, &endp, 10);

	if (endp == NULL || *endp != '\0')
	    goto lookup_cname;	/* might be a cnode name */
	    
	if (val < 0 || val > MAX_CNODE) /* numeric, but out of range */
	    return -1;
	return val;
    }

lookup_cname:
    /*
     * Name must now be a cnode name, get the entry from
     * /etc/clusterconf and return the ID.  If the name isn't found,
     * return -1.
     */
    if ((cnode_cct_entry = getccnam(name)) != (struct cct_entry *)0)
	return cnode_cct_entry->cnode_id;
    else
	return -1;
}
#endif  /* CNODE_DEV */
