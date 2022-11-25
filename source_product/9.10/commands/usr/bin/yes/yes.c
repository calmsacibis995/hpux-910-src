static char *HPUX_ID = "@(#) $Revision: 64.1 $";

main(argc, argv)
int argc;
char *argv[];
{
    register char *str = argc > 1 ? argv[1] : "y";
    register int len = strlen(str);

    for (;;)
    {
	write(1, str, len);
	write(1, "\n", 1);
    }
}
