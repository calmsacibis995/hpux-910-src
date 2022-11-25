static char *HPUX_ID = "@(#) $Revision: 1.1 $";
/* HPUX_ID: @(#) $Revision: 1.1 $  */

/* The following three defines are for the offset of the ECC registers
   from the base address */

#define  status  0x1
#define  error1  0x4
#define  error2  0x8

#define ECC_REGS "/dev/eccregs"
#define ECC_MEM  "/dev/eccmem"
#define MAX_BOARDS 8  /* The maximum number of ECC boards in the system */

#include <sys/ioctl.h>
#include <errno.h>
#include <sys/iomap.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/sysmacros.h>

typedef struct  /* Holds data for the ECC boards in the system */
{
	unsigned int size,  /* The size of the board in 64K chunks */
		     base;  /* The base address, used for finding registers */
	unsigned int addr; /* The lowest address on the board */
} board_info; 

board_info board_data[MAX_BOARDS]; /* Where the data about the boards goes */

jmp_buf env;  /* Used for setjmp/longjmp */

int register_descriptor,  /* The file descriptor for the node containing the registers */
    memory_descriptor,    /* The file descriptor for the node corresponding to ECC memory */
    log_file,		  /* The file descriptor for the error log */
    i,
    j,
    word,
    size, 
    board_num,
    board_count=0;	/* The number of ECC boards in the system */

unsigned char *ecc_byte_regs, /* The byte pointer to the ECC register space */
	      *ecc_byte_memory; /* The byte pointer to the 4 Mb section mapped in */

unsigned int current_addr;

unsigned short  *ecc_word_regs, /* The word pointer to the ECC register space */
		stat,
		err1,
		err2,
		base;

unsigned long *ecc_long_memory,
	      *tmp_ptr;
	      dummy;


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
	char * argv[];
{


	/* First, open the address space where the ECC registers live */

	unlink(ECC_REGS);  /* delete any old node */

	/* create a node for memory at address 0x005B0000, containing 64K bytes */

	mknod(ECC_REGS,0020600,makedev(10,0x005B01));

	if ((register_descriptor = open("/dev/eccregs",2)) < 0) 
	{
		perror("open of eccregs failed");
		exit(1);
	}

	ecc_byte_regs = (unsigned char *) 0x000000;

	if (ioctl(register_descriptor,IOMAPMAP,&ecc_byte_regs) < 0) 
	{
		perror("IOMAPMAP of eccregs failed");
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
			base = 0xF000+(0xFC-(i<<3)<<4);
			stat =  base+status; 
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

	board_num = atoi(argv[1]);
	if (board_num < board_count)
	{
		printf("About to set a single bit error for board %d \n",board_num);

		base = board_data[board_num].base;
		stat = base + status;
		ecc_byte_regs[stat] = ecc_byte_regs[stat] | 0x10;
	}	
	else
		printf("No such board in the system \n");


	ioctl(register_descriptor,IOMAPUNMAP,ecc_byte_regs);
	close(register_descriptor);
	unlink(ECC_REGS);
	return(0);
}
