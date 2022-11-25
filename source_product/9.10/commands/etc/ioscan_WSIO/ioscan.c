#include <sys/libio.h>
#include "./ioscan.h"
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/cs80.h>
#include <sys/hpibio.h>
#include <sys/diskio.h>
#include <sys/ioctl.h>
#include <sys/scsi.h>
#include <fcntl.h>
#include <machine/dconfig.h>
#include <sys/mknod.h>

#include <stdlib.h>
#include <sys/sem.h>
#include <sys/errno.h>

#ifdef __hp9000s300
#include <sio/autoch.h>
#endif /* __hp9000s300 */


/***************************************************************
 *  declarations used by get_autoch_devices
 */

#define sbit8 signed char
#define sbit16 short
#define sbit32 long
#define ubit8 unsigned char
#define ubit16 unsigned short
#define ubit32 unsigned long

struct el_status_data {
    ubit16 first_el_addr;
    ubit16 num_el;
    ubit32 resrv: 8;
    ubit32 byte_cnt : 24;
};

struct el_status_page {
    ubit8 el_type_code;
    ubit8 p_vol_tag : 1;
    ubit8 a_vol_tag : 1;
    ubit8 resrv0: 6;
    ubit16 el_desc_len;
    ubit32 resrv1: 8;
    ubit32 byte_cnt : 24;
};


struct dt_el_desc {
    struct el_status_data el_status;
    struct el_status_page el_page;
    ubit16 el_addr;
    ubit8 resrv0: 4;
    ubit8 access : 1;
    ubit8 except : 1;
    ubit8 resrv1: 1;
    ubit8 full : 1;
    ubit8 resrv2;
    ubit8 add_sense_code;
    ubit8 add_sense_code_qual;
    ubit8 not_bus : 1;
    ubit8 resrv3: 1;
    ubit8 id_valid : 1;
    ubit8 lu_valid : 1;
    ubit8 resrv4: 1;
    ubit8 lun : 3;
    ubit8 scsi_bus_addr;
    ubit8 resrv5;
    ubit8 s_valid : 1;
    ubit8 invert : 1;
    ubit8 resrv6: 6;
    ubit16 src_st_el_addr;
};


struct mode_param_hdr {
    ubit8 mode_data_len;
    ubit8 medium_type;
    ubit8 dev_spec_param;
    ubit8 blk_desc_len;
};

struct el_addr_assign_page {
    struct mode_param_hdr hdr;
    ubit8 ps : 1;
    ubit8 resrv0: 1;
    ubit8 page_code : 6;
    ubit8 param_len;
    ubit16 first_mt_el_addr;
    ubit16 num_mt_el_addr;
    ubit16 first_st_el_addr;
    ubit16 num_st_el_addr;
    ubit16 first_ie_el_addr;
    ubit16 num_ie_el;
    ubit16 first_dt_el_addr;
    ubit16 num_dt_el;
    ubit16 resrv1;
};

#define DT_EL_MAX 4

#define NODEV_FOUND 0x00
#define ADD_DEVICE 0x02

#ifdef __hp9000s700

#define TRY_NXT_LUN 0x01
#define FLOPPY_FOUND 0x04

#define STINGRAY_ID "C2425D"
#endif

/*
 **************************************************************
 */


#ifdef __hp9000s700

/*
 *  These defines are used to access elements of the
 *  hdw_path_type array.
 */
#define VSC       0
#define SLOT      1
#define FTN       2
#define SCSI_BA   3
#define HPIB_BA   3
#define SCSI_LUN  4
#define HPIB_UNIT 4


typedef struct{
      int null:8;
      int vsc:4;
      int slot:4;
      int ftn:4;
      int ba:4;
      int lu:4;
      int part:4;
} ScsiMinorType;

typedef struct{
      int null:8;
      int vsc:4;
      int slot:4;
      int ba:8;
      int unit:4;
      int vol:4;
} Cs80MinorType;


#else

/*
 *  These defines are used to access elements of the
 *  hdw_path_type array.
 *
 *  The hardware path for a SCSI device is:
 *
 *         <select code><slot num><ftn><ba><lun>
 *
 *
 *  The hardware path for an HPIB device is:
 *
 *         <select code><slot num><ba><unit>
 */
#define SELCODE   0
#define SCSI_BA	  1
#define SCSI_LUN  2

#define HPIB_BA	  1
#define HPIB_UNIT 2


typedef struct{
      int null:8;
      int selcode:8;
      int ba:8;
      int lu:4;
      int part:4;
} ScsiMinorType;

typedef struct{
      int null:8;
      int selcode:8;
      int ba:8;
      int unit:4;
      int vol:4;
} Cs80MinorType;

#endif  /* hp9000s700 */

typedef union{
        Cs80MinorType bitfld;
        int           num;
}Cs80MinorNum;

typedef union{
        ScsiMinorType bitfld;
        int           num;
}ScsiMinorNum;


#define DCONFIG_DRV_PATH "/dev/config"
#define DCONFIG_DRV_MAJOR 69
#define DCONFIG_DRV_MINOR 0x000000

extern int errno;


#define MAX_UNIT 2

/*************************************************************
*
*   NAME	:  ioscan_err
*
*   DESCRIPTION	:  This command prints the error message and
*		   then exits.
*
*   PARAMETERS	:  
*
*	passed in
*		   err_str -  the error message.
*	returned
*
**************************************************************
*/
void
ioscan_err( err_str )
     char *err_str;

{
         fprintf(stderr,"ioscan: %s\n", err_str);
	 exit(1);

} /* ioscan_err */


int
init_sem()
{
     int semid;

     semid = semget(ftok("/etc/ioscan_lock", 74 ), 1, IPC_CREAT|IPC_EXCL|0600);
     if( semid >= 0 )
     {
          (void)semctl(semid, 0, SETVAL, 1);
     }
     else
     {
          semid = semget(ftok("/etc/ioscan_lock", 74 ), 1, IPC_CREAT|0600);
          if(semid < 0)
              ioscan_err("semaphore failed");
     }
     return(semid);
}


int
rel_sem(semid)
int semid;
{
     int status;
     struct sembuf op;

     op.sem_num = 0;
     op.sem_op  = 1;
     op.sem_flg = SEM_UNDO;
     status = semop(semid, &op, 1);

     return(status);
} 


int
get_sem(semid)
     int semid;
{
     int status;
     struct sembuf op;

     op.sem_num = 0;
     op.sem_op  = -1;
     op.sem_flg = SEM_UNDO;
     while( (( status = semop(semid, &op, 1)) < 0) && ( errno == EINTR) )
     {
         sleep(1);
         status = 0;
     }

     if(status == -1)
	  ioscan_err("get_sem failed");
    else
          return( status );

}

/*************************************************************
*
*   NAME	: remove_devfile
*
*   DESCRIPTION	: This function removes a device special
*		  file by making a system call.
*
*   PARAMETERS	:  
*
*	passed in
*		  A string with the device special file name.
*	returned
*		  The results of the "rm" command.
*
**************************************************************
*/
int
remove_devfile(filename)
     char *filename;
{
     int err;

     err = unlink(filename);

     return(err);

}  /* remove_devfile */

/*************************************************************
*
*   NAME	: make_scsi_dev
*
*   DESCRIPTION	: This function makes a SCSI special device 
*		  file using the system call "mknod".
*
*   PARAMETERS	:  
*
*	passed in
*		  file_name - A string with the full path
*			      file name.
*		  dev_minor - The scsi minor number record.
*		  drv_major - The major number.
*		  bus_addr  - The bus address.
*	returned
*		  The return status of the system call "mknod".
*
**************************************************************
*/
int
make_scsi_dev(file_name, dev_minor, drv_major, bus_addr, lun)
     char *file_name;
     ScsiMinorNum dev_minor;
     int drv_major;
     int bus_addr;
     int lun;
{
     int err;
     mode_t mode;
     dev_t dev;

     dev_minor.bitfld.ba = bus_addr;
     dev_minor.bitfld.lu = lun;

     mode = ( S_IRWXU | _S_IFCHR );
     dev = makedev(drv_major, dev_minor.num);
     err = mknod(file_name, mode, dev, 0);

     return(err);

}  /* make_scsi_dev */


/*************************************************************
*
*   NAME	: make_hpib_dev
*
*   DESCRIPTION	: This function creates a HPIB device special
*		  file by making a system call, mknod.
*
*   PARAMETERS	:  
*
*	passed in
*		  file_name - A string with the full path
*			      file name.
*		  dev_minor - The hpib minor number record.
*		  drv_major - The major number.
*		  bus_addr  - The bus address.
*	returned
*		  The return status of the system call "mknod".
*
**************************************************************
*/
int
make_hpib_dev(file_name, dev_minor, drv_major, bus_addr, unit)
     char *file_name;
     Cs80MinorNum dev_minor;
     int drv_major;
     int bus_addr;
     int unit;
{
     int err;
     mode_t mode;
     dev_t dev;

     dev_minor.bitfld.ba = bus_addr;
     dev_minor.bitfld.unit = unit;

     mode = ( S_IRWXU | _S_IFCHR );
     dev = makedev(drv_major, dev_minor.num);
     err = mknod(file_name, mode, dev, 0);

     return(err);

}  /* make_hpib_dev */


/*************************************************************
*
*   NAME	:  init_hpib_minor
*
*   DESCRIPTION	:  This function initializes the upper 8 bits
*		   of an HPIB minor number.  This include the
*		   <vsc/slot> number on 700 and the <sc>
*		   on 300/400.
*   PARAMETERS	:  
*
*	passed in
*		   dev_minor - A pointer of the hpib minor number
*		               record.
*		   hdw_path  - A string representing the hard
*			       path.
*	returned
*		   dev_minor     - The hpib minor number initialized.
*
**************************************************************
*/
init_hpib_minor( dev_minor, hdw_path )
     Cs80MinorNum *dev_minor;
     char *hdw_path;
{
     char *str1, *str2;

     str1 = hdw_path;
     dev_minor->num = 0x00000000;

#ifdef __hp9000s700
     dev_minor->bitfld.vsc  = (*str1 != NULL_STR)? strtol(str1, &str2, 10) : -1;
     dev_minor->bitfld.slot = (*str2 != NULL_STR)? strtol(++str2, NULL, 10) : -1;
#else
     dev_minor->bitfld.selcode = (*str1 != NULL_STR)? strtol(str1, NULL, 10) : -1;
#endif


}   /* init_hpib_minor */


/*************************************************************
*
*   NAME	:  init_scsi_minor
*
*   DESCRIPTION	:  This function intializes the upper 12 bits
*		   of a 24 bit minor number.  On the 700 this
*		   includes the <vsc/slot/ftn> numbers, on the
*		   300/400 it includes the <sc/ftn> numbers.
*
*   PARAMETERS	:  
*
*	passed in
*		   dev_minor - A pointer of the scsi minor number
*		               record.
*		   hdw_path  - A string representing the hard
*                              path.
*	returned
*		   dev_minor     - The scsi minor number initialized.
*
**************************************************************
*/
init_scsi_minor( dev_minor, hdw_path )
     ScsiMinorNum *dev_minor;
     char *hdw_path;
{
     char *str1, *str2;

     str1 = hdw_path;
     dev_minor->num = 0x00000000;

#ifdef __hp9000s700
     dev_minor->bitfld.vsc  = (*str1 != NULL_STR)? strtol(str1, &str2, 10) : -1;
     dev_minor->bitfld.slot = (*str2 != NULL_STR)? strtol(++str2, &str1, 10) : -1;
     dev_minor->bitfld.ftn = (*str1 != NULL_STR)? strtol(++str1, NULL, 10) : 0;
#else
     dev_minor->bitfld.selcode = (*str1 != NULL_STR)? strtol(str1, &str2, 10) : -1;
#endif

}  /* init_scsi_minor */



/*************************************************************
*
*   NAME	: init_masks
*
*   DESCRIPTION	: This function initializes the masks passed
*	 	  in to all bits set.
*
*   PARAMETERS	:
*
*	passed in
*		  drv_mask   - A pointer to the driver mask.
*		  class_mask - A pointer to the class mask.
*	returned
*
*		  The above masks initialized to all ones.
*
**************************************************************
*/
void
init_masks( drv_mask, class_mask )
     MaskType *drv_mask;
     MaskType *class_mask;
{
     MaskType bit_pos;
     int i;

	/*
	   initialize drv_mask and class_mask to all bits set;
	*/

	SET_RIGHTMOST_BIT( bit_pos );
	for( i = 0; i < NUM_CLASS+1; i++)
	{
	     SET_BITS( *class_mask, bit_pos );
	     SHIFT_LEFT_MASK( bit_pos, 1 );
	}

	SET_RIGHTMOST_BIT( bit_pos );
	for( i = 0; i < DRV_TBL_SIZE+1; i++)
	{
	     SET_BITS( *drv_mask, bit_pos );
	     SHIFT_LEFT_MASK( bit_pos, 1 );
	}

}  /* init_masks */


/*************************************************************
*
*   NAME	: update_fld_len 
*
*   DESCRIPTION	: This functions updates the maximum field lengths
*                 in the header record of io map according to the
*		  new record added to the list.  If the length of
*		  any of the fields of the new record exceed the
*		  old maximum length then the maximum field length
*		  is adjusted.
*		  The maximum field lengths are used to format the
*		  output listing, justifying the columns.
*
*   PARAMETERS	:
*
*	passed in
*		io_map - A pointer to the header record of the
*			 io map.
*
*		new_rec - A pointer to the new record add to the
*			  io map.
*
*	returned
*		io_map - The fields of the header record updated.
*
**************************************************************
*/
void
update_fld_len( io_map, new_rec )
	IoMapType io_map[];
	IoMapType *new_rec;
{
	int len;

	/*
	 *  Adjust the maximum field length variable for each
	 *  field if the new entries exceed the current maximums.
	 */
	len = (int)strlen( ClassList[new_rec->mod_rec.class] );
	if( len > io_map[0].hdr_rec.max_class_len )
	     io_map[0].hdr_rec.max_class_len = len;

	len = (int)strlen( drv_tbl[new_rec->mod_rec.sw_path].drv_name );
	if( len > io_map[0].hdr_rec.max_sw_path_len )
	     io_map[0].hdr_rec.max_sw_path_len = len;

	len = (int)strlen( new_rec->mod_rec.desc );
	if( len > io_map[0].hdr_rec.max_desc_len )
	     io_map[0].hdr_rec.max_desc_len = len;

	len = (int)strlen( new_rec->mod_rec.hw_status );
	if( len > io_map[0].hdr_rec.max_hw_status_len )
	     io_map[0].hdr_rec.max_hw_status_len = len;

	len = (int)strlen(new_rec->mod_rec.hw_path);
	if( len > io_map[0].hdr_rec.max_hw_path_len )
	     io_map[0].hdr_rec.max_hw_path_len = len;

	len = (int)strlen(new_rec->mod_rec.sw_status);
	if( len > io_map[0].hdr_rec.max_sw_status_len )
	     io_map[0].hdr_rec.max_sw_status_len = len;

} /* update_fld_len */


/*************************************************************
*
*   NAME	: add_dev_rec_to_io_tree
*
*   DESCRIPTION	: This function adds an device record to the
*		  io map list.
*
*   PARAMETERS	:
*
*	passed in
*		  dev_info - device information.
*		  io_map   - pointer to the first record of the
*			     I/O map list.
*		  class    - Index into the class list for device.
*		  driver   - Index into the driver table for device.
*		  path     - The hardware path record.
*		  ba       - bus address of device.
*		  lun	   - Logical unit number if applicable.
*		  desc     - Description string (SCSI only).
*		  print_it - 1 if record should be printed, else 0.
*
*	returned
*		  return   - index of new record.
*               
*
**************************************************************
*/
add_dev_rec_to_io_tree( dev_info, io_map, class, driver, path, ba, lun, desc,
					                              print_it)
     int dev_info;
     IoMapType io_map[];
     int class;
     int driver;
     HwPathType path;
     int ba;
     int lun; 
     char *desc;
     int print_it;
{

     IoMapType *new_rec;
     int rec_idx;

     rec_idx = io_map[0].hdr_rec.nxt_avail++;
     new_rec = &(io_map[rec_idx]);


     /*
      * set class index
      */
     new_rec->mod_rec.class = class;

     /*
      * copy driver index to software path
      */
     new_rec->mod_rec.sw_path = driver;

     /*
      * set hardware path field
      */

#ifdef __hp9000s700
     if( lun >= 0 )
          sprintf( new_rec->mod_rec.hw_path,"%s.%d.%d",
						  path,
						  ba,
						  lun
						 );
     else
          sprintf( new_rec->mod_rec.hw_path,"%s.%d",
						  path,
						  ba
						 );
#else
     sprintf( new_rec->mod_rec.hw_path,"%s.%d",
						  path,
						  ba
						 );
#endif

     if( *desc != NULL_STR )
          strcpy( new_rec->mod_rec.desc, desc );
     else
          new_rec->mod_rec.desc[0] = NULL_STR;

     sprintf( new_rec->mod_rec.hw_status,"ok(0x%x)", dev_info );

     new_rec->mod_rec.hdw_type = DEVICE;

     strcpy( new_rec->mod_rec.sw_status, SW_STATUS_OK );

     /*
      * Print this node if specified and update the
      * field lengths.
      */
     if( print_it )
     {
          new_rec->mod_rec.print_node = TRUE;
          io_map[0].hdr_rec.num_print++;
          update_fld_len( io_map, new_rec );
     }
     else
          new_rec->mod_rec.print_node = FALSE;

    return( rec_idx );

}  /* add_dev_rec_to_io_tree */



/*************************************************************
*
*   NAME	: check_hpib_device
*
*   DESCRIPTION	: This function checks for an hpib device.  It
*		  attempts to open the device file (passed in
*		  as a parameter).  If the open succeeds it does
*		  an HPIB identify command.  If the identify command
*		  works, it returns the HPIB identify bytes to 
*		  the caller.
*
*   PARAMETERS	:
*	passed in
*		device_name - A string representing the full path
*			      name of the device file.
*		dev_info    - A pointer to an integer where the
*			      HPIB identify data is to be returned.
*	returned
*		dev_info    - The HPIB identify data if a device was
*			      found.
*		return	    - TRUE if a device has been identified,
*			      otherwise FALSE.
*               
*
**************************************************************
*/
check_hpib_device( device_name, dev_info )
	char *device_name;
	struct io_hpib_ident *dev_info;
{

  int fd;
  int found;
  struct io_hpib_ident info;
  extern int errno;
  

  found = FALSE;
  fd = open( device_name, O_RDONLY );
  if( fd >= 0 )
  {
      if( (ioctl( fd, IO_HPIB_IDENT, &info )) >= 0 )
      {  
	   found = TRUE;
	   dev_info->ident = info.ident;
	   dev_info->dev_type = info.dev_type;
      }
      close( fd );
  }
  return( found );
}


/*************************************************************
*
*   NAME	: check_scsi_dev
*
*   DESCRIPTION	: This function attempts to open and identify
*		  a SCSI device.  If it succeeds, it returns
*		  three pieces of information, see below.
*
*   PARAMETERS	:
*
*	passed in
*		  dev_file - the device file to be opened.
*
*	returned
*		  dev_info - If the device was opened, and the 
*			     SIOC_INQUIRY command succeed, the
*			     first 4 bytes of the inquiry record.
*		  dev_type - If a device was opened, the dev_type 
*			     field of the inquiry record.
*		  desc     - If a device was opened, a string with
*			     the vendor id and product id fields
*			     of the inquiry record.
*	 	  return   - 1 if a device was identified, otherwise
*			     0.
*
**************************************************************
*/
unsigned int
check_scsi_dev(dev_file, dev_info, dev_type, desc)
     char     *dev_file;
     int      *dev_info;
     int      *dev_type;
     char     *desc;
{
     int fd;
     unsigned int found;
     int i;
     union inquiry_data iqr_rec;
     char tmp_str[MAX_DESC];
     int tmpinfo;

#ifdef __hp9000s700
     struct sctl_io sctl_io;
     unsigned char sense_data[255];
#endif
    
     found = NODEV_FOUND;

     for( i = 0; i < MAX_DESC; i++)
        tmp_str[i] = ' ';

     fd = open( dev_file, O_RDONLY);
     if( fd >= 0 )
     {
         if (ioctl(fd, SIOC_INQUIRY, &iqr_rec) >= 0 )
         {
            found = ADD_DEVICE;
            *dev_type = iqr_rec.inq2.dev_type;
#ifdef __hp9000s700
	    if( *dev_type != NO_SCSI_DEV )
	    {
#endif
                 *dev_info = *((int *)&iqr_rec);
	         strncpy(tmp_str, iqr_rec.inq2.vendor_id, 8);
                 tmp_str[8] = '\0';
	         strncat(tmp_str, iqr_rec.inq2.product_id, 16);
                 tmp_str[24] = '\0';

#ifdef __hp9000s700
                 /*
		  * this is a kludge to identify floppy
		  * devices on the 700.
		  */
                 if( *dev_type == SCSI_DIRECT_ACCESS )
		 {
		      memset(sctl_io, 0, sizeof(sctl_io));
		      sctl_io.flags = SCTL_READ;
		      sctl_io.cdb[0] = CMDmode_sense;
		      sctl_io.cdb[1] = 0x00;
		      sctl_io.cdb[2] = 5;
		      sctl_io.cdb[3] = 0x00;
		      sctl_io.cdb[4] = 255;
		      sctl_io.cdb[5] = 0x00;
		      sctl_io.cdb_length = 6;
		      sctl_io.data = &sense_data[0];
		      sctl_io.data_length = 255;
		      sctl_io.max_msecs = 10000;

		      if( (ioctl(fd, SIOC_IO, &sctl_io) >= 0) &&
			( sctl_io.cdb_status == S_GOOD ) )
			     found |= FLOPPY_FOUND;
		 }
                 /*
		  * If we found a stringray disc array then we need to
		  * inform the calling process since stringrays support
		  * multiple luns.  
		  * If the periph_qualifier field is 0 then this is a
		  * valid lun for the stingray and we need to add the
		  * device, otherwise it is not we just need to check
		  * the next lun.
		  */
		 if( !(strncmp(STINGRAY_ID, iqr_rec.inq2.product_id, 6)) )
		 {
		      if(iqr_rec.inq2.periph_qualifier == 0)
		           found |= TRY_NXT_LUN;
		      else
		           found = TRY_NXT_LUN;
		 }
#endif

#ifdef __hp9000s700
	    }
#endif
         }
         close(fd);
     }

     strncpy(desc, &tmp_str[0], MAX_DESC);
     return( found );

}  /* check scsi_disk */


/*************************************************************
*
*   NAME	: scan_for_hpib_devices
*
*   DESCRIPTION	:  This functions scans the specified HPIB bus
*		   for peripheral devices.  It walks the specified
*		   range of HPIB bus addresses querying for devices.
*		   If a hardware path has been specified by the -H
*		   option, and the path includes a bus address we
*		   only use that bus address otherwise we default
*		   to the full range of bus addresses.
*	           For each HPIB device driver specified in the
*	           array "hpib_dev_drvs" we query for a device.
*		   If a device responds, we record it and go to the
*		   next address otherwise we try the next driver.
*
*   PARAMETERS	:
*	passed in
*		io_map     - The io map.
*		if_rec     - The index of the HPIB interface card in
*			     the io_map.
*		hw_path    - The hardware path record, if one was
*			     specified by the -H option,
*		drv_mask   - The driver mask indicating which drivers
*		class_mask - The class mask indicating what classes
*			     of devices to look for.
*	returned
*		io_map     - The io map updated to include new records
*			     for any HPIB devices found.
*               
*
**************************************************************
*/
int
scan_for_hpib_devices( io_map, if_rec, hw_path, drv_mask, class_mask )
     IoMapType *io_map;
     int if_rec;
     hdw_path_type hw_path;
     MaskType drv_mask;
     MaskType class_mask;
{
   

     int err;
     IoMapType *isc_rec;
     HwPathType path;
     int first_ba, last_ba;
     Cs80MinorNum dev_minor;
     char dev_file[30];
     char cmd[80];
     int bus_addr;
     int found;
     struct io_hpib_ident dev_info;
     int idx;
     int pid;
     int drv_major;
     unsigned char dev_type;
     int add_dev;
     int nxt_drv;
     int drv_idx;
     int check_next_unit;
     int unit;

     /*
      * Test to see if any of the HPIB device drivers
      * are included in the driver mask.  If not then
      * just return.
      */
     if( !(TEST_BITS(drv_mask, hpib_drv_mask)) )
          return;

     isc_rec = &(io_map[if_rec]);
     strcpy( path, isc_rec->mod_rec.hw_path );

     init_hpib_minor( &dev_minor, 
		      path
		    );

     /*
      * If a hardware path has been specified and it includes
      * the bus address, then only search that bus address,
      * otherwise scan the range of HPIB bus addresses.
      */
     if( hw_path.num_elements > HPIB_BA )
     { 
  	  first_ba = last_ba = hw_path.addr[HPIB_BA];
     }
     else
     {
         first_ba = HPIB_START_ADDR;
         last_ba  = MAX_HPIB_ADDR - 1;
     }

     /*
      * Create a unique device file name for
      * using the pid of this process.
      */
     strcpy( dev_file, IOSCAN_DEV_FILE );
     pid = getpid();
     strcat( dev_file, (char *)ltostr( pid, 10 ) );

     for( bus_addr = first_ba; bus_addr <= last_ba; bus_addr++ ) 
     {
	  found = FALSE;
	  nxt_drv = 0;
	  while( (found == FALSE) && ( nxt_drv < MAX_HPIB_DRVS) )
	  {
               remove_devfile(dev_file);
	       drv_idx = hpib_dev_drvs[nxt_drv];
	       if(( drv_idx >= 0 ) &&
	          ( TEST_BITS(drv_mask, drv_tbl[drv_idx].drv_mask )) )
	       {
		         drv_major = drv_tbl[drv_idx].drv_major;

			 check_next_unit = FALSE;
			 unit = 0;

			 do
			 {

                         err = make_hpib_dev(dev_file, 
				             dev_minor, 
				             drv_major, 
				             bus_addr,
				             unit);
                         /*
		          * This shouldn't happen but if it does
		          * then bailout.
		          */
                         if(err != 0)
			     ioscan_err("mknod failed");

		         found = check_hpib_device( dev_file, &dev_info );
		         if ( found )
		         {
		                add_dev = FALSE;
		                switch( dev_info.dev_type )
		                {
		                   case 0:   /* fixed disk */
			           case 1:  /* removable disk or combination */

			   	          if( TEST_BITS(class_mask, DISK_CLASS) )
		      	 	          {
				             add_dev = TRUE;
			                     dev_type = DISK_C_ENT;
		      	 	          }
				          break;
			           case 2:  /* tape */

			  	          if( TEST_BITS(class_mask, TAPE_CLASS) )
		          	          {
					          add_dev = TRUE;
			        	          dev_type = TAPE_C_ENT;
		          	          }
				                  break;
		                }  /* switch */
               		        idx = add_dev_rec_to_io_tree( dev_info.ident,
					    		      io_map,
					    		      dev_type,
					    		      drv_idx,
					   		      path,
					    		      bus_addr,
					    		      unit,
					    		      NULL_STR,
					    		      add_dev
			     		  		    );
		         if( ( !(strcmp(drv_tbl[drv_idx].drv_name, "cs80")) ||
		             !(strcmp(drv_tbl[drv_idx].drv_name, "amigo")) ) &&
			     ( unit < MAX_UNIT) )
			 {
               			remove_devfile(dev_file);
				check_next_unit = TRUE;
				unit++;
			 }
			 else
				check_next_unit = FALSE;

		         }  /* if found */

			 }
			 while( (found ) && ( check_next_unit ) );

	     }  /* if drv_idx */

	     nxt_drv++;

         }  /* while */
     }
     remove_devfile( dev_file );

} /* scan_for_hpib_devices */


#ifdef __hp9000s300

#define BUF_SIZE 4096
#define STATUS_HDR_LEN 0x8
#define PC_HDR_LEN 0x8

#define MAKE_WORD( hb, lb) (((unsigned)(hb & 0xff) << 8) + ((unsigned)(lb & 0xff)))

try_autoch_driver(dev_file, drvs_ba)
     char *dev_file;
     int *drvs_ba;
{

     int fd;
     int i;
     int page_len, desc_len, num_recs, desc_offset;
     unsigned char byte1, byte2;
     unsigned char type_code;
     unsigned char drive_num;
     int data_len;
     int offset;
     int err;
     int bytes_read;
     char *buf;

     fd = open(dev_file, O_RDWR);
     if( fd  >= 0 )
     {
	  err = ioctl(fd, ACIOC_READ_ELEMENT_STATUS);
	  if( !err )
	  {
	       buf = malloc( BUF_SIZE );
	       bytes_read = read(fd, &buf[0], BUF_SIZE);
	       if( bytes_read > 0 )
	       {
		    offset = 0;
		    data_len = MAKE_WORD( buf[6], buf[7] );
		    while( offset < data_len )
		    {
			 type_code = buf[ offset + STATUS_HDR_LEN ];

			 byte1 = buf[ offset + STATUS_HDR_LEN + 2 ];
			 byte2 = buf[ offset + STATUS_HDR_LEN + 3 ];
			 desc_len = MAKE_WORD( byte1, byte2 );

			 byte1 = buf[ offset + STATUS_HDR_LEN + 6 ];
			 byte2 = buf[ offset + STATUS_HDR_LEN + 7 ];
			 page_len = MAKE_WORD( byte1, byte2 );

			 switch ( type_code )
			 {
			 case 0x04:
				 num_recs = page_len / desc_len;

				 desc_offset = STATUS_HDR_LEN+PC_HDR_LEN+offset;
				 for( i = 0; i < num_recs; i++ )
				 {
				      drive_num = buf[ desc_offset + 7 ];
				      drvs_ba[drive_num] = TRUE;
				      desc_offset += desc_len;
				 }
				  
				  break;
			 default:
				   break;

			 }  /* switch */

			 offset += (page_len + PC_HDR_LEN);

		    }  /* while */

	       }
	       free( buf );
	  }
	  close(fd);

     }


}

#endif /* __hp9000s300 */

/*************************************************************
*
*   NAME	: get_autoch_drivers
*
*   DESCRIPTION	: This function reads the element status
*		  data from an autochanger device to get the
*		  bus addresses of the autochanger drives.
*		  It then records the drive addresses in the
*		  array "dev_addrs".
*		  It uses command mode to read the element status
*		  data in the following way.
*
* 		  Go to command mode.  If successful send the command
*		  to set up the read of the mode sense page and then
*		  read the mode sense page.  Determine the number of data 
*		  transfer elements to be read (from the mode sense page)
*		  and then for each data transfer element, send the
*		  command to set up the read and then read it.
*
*
*   PARAMETERS	:  
*	passed in
*		  devfile   - The name of the device special file.
*			      which is the autochanger.
*		  drv_addrs - The address of an array of integers
*	returned
*	 	  drv_addrs - For each autochanger driver found,
*			      the entry that corresponds to the
*			      bus address of the driver set to 
*			      TRUE.
*
**************************************************************
*/
int
get_autoch_drivers(devfile, drv_addrs)
     char *devfile;
     int *drv_addrs;
{

     int fd;
     int ret;
#ifdef __hp9000s700
     int err;
     int i;
     int drive, num_drives;
     struct element_addresses addr_struct;
     struct element_status status_struct;

     ret = FALSE;

     fd = open( devfile, O_RDONLY );
     if( fd >= 0 )
     {
	  err = ioctl(fd, SIOC_ELEMENT_ADDRESSES, &addr_struct);
	  if( err >= 0 )
	  {
	       drive = addr_struct.first_data_transfer;
	       num_drives  = addr_struct.num_data_transfers;

	       for( i = 0; i < num_drives; i++, drive++ )
	       {
		    status_struct.element = drive;
	            err = ioctl( fd, SIOC_ELEMENT_STATUS, &status_struct );
		    if( ( err >= 0 ) && ( status_struct.id_valid ) )
			      drv_addrs[status_struct.bus_address] = TRUE;
	       }
	  }

	  close(fd);
     }
     else 
	  ret = errno;

     return( ret );

#else
     int flag;
     struct scsi_cmd_parms cmd;
     struct el_addr_assign_page assign_page;
     int page_size = sizeof(struct el_addr_assign_page);
     struct dt_el_desc desc;
     int desc_size = sizeof(struct dt_el_desc);
     int el;
     int idx;


     ret = FALSE;

     if ((fd = open(devfile, O_RDWR)) != -1)
     {
          flag = 1;

          /*
	   * Go to command mode.  If successful send the command
	   * to set up the read of the mode sense page and then
	   * read the mode sense page.  Determine the number of data 
	   * transfer elements to be read (from the mode sense page)
	   * and then for each data transfer element, send the
	   * command to set up the read and then read it.
	   */
          if (ioctl(fd, SIOC_CMD_MODE, &flag) != -1)
          {

    		/*
     	 	 * initialize the mode sense data command
     		 * and send the ioctl command down.
    		 * If the ioctl succeeds then read the data.
     		 */
    		cmd.cmd_type = 6;
    		cmd.cmd_mode = 1;
    		cmd.clock_ticks = 300;
    		cmd.command[0] = 0x1a;
    		cmd.command[1] = 0x00;
    		cmd.command[2] = 0x1d;
    		cmd.command[3] = 0x00;
    		cmd.command[4] = page_size;
    		cmd.command[5] = 0x00;

    		if (ioctl( fd, SIOC_SET_CMD, &cmd ))
	 		return(0);

    		if (read(fd, &assign_page, page_size) != page_size)
	 		return(0);

    		/*
     		*  If there are more data transfer elements
     		*  than bus address we've got a problem.
     		*/
    		if (assign_page.num_dt_el > DT_EL_MAX)
	 		return(0);


     		/*
      		* Initialize the command record.
      		*/
     		cmd.cmd_type = 12;
     		cmd.cmd_mode = 1;
     		cmd.clock_ticks = 300;
     		cmd.command[0] = 0xb8;
     		cmd.command[1] = 0x04;
     		cmd.command[4] = 0x00;
     		cmd.command[5] = 0x01;
     		cmd.command[6] = 0x00;
     		cmd.command[7] = desc_size >> 16;
     		cmd.command[8] = desc_size >> 8;
     		cmd.command[9] = desc_size;
     		cmd.command[10] = 0x00;
     		cmd.command[11] = 0x00;
 
     		/*
      		* now for each data transport record, lets send a
      		* command to read the record in.
      		*/
     		for (idx = 0; idx < assign_page.num_dt_el; idx++)
     		{
         		el = assign_page.first_dt_el_addr + idx;
 
         		cmd.command[2] = el >> 8;
         		cmd.command[3] = el;
 
         		if (ioctl(fd, SIOC_SET_CMD, &cmd) != -1)
              		if (read(fd, &desc, desc_size) == desc_size)
 	 	  		drv_addrs[desc.scsi_bus_addr] = TRUE;
    		 }

          }
           else
	       ret =  errno;

          close(fd);
     }
     else
	  ret =  errno;

     return( ret );

#endif

}  /* get_autoch_drivers */


/*************************************************************
*
*   NAME	:  convert_index_to_bit_position
*
*   DESCRIPTION	:  This function takes a index number indicating
*                  a bit position and returns a mask with
*                  the bit set.  The rightmost bit is treated as
*		   bit 0 which corresponds to index 0.
*
*   PARAMETERS	:
*
*	passed in
*	         index - bit position
*
*	returned
*                mask  - mask with bit set.
*
**************************************************************
*/
void
convert_index_to_bit_position( mask, index )
	MaskType *mask;
	unsigned int index;
{
  SET_RIGHTMOST_BIT( *mask );
  SHIFT_LEFT_MASK( *mask, index );
}
 


/*************************************************************
*
*   NAME	: scan_for_scsi_devices
*
*   DESCRIPTION	: This function scans the SCSI bus looking for
*		  SCSI devices. 
*
*   PARAMETERS	:
*	passed in
*		io_map   - A pointer to the array of io map records.
*		if_rec   - The index of the record for the SCSI 
*		 	   interface card.
*		hw_path  - Any hardware path specified by the -H
*			   option.
*		drv_mask - The driver mask.
*
*	returned
*		io_map   - The io map updated with any new device
*			   records.
*               
*
**************************************************************
*/
scan_for_scsi_devices( io_map, if_rec, hw_path, drv_mask, class_mask )
	IoMapType *io_map;
	int if_rec;
	hdw_path_type hw_path;
	MaskType drv_mask;
	MaskType class_mask;
{
    int err;
    int dev_info;
    int dev_type;
    int i, j;
    unsigned int found;
    int rec_idx[MAX_SCSI_ADDR];
    int drvs_ba[MAX_SCSI_ADDR];
    IoMapType *isc_rec;
    unsigned int driver;
    unsigned int class;
    int scsi_major;
    int autox_idx;
    int this_drv;
    int autox_major;
    int pid;
    int first_ba, last_ba;
    int idx;
    char cmd[80];
    char dev_file[30];
    unsigned char autoch_ba;
    ScsiMinorNum scsi_minor;
    HwPathType path;
    extern MaskType scsi_drv_mask;
    char desc[MAX_DESC];
    int add_device;
    int nxt_drv;

   
    /*
     * Determine if any SCSI drivers have been
     * specified in the drv_mask, if not return.
     */
     if( !TEST_BITS(drv_mask, scsi_drv_mask) )
  	  return;

     /*
      * Get the record for the interface card and initialize
      * the SCSI minor number using the hard path of the
      * interface card.
      */
     isc_rec = &(io_map[if_rec]);
     strcpy(path, isc_rec->mod_rec.hw_path);
     init_scsi_minor( &scsi_minor, 
		      path
		    );
 
     /*
      *  Determine the range of scsi bus addresses to test.
      *  If the hardware path (-H option) includes a bus 
      *  address then use it, otherwise use the full range
      *  of SCSI bus addresses.
      */
      if( hw_path.num_elements > SCSI_BA )
      {
           first_ba = last_ba = hw_path.addr[SCSI_BA];
           if( (first_ba < SCSI_START_ADDR) || (first_ba >=  MAX_SCSI_ADDR) )
                return;
      }
      else
      {
          first_ba = SCSI_START_ADDR;
          last_ba  = MAX_SCSI_ADDR - 1;
      }
      for(i = first_ba; i <= last_ba; i++)
      {
	   drvs_ba[i] = FALSE;
	   rec_idx[i] = FALSE;
      }

      /*
       * Create a unique device file name for this process using
       * the procesess pid.  This is necessary since there can be
       * be multiple concurrent processes executing ioscan.
       */
      strcpy(dev_file, IOSCAN_DEV_FILE);
      pid = getpid();
      strcat(dev_file, (char *)ltostr( pid, 10) );

      /*
       * Now lets walk the range of bus addresses/luns looking
       * for SCSI devices and using the drivers specified in 
       * "scsi_dev_drvs".  If we find a device, determine what type
       * or class of devices it is and then add it to the io map.
       * If we don't find a device for a given bus address, lun
       * combination the break out of the lun loop and go to
       * the next bus address.
       * The assumption here is that all lun's are contiguous, 
       * therefore if there is nothing at lun(x) then there
       * will also be nothing at lun(x+1).
       * If a device is an autochanger then we need to identify
       * which addresses are autochanger drivers.
       */
      for( i = first_ba; i <= last_ba; i++ ) 
      {
	   for( j = FIRST_LU; j < MAX_SCSI_LU ; j++)
	   {
		nxt_drv = 0;
	        found = FALSE;

               /*
	        * Use each of the drivers specified in the "scsi_dev_drvs"
	        * to probe for devices.
	        */
		while( nxt_drv < MAX_SCSI_DEV_DRVS )
		{
		     scsi_major = drv_tbl[scsi_dev_drvs[nxt_drv]].drv_major;
                     remove_devfile(dev_file);

                     err = make_scsi_dev(dev_file,
				         scsi_minor,
				         scsi_major, 
				         i, 
				         j);

                     /*
		      * This shouldn't happen but if it does
		      * then bailout.
		      */
                     if(err != 0)
			 ioscan_err("mknod failed");

		     found = check_scsi_dev( dev_file,
			                     &dev_info,
			                     &dev_type,
				             desc
				           );
		     if( found != NODEV_FOUND )
			  break;
		     else
			  nxt_drv++;
		}
		if ( found & ADD_DEVICE )
		{
		     add_device = FALSE;
		     if(( dev_type >= 0 ) && ( dev_type < MAX_SCSI_DEV_TYPES ))
		          driver = scsi_dev_types[ dev_type ];
		     else
			  driver = UNKNW_D_ENT;

		     switch( dev_type )
		     {
			 /*
			  *  This is a kludge since one of the scanner devices
			  *  currently supported reports its dev type as a
			  *  processor.
			  */
			 case SCSI_PROCESSOR:
			 case 0x6:		/* scanner dev_type */  
			           class = SCANNER_C_ENT;
				   break;
		         case SCSI_DIRECT_ACCESS:
#ifdef __hp9000s700
			           if( found & FLOPPY_FOUND )
				         driver = FLOPPY_D_ENT;
#endif
		         case SCSI_WORM:
		         case SCSI_CDROM:
		         case SCSI_MO:
			           class = DISK_C_ENT;
			           break;
		         case SCSI_SEQUENTIAL_ACCESS:
			           class = TAPE_C_ENT;
			           break;
		         case SCSI_AUTOCHANGER:
#ifdef __hp9000s700
			           class = AUTOCH_C_ENT;
                                   remove_devfile(dev_file);

			           autox_idx = scsi_dev_types[dev_type];
	                           autox_major = drv_tbl[autox_idx].drv_major;

                                   err = make_scsi_dev(dev_file,
			              		       scsi_minor,
			          		       autox_major, 
			           		       i, 
			           		       j);

			           err = get_autoch_drivers( dev_file,
				    		             &drvs_ba[0] );
#else
			           class = AUTOCH_C_ENT;
			           err = get_autoch_drivers( dev_file,
						             &drvs_ba[0] );
				   if( err == EBUSY )
				   {
                                        remove_devfile(dev_file);
					autoch_ba = ((i << 4) & 0xf0);
                                        err = make_scsi_dev(dev_file,
			              		            scsi_minor,
			          		            55, 
			           		            autoch_ba, 
			           		            j);

				        try_autoch_driver( dev_file,
				    		           &drvs_ba[0] );
				   }
#endif
			           break;
		         default:
			           class = UNKNW_C_ENT;
			           break;
		         }

		         if( (TEST_BITS(drv_mask, (DEF_MASK(driver)) )) &&
		           (TEST_BITS(class_mask, (DEF_MASK(class)) )))
		               add_device = TRUE;

		         idx = add_dev_rec_to_io_tree( dev_info,
			    			       io_map,
						       class,
						       driver,
						       path,
						       i,
						       j,
						       desc,
						       add_device
						     );
		         rec_idx[i] = idx;

	       }  /* if found */

#ifdef __hp9000s700
                  /*
		   * If we found a stingray then we want to
		   * cycle through the range of lun addresses
		   * since luns are only being supported on
		   * stringrays.
		   * Otherwise lets break out of the lun loop.
		   */
		  if( !(found & TRY_NXT_LUN) )
		       break;

#endif

	  }  /* for j */
        }  /* for i */
   remove_devfile(dev_file);

   /*
    * Update any autochanger driver records.
    */
   for( i = first_ba; i <= last_ba; i++ )
	if( ( drvs_ba[i] ) && ( rec_idx[i] ) )
	{
	     io_map[rec_idx[i]].mod_rec.class = AUTOCH_C_ENT;
	     this_drv = io_map[rec_idx[i]].mod_rec.sw_path;

	     if( (TEST_BITS(class_mask, AUTOCH_CLASS)) &&
		 (TEST_BITS(drv_mask, (DEF_MASK(this_drv)))) )
	     {
	          io_map[rec_idx[i]].mod_rec.print_node = TRUE;
                  update_fld_len( io_map, &(io_map[ rec_idx[i] ]) );
	     }
	     else
	          io_map[rec_idx[i]].mod_rec.print_node = FALSE;
	}

}  /* scan_for_scsi_devices */


/*************************************************************
*
*   NAME	: usage
*
*   DESCRIPTION	: This function prints out the error message
*		  (passed in) along with the command usage syntax
*		  and then exits.
*
*   PARAMETERS	:
*
*	passed in
*	         err_str - The error message.
*
*	returned
*
**************************************************************
*/
void
usage(err_str)
    char *err_str;
{
    if( err_str != NULL_STR )
         fprintf(stderr,"ioscan: %s\n", err_str);

    fprintf(stderr,"\nusage: ioscan [-d driver | -C class] [-H hw_path] [-f]\n");
    fprintf(stderr,"       -d: show hardware controlled by specified driver\n");
    fprintf(stderr,"       -C: show hardware in specified class\n");
    fprintf(stderr,"       -H: show hardware at specified path\n");
    fprintf(stderr,"       -f: give full listing\n");
    exit(1);
}



/*************************************************************
*
*   NAME	:  driver_in_kernel
*
*   DESCRIPTION	:  This function checks if a driver is configured
*	           in the kernel.  There are two different cases we
*		   must handle, device drivers and interface drivers.
*		   If the driver is a device driver then we need to
*		   know if it is in the kernel since it will be used
*		   to probe for disk and tape devices.  If it is an
*		   interface driver then we will just return (1) since
*		   we do not use the interface driver to look for hardware
* 		   (interface cards).  Interface cards will be found
*		   by scanning the isc_table. In order for a card to
*		   have an entry in the isc table the interface driver
*		   must be in the kernel.
*
*   PARAMETERS	:
*	passed in
*		   drv_index - index of driver in the driver table.
*	returned
*               
*
**************************************************************
*/
driver_in_kernel( drv_index )
     int drv_index;
{
     int drv_major;
     int drv_type;
     int found;
     int err;
     int fd;
     config_drv_status drv_rec;

     if( (drv_index >= 0) && (drv_index <= DRV_TBL_SIZE) )
     {
	  drv_type = drv_tbl[drv_index].drv_type;
	  drv_major    = drv_tbl[drv_index].drv_major;

	  switch(drv_type)
	  {
	  case IF_DRV:
	            found = TRUE;
	            break;
	  case DEV_DRV:
	  case DEV_IF_DRV:
	            if(drv_major >= 0)
		    {
		         if( (fd = open(DCONFIG_DRV_PATH, O_RDONLY)) >= 0)
		         {
			       drv_rec.drv_major = drv_major;
			       err = ioctl(fd, CONFIG_DRIVER_STATUS, &drv_rec);
			       if( (err != -1) && (drv_rec.drv_status == 0) )
			           found = TRUE;
			       else
			           found = FALSE;

                               close(fd);
		         }
	 	    }
		    else
		         found = FALSE;
		    break;
	  default:
	            found = TRUE;
		    break;

	  } /* switch */
     }  /* if */
     else
         found = FALSE;

     return(found);

} /* drv_configured */


/*************************************************************
*
*   NAME	: convert_bit_position_to_index
*
*   DESCRIPTION	: This function takes a bit mask with one
*                 bit set and returns the position of the
*                 bit.  If more than one bit is set it will
*                 return the bit position of the rightmost bit
*                 set.  If no bits are set it will return a -1.
*
*   PARAMETERS	:
*
*	passed in
*		mask - mask to be mapped to index
*
*	returned
*               return - integer indicating which bit is set.
*               
*
**************************************************************
*/
int
convert_bit_position_to_index(mask)
	MaskType mask;
{
	int i;
	MaskType bit_pos;

	if( DIFF_MASKS( mask, NULL_MASK ) )
	{
             SET_RIGHTMOST_BIT( bit_pos );
	     for( i = 0; ; i++ )
	     {
	          if( TEST_BITS( mask, bit_pos ) )
	               break;
	          else
                       SHIFT_LEFT_MASK( bit_pos, 1 );
	     }
	}
	else
	     i = -1;

	return( i );
}


/*************************************************************
*
*   NAME	:  set_drv_mask_to_class_defaults
*
*   DESCRIPTION	:  This function defaults the drv_mask according to
*		   the bits set in the class_mask.  It searches the
*		   field drv_class in the driver table for any device
*		   drivers that support the class of devices.  For
*		   each driver found it sets the appropriate bit
*		   in the drv_mask by calling the function set_driver.
*	           Set_drv_mask_to_class_defaults returns TRUE if at
*		   least one driver which controls the class of 
*		   devices specified in class_mask was set in the
*		   drv_mask.
*
*
*   PARAMETERS	:
*
*	passed in
*	         class_mask - class mask specifying what classes.
*
*	returned
*                drv_mask   - driver mask set for corresponding
*		              drivers.
*
**************************************************************
*/
int
set_drv_mask_to_class_defaults(drv_mask, class_mask)

	MaskType *drv_mask;
	MaskType *class_mask;

{
     MaskType drv_bit;
     int drv_index;
     int driver_found;

     SET_MASK( *drv_mask, NULL_MASK );
     driver_found = FALSE;

     for( drv_index = 0; drv_index < DRV_TBL_SIZE; drv_index++ )
     {
         if( TEST_BITS( drv_tbl[drv_index].drv_class, *class_mask ) )
         {
	      if( set_driver( drv_index, drv_mask ) )
		    driver_found = TRUE;
         }
     }
     return(driver_found);

}  /* set_drv_mask_to_class_defaults */


/*************************************************************
*
*   NAME	: set_driver
*
*   DESCRIPTION	: This function sets the specified driver bit 
*		  in the driver mask if the driver is in the
*		  kernel and any required drivers are also available.
*
*   PARAMETERS	:
*
*		passed in
*
*			drv_index - The index of the driver whose
*				    required driver field is checked.
*
*			drv_mask - The driver mask with specified
*				   drivers.
*
*		returned
*
*			drv_mask - The driver mask updated with
*				   any additional drivers.
*
*			return   - TRUE if driver bit was set in mask,
*				   FALSE if not.
*
* **************************************************************
*/
int
set_driver(drv_index, drv_mask)
     int drv_index;
     MaskType *drv_mask;
{
     int set;
     int ok;
     MaskType drv_bit;

     set = FALSE;
     if( driver_in_kernel(drv_index) )
     {
	  ok = set_req_drivers(drv_index, drv_mask);
	  if( ok )
	  {
	       convert_index_to_bit_position( &drv_bit, drv_index );
	       SET_BITS( *drv_mask, drv_bit );
	       set = TRUE;

	  }
     }

     return(set);

}  /* set_driver */



/*************************************************************
*
*   NAME	: set_req_drivers
*
*   DESCRIPTION	: This function updates the drv_mask by adding
*		  in any additional drivers required by the driver
*		  specified.  The required driver is added to the
*		  mask if it is included in the kernel and any of
*		  its required drivers are also included.
*		  This function calls itsself recursively
*		  to check if any of the required drivers also
*		  require additional drivers.
*		  NOTE ! it is assumed that there are no circular
*		  driver dependancies.  If there is then this
*		  routine would get caught in an infinite loop.
*
*   PARAMETERS	:
*
*		passed in
*
*			drv_mask - The driver mask with specified
*				   drivers.
*			drv_index - The index of the driver whose
*				    required driver field is checked.
*
*		returned
*
*			drv_mask - The driver mask updated with
*				   any additional drivers.
*			return   - TRUE if any of the required drivers
*				    are set else FALSE.
*
* **************************************************************
*/
int
set_req_drivers( drv_index, drv_mask )
	 int drv_index;
	 MaskType *drv_mask;
{
     MaskType req_drvs, drv_bit;
     int req_index;
     int found, i;
     
     found = FALSE;
     SET_MASK( req_drvs, drv_tbl[drv_index].req_drv );
     if( DIFF_MASKS( req_drvs, NULL_MASK ) )
     {
          SET_RIGHTMOST_BIT( drv_bit );
	  for( i = 0; i < MASK_BIT_SIZE; i++ )
	  {
	       if( TEST_BITS( req_drvs, drv_bit ) )
	       {
	            req_index = convert_bit_position_to_index( drv_bit );
	            if( driver_in_kernel( req_index ) )
	            {
	                  if( set_req_drivers( req_index, drv_mask ) )
	                  {
		               found = TRUE;
		               SET_BITS( *drv_mask, drv_bit );
		          }
	            }
	       }
	            SHIFT_LEFT_MASK( drv_bit, 1 );
	  }

     }
     else
         found = TRUE;

     return( found );

}  /* set_req_drivers */


/*************************************************************
*
*   NAME	: calculate_sizeof_io_map
*
*   DESCRIPTION	:
*		This routine will calculate the maximum number of
*		elements needed for the I/O map tree.  The formula is
*		based on the number of elements in the isc_array 
*		plus the number of SCSI and HPIB interfaces times the
*		the maximum number of devices that could be attached
*		to each one.
*
*   PARAMETERS	:
*
*	passed in
*
*		isc_list - Pointer to the list of isc records.
*		isc_list_size - Number of records in the isc_list.
*
*	returned
*
*		return   - maximum number of records need for the
*                          I/O map tree.
*
**************************************************************
*/
int
calculate_sizeof_io_map ( isc_list, isc_list_size )

	isc_entry_type *isc_list;
	int isc_list_size;

{
   int num_scsi_interfaces;
   int num_hpib_interfaces;
   int isc_cnt;
   int if_id;
   int i;
   int size;
   isc_entry_type *isc_entry;

   num_scsi_interfaces = 0;
   num_hpib_interfaces = 0;

   isc_entry = isc_list;
   for( isc_cnt = 0; isc_cnt < isc_list_size; isc_cnt++, isc_entry++ )
   {
       if_id = isc_entry->if_id;
       for( i = 0; i < SIZE_OF_HDW_TBL; i++ )
       {
	  if( if_id == hdw_tbl[i].hdw_id )
	      break;
       }
       if( i < SIZE_OF_HDW_TBL )
       {
	    if( hdw_tbl[i].hdw_class == SCSI_C_ENT )
	         num_scsi_interfaces++;
	     else
	         if( hdw_tbl[i].hdw_class == HPIB_C_ENT )
	             num_hpib_interfaces++;
       }
   }

   size = isc_list_size +
	  (num_scsi_interfaces * MAX_SCSI_ADDR * MAX_SCSI_LU) +
	  (num_hpib_interfaces * MAX_HPIB_ADDR);

  return(size);
}



/*************************************************************
*
*   NAME	: get_if_cards_to_search
*
*   DESCRIPTION	:      This function selects the isc_table entries that 
*                 are to be included in the search tree.  The selection 
*                 criteria can be based on either a hardware address or
*                 a list of interface drivers.  Any isc_table entries
*                 that meet the criteria are added to the list.
*		  The routine io_search_isc() is called to fetch the list
*                 of isc table entries.  The first call to io_search_isc()
*                 returns the first record and the number of remaining
*                 records.  Subsequent calls to io_search_isc return
*                 the next record and the count of the remaining records.
*		  An array is allocated to hold all of the records and
*		  the specified records are selected and returned in the
*		  array.
*
*   PARAMETERS	:
*
*		passed in:
*
*			isc_list - A pointer to the uninitialized isc list.
*			drv_mask - The driver mask.
*		     	hdw_path - The hardware path record.
*
*		returned:
*
*		        isc_list - allocated list of isc table records.
* 
*                    return      - number of interface cards included
*                                  in list.
*
**************************************************************
*/
get_if_cards_to_search( isc_list, drv_mask, hdw_path )

	isc_entry_type **isc_list;
	MaskType drv_mask;
	hdw_path_type hdw_path;
{
	int count;
	int qualifier;
	int key_type;
	int isc_num;
	int isc_cnt;
	char isc_str[3];
	isc_entry_type isc_rec;
	isc_entry_type *isc_ptr;

if( hdw_path.num_elements > 0 )
{
    key_type = KEY_HDW_PATH;
    isc_rec.hdw_path = hdw_path;

#ifdef __hp9000s700
    /* 
     * Check to see if a function number has been specified.
     */
    if( isc_rec.hdw_path.addr[ FTN ] != 0 )
    {
	 isc_rec.ftn_no = isc_rec.hdw_path.addr[FTN];
	 qualifier = QUAL_FTN_NO;
    }
    else
	 qualifier = QUAL_NONE;
#endif
}
else
{
    key_type = KEY_ALL;
    qualifier = QUAL_NONE;
}

    count = 0;
    io_init( O_RDONLY );
    isc_cnt = io_search_isc( SEARCH_FIRST,
			     key_type,
			     qualifier,
			     &isc_rec );
    if( isc_cnt >= 0 )
    {
         *isc_list = (isc_entry_type *)calloc((isc_cnt + 1),
   		      sizeof(isc_entry_type));

	 if( *isc_list == NULL )
	      ioscan_err("out of memory");
	     
         isc_ptr = *isc_list;

         /*
           Only copy the record into the list if the cards interface
           type and driver are specified in the search criteria.
         */
         if( include_card_in_search( drv_mask, isc_rec.if_id ) )
         {
              *isc_ptr = isc_rec;
              count++;
              isc_ptr++;
         }

         /*
            call libio routine "io_search_isc()" repeatedly to get all of 
            the specified isc records.
         */

         while( isc_cnt > 0 )
         {
	       isc_cnt = io_search_isc( SEARCH_NEXT,
			     		key_type,
			     		qualifier,
			     		&isc_rec );

              if( isc_cnt >= 0 ) 
	      {
                   if( include_card_in_search( drv_mask, isc_rec.if_id ) )
	           {
                        *isc_ptr = isc_rec;
                        count++;
                        isc_ptr++;
	           }
	      }
         }
	 /*
	  * if no records were included in the list then
	  * free the isc_list.
	  */
	 if( count == 0 )
	      free( *isc_list );
    }
    io_end();

return( count );
}


/*************************************************************
*
*   NAME	: add_isc_record_to_io_map
*
*   DESCRIPTION	:
*		  This function copies an isc_entry_type record
*                 into the next available record in the io_map.
*		  Pointers to both the isc_entry_type record and
*		  the io_map are passed in as parameters.
*
*   PARAMETERS	:
*
*		passed in
*
*			rec_ptr    - A pointer to the isc_entry_type
*		                     record.
*                       io_map     - The io map record.
*			class_mask - A copy of the ClassMask. 
*			drv_mask   - A copy of the DrvMask.
*			hdw_path   - A pointer to the hdw_path string.
*
*		returned
*                       io_map   - The io map record undated.
*			return   - A pointer to the new record.
*
**************************************************************
*/
int
add_isc_record_to_io_map( rec_ptr, io_map, class_mask, drv_mask, hdw_path )
	isc_entry_type *rec_ptr;
	IoMapType io_map[];
        MaskType class_mask;
        MaskType drv_mask;
	hdw_path_type *hdw_path;
{
	IoMapType *next_rec;
	int next_id;
	int len;
	int idx;
	int i;
	short hw_status;
	long tmp;
	MaskType c_mask;

        /*
         *  Get the next available record
	 *  in the I/O map
         */
	idx = io_map[0].hdr_rec.nxt_avail++;
	next_rec = &(io_map[ idx ]);
	next_rec->mod_rec.hdw_type = IF_CARD;

        /*
         * Scan the hardware table for the hardware
         * id.
         */
	for( next_id = 0; next_id < SIZE_OF_HDW_TBL; next_id++ )
	{
	     if( rec_ptr->if_id == hdw_tbl[ next_id ].hdw_id )
		  break;
	}

         /*
	  *  Set class field and
	  *  Check if this record should be printed.
	  *  It should be printed if its class is included
	  *  in the class mask and its hardware path is 
	  *  included in any subtree specified by hdw_path.
	  */
	 next_rec->mod_rec.class = hdw_tbl[next_id].hdw_class;

	 convert_index_to_bit_position( &c_mask,
			                next_rec->mod_rec.class
				      );


	/*
	 *  Set driver field
 	 */
	 next_rec->mod_rec.sw_path = hdw_tbl[next_id].hdw_drv;

	/*
	 *  Generate hw_status string  hw_id = bus_id | if_id
         */
	hw_status = ( rec_ptr->bus_type & 0x0ff );
	hw_status = ( (hw_status << 8) | (rec_ptr->if_id & 0xff) ) ;
	sprintf(next_rec->mod_rec.hw_status,"ok(0x%x)", hw_status );

	/*
	 *  Start hardware path string
	 */
	strcat( next_rec->mod_rec.hw_path,
	        (char *)ltostr(rec_ptr->hdw_path.addr[0], 10) );

	for(i = 1; i < rec_ptr->hdw_path.num_elements; i++)
	{
	      strcat(next_rec->mod_rec.hw_path, ".");
	      strcat( next_rec->mod_rec.hw_path,
	 	    (char *)ltostr(rec_ptr->hdw_path.addr[i], 10) );
	}
	/*
	 * Default the remaining fields
	 */
	 next_rec->mod_rec.desc[0] = NULL_STR;

	 strcpy( next_rec->mod_rec.sw_status, SW_STATUS_OK );


	 if( (TEST_BITS(class_mask, c_mask)) &&
#ifdef __hp9000s700
	   (hdw_path->num_elements <= FTN+1) )
#else
	   (hdw_path->num_elements <= SELCODE+1) )
#endif
	   {
	         next_rec->mod_rec.print_node = TRUE;
	         io_map[0].hdr_rec.num_print++;
                 update_fld_len( io_map, next_rec );
	   }

	return( idx );

}  /* add_isc_record_to_io_map */



/*************************************************************
*
*   NAME	: include_card_in_search
*
*   DESCRIPTION	:
*  		 This function determines if the interface card
*   		 whose if_id is passed in should be included in the
*   		 search tree.
*   		      The function searches the hardware table until
*	         it finds the if_id of the card or hits the last
*		 entry in the table (unknown hardware).  It then
*		 tests converts the driver index into a bit mask
*		 and checks it against the driver mask.
*		 If the hardware is unknown and the driver mask
*		 includes all drivers then the hardware will be
*		 included.
*		
*   PARAMETERS	:
*
*		 passed in
*			  drv_mask - The initialized driver mask.
*                         if_id	   - The interface id of the card.
*		 returned
*			  return   - TRUE if the cards driver in set
*				     in the driver mask, else FALSE.
*
**************************************************************
*/
int
include_card_in_search( drv_mask, if_id )
	MaskType drv_mask;
	int if_id;
{
        int next_id;
	MaskType d_mask;

	for( next_id = 0; next_id < SIZE_OF_HDW_TBL; next_id++ )
	{
	     if( if_id == hdw_tbl[ next_id ].hdw_id )
		  break;
	}
	 convert_index_to_bit_position( &d_mask,
			                hdw_tbl[ next_id ].hdw_drv
				      );

	 if( TEST_BITS(drv_mask, d_mask) )
	      return(TRUE);
	 else
	      return(FALSE);

} /* include_card_in_search */


/*************************************************************
*
*   NAME	: init_map
*
*   DESCRIPTION	: This function initializes the io_map.  It first
*		  calls calculate_sizeof_io_map() to calculate
*		  the maximum number of records that io_map could
*		  use and then calls calloc to allocates a block of
*		  memory large enough to accomodate them.  It uses
*		  the first record in the list as the header record
*		  and initializes all of its fields.
*
*   PARAMETERS	:
*
*		passed in
*			io_map   - The address of the unintialized
*				   pointer to the io map.
*			isc_list - The list of isc records retrieved
*				   from kernel isc_table.
*			isc_cnt  - The number of isc records in the
*				   list.
*
**************************************************************
*/
void
init_map( io_map, isc_list, isc_cnt )
	IoMapType   **io_map;
	isc_entry_type *isc_list;
	int isc_cnt;
{
	IoMapType   *hdr_rec;
	int num_records;

       /*
	*  Allocate the records for the io map. Add one
	*  to num_records for the header record of io_map.
	*/
       num_records = calculate_sizeof_io_map( isc_list,
					      isc_cnt );
       num_records++;
       *io_map = (IoMapType *)calloc( num_records,
		                      sizeof(IoMapType) );
       if( *io_map == NULL )
           ioscan_err("out of memory");

       hdr_rec = *io_map;

       hdr_rec->hdr_rec.nxt_avail         = 1;
       hdr_rec->hdr_rec.num_print         = 0;
       hdr_rec->hdr_rec.max_class_len     = strlen(CLASS_HDR);
       hdr_rec->hdr_rec.max_hw_path_len   = strlen(HW_PATH_HDR);
       hdr_rec->hdr_rec.max_sw_path_len   = strlen(SW_PATH_HDR);
       hdr_rec->hdr_rec.max_desc_len      = strlen(DESC_HDR);
       hdr_rec->hdr_rec.max_hw_status_len = strlen(HW_STATUS_HDR);
       hdr_rec->hdr_rec.max_sw_status_len = strlen(SW_STATUS_HDR);

}  /* init_map */


/*************************************************************
*
*   NAME	: build_io_map_tree
*
*   DESCRIPTION	:
*		This routine builds the io_map_table.
*		It calls get_if_cards_to_search() to get the
*               isc_table entries, then it processes each entry
*               adding them to the io_map.  If any cards
*		are HPIB or SCSI it calls scan_for_hpib_devices()
*		or scan_for_scsi_devices() to look for any devices
*		attached to the bus.
*
*   PARAMETERS	:
*
*		passed in
*			io_map     - The address of the uninitialized
*				     pointer to the io map.
*			drv_mask   - The driver mask intialized.
*			class_mask - The class mask intialized.
*			H_option   - A string, with either the hardware
*			H_option   - path, if specified, or NULL. 
*
*		returned
*			io_map     - The initialized pointer to the io_map.
*
**************************************************************
*/
void
build_io_map_tree( io_map, drv_mask, class_mask, H_option )

	IoMapType   **io_map;
	MaskType    drv_mask;
	MaskType    class_mask;
	HwPathType  H_option;
{
	int             isc_cnt;
	int             num_records;
	int		i;
	int		rec_idx;
	IoMapType       *new_rec;
	isc_entry_type  *next_rec;
	isc_entry_type  *isc_list;
	char		err_str[60];
	MaskType	c_mask;
	hdw_path_type   hdw_path;
	int		err;


   if( H_option[0] != NULL_STR )
   {
        err = string_to_hdw_path( &hdw_path, H_option );
        if(err)
        {
             strcpy(err_str, "Invalid hardware path :" );
             strcat(err_str, H_option );
             ioscan_err(err_str);
        }
   }
   else
	hdw_path.num_elements = 0;

   isc_cnt = get_if_cards_to_search( &isc_list,
				     drv_mask,
				     hdw_path
				   );

  if( isc_cnt > 0 )
  {

       init_map( io_map,
		 isc_list,
		 isc_cnt
	       );

       next_rec = isc_list;

       for( i = 0; i < isc_cnt; i++, next_rec++ )
       {
           rec_idx = add_isc_record_to_io_map( next_rec,
					       *io_map,
					       class_mask,
					       drv_mask,
					       &hdw_path
					     );

	   new_rec = ( (IoMapType *)(*io_map + rec_idx) );
           switch( new_rec->mod_rec.class )
           {
            case SCSI_C_ENT:
		            scan_for_scsi_devices( *io_map, 
						    rec_idx,
					     	    hdw_path,
						    drv_mask,
						    class_mask
					   	  );
		      break;
            case HPIB_C_ENT:
		            scan_for_hpib_devices( *io_map,
					     	    rec_idx,
					            hdw_path,
						    drv_mask,
						    class_mask
					          );
		      break;
            default:
		      break;
           }

       }
       free(isc_list);

     } /* if isc_cnt */
     else
     {
	 strcpy(err_str, "No hardware at :" );
	 strcat(err_str, H_option );
         ioscan_err(err_str);
     }

}   /* build_io_map_tree */


/*************************************************************
*
*   NAME	: get_cmd_options
*
*   DESCRIPTION	: This routine processes the ioscan command line
*                 options and stores them in the record opt_rec.
*                 The command line options it looks for are:
*
*                                 -d "driver"
*				  -C "class"
*				  -H "hardware path"
*				  -f 
*				  -b 
*
*		  ioscan [-d driver | -C class] [-H hw_path] [-f] [-b]
*                 
*		  It assumes that every option string is followed by
*                 a single qualifier string, except for the -f and -b
*		  options which has no qualifiers.  For example if the
*		  specifier string "-C" is an argument, then the next
*                 string in the array should be the name of the class
*                 The only error checking done is to see if both a 
*		  -d and -C option have been specified.  According
*		  to the command syntax, the user can specify either
*		  a -d or a -C option, but not both.  Checking for
*		  valid qualifier names will be done later in the routine
*		  build_search_criteria.
*		  The -b option is an undocumented option used by SAM.
*		  When specified, the product id and vendor id are printed
*		  for all SCSI devices.
*
*   PARAMETERS	:
*
*		passed in
*                    arg_cnt - arg count of parameters on command line.
*		     args    - vector of strings containing parmeters.
*		     opt_rec - pointer to an uninitialized option record.
*
*		returned
*		     opt_rec - initialized with search criteria
*                              ( -d -C -H and -f -b options).
*
**************************************************************
*/
void
get_cmd_options( arg_cnt, args, opt_rec )
     int arg_cnt;
     char *args[];
     OptRecType *opt_rec;

{
     int i;
     int c;
     extern char *optarg;
     extern int optind, opterr, optopt;
     char err_str[30];

     opt_rec->d_option[0] = NULL_STR;
     opt_rec->C_option[0] = NULL_STR;
     opt_rec->H_option[0] = NULL_STR;
     opt_rec->f_list      = FALSE;
     opt_rec->b_opt       = FALSE;

     while( ( c = getopt(arg_cnt, args, "C:d:H:fb") ) != EOF)
     {
          switch( c )
	  {
	      case 'C':
	               strcpy( opt_rec->C_option, optarg );
		       break;
	      case 'd':
		       strcpy( opt_rec->d_option, optarg );
		       break;
	      case 'H':
		       strcpy( opt_rec->H_option, optarg );
		       break;
	      case 'f':
		       opt_rec->f_list = TRUE;
		       break;
	      case 'b':
	               opt_rec->b_opt = TRUE;
		       break;
	      case '?':
	               usage(NULL_STR);
		       break;
	      default:
		       sprintf(err_str,"illegal option -- %c",c );
		       usage("err_str");
		       break;

	 } /* c */

     }    /* while */

     /*
	Cannot have both a -C option and a -d option
     */
     if( ( opt_rec->C_option[0] != NULL_STR ) && 
         ( opt_rec->d_option[0] != NULL_STR ) )
     {
	usage("Invalid combination of options");
     }

}   /* get_cmd_options */



/*************************************************************
*
*   NAME	: build_search_criteria
*
*   DESCRIPTION	: This routine sets the search criteria masks.  First
*		  it defaults the masks to all bits set.  Next
*		  it checks the -C option.  If a -C option has been
*                 specified, ( C_option is not the NULL_STRING ),
*                 the class_mask is set and the drv_mask is defaulted
*                 to all those drivers that control devices of the
*                 specified class.
*                 If no -C option has been specied then the -d option
*		  is checked.  If a -d option has been specified then
*                 the driver mask is set accordingly.
*
*   PARAMETERS	:  
*
*	passed in
*		 drv_mask   - Pointer to uninitialize driver mask.
*		 class_mask - Pointer to uninitialize class mask.
*		 opt_rec    - Record with the command line options. 
*
*	returned
*		 drv_mask   - bits set specifying which drivers to 
*			      include in search.
*		 class_mask - bits set specifing what types of devices
*			      to include in search.
*
**************************************************************
*/
void
build_search_criteria( drv_mask, class_mask, opt_rec )
	MaskType     *drv_mask;
	MaskType     *class_mask;
	OptRecType   *opt_rec;
{
	int nxt_class;
	int nxt_drv;
	int found;
	int ok;
	char err_str[60];
	MaskType bit_mask;

	/*
	   initialize drv_mask and class_mask to all bits set;
	*/
        init_masks( drv_mask, class_mask );
	found = TRUE;

	/*
	  Process any -C options.
	*/
	if( opt_rec->C_option[0] != NULL_STR )
	{
	     found = FALSE;
	     for( nxt_class = 0; nxt_class < NUM_CLASS; nxt_class++)
	     {
	       if((strcasecmp(opt_rec->C_option, ClassList[ nxt_class ])) == 0)
	       {
		    found = TRUE;
		    convert_index_to_bit_position( &bit_mask, nxt_class );
		    SET_MASK( *class_mask, bit_mask );
		    break;
	       }
	     }
	     if( found )
	     {
	           ok = set_drv_mask_to_class_defaults( drv_mask, class_mask );
	     }
	     else
		  sprintf(err_str,"ioscan: Device class %s is not in the kernel"
			  ,opt_rec->C_option);
	 }
	 else
	 {
	   /*
	     Process any -d options.
	   */
	   if( opt_rec->d_option[0] != NULL_STR  )
	   {
	        SET_MASK(*drv_mask, NULL_MASK);
		found = FALSE;
	        for( nxt_drv = 0; nxt_drv < DRV_TBL_SIZE; nxt_drv++ )
	        {
	             if((strcasecmp(opt_rec->d_option,
				    drv_tbl[nxt_drv].drv_name) == 0))
		     {
			  found = set_driver( nxt_drv, drv_mask );
		          break;
		     }
	        }
	        if( found )
		{
                     SET_MASK( *class_mask, drv_tbl[nxt_drv].drv_class );
		}
		else
	   	     sprintf(err_str,"ioscan: Device driver %s is not in the kernel"
			  ,opt_rec->d_option);
	   }
	    /*
	     * Since neither a -d or -C option was specified
	     * the defaults is that all drivers are valid, 
	     * therefore check each driver to make sure that it 
	     * is in the kernel.
	     */
	    else
	    {
	        SET_MASK(*drv_mask, NULL_MASK);

	        for( nxt_drv = 0; nxt_drv <= DRV_TBL_SIZE; nxt_drv++ )
		     set_driver( nxt_drv, drv_mask);

                /*
		 * always default the unknown driver in
		 */
	        SET_BITS( *drv_mask, UNKNW_DRV );
	    }
	}
	if( found == FALSE )
	     usage( err_str );

}  /* build_search_criteria */


/*************************************************************
*
*   NAME	:  print_io_map
*
*   DESCRIPTION :  This function prints out the I/O map.
*		   The format of the list printed is dependent
*		   upon the two flags, f_list and b_opt which
*		   are passed in as a parameter.
*
*		passed in
*			io_map - A pointer to the head record of
*				 the I/O map.
*			f_list - a flag : 1 == print long form.
*					  0 == print the short form.
*			b_opt  - A flag : 1 == print additional
*					  info on SCSI devices.
*
*		returned
*
**************************************************************
*/
print_io_map( io_map, f_list, b_opt )

	IoMapType io_map[];
	int f_list;
	int b_opt;
{
	IoMapType *next_rec;
	int total_len;
	int rec_cnt;
	int num_recs;
	int len;

	int class_len;
	int sw_path_len;
	int hw_path_len;
 	int hw_status_len;
 	int sw_status_len;
 	int desc_len;
	char err_str[60];

	if( io_map[0].hdr_rec.num_print == 0 )
	{
	     strcpy(err_str, "No hardware found" );
             ioscan_err(err_str);
	}

	next_rec = &(io_map[1]);
	num_recs = io_map[0].hdr_rec.nxt_avail - 1;

	class_len	= io_map[0].hdr_rec.max_class_len;
	sw_path_len	= io_map[0].hdr_rec.max_sw_path_len;
	hw_path_len	= io_map[0].hdr_rec.max_hw_path_len;
	hw_status_len	= io_map[0].hdr_rec.max_hw_status_len;
	sw_status_len	= io_map[0].hdr_rec.max_sw_status_len;
	desc_len	= io_map[0].hdr_rec.max_desc_len;


        if( f_list )
	{
	     total_len = class_len + sw_path_len + hw_path_len +
	                 hw_status_len + sw_status_len + NUM_COLS;
	     if( b_opt )
	     {
		  total_len += desc_len;

	     printf("%-*s %-*s %-*s %-*s %-*s %-*s\n",
						class_len,     CLASS_HDR,
						hw_path_len,   HW_PATH_HDR,
						sw_path_len,   SW_PATH_HDR,
						hw_status_len, HW_STATUS_HDR,
						sw_status_len, SW_STATUS_HDR,
						desc_len,      DESC_HDR
						);
	     while( total_len-- > 0 )
		  putchar('=');
	     putchar('\n');

	     for( rec_cnt = 0; rec_cnt < num_recs; rec_cnt++, next_rec++)
	     {
		  if( next_rec->mod_rec.print_node)
		  {
		        printf("%-*s %-*s %-*s %-*s %-*s %-*s\n",
	          		class_len,     ClassList[next_rec->mod_rec.class],
		  		hw_path_len,   next_rec->mod_rec.hw_path,
		  		sw_path_len,   drv_tbl[next_rec->mod_rec.sw_path].drv_name,
		  		hw_status_len, next_rec->mod_rec.hw_status,
		  		sw_status_len, next_rec->mod_rec.sw_status,
		  		desc_len,      next_rec->mod_rec.desc
				);
		  }
	     }
	     }  /* if b_opt */
	     else{
	     printf("%-*s %-*s %-*s %-*s %-*s \n",
						class_len,     CLASS_HDR,
						hw_path_len,   HW_PATH_HDR,
						sw_path_len,   SW_PATH_HDR,
						hw_status_len, HW_STATUS_HDR,
						sw_status_len, SW_STATUS_HDR
						);
	     while( total_len-- > 0 )
		  putchar('=');
	     putchar('\n');

	     for( rec_cnt = 0; rec_cnt < num_recs; rec_cnt++, next_rec++)
	     {
		  if( next_rec->mod_rec.print_node)
		  {
		        printf("%-*s %-*s %-*s %-*s %-*s \n",
	          		class_len,     ClassList[next_rec->mod_rec.class],
		  		hw_path_len,   next_rec->mod_rec.hw_path,
		  		sw_path_len,   drv_tbl[next_rec->mod_rec.sw_path].drv_name,
		  		hw_status_len, next_rec->mod_rec.hw_status,
		  		sw_status_len, next_rec->mod_rec.sw_status
				);
		  }
	     }

	     } /* else b_opt */
	}
	else
	{
	     len = (int)strlen(DESC_HDR);
	     if( len > class_len )
		  class_len = len;
		  
	     total_len = class_len + hw_path_len + hw_status_len ;

	     printf("%-*s %-*s %-*s\n",
					 hw_path_len,      HW_PATH_HDR,
					 class_len,        DESC_HDR,
					 hw_status_len,    STATUS_HDR
			       		);

	     while( total_len-- > 0 )
		  putchar('=');
	     putchar('\n');

	     for( rec_cnt = 0; rec_cnt < num_recs; rec_cnt++, next_rec++)
	     {
		  if( next_rec->mod_rec.print_node)
		  {
		       printf("%-*s %-*s %-*s\n",
		  		hw_path_len,	next_rec->mod_rec.hw_path,
	          		class_len,	ClassList[next_rec->mod_rec.class],
		  		hw_status_len,	next_rec->mod_rec.hw_status
				);
		  }
	     }

	}
	printf("\n");

}  /* print_io_map */




/*************************************************************
*
*   NAME	: init_ioscan  
*
*   DESCRIPTION	:  This file tests for the existance of the 
*		   device file for the deconfig device driver.
*		   if the file does not exist it attempts to
*		   create it.
*
*   PARAMETERS	:
*
*		passed in
*
*		returned
*		   
*		   0 - if the command succeeded else the error
*		       code returned by the system call.
*
**************************************************************
*/
int
init_ioscan()
{
     int fd;
     int err;
     mode_t mode;
     dev_t dev;

     fd = open( DCONFIG_DRV_PATH, O_RDONLY);

     if( fd == -1 )
     {
          mode = ( S_IRWXU | _S_IFCHR );
          dev = makedev(DCONFIG_DRV_MAJOR, DCONFIG_DRV_MINOR);
          err = mknod(DCONFIG_DRV_PATH, mode, dev, 0);
     }
     else
     {
	  err = 0;
	  close(fd);
     }

     return( err );


}   /* init_ioscan */

                   

/*************************************************************
*
*   NAME	:  main
*
*   DESCRIPTION	:  This is the main routine of the ioscan command.
*                  It calls the appropriate routines to process the
*		   command line options, build the search criteria,
*                  build the I/O map tree and printing the results.
*
*   PARAMETERS	:
*
*		passed in
*
*                  argc   -  total number of strings on command line.
*                            Argc is always >= 1 since the name
*                            of the command is counted as one of the
*                            strings.
*
*		   argv   - pointer array to command line strings.
*
*		returned
*
**************************************************************
*/
main(argc, argv )
int argc;
char *argv[];
{
     OptRecType opt_rec;
     MaskType drv_mask, class_mask;
     HwPathType hw_path;

     IoMapType *io_map;
     int sem_id;
     int err;
     
     if( (init_ioscan()) != 0 )
	  ioscan_err(" bad device file : DCONFIG_DRV_PATH file ");

     get_cmd_options( argc, 
               	      argv,
              	      &opt_rec );

     build_search_criteria( &drv_mask, 
                            &class_mask,
                            &opt_rec );

     sem_id = init_sem();
     get_sem(sem_id);

     build_io_map_tree( &io_map,
			drv_mask,
			class_mask,
                        opt_rec.H_option
		      );
     rel_sem(sem_id);

     print_io_map( io_map,
		   opt_rec.f_list,
		   opt_rec.b_opt
		 );

     free( io_map );


     exit(0);
}
