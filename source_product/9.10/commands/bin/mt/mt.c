static char *HPUX_ID = "@(#) $Revision: 72.1 $";
/*
 * mt
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#ifdef SecureWare
#include <sys/security.h>
#endif

/* #define MTWEOF	0	/* write an end-of-file record */
/* #define MTFSF	1	/* forward space file */
/* #define MTBSF	2	/* backward space file */
/* #define MTFSR	3	/* forward space record */
/* #define MTBSR	4	/* backward space record */
/* #define MTREW	5	/* rewind */
/* #define MTOFFL	6	/* rewind and put the drive offline */
/* #define MTNOP	7	/* no operation, sets status only */
/* #define MTEOD	8	/* (DDS only) seek to EOD point*/
/* #define MTWSS	9	/* (DDS only) write mt_count save setmarks*/
/* #define MTFSS	10	/* (DDS only) forward mt_count save setmarks */
/* #define MTBSS	11	/* (DDS only) backward mt_count save setmarks*/

struct commands {
	char *c_name;
	int c_code;
	int c_ronly;
} com[] = {
	"eof",		MTWEOF,	0,
	"fsf",		MTFSF,	1,
	"bsf",		MTBSF,	1,
	"fsr",		MTFSR,	1,
	"bsr",		MTBSR,	1,
	"rewind",	MTREW,	1,
	"offline",	MTOFFL,	1,
	"eod",		MTEOD,	1,
	"smk",		MTWSS,	0,
	"fss",		MTFSS,	1,
	"bss",		MTBSS,	1,
	0,0
};

int mtfd;
struct mtop mt_com;
char *tape;

main(argc, argv)
char **argv;
{
	char line[80], *getenv();
	register char *cp;
	register struct commands *comp;
	int cp_strlen;

#ifdef SecureWare
	if (ISSECURE)  {
		set_auth_parameters(argc, argv);

#ifdef B1
		if (ISB1) {
			initprivs();
			(void) forcepriv(SEC_ALLOWMACACCESS);
		} 
#endif

		if (!authorized_user("tape"))  {
			fprintf(stderr, "mt: no authorization to use mt\n");
			exit (1);
		}
   }
#endif
	if (argc < 2) {
		fprintf(stderr, "usage: mt [ -t tape ] command [ count ]\n");
		exit(1);
	}

	if ((strcmp(argv[1], "-t") == 0) && argc > 2) {
		argc -= 2;
		tape = argv[2];
		argv += 2;
	} else
		if ((tape = getenv("TAPE")) == NULL) {
			tape = "/dev/rmt/0mn";
		}
	cp = argv[1];
        cp_strlen = strlen(cp);
	/* cp_strlen == 0 indicates no command specified */
	if (!cp_strlen) { 
		fprintf(stderr, "usage: mt [ -t tape ] command [ count ]\n");
		exit(1);
	} 

	for (comp = com; comp->c_name != NULL; comp++)
		if (strncmp(cp, comp->c_name, cp_strlen) == 0)
			break;
	if (comp->c_name == NULL) {
		fprintf(stderr, "mt: unknown command \"%s\"\n", cp);
		exit(1);
	}
	if ((mtfd = open(tape, comp->c_ronly ? 0 : 2)) < 0) {
		perror(tape);
		exit(1);
	}
	
	mt_com.mt_count = (argc > 2 ? atoi(argv[2]) : 1);
	mt_com.mt_op = comp->c_code;

	if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0) {
		fprintf(stderr, "%s %d ", comp->c_name, mt_com.mt_count);
		perror("failed");
		exit(1);
	}
	exit(0);
}
