static char *HPUX_ID = "@(#) $Revision: 56.1 $";
/* HPUX_ID: @(#) $Revision: 56.1 $  */
#include  <sys/types.h>
#include  <sys/sysmacros.h>
main ()
{
static char console[] = "/disc/dev/console";
static char syscon[] = "/disc/dev/syscon";
static char systty[] = "/disc/dev/systty";
static int result;

	unlink(console);
	if (result=mknod (console, 0020622, makedev(0,0)) != 0) exit (result);
	unlink(syscon);
	unlink(systty);
	link(console,syscon);
	link(console,systty);
	exit (0);
}
