#ifdef __hp9000s300
/* HPUX_ID: @(#) $Revision: 66.1 $ */

/*
** Amigo mediainit
*/


/*
** include files
*/
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/amigo.h>

/*
** definitions for the signal handler
*/
#include <signal.h>
void sigcatch();
struct sigvec vec, ovec;


/*
** defines
*/
#define 	SCG_BASE	0x60	/* secondary addr base 		*/
#define 	HZ		50	/* ticks/sec in kernel land	*/
#define		OVERRIDE_FMT_BIT	0x80
#define		hp_FORMAT		0x02
#define		IBM_FORMAT		0x08



/*
** globals from the parser
*/
extern int verbose;
extern int recertify;
extern int guru;
extern int fmt_optn;
extern int interleave;
extern int debug;
extern int blkno;
extern int maj;
extern int minr;
extern int fd;
extern int unit;
extern int volume;
extern int iotype;
extern char *name;


#ifdef AMIGO_DEBUG
extern int debug;
#endif AMIGO_DEBUG

enum amigo_ioctl__dev_type  { hp_9895_SS_BLNK, hp9895_SS, hp9895_DS_BLNK,
                              hp9895_DS, hp9895_IBM, hp8290X, hp9121,
                              hp913X_A, hp913X_B, hp913X_C, nounit};



/*
** Device ident numbering from amigo device driver's device_maps[]
*/
#define		UNSUPPORTED		0x0002

/* hp7906	0x0002				-  0 Not supported 	*/
/* hp7905_L	0x0002				-  1 Not supported 	*/
/* hp7905_U	0x0002				-  2 Not supported 	*/
/* hp7920	0x0002				-  3 Not supported 	*/
/* hp7925	0x0002				-  4 Not supported 	*/

#define 	hp9895_SS_DS_IBM	0x0081
#define		hp8290X_9121		0x0104
#define		hp913X_A		0x0106 
#define		hp913X_B		0x010A	
#define		hp913X_C		0x010F		
#define		NOUNIT	 		0x0000	



/*
** Command type definitions, used as an index into the issue_command(~~)
** command table declarations. This list is basically the same as the
** enum command_type in amigo.h, with a few additions.
*/
enum ioctl_command_type {
	io_req_status,		/* request status		*/
	io_req_log_addr,	/* request logical address 	*/
	io_seek_cmd,		/* seek				*/
	io_verify_cmd,		/* verify			*/
	io_init_d_cmd,		/* initiialize, setting D bits  */
	io_format_cmd,		/* format			*/
	/*							*/
	/* ---------------------------------------------------- */
	io_init_dummy_byte,	/* Write dummy byte, init D bits*/
	io_get_log_addr,	/* get the logical address bytes*/
	io_get_status		/* get the status bytes		*/
	};
	




#ifdef OLD9895
/*
**  Media addressing parameters (and other things).
**  Cylinders is effective (after sparing), not physical count
*/
struct map_type device_maps [] = {
/*                                                             file           */
/*               cyl  head sect  ident     model  rps log2blk  mask   flags   */
/*               ---  ---- ----  -----     -----  ---  ------  ----   -----   */
{/* hp9895_SS  */ 73,   1,  30, 0x0081,        2,  6,    8,     0,     0},
{/* hp9895_DS  */ 75,   2,  30, 0x0081,        6,  6,    8,     0,     0},
{/* hp9895_IBM */ 77,   1,  26, 0x0081,        8,  6,    7,     0,     0},
{/* hp8290X    */ 33,   2,  16, 0x0104,  R_BIT_0,  5,    8,     0, MUST_BUFFER},
{/* hp9121     */ 33,   2,  16, 0x0104,  R_BIT_1, 10,    8,     0,     0},
{/* hp913X_A   */ 152,  4,  31, 0x0106, NO_MODEL, 60,    8,     0,     0},
{/* hp913X_B   */ 305,  4,  31, 0x010A, NO_MODEL, 60,    8,     0,     0},
{/* hp913X_C   */ 305,  6,  31, 0x010F, NO_MODEL, 60,    8,     0,     0},
{/* NOUNIT     */   0,  0,   0,      0,	       0,  0,    0,     0,     0},
};
#else
/*
**  Media addressing parameters (and other things).
**  Cylinders is effective (after sparing), not physical count
*/
struct map_type device_maps [] = {
/*                                                             file           */
/*               cyl  head sect  ident     model  rps log2blk  mask   flags   */
/*               ---  ---- ----  -----     -----  ---  ------  ----   -----   */
{/* hp9895_SS_BLNK  */ 
                  73,   1,  30, 0x0081,        1,  6,    8,     0,     0},
{/* hp9895_SS  */ 73,   1,  30, 0x0081,        2,  6,    8,     0,     0},
{/* hp9895_DS_BLNK  */
                  75,   2,  30, 0x0081,        5,  6,    8,     0,     0},
{/* hp9895_DS  */ 75,   2,  30, 0x0081,        6,  6,    8,     0,     0},
{/* hp9895_IBM */ 77,   1,  26, 0x0081,        8,  6,    7,     0,     0},
{/* hp8290X    */ 33,   2,  16, 0x0104,  R_BIT_0,  5,    8,     0, MUST_BUFFER},
{/* hp9121     */ 33,   2,  16, 0x0104,  R_BIT_1, 10,    8,     0,     0},
{/* hp913X_A   */ 152,  4,  31, 0x0106, NO_MODEL, 60,    8,     0,     0},
{/* hp913X_B   */ 305,  4,  31, 0x010A, NO_MODEL, 60,    8,     0,     0},
{/* hp913X_C   */ 305,  6,  31, 0x010F, NO_MODEL, 60,    8,     0,     0},
{/* NOUNIT     */   0,  0,   0,      0,	       0,  0,    0,     0,     0},
};
#endif OLD9895





/*
** Request and obtain the device's current status bytes.
*/
struct combined_buf *status_ret_buf_ptr;
struct status_type Req_status()
{
struct status_type status;
struct combined_buf cmd;

	if( issue_cmd( io_req_status, cmd ) < 0 )
		(void) err(errno, "Req_status: Amigo request status error!");

	if( issue_cmd( io_get_status, status_ret_buf_ptr ) < 0 )
		(void) err(errno, "Req_status: Amigo Get status bytes error!");


	/*
	** The following is ugly! All this work just to make it look
	** like the structure used in the kernel!
	*/
	status.s    = (status_ret_buf_ptr->cmd_buf[0] & 0x80) >> 7;
	status.p    = (status_ret_buf_ptr->cmd_buf[0] & 0x40) >> 6;
	status.d    = (status_ret_buf_ptr->cmd_buf[0] & 0x20) >> 5;
	status.s1   = (status_ret_buf_ptr->cmd_buf[0] & 0x1f);

	status.unit = (status_ret_buf_ptr->cmd_buf[1] & 0xff);

	status.star = (status_ret_buf_ptr->cmd_buf[2] & 0x80) >> 7;
	status.xx   = (status_ret_buf_ptr->cmd_buf[2] & 0x60) >> 5;
	status.tttt = (status_ret_buf_ptr->cmd_buf[2] & 0x1e) >> 1;
	status.r    = (status_ret_buf_ptr->cmd_buf[2] & 0x01);

	status.a    = (status_ret_buf_ptr->cmd_buf[3] & 0x80) >> 7; 
	status.w    = (status_ret_buf_ptr->cmd_buf[3] & 0x40) >> 6;
	status.fmt  = (status_ret_buf_ptr->cmd_buf[3] & 0x20) >> 5;
	status.e    = (status_ret_buf_ptr->cmd_buf[3] & 0x10) >> 4;
	status.f    = (status_ret_buf_ptr->cmd_buf[3] & 0x08) >> 3;
	status.c    = (status_ret_buf_ptr->cmd_buf[3] & 0x04) >> 2;
	status.ss   = (status_ret_buf_ptr->cmd_buf[3] & 0x03);


#ifdef AMIGO_DEBUG
	if( debug )
	{
		(void) printf("	status.s    = %d\n", status.s);
		(void) printf("	status.p    = %d\n", status.p);
		(void) printf("	status.d    = %d\n", status.d);
		(void) printf("	status.s1   = %d\n", status.s1);
		(void) printf("	status.unit = %d\n", status.unit);
		(void) printf("	status.star = %d\n", status.star);
		(void) printf("	status.xx   = %d\n", status.xx);
		(void) printf("	status.tttt = %d\n", status.tttt);
		(void) printf("	status.r    = %d\n", status.r);
		(void) printf("	status.a    = %d\n", status.a);
		(void) printf("	status.w    = %d\n", status.w);
		(void) printf("	status.fmt  = %d\n", status.fmt);
		(void) printf("	status.e    = %d\n", status.e);
		(void) printf("	status.f    = %d\n", status.f);
		(void) printf("	status.c    = %d\n", status.c);
		(void) printf("	status.ss   = %d\n", status.ss);
	}  /* debug */
#endif AMIGO_DEBUG

	return status;

}	/* Req_status */

	




/*
** Amigo driver ioctl interface
*/
mi_amigo()
{
register struct map_type *map;
struct status_type status;
struct amigo_identify identify;
long tracks_per_medium;
register int i;

	if(guru)
	    (void) err(0,"Sorry - guru mode not supported for amigo devices.");

	vec.sv_handler = sigcatch;
	for( i = 1; i < 24; i++ )
		(void) sigvector(i, &vec, &ovec);

	verb("Locking the device");
	if( ioctl( fd, AMIGO_LOCK_DEVICE ) < 0 )
	     (void) err(errno,"Mi_amigo: amigo ioctl AMIGO_LOCK_DEVICE error");

	status = Req_status();

	if( status.f == 1 ) 
		(void) err(0, "ERROR: medium may have been changed.");

	if( status.ss == 3 ) 
		(void) err(0, "ERROR: medium absent.");

	if( status.e )
		(void) err(0,"ERROR: possible drive not ready after data xfer");

	if( status.w == 1 ) 
		(void) err(0, "ERROR: medium is write protected.");



#ifdef AMIGO_DEBUG
	if( debug )
	{
	   (void) printf("\nIssue AMIGO_IDENTIFY ioctl? <y/n>  <^c, kill>:\n");
	   while( ( getchar() ) != 'y')
	    	(void) printf("Ready ? <y/n>  <^c, kill>:\n");
	}
#endif AMIGO_DEBUG


	if(ioctl(fd, AMIGO_IDENTIFY, &identify) < 0)
		(void) err(errno,"Mi_amigo: Amigo ioctl AMIGO_IDENTIFY error!");


#ifdef AMIGO_DEBUG
	if( debug )
		(void) printf(" Identify.id = %d\n\n", identify.id);
#endif AMIGO_DEBUG


	if( identify.id == 0x0003  || identify.id == 0x0002 )
		(void) print_inv_type();

	for( map = device_maps; map->ident != identify.id; map++)
		if( map->cyl_per_med == 0)
			(void) print_inv_type();

	if( identify.id == 0x0104 )
		map = device_maps + (status.r  ? (int)hp9121 : (int)hp8290X);
	else if (map->model != NO_MODEL )
	   for( ; map->ident != identify.id || map->model != status.tttt; map++)
		if( map->cyl_per_med == 0 )
			(void) print_inv_type();


#ifdef AMIGO_DEBUG
	if(debug)
	{
		(void) printf("	map->cyl_per_media = %d\n", map->cyl_per_med);
		(void) printf("	map->track_per_cyl = %d\n", map->trk_per_cyl);
		(void) printf("	map->sec_per_track = %d\n", map->sec_per_trk);
		(void) printf("	map->ident         = %x\n", map->ident);
		(void) printf("	map->model         = %d\n", map->model);
		(void) printf("	map->rps           = %d\n", map->rps);
		(void) printf("	map->log2blk       = %d\n", map->log2blk);
		(void) printf("	map->file_mask     = %d\n", map->file_mask);
		(void) printf("	map->flag          = %d\n\n", map->flag);
	}
#endif AMIGO_DEBUG



	/*
	** Last little bit of wierdness goes on here. If the device is 
	** not an HP9895_SS and the IBM_FORMAT option was selected by the
	** user, then abort. IBM_FORMAT will ONLY work for the 9895_SS
	** 8" floppy. This option has never been tested on an IBM. The
	** media actually appears to be formatted IBM style, but never tested.
	*/

#ifdef 9895OLD
	if(fmt_optn == 8)
	   if( (map->ident == hp9895_SS_DS_IBM) &&
	   	( (map->model == 2) || (map->model == 8) ) )
			;
	   else	
	  {
	   (void) close(fd);
	   (void) err(0,"IBM format only valid for 8\" Single-Sided Floppies");	
	  }
#else
	if(fmt_optn == 8)
	{
	   if( map->ident == hp9895_SS_DS_IBM )
	   {
		switch(map->model)
		{
		case 1:   /* SS_blank      */
		case 2:   /* HP format SS  */
		case 8:   /* IBM format SS */
			break;

		default:
	   		(void) close(fd);
	   (void) err(0,"IBM format only valid for 8\" Single-Sided Floppies");	
		}
	   }
       }
#endif 9895OLD


	tracks_per_medium = map->trk_per_cyl * map->cyl_per_med;

	if( tracks_per_medium <= 0 )
	     (void) err(0,"Mi_amigo: Invalid media addressing parameters.");




	switch( map->ident )
	{
		case hp8290X_9121:
		case hp9895_SS_DS_IBM:

			(void) init_9895_913X_Chinook( map, tracks_per_medium );
			break;


		case hp913X_A:
		case hp913X_B:
		case hp913X_C:

			(void) init_913X_ABC( map, tracks_per_medium );
			break;


		default:

			(void) print_inv_type();
	} 


} /* mi_amigo */





/*
** Signal handler 
*/
void
sigcatch(n, code, scp)
int n, code;
struct sigcontext *scp;
{
static char * signame[] = {
	"OTHER", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP",
	"SIGIOT","SIGEMT", "SIGFPE", "SIGKILL", "SIGBUS", "SIGSEGV",
	"SIGSYS", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGUSR1", "SIGUSR2",
	"SIGCLD", "SIGPWR"};

	verb("Sigcatch: caught user initiated signal %s", signame[n]);
	(void) err(0, "Sigcatch: mediainit aborting!");
	(void) close(fd);
	fflush(stdout);
	exit(1);
}





/*
** Inappropriate device was chosen.
*/
print_inv_type()
{
	(void) verb("ERROR: Unsupported device.");
	(void) verb("     : HP7906, HP7905_L, HP7905_U,");
	(void) verb("     : HP7920, and HP7925 are Invalid Types.");
	(void) err(0, "Mediainit aborting!");

}	/* print_inv_type */





/*
** Request and obtain the devices current logical address. It is
** returned in the form of a current cylinder, head, and sector.
** Return this info in the t_la structure.
*/
struct combined_buf *log_addr_buf_ptr;
struct tva_type  logical_addr()
{
struct tva_type t_la;
struct combined_buf cmd;
short temp_cyl;

	if( issue_cmd( io_req_log_addr, cmd ) < 0 )
		(void) err(errno,"Logical_addr: Amigo Req. Logical Addr error");

	if( issue_cmd( io_get_log_addr, log_addr_buf_ptr ) < 0 )
		(void) err(errno,"Logical_addr: Amigo Get Logical Addr error");


	/*
	**  Packing the tva_type structure as follows:
	**
	**  cmd.cmb_buf[0]   cmd.cmd_buf[1]
	**  -------------------------------
	**  |  upper byte  |  lower byte  | =>   t_la.cyl (cylinder location)  
	**  -------------------------------
	**
	**  cmd.cmd_buf[2]
	**  ________________
	**  |     byte     | =>  t_la.head  (head utilized)
	**  ----------------
	**
	**  cmd.cmd_buf[3]
	**  ________________
	**  |     byte     | =>  t_la.sect (current sector)
	**  ----------------
	*/

	temp_cyl = log_addr_buf_ptr->cmd_buf[0];

	t_la.cyl = (( (temp_cyl & 0x007f) << 8) |
		      (log_addr_buf_ptr->cmd_buf[1] & 0x00ff));

	t_la.head = log_addr_buf_ptr->cmd_buf[2];
	t_la.sect = log_addr_buf_ptr->cmd_buf[3];


	return t_la;

}  	/* logical_addr  */





/*
** Main function that handles floppies
*/
unsigned char test_pattern[] = { 198, 99, 219, 136 };

init_9895_913X_Chinook( map, tracks_per_medium )
struct map_type *map;
long tracks_per_medium;
{

#define	testpasses 4		/* Number of Test Passes to run      */
#define max_bt	   4		/* Max. Number of bad tracks allowed */
#define	TRUE	1
#define FALSE   0

struct tva_type tva;		/* Used to obtain a logical blk address	*/

int bt_count = 0;		/* Encountered bad track counter	*/
register int test_pass;		/* Maximum number of test passes	*/
int verify_starting_track;
int verify_pass_complete;	/* Used as a boolean			*/
int bt_already_logged;		/* Previously logged bad track		*/
int bad_track;			/* Currently encountered bad track	*/
int bt_index;			/* Bad track index			*/
int bt_table[max_bt];		/* Bad track table			*/


(void)  verb("Formatting media, override old-format bit in effect");
for( test_pass = 0; test_pass < testpasses; test_pass++)
{

     if( format(    1,    test_pattern[test_pass],     TRUE) < 0 )
	(void) err(errno, "Init_9895_913X: Unrecoverable Formatting failure.");

     verify_pass_complete  = FALSE;
     verify_starting_track = 0;



     do {

            if( seek( map, verify_starting_track * map->sec_per_trk ) < 0 )
		(void) err(errno,"Init_9895_913X: Unrecoverable Seek failure.");

            /* Perform a media verify with reduced margins. */
            if( verify( (tracks_per_medium + max_bt - verify_starting_track) * 
			map->sec_per_trk) == 0 )
	    {
	        verify_pass_complete = TRUE;
		verb("Pass %d: verification of media complete!", test_pass+1);

	    } else {


		     /*
		     ** Proceed with a new  status check. Can we recover
		     **	from this error at the present time?
	             */	
		     if( check_status() < 0 )
	                  (void) err(0, "Init_9895_913X: Status Check failure");



		     /* Get the logical tva address, current seek position */
		     tva = logical_addr();

		     bad_track = ( decoded_addr( tva, map ) / map->sec_per_trk);
		     bt_already_logged = FALSE;



		     /* Determin if this track is already logged.  */
		     for( bt_index = 0; bt_index < bt_count; bt_index++)
			   if( bt_table[bt_index] == bad_track)
					bt_already_logged = TRUE;



		    /* Prepare to log the bad track  */
		    if( bt_already_logged == FALSE ) {

		        if( bt_count >= max_bt )
			      (void) err(0,"Max number Bad Tracks exceeded!");

		   	bt_table[bt_count] = bad_track;
			verb("Logged bad track - %d", bt_table[bt_count]);
		   	bt_count++;

		     }


		     bad_track++;
		     verify_starting_track = bad_track;

		    if( verify_starting_track >= (tracks_per_medium + max_bt))
				verify_pass_complete = TRUE;


             }  /* else */


	} while ( verify_pass_complete != TRUE );


}  /* for */



	/* Init the d-bits of the previously logged bad tracks */
	for( bt_index = 0; bt_index < bt_count; bt_index++ ) {

	   if( seek( map, bt_table[bt_index] * map->sec_per_trk ) < 0)
		(void) err(errno, "Init_9895_913X: Unrecoverable Seek failure");

	   if( init_with_d_bits() < 0 )
		(void) err(errno, "Init_9895_913X: Unrecoverable Init failure");
	} 



	/* Perform interleave parameter verification. */
	(void) verify_interleave( map );


	verb("Final formatting pass - override no longer in effect");
	if( format( interleave, 0, FALSE ) < 0 )
	       (void) err(errno,"Init_9895_913X: Unrecoverable Format failure");


	/* Perform Final seek - track 0 */
	if( seek( map, (fmt_optn == IBM_FORMAT ? 1 : 0 )) < 0 )
		(void) err(errno,"Init_9895_913X: Final Seek, track 0 failure");


	verb("Performing Final verify - all logical tracks");
	if( verify( tracks_per_medium * map->sec_per_trk ) < 0)
	{
		verb("Final Verification Failure - RE-RUN MEDIAINIT");
		(void) err(errno,"Init_9895_913X: Media is suspect.");
	}



}	 /* init_9895_913X_Chinook */






/*
** Determine the format attributes and actually format the media.
*/
format( intlve, d_byte, override)
int intlve, d_byte;
int override;
{

struct combined_buf cmd;


	if( override == 1 )
		cmd.cmd_buf[2] = OVERRIDE_FMT_BIT | hp_FORMAT;

	else

	   {
		/* Initially assume hp-format */
		cmd.cmd_buf[2] = hp_FORMAT;  

		switch( fmt_optn )
		{

		  case hp_FORMAT:

			verb("Media formatting option = HP FORMAT");
			break;


		  case IBM_FORMAT:

			verb("Media formatting option = IBM FORMAT");
			cmd.cmd_buf[2] = IBM_FORMAT; 	
			break;


		  default:

			verb("Default format type is HP style.");
			break;


		}	/* end case */

	    }	/* end if-then-else */



	cmd.cmd_buf[3] = intlve;	     /* Interleave Factor            */
	cmd.cmd_buf[4] = d_byte;	     /* Data Byte		     */

	return ( (issue_cmd( io_format_cmd, cmd ) < 0)  ? -1  :  0 );

}  	/* format */





/*
** Using the record address and the media addressing parameters,
** return a populated tva structure with the correct cylinder, head
** and sector elements.
*/
struct tva_type coded_addr( map, record_addr )
struct map_type *map;
daddr_t record_addr;
{
short track;
struct tva_type t_ca;

	/*
	** If the user specified format option is IBM_FORMAT, then we have
	** to add one to the sectors_per_track, IBM counts from 1-26 instead
	** of from 0-25! Also, this will only work for 8" single-sided 
	** floppies (HP9895's). 
	*/

		t_ca.sect = (short )( record_addr % (map->sec_per_trk +
		      (  (fmt_optn == IBM_FORMAT ) ? 1 : 0) ) );

		track = (short )( record_addr / (map->sec_per_trk +
		      (  (fmt_optn == IBM_FORMAT ) ? 1 : 0) ) );

		t_ca.head = track % map->trk_per_cyl;
		t_ca.cyl  = track / map->trk_per_cyl;

		return t_ca;

}	 /*  coded_addr */





/*
** Using an already populated tva structure and the devices
** media addressing parameters, return a decoded address
*/
int decoded_addr( tva, map )
struct tva_type tva;
struct map_type *map;
{
register int track;

	track =	tva.cyl * map->trk_per_cyl + tva.head;

	return (track * map->sec_per_trk + tva.sect);

}	  /* decoded_addr */






/*
** Using the record address and the devices media addressing parameters
** seek to the requested cylinder, head, and sector.
*/
seek( map, record_addr )
struct map_type *map;
daddr_t record_addr;
{
struct combined_buf cmd;
struct tva_type tva;

		tva = coded_addr( map, record_addr );
		
		/*
		**    cmd_buf[2]	cmd_buf[3]
		** ______________________________
		** |  upper-byte   | lower-byte  | <=  tva.cyl (short)
		** -------------------------------
		** ________________
		** | cmd_buf[4]    | <=  tva.head
		** -----------------
		** ________________
		** | cmd_buf[5]    | <=  tva.sect
		** -----------------
		*/

		cmd.cmd_buf[2] = (tva.cyl & 0x7f00) >> 8;
		cmd.cmd_buf[3] = (tva.cyl & 0x00ff);
		cmd.cmd_buf[4] = tva.head;
		cmd.cmd_buf[5] = tva.sect;

		return ( ( issue_cmd( io_seek_cmd, cmd) < 0 ) ? -1  : 0 );

}  	/* seek */






/*
** Initialize the current seek positions D-bits
** This is only a two byte command cmd_buf[0] & [1]  
*/
init_with_d_bits()
{
struct combined_buf cmd;

	/* 1st issue the init command                    */
	if( issue_cmd( io_init_d_cmd, cmd ) < 0 ) return -1;

	/* 2nd give the controller a dummy byte to write */
	return ( (issue_cmd( io_init_dummy_byte, cmd ) < 0 ) ? -1 : 0 );

}	/* init_with_d_bits */





/*
**	Issue an amigo verify command starting at sector = nsectors, 
**	using reduced margins continue until the sector count given expires.
*/
verify( nsectors )
unsigned int nsectors;
{
struct combined_buf cmd;

		/*
		**    cmd_buf[2]	cmd_buf[3]
		** ______________________________
		** |  upper-byte   | lower-byte  | <=  nsectors = sector count 
		** -------------------------------
		*/

	cmd.cmd_buf[2] = ( nsectors & 0xff00 ) >> 8;
	cmd.cmd_buf[3] = ( nsectors & 0x00ff );

	return ( (issue_cmd( io_verify_cmd, cmd ) < 0 ) ? -1  :  0 );

}	 /* verify */





/*
** Request and obtain devices current status bytes.
*/
check_status()
{
struct status_type status;

	status = Req_status();

	if( status.c ){
		    verb("Check_status: Drive attention - Seek check failure.");
		    return -1;
	}

	switch( status.s1 )
	{

	case NORMAL_COMPLETION:
	case UNCORRECTABLE_DATA_ERROR:
	case HEAD_SECTOR_COMPARE_ERROR:
	case ACCESS_NOT_READY_DURING_DATA_OP:
	case DRIVE_ATTENTION:

			break;

	default:

			verb("Check_status: Medium formatting/sparing failed"); 
			return -1;
			break;

	} /* end case */


}  	/* status_check */
			





/*
** Verify user supplied interleave arguments ( if applicable )
*/
verify_interleave( map )
struct map_type *map;
{

struct status_type status;

	status = Req_status();

	switch( map->ident )
	{

		case hp9895_SS_DS_IBM:

			if( (interleave > 29)  || (interleave < 1) ) {
				verb("Using default interleave factor = 2");
				interleave = 2;
			} 
			else
				verb("Interleave factor = %d", interleave);
			break;


		case hp8290X_9121:

			if( (interleave > 15) || (interleave < 1) )
			{
			   /* Sparrow = 2, Chinook = 3 */
			   interleave = ( status.r ) ? 2 : 3;
			   verb("Using default interleave = %d",interleave);
			}
			else
				verb("Interleave factor = %d", interleave);
			break;


		case hp913X_A:
		case hp913X_B:
		case hp913X_C:

			verb("Using default interleave = 9");
			interleave = 9;
			break;


		default:

			verb("Verify_interleave: Unknown/Unsupported device");
			(void) err(0, "Mediainit aborting - RE-RUN MEDIAINIT");

	}	/* case  */


}	  /* verify_interleave */






/*
** Main function that handles winchester devices
*/
init_913X_ABC( map, tracks_per_medium )
struct map_type *map;
long tracks_per_medium;
{
#define	min_vp	5
#define max_vp	15
#define min_csvp 2
#define TRUE	1
#define FALSE	0

register int verify_pass = 0;
register int csvp = 0;		/* Consecutive successful verify passes */
register int verify_starting_track;
register int verify_pass_complete = FALSE;
struct tva_type tva;


    (void) verify_interleave( map );

    verb("Formatting the media with interleave value of %d", interleave); 
    if( format( interleave, 0, TRUE) < 0 )
	(void) err(errno, "Init_913X_ABC: Unrecoverable Formatting Failure.");

    verb("Verifying media:");
    do
    {

	verify_pass++;
	verify_starting_track = 0;


	do
	{

	  if( seek( map, verify_starting_track * map->sec_per_trk ) < 0 )
		(void) err(errno,"Init_913X_ABC: Unrecoverable Seek failure.");

	  if( verify( (tracks_per_medium - verify_starting_track) *
			map->sec_per_trk) == 0 ) {
	     verify_pass_complete = TRUE;
	     verb("Pass %d: verification of media complete", verify_pass);
	  }

	  else
	  {

	     if( check_status() < 0)
		(void) err(0,"Init_913X_ABC: Status check failure.");
	     
	     tva = logical_addr();

	     verify_starting_track=(decoded_addr(tva,map)/(map->sec_per_trk+1));

	     if( init_with_d_bits() < 0)
		 (void) err(errno,"Init_913X_ABC: Unrecoverable D-bit failure");

             verb("Spared Bad Track number %d", verify_starting_track);

	     if( verify_starting_track >= tracks_per_medium ) {
		   verify_pass_complete = TRUE;
	           verb("Pass %d: verification of media complete", verify_pass);
	     }

	  }	/* else */



	} while( verify_pass_complete == FALSE);


		if( verify_starting_track == 0 ) 
			csvp++;	
		else
			csvp = 0;


    } while( (( verify_pass <= min_vp )  &&  ( csvp <= min_csvp ))
		|| ( verify_pass >= max_vp )   );



    if( csvp < min_csvp )
           (void) err(0,"Init_913X_ABC: Consecutive verify passes failed!");


}   /*  init_913X_ABC 	*/








#ifdef AMIGO_DEBUG
char *ioctl_errors[] = {
	"I/O request status error",
	"I/O request logical address error",
	"I/O seek command error",
	"I/O verify command error",
	"I/O initialize with D-bits command error",
	"I/O format command error",
	"I/O Write initialize with dummy byte",
	"I/O Get logical address bytes",
	"I/O Get status bytes"
	};


char *command_request[] = {
	"Request Status Command",
	"Request Logical Address Command",
	"Seek Command",
	"Verify Command",
	"Initialize with D-bits Command",
	"Format Command",
	"Write Initialize with Dummy Byte",
	"Get Logical Address Bytes",
	"Get Status Bytes"
	};
#endif AMIGO_DEBUG





/*
** Actually issue the requested amigo command.
*/
issue_cmd( command, cmd )
enum ioctl_command_type command;
struct combined_buf cmd;
{

#define	YES_PPOLL	1
#define NO_PPOLL	0
#define YES_DSJ		1
#define NO_DSJ		0


static struct ctet	        /* command table entry type         */ {
	short    sec;	        /* secondary command	             */
	char      oc;		/* opcode	                     */
	char      nb;		/* number of data bytes              */
	char    poll;           /* Does this command require a ppoll */
	unsigned int timeout;   /* if yes, how long should it take   */

	int iod_mesg_arg;	/* 2nd arg to iod_mesg		     */
				/* #define T_WRITE	0x1  	     */
				/* #define T_EOI	0x2          */
				/* #define T_PPL	0x4          */
	char dsj;		/* Does this command require a dsj?  */


}  ioctl_command_table[] = {


/*   command			sec    oc  nb    poll   time   mesg    dsj  */
/*   -------			---    --  --    ----   ----   ----    ---  */

{ /*  io_req_status	*/   SEC_OP1,   3,  2, YES_PPOLL,  40,  7,   NO_DSJ  },
{ /*  io_req_log_addr	*/   SEC_OP1,  20,  2, YES_PPOLL,  40,  7,   NO_DSJ  },
{ /*  io_seek_cmd	*/   SEC_OP1,   2,  6, YES_PPOLL,  40,  7,   YES_DSJ },
{ /*  io_verify_cmd	*/   SEC_OP1,   7,  4, YES_PPOLL,1500,  7,   YES_DSJ },
{ /*  io_init_d_cmd	*/   SEC_OP1,  43,  2, YES_PPOLL, 200,  7,   NO_DSJ  },
{ /*  io_format_cmd	*/   SEC_OP4,  24,  5, YES_PPOLL,1500,  7,   YES_DSJ },
  /*							                    */
  /*							          	    */
{ /*  io_init_dummy_byte*/       96,    0,  1, YES_PPOLL,  90,  7,   YES_DSJ },
{ /*  io_get_log_addr   */   SEC_RSTA,  0,  4, YES_PPOLL, 200,  0,   YES_DSJ },
{ /*  io_get_status     */   SEC_RSTA,  0,  4, YES_PPOLL, 100,  0,   NO_DSJ },
};

	/*
	** All requests go through the same ioctl, the packing of the 
	** bytes is done here.
	*/


	switch( command )
	{

	case io_req_status:		/* request status		*/
	case io_req_log_addr:		/* request logical address 	*/
	case io_seek_cmd:		/* seek				*/
	case io_verify_cmd:		/* verify			*/
	case io_init_d_cmd:		/* initiialize, setting D bits  */
	case io_format_cmd:		/* format			*/
	case io_init_dummy_byte:	/* D-bit init with dummy byte   */ 
	case io_get_log_addr:		/* Get the logical address bytes*/
	case io_get_status:		/* Get the status bytes	        */

	     cmd.command_name = (int)command;
	     cmd.cmd_sec      = ioctl_command_table[(int)command].sec;
	     cmd.cmd_nb       = ioctl_command_table[(int)command].nb;
	     cmd.timeout_val  = ioctl_command_table[(int)command].timeout * HZ;
	     cmd.ppoll_reqd   = ioctl_command_table[(int)command].poll;
	     cmd.mesg_type    = ioctl_command_table[(int)command].iod_mesg_arg;
	     cmd.dsj_reqd     = ioctl_command_table[(int)command].dsj;


	     /*
	     ** The following two bytes are the functional equivalent
	     ** of the struct ftcb_type  structure used in the driver
	     */
	     cmd.cmd_buf[0]   = ioctl_command_table[(int)command].oc; 
	     cmd.cmd_buf[1]   = unit; 



#ifdef AMIGO_DEBUG
 	     if( debug  )
	     {
	     register int i;

	     (void) printf("\nIoctl request to be executed is - %s\n",
			command_request[(int)command]);
	     (void) printf("cmd.cmd_nb       = %d\n", cmd.cmd_nb);
	     (void) printf("cmd.timeout_val  = %d\n", cmd.timeout_val);
	     (void) printf("cmd.ppoll_reqd   = %d\n", cmd.ppoll_reqd);
	     (void) printf("cmd.mesg_type    = %d\n", cmd.mesg_type);
	     (void) printf("cmd.dsj_reqd     = %d\n", cmd.dsj_reqd);

	     (void) printf("                    decimal     octal       hex\n");
	     (void) printf("\n");
	     (void) printf("cmd.cmd_sec     = %8d, %8o, %8x\n", cmd.cmd_sec,
				cmd.cmd_sec, cmd.cmd_sec);


	     for ( i = 0; i < cmd.cmd_nb; i++)
		   	(void) printf("cmd.cmd_buf[%d] = %8d, %8o, %8x\n",i,
			cmd.cmd_buf[i], cmd.cmd_buf[i], cmd.cmd_buf[i]);


	     (void) printf("\nIssue AMIGO_IOCMD_IN ioctl? <y/n> <^c, kill>:\n");
	     while( ( getchar() ) != 'y')
		    	(void) printf("Ready ? <y/n>  <^c, kill>:\n");


	     }  	/* debug */

#endif AMIGO_DEBUG


		/*
		** The ioctl is returned a value of 0 if the dsj
		** following the requested command was error free,
		** else a -1
		*/
		if( ioctl(fd, AMIGO_IOCMD_IN, &cmd) < 0){

#ifdef AMIGO_DEBUG
			if(debug)
				verb("ERROR: %s", ioctl_errors[(int)command]);
#endif AMIGO_DEBUG
	           	return -1;	/* error back to calling function */
		}
			


		switch(command)
		{
			case io_get_log_addr:

				log_addr_buf_ptr = &cmd;
				break;

			case io_get_status:

				status_ret_buf_ptr = &cmd;
				break;
		}


		break;


	default:

		(void) err( 0, "Invalid command type specified!");


	} 		/* end case */



	return 0;	/* Successful Completion */


}	/* issue_cmd */ 
#endif /* __hp9000s300 */
