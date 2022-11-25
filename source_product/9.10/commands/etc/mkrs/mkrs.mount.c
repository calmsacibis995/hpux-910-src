static char *HPUX_ID = "@(#) $Revision: 56.1 $";
/* HPUX_ID: @(#) $Revision: 56.1 $  */
main ()
{
static	char spec[]="/dev/real.root";
static	char dir[]="/disc";

	exit (mount (spec,dir,0));

}
