static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/* HPUX_ID: @(#) $Revision: 70.1 $  */

extern char *mktemp(), *strcpy();
extern long time(), lseek();
extern void exit(), perror();

/* The following three defines are for the offset of the ECC registers
   from the base address */

#define  status  0x1
#define  error1  0x4
#define  error2  0x8

char ECC_REGS[] = {"/tmp/eccregsXXXXXX"};
#define MAX_BOARDS 8  /* The maximum number of ECC boards in the system */
#define SCRUBBER "/etc/eccscrub"
#define LOG_FILE "/etc/ecclog"
#define LOG_FILE_SIZE 100 /* The default number of entries in the log file */

#include <sys/ioctl.h>
#include <errno.h>
#include <sys/iomap.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/utsname.h>


typedef struct  /* Holds data for the ECC boards in the system */
{
	unsigned int size,  /* The size of the board in 64K chunks */
		     base;  /* The base address, used for finding registers */
	unsigned int addr; /* The lowest address on the board */
} board_info; 

board_info board_data[MAX_BOARDS]; /* Where the data about the boards goes */

jmp_buf env;  /* Used for setjmp/longjmp */

int register_descriptor,  /* The file descriptor for the node containing the registers */
    log_file,		  /* The file descriptor for the error log */
    log_record_size,
    i,
    record_number=0,
    record_count = LOG_FILE_SIZE,
    byte_count,
    size, 
    board_count=0;	/* The number of ECC boards in the system */

unsigned char *ecc_byte_regs; /* The byte pointer to the ECC register space */

unsigned int addr;

unsigned short  *ecc_word_regs, /* The word pointer to the ECC register space */
		estat,
		err1,
		err2,
		base;

struct tm *ct; /* Current Time */

long time_val;

char log_buffer[128], /* temporary storage made plenty big */
     temp_buffer[128],
     oldest[128];

char *pname;		/* program name */

/* ============================================================================================ */

/* handle_memory_error is "called" by signal when a bus error occurs.
   It will allow the program to detect the presence of ECC boards */

int handle_memory_error()
{
	longjmp(env,1);
}

/* ============================================================================================ */

main(argc,argv)
	int argc;
	char *argv[];
{
	struct utsname un;
	int force=0;		/* don't force running on a 360 */

	pname = argv[0];		/* program name */
	if (argc>1 && strcmp(argv[1], "-f")==0) {
		force=1;
		argv++;
		argc--;
	}
		
	if (argc == 2)
		record_count = atoi(argv[1]);
	if (record_count < 0)
		record_count = -1*record_count;  /* Allow ecclog -100 to mean 100 entries */

	/*
	 * There is a bug in the 9000/360 hardware that makes the system
	 * panic when you try to read from the ECC registers and you don't
	 * have any ECC memory.  Fortunately, ECC is not supported on the 360.
	 * If we're running on a 360, don't let them do it.
	 */
	uname(&un);
	if (!force && strcmp(un.machine, "9000/360")==0) {
		fprintf(stderr, "%s: ECC memory is not supported on a %s.\n",
			pname, un.machine);
		fprintf(stderr, "Use the -f option is you really want to run %s.\n", pname);
		fprintf(stderr, "However, if you don't have ECC memory, your system may panic and lose data.\n");
		exit(1);
	}

	/* create a node for memory at address 0x005B0000, containing 64K bytes */

	mktemp(ECC_REGS);		/* Convert to unique filename */
	if (mknod(ECC_REGS,0020600,makedev(10,0x005B01)) == -1) {
		perror(ECC_REGS);
		fprintf(stderr, "%s: can't mknod %s\n", pname, ECC_REGS);
		exit(1);
	}

	if ((register_descriptor = open(ECC_REGS,2)) < 0) 
	{
		perror(ECC_REGS);
		fprintf(stderr, "%s: open of %s failed\n", pname, ECC_REGS);
		exit(1);
	}

	unlink(ECC_REGS); /* get rid of the file in case of errrors */

	ecc_byte_regs = (unsigned char *) 0x000000;

	if (ioctl(register_descriptor,IOMAPMAP,&ecc_byte_regs) < 0) 
	{
		perror(ECC_REGS);
		fprintf(stderr, "%s: IOMAPMAP of %s failed\n", pname, ECC_REGS);
		exit(1);
	}

	ecc_word_regs = (unsigned short *)ecc_byte_regs;

	/* Now go hunting for ECC boards. Scan down the addresses where the
	   ECC boards would be.  If a bus error occurs, there is no board. 
	   If not, record the size and address of the board */
	

	for(i=0; i < MAX_BOARDS ;i++)
	{
		signal(SIGBUS,handle_memory_error);
		if (!setjmp(env))
		{	
			/* calculate the data for the ECC register locations
			   for this board, if it exists */
			base = 0xF000+((0xFC-(i<<3))<<4);
			estat =  base+status; 
			err1 = (base+error1)/2;
			err2 = (base+error2)/2;
			size = ecc_word_regs[err2]&0x0003;  
				/* This gets the size of the board if it is there, 
				   or a bus error otherwise */
			/* NOTE: The following will only be executed if there is no
				 bus error */ 
			board_data[board_count].size = 8*(0x1<<size)<<4; 
			board_data[board_count].base = base;
			board_data[board_count].addr = (unsigned int)((base<<16)|0x3FFFFF) - 
				(unsigned int)(board_data[board_count].size << 16)+1;
			board_count++;
		}
	}

	signal(SIGBUS,SIG_DFL); /* Quit looking for bus errors */

	/* The daemon portion of the program follows */

	for (i=0;i<board_count;i++)
	{
		base = board_data[i].base;
		estat = base + status;
		err1 = (base + error1)/2;
		err2 = (base + error2)/2;
		if (ecc_byte_regs[estat] & 0x10)  /* Has an error occured */
		{
			addr = board_data[i].addr | 
				((ecc_word_regs[err1] & 0x00FF)<<17) |
				((ecc_word_regs[err2] & 0xFFF8)<<1);
			time_val = time(0);
			ct = localtime(&time_val);

			sprintf(log_buffer,"%.2d%.2d%.2d%.2d%.2d%.2d 0x%.8X 0x%.2X\n",ct->tm_year,
				ct->tm_mon+1,ct->tm_mday,ct->tm_hour,
				ct->tm_min,ct->tm_sec,addr,(ecc_word_regs[err1] & 0x7F00)>>8); 

			log_record_size = strlen(log_buffer);
						
			/* open the log file and find the oldest entry so we know where to 
			   write to */

			if ( (log_file = open(LOG_FILE,O_APPEND|O_WRONLY|O_CREAT,0644)) == -1)
			{
				perror(LOG_FILE);
				fprintf(stderr, "%s: can't open error log file %s\n", pname, LOG_FILE);
				exit(1);
			}

			if (write(log_file, log_buffer, log_record_size) == -1)
			{
				perror(LOG_FILE);
				fprintf(stderr, "%s: write to log file %s failed\n", pname, LOG_FILE);
				exit(1);
			}
			close(log_file);

			/* Trip the log file to the right size */
			tail(LOG_FILE, record_count, log_record_size);

			/* schedule the scrubber before resetting error register */		
			switch (fork())
			{
				case  -1: 	perror("Error invoking memory scrubber.");
					  	break;
				case 0:		execl(SCRUBBER,SCRUBBER,0);
						break;
				default:	wait((int *)0);
						break;
			}	

			ecc_byte_regs[estat] = ecc_byte_regs[estat] & 0xEF; /* reset error flag */
		}

	}

	return(0); /* Success */
}


#include <sys/stat.h>

tail(filename, count, record_size)
char *filename;
{
	struct stat statbuf;
	int rfd, wfd, len, rc;
	char buf[1024];			/* big enough to hold a log entry */

	if (stat(filename, &statbuf) == -1) {
		perror(filename);
		fprintf(stderr, "%s: can't stat %s\n", pname, filename);
		exit(1);
	}

	if (statbuf.st_size<count*record_size)
		return;
		
	printf("%s: Your system's ECC single bit error log %s has reached\n",
		pname, filename);
	printf("%*s%d entries\n", strlen(filename)+2, "", count);
	printf("\nContact your local HP Sales & Service office for further information.\n");

	rfd = open(filename, O_RDONLY);
	wfd = open(filename, O_WRONLY);
	if (rfd==-1 || wfd==-1) {
		perror(filename);
		fprintf(stderr, "%s: can't open %s.\n", pname, filename);
		exit(1);
	}

	/* Move read pointer past now-useless messages */
	if (lseek(rfd, statbuf.st_size-count*record_size, 0) == -1) {
		perror(filename);
		fprintf(stderr, "%s: can't seek.\n", pname);
		exit(1);
	}

	for (;;) {
		len = read(rfd, buf, record_size);
		if (len==0)
			break;
		if (len!=record_size) {
			perror(filename);
			fprintf(stderr, "%s: read returned %d\n", pname, len);
			exit(1);
		}
		rc = write(wfd, buf, len);
		if (rc!=len) {
			perror(filename);
			fprintf(stderr, "%s: write returned %d\n", pname, rc);
			exit(1);
		}

	}

	ftruncate(wfd, count*record_size);
	close(rfd); close(wfd);
}
