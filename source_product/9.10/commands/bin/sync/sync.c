static char *HPUX_ID = "@(#) $Revision: 64.1 $";

/*
 * Define our own exit routine to avoid bringing in stdio stuff
 */
#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF ___exit exit
#   define exit ___exit
#endif

int
exit(code)
int code;
{
    _exit(code);
}

main(argc, argv)
int argc;
char *argv[];
{
#if defined(DUX) || defined(DISKLESS)
    /* 
     *  sync -l means sync only the local system.
     */
    if (argc == 2 &&
	argv[1][0] == '-' && argv[1][1] == 'l' && argv[1][2] == '\0')
	lsync();
    else
#endif
	sync();
    exit(0);
}
