#include "monitor.h"
#include "kdefs.h"
#ifdef hp9000s800
#include <sys/libIO.h>
#endif

extern char *strrchr(), *strcpy();

#ifdef hp9000s800
#define UNDERSCORE_OFFSET 0
#else
#define UNDERSCORE_OFFSET 1
#endif

extern struct nlist nl[];


static char *HPUX_ID = "$Revision: 70.1.1.1 $";

#define NCONFIG	(X_LAST_CONFIG - X_FIRST_CONFIG + 1)

struct config {
	char	*cname;
	int	cvalue;
} config[NCONFIG];

int nextconfig=0;

#ifdef hp9000s200
#define NDRIVERS (X_LAST_DRIVER - X_FIRST_DRIVER + 1)
#else
#define NDRIVERS 50
#endif
struct drivers {
	char	dname[21];
} drivers[NDRIVERS];
int nextdriver=0;

#ifdef hp9000s200 
struct xlate {
	char	*from, *to;
} xlate[] = {	"autox0",	"autox",
		"dragon",	"fpa",
		"eeprom",	"eisa",
		"lp",		"printer",
		"nm",		"netman",
		"pdn0",		"x25",
		"pt",		"plot",
		"ptym",		"ptymas",
		"ptys",		"ptyslv",
		"ram",		"ramdisc",
		"rfai",		"rfa",
		"simon",	"98625",
		"sio626",	"98626",
		"sio628",	"98628",
		"sio642",	"98642",
		"srm629",	"srm",
		"stealth",	"vme2",
		"stp",		"stape",
		"ti9914",	"98624",
		"tp",		"tape",
		"vtest",	"vmetest",
		NULL,		NULL
};
#endif hp9000s200 

#ifdef hp9000s800
io_sw_mod_type *io_tab;

#define X_FIRST_DRIVER 0
#define X_LAST_DRIVER 19

struct sn_xlate {
	char	*from, *to;
} sn_xlate[] = { "lan2",	"lan01",
		 "autox0",	"autox",
		 "nfsc",	"nfs",
		 "pci",		"apci",
		 "ram",		"ramdisc",
		 "s2disk",	"scsi",
		 "s2tape",	"scsitape",
		 "CentIf",	"parallel",
		 "eeprom",	"eisa",
		 NULL,		NULL
};

struct nlist nl_snakes[] = {
	 {"CharDrv_ioctl"},
	 {"CentIf_link"},
	 {"asio0_ioctl"},
	 {"atalk_link"},
	 {"autoch_ioctl"},
	 {"autox0_ioctl"},
	 {"cdfs_fsctl"},
	 {"cs80_open"},
	 {"dconfig_ioctl"},
	 {"diag1_ioctl"},
	 {"dmem_ioctl"},
	 {"dskless_initialized"},
	 {"eeprom_ioctl"},
	 {"etest_ioctl"},
	 {"hpib_open"},
	 {"ica_handler"},
	 {"inet_link"},
	 {"ktest_ioctl"},
	 {"ktestio_ioctl"},
	 {"lan2_link"},
	 {"nfsc_link"},
	 {"nipc_link"},
	 {"pci_ioctl"},
	 {"pty0_ioctl"},
	 {"pty1_ioctl"},
	 {"ram_open"},
	 {"rdu_ioctl"},
	 {"s2disk_ioctl"},
	 {"s2tape_ioctl"},
	 {"sim0_init"},
	 {"uipc_link"},
	 {"xns_link"},
	 { NULL } 
};
#endif /* hp9000s800 */

mode_c_init()
{
	register int i, k, j, ret;
	register char *d, *p;
	register struct nlist *n;

#ifdef hp9000s800

	if (sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO) { /* if Snakes */
	    if (nlist("/hp-ux",nl_snakes) != 0) {
	       perror("nlist");
               fprintf(stderr,"Can't do an nlist on /hp-ux\n");
               exit(-1);
	    }
	}

#endif /* hp9000s800 */

#ifdef X_SHMINFO

	nl[X_SHMMAX].n_name = "_shmmax"+1-UNDERSCORE_OFFSET;
	nl[X_SHMMAX].n_value = nl[X_SHMINFO].n_value +
				(int) &((struct shminfo *) 0)->shmmax;

	nl[X_SHMMIN].n_name = "_shmmin"+1-UNDERSCORE_OFFSET;
	nl[X_SHMMIN].n_value = nl[X_SHMINFO].n_value +
				(int) &((struct shminfo *) 0)->shmmin;

	nl[X_SHMMNI].n_name = "_shmmni"+1-UNDERSCORE_OFFSET;
	nl[X_SHMMNI].n_value = nl[X_SHMINFO].n_value +
				(int) &((struct shminfo *) 0)->shmmni;

	nl[X_SHMSEG].n_name = "_shmseg"+1-UNDERSCORE_OFFSET;
	nl[X_SHMSEG].n_value = nl[X_SHMINFO].n_value +
				(int) &((struct shminfo *) 0)->shmseg;

#ifdef hp9000s800
	nl[X_SHMBRK].n_name = "_shmbrk"+1-UNDERSCORE_OFFSET;
	nl[X_SHMBRK].n_value = nl[X_SHMINFO].n_value +
				(int) &((struct shminfo *) 0)->shmbrk; 

	nl[X_SHMALL].n_name = "_shmall"+1-UNDERSCORE_OFFSET;
	nl[X_SHMALL].n_value = nl[X_SHMINFO].n_value +
				(int) &((struct shminfo *) 0)->shmall;
#endif /*hp9000s800*/

#endif

	/* Get the tunable parameter information */
	for (i = X_FIRST_CONFIG; i <= X_LAST_CONFIG; i++) {
		n = &nl[i];
		if (n->n_value==0)		/* Is this in the kernel? */
			continue;			/* Nope, skip it. */

		/* Store value plus name possibly without leading underscore */
		config[nextconfig].cname = &n->n_name[UNDERSCORE_OFFSET];
		config[nextconfig].cvalue = getvalue(n->n_value);

#ifdef X_NETISR_PRIORITY
		/* netisr_priority is really a byte, but we fetched an int */
		if (i==X_NETISR_PRIORITY) {
			config[nextconfig].cvalue >>= 24;
			config[nextconfig].cvalue &= 0xff;
		}
#endif
		nextconfig++;
	}

#ifdef hp9000s200
	/* Get list of drivers present in system */
	for (i = X_FIRST_DRIVER; i <= X_LAST_DRIVER; i++) {
		if (nl[i].n_value==0)		/* Is this driver loaded? */
			continue;		/* No, ignore it. */

		d = drivers[nextdriver].dname;

		/* Fetch name stripping leading underscore */
		strcpy(d, &nl[i].n_name[UNDERSCORE_OFFSET]);

		/* Also strip everything after last underscore */
		p = strrchr(d, '_');
		if (p!=NULL)
			*p = '\0';

		/* Something went wrong if no string is left */
		if (*d=='\0') {
			fprintf(stderr,"Bad driver name symbol #%d: \"%s\"\n",
				i, nl[i].n_name);
			abnorm_exit(1);
		}

		/* Change handles to dfile names if different */
		for (k = 0; xlate[k].from != NULL; k++) {
			if (!strcmp(d, xlate[k].from)) {
				strcpy(d, xlate[k].to);
				break;
			}
		}

		nextdriver++;		/* Next slot in the array */
	}
#endif hp9000s200

#ifdef hp9000s800

	/* Get list of drivers present in system */
	if (sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO) { /* if Snakes */
	    for (i = X_FIRST_DRIVER; i <= X_LAST_DRIVER; i++) {
		if (nl_snakes[i].n_value==0)	/* Is this driver loaded? */
			continue;		/* No, ignore it. */

		d = drivers[nextdriver].dname;

		/* Fetch name stripping leading underscore */
		strcpy(d, &nl_snakes[i].n_name[UNDERSCORE_OFFSET]);

		/* Also strip everything after last underscore */
		p = strrchr(d, '_');
		if (p!=NULL)
			*p = '\0';

		/* Something went wrong if no string is left */
		if (*d=='\0') {
			fprintf(stderr,"Bad driver name symbol #%d: \"%s\"\n",
				i, nl_snakes[i].n_name);
			abnorm_exit(1);
		}

		for (k = 0; sn_xlate[k].from != NULL; k++) {
                        if (!strcmp(d, sn_xlate[k].from)) {
                                strcpy(d, sn_xlate[k].to);
                                break;
                        }
                }

		nextdriver++;		/* Next slot in the array */
	    }
	} else { /* 800, non-snake */
	    if ((ret=io_init(O_RDONLY)) == 0) {   /* /dev/config read ok */
		if((ret=io_get_table(T_IO_SW_MOD_TABLE, (void**)&io_tab)) >=0) {
			for (j = 0; j < ret; j++) {
			    strcpy(drivers[j].dname, io_tab[j].mod_name);
			}
		}
		io_free_table(T_IO_SW_MOD_TABLE, (void**)&io_tab);
		io_end();
		nextdriver = ret;
	    } else {
		printf("/dev/config read failed.\n");
	    }
	}

#endif hp9000s800

}



mode_c()
{
	register int i;
	register struct nlist *n;

	atf(0, 2, " CONFIGURATION PARAMETER   HEXADECIMAL  DECIMAL         DRIVERS CONFIGURED\n");
	start_scrolling_here();

	/* Display tunable parameters */
	for (i=0; i<nextconfig; i++) {
	 	n = &nl[i + X_FIRST_CONFIG + 1];
		config[i].cvalue = getvalue(n->n_value);
		addf("%24.24s = 0x%08X = %d\n",
			config[i].cname, config[i].cvalue, config[i].cvalue);
	}

	/* Display loaded drivers */
	for (i=0; i<nextdriver; i++) {
		atf(50, 3+i, "%24.24s", drivers[i].dname);
	}
}
