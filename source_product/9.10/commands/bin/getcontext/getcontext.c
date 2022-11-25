static char *HPUX_ID = "@(#) $Revision: 66.1 $";

#define MAXCXTLEN  1024

main()
{
    register int len;
    char buf[MAXCXTLEN];

    if ((len = getcontext(buf, MAXCXTLEN)) == -1)
	return 1;

    buf[len - 1] = '\n';  /* overwrite '\0' with '\n' */
    if (write(1, buf, len) == len)
	return 0;
    return 1;
}

/*
 * Define our own exit so that crt0.o calls it instead of
 * the one that brings in all of the standard i/o stuff
 */
#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF ___exit exit
#   define exit ___exit
#endif

int
exit(code)
{
    _exit(code);
}
