/* @(#) $Revision: 66.2 $ */    
static char *HPUX_ID = "@(#) $Revision: 66.2 $";
/*
 * List device drivers.
 * Note: Duplicate numbers are allowed in the list.
 */


#include <stdio.h>

#define HEADING "    Character     Block       Driver\n"
#define	FORMAT	" %8d    %8d         %s\n"	/* format for each line */
#define	ERROR	" %8d       %s\n"	/* format for each line */
#define	DEFAULT	"***  no such driver  ***"

struct{	int	charnum;		/* driver number = major for char  */
	int	blocknum;		/* driver number = major for block */
	char	*name;			/* driver name			*/
}	driver[] = {			/* explicit list:		*/
	 0,	-1,   "/dev/console",
	 1,	-1,   "HP98628, HP98626, and HP98642",
	 2,	-1,   "/dev/tty",
	 3,	-1,   "/dev/mem, /dev/kmem, and /dev/null",
	 4,	 0,   "CS80 disk",
	 5,	-1,   "HP7970/HP7971 nine-track magnetic tape",
	 6,	 1,   "HP9826/HP9836 internal flex disk",
	 7,	-1,   "HP-UX printer",
	 8,	-1,   "/dev/swap",
	 9,	-1,   "HP7974/HP7978 nine-track magnetic tape",
	 11,	 2,   "AMIGO disk",
	 13,	-1,   "SRM option",
	 16,	-1,   "Master pty",
	 17,	-1,   "Slave pty",
	 18,	-1,   "IEEE 802 device",
	 19,	-1,   "ETHERNET device",
	 21,	-1,   "HPIB DIL",
	 22,	-1,   "GPIO DIL",
	 23,	-1,   "raw 8042 HIL",
	 24,	-1,   "HIL",
	 25,	-1,   "HIL cooked keyboards",
	 26,	-1,   "ciper printer",
	 47,	 7,   "SCSI disk",
	 54,	-1,   "SCSI tape",
	 55,	10,   "optical autochanger",
	 -1,	255,   "/dev/root",
	 -2,	-2,  ""			/* terminator */
};


main (argc, argv)
	int	argc;
	char	**argv;
{
	int	i, num, found, secondarg;

/*
 * Print the whole list:
 */
	if (argc < 2){
	    printf (HEADING);
	    for (i = 0; driver[i].charnum >= 0; i++)
		printf (FORMAT, driver[i].charnum, driver[i].blocknum, driver[i].name);
	}

/*
 * Print selected entries:
 */
	else
	    secondarg = 0;
	    while (*(++argv)) {
		num = atoi (*argv);
		found = 0;
		for (i = 0; driver[i].charnum >= -1; i++)
		    if (driver[i].charnum == num) {
			if (!secondarg)
			     printf (HEADING);
			printf (FORMAT, driver[i].charnum, driver[i].blocknum,
			driver[i].name);
			found = 1;
		    }
		for (i = 0; driver[i].blocknum >= -1; i++)
		    if (driver[i].blocknum == num) {
			if (!found & !secondarg)
			     printf (HEADING);
			printf (FORMAT, driver[i].charnum, driver[i].blocknum,
			driver[i].name);
			found = 1;
		    }
		if (!found)
		    printf (ERROR, num, DEFAULT);
		secondarg = 1;
	    }
}
