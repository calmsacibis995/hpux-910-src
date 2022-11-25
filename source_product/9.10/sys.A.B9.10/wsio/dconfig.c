/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/dconfig.c,v $
 * $Revision: 1.2.83.6 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/04/21 14:21:04 $
 */

#ifndef lint
static char *revision = "$Header: dconfig.c,v 1.2.83.6 94/04/21 14:21:04 kcs Exp $";
#endif /* lint */

#include "../h/param.h"
#include "../h/io.h"
#include "../wsio/eisa.h"
#include "../machine/cpu.h"

#include "../h/dir.h"
#include "../h/file.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/malloc.h"
#include "../h/libio.h"
#include "../wsio/dconfig.h"
#include "../h/conf.h"


#ifdef __hp9000s300
#define ISC_TBL_SIZE ISC_TABLE_SIZE
#else
#define ISC_TBL_SIZE PA_ISC_TABLE_SIZE
#endif

#ifdef IOSCAN
#define TP_MAJOR    5
#define STP_MAJOR   9
#define AMIGO_MAJOR 11
#define CS80_MAJOR  4
#define SCSI_MAJOR  47
#endif /* IOSCAN */

/* A handy macro */

#define	IS_NULL_NAME(name)  (name[0] == '\0')

#ifdef __hp9000s700
#define MEM_CONFIG_ADDR 0x54
#endif /* hp9000s700 */


/* AUTOBAHN - most (all?) of these should be static */
/* Global variables */

/* Pseudo-driver state variables */
int	Config_Open = 0;	/* already open? - we only allow one */
int	Config_Last_Cmd = 0;	/* "cmd" value of last successful ioctl */


#ifdef __hp9000s700

/* ERROR - This is here for Snakes as I/O still uses it in error */
#include "../sio/llio.h"
io_tree_entry   *find_child();

#endif /* hp9000s700 */

int     Config_Changed = FALSE;


#ifdef __hp9000s700

typedef struct {
	io_name_type    name[MAX_IO_PATH_ELEMENTS];
	int             num_elements;
} module_path_type;


/* Variables for io_get_table() */
config_get_table_t  Cs_Get_Table;   /* saved args from ioctl */
return_vsc_mod_entry	    *User_Mod; /* pointer to buffer we build user's mod
			   	       table in - it's global so we don't have
				       to pass it as a parameter to the
				       recursive function */
int		    User_Mod_Index; /* current index in user's mod table -
			   	       it's global so we don't have to pass it
			   	       as a parameter to the recursive
				       function */
#endif /* hp9000s700 */

/* Variables for io_search_isc() */
int		    Search_Count;   /* number of matching entry */
				    /* pointer to entry found by a
				       SEARCH_SINGLE operation */
struct isc_table_type	*Search_Single_Node;
				    /* list of entries found by SEARCH_FIRST */
config_search_list_t *Search_List     = NULL_SEARCH_LIST_PTR; /* head of list */
config_search_list_t *Search_List_End = NULL_SEARCH_LIST_PTR; /* tail of list */
int	    Search_First = FALSE;    /* have we done a SEARCH_FIRST? */
int	    Search_Key;	     /* saved search_key */
int	    Search_Type;     /* saved search_type */
int	    Search_Qual;     /* saved search_qual */
int	    Search_Init;     /* search qualifier init set */
int	    Search_Ftn_No;   /* saved function number */

/* Function types */

struct isc_table_type	*get_search_list_entry();
void		copy_entry_data(), free_search_list(), copy_hdw_path();

#ifdef __hp9000s700
void		copy_mod_table(), fix_mod_table(),
		copy_mod_entry(); 
#endif /* hp9000s700 */

extern int nchrdev;
extern struct cdevsw cdevsw[];
extern nodev();

/* HP-UX entry points */

/**************************************************************************
 * dconfig_open(dev, flag)
 **************************************************************************
 *
 * Description:
 *	Open the dconfig pseudo-driver.
 *
 * Input Parameters:
 *	dev	Device number of the file.  Ignored.
 *
 *	flag    Value corresponding to "oflag" in open(2).  Ignored.
 *
 * Output Parameters: None
 *
 * Returns:
 *	SUCCESS (0)	Success.
 *	EBUSY		Already open.
 *
 * Globals Referenced:
 *	Config_Open	Already open?.
 *
 * External Calls: None
 *
 * Algorithm:
 *	Check "Config_Open" and return EBUSY if TRUE.
 *	Clear "Config_Changed".
 *	Set "Config_Open" and return SUCCESS.
 *
 **************************************************************************/

/*ARGSUSED*/
dconfig_open(dev, flag)
  dev_t dev;
  int	flag;
{
    int s;

    s = spl6();
    if (Config_Open)
    {
        if( (flag & FNDELAY) == 0x0000 )
        {
             while( Config_Open )
                   sleep( (caddr_t)&Config_Open, PSLEP);

             Config_Open = u.u_procp->p_pid;
             splx(s);
        }
        else
        {
             splx(s);
	     return(EBUSY);
        }
    }
    else
    {
        Config_Open = u.u_procp->p_pid;
        splx(s);
    }

    return(SUCCESS);
}

/************************************************************************
 *  dconfig_close(dev, flag)
 ************************************************************************
 *
 * Description:
 *	Close the dconfig pseudo-driver and release any dynamic memory
 *	allocated by config_search_isc().
 *
 * Input Parameters:
 *	dev	Device number of the file.  Ignored.
 *
 *	flag    Value corresponding to oflag in open(2).  Ignored.
 *
 * Output Parameters: None
 *
 * Returns:
 *	SUCCESS (0)	Success.
 *
 * Globals Referenced:
 *	Config_Open
 *
 * External Calls: None
 *
 * Algorithm:
 *	Call free_search_list() to release any dynamic
 *	memory allocated to it's list.
 *	Set "config_open" to FALSE and return SUCCESS.
 *
 **************************************************************************/

/*ARGSUSED*/
dconfig_close(dev, flag)	
  dev_t dev;
  int	flag;
{
    int s;

    if( Config_Open == u.u_procp->p_pid )
    {
         free_search_list();

         s = spl6();
         Config_Open = 0;
         wakeup( (caddr_t)&Config_Open);
         splx(s);

         return(SUCCESS);
    }
    else
         return( EBUSY );

}

/**************************************************************************
 * dconfig_read(dev, uio)
 **************************************************************************
 *
 * Description:
 *	Read from the dconfig pseudo-driver.
 *
 * Input Parameters:
 *	dev	Device number of the file.  Ignored.
 *
 *	uio	Pointer to a uio structure.
 *
 * Output Parameters: None
 *
 * Returns:
 *	SUCCESS (0)	Success.
 *	EINVAL		Read size does not match size of requested object.
 *	ENODEV		Last ioctl executed does not require a read(2).
 *	other		Return value from sub-function.
 *
 * Globals Referenced:
 *	Config_Last_Cmd		Last ioctl executed.
 *
 * External Calls: None
 *
 * Algorithm:
 *	If "Config_Last_Cmd" does not match an ioctl cmd which requires a
 *	corresponding read(), return ENODEV.
 *	Switch on "Config_Last_Cmd" and call the appropriate sub-function.
 *	Return the value we got back from the sub-function.
 *
 **************************************************************************/

/*ARGSUSED*/
dconfig_read(dev, uio)
  dev_t		dev;
  struct uio	*uio;
{
    int status = SUCCESS;

    switch (Config_Last_Cmd) {
#ifdef __hp9000s700
    case CONFIG_GET_TABLE:
	status = config_read_table(uio);
	break;
#endif /* hp9000s700 */
    default:
	status = ENODEV;
	break;
    }

    return(status);
}

#ifdef __hp9000s700
/**************************************************************************
 * config_read_table(uio)
 **************************************************************************
 *
 * Description:
 *	Read an I/O system table and return it to the user.
 *
 *	NOTE: This routine may sleep in MALLOC().
 *
 * Input Parameters:
 *	uio	Pointer to a uio structure.
 *
 * Output Parameters: None
 *
 * Returns:
 *	SUCCESS (0)	Success.
 *	EINVAL		Which_table is invalid.
 *	EINVAL		Read request size doesn't match table size.
 *	ENOMEM		Unable to MALLOC() memory.
 *	other		Value returned by uiomove().
 *
 * Globals Referenced:
 *	Cs_Get_Table  User_Mod  User_Mod_Index
 *
 * External Calls:
 *	MALLOC()  FREE()  bcopy()  uiomove()
 *
 * Algorithm:
 *	If the user asked for an invalid table in the previous ioctl
 *	(Cs_Get_Table.status set to INVALID_TYPE by config_get_table),
 *	return EINVAL.
 *	Compare the read request size against the size of the previously
 *	requested table and return EINVAL if they do not match.
 *	Allocate a kernel buffer to hold the read data while we assemble it.
 *	Use Cs_Get_Table.which_table to determine which table the user
 *	asked for in the ioctl (we know by now that it's valid).
 *	Copy the appropriate portions of the kernel data structures to the
 *	kernel buffer.  For T_IO_MOD_TABLE we use two recursive functions
 *	to do the copy - copy_mod_table() does the copy, and
 *	fix_mod_table() fixes up the array indices. 
 *	Call uiomove() to copy the kernel buffer to the user's buffer.
 *	Free the kernel buffer.
 *	Return the value we got back from uiomove().
 *
 **************************************************************************/

config_read_table(uio)
  struct uio	*uio;
{
    int status,valid_size;
    caddr_t buf;

    if (Cs_Get_Table.status == INVALID_TYPE)
	return(EINVAL);

    if (uio->uio_resid != Cs_Get_Table.size)
	return(EINVAL);

    /* might sleep here */
    MALLOC(buf, caddr_t, Cs_Get_Table.size, M_TEMP, M_WAITOK);
    if (buf == (caddr_t)0)
	return(ENOMEM);

    switch(Cs_Get_Table.which_table) 
    {
    case T_IO_MOD_TABLE:
	User_Mod = (return_vsc_mod_entry *)buf;
	User_Mod_Index = 0;
	copy_mod_table();
	fix_mod_table();
	break;
    case T_MEM_DATA_TABLE:
	/******************************************************************
	 * Call load_real to read page zero at specified location for
	 * memory configuration data.  Read 32 words of information.
	 * Since page zero is never mapped, don't need to flush the
	 * data.  MEM_CONFIG_ADDR must be word aligned address.
	 ******************************************************************/
	load_real(buf,MEM_CONFIG_ADDR,32);
	break;
    }

    status = uiomove(buf, Cs_Get_Table.size, UIO_READ, uio);
    FREE(buf, M_TEMP);
    switch(Cs_Get_Table.which_table)
    {
    case T_IO_MOD_TABLE:
        /* 
	** User_Mod_Index contains number of valid entries copied
	** Set uio_resid in uio to number of bytes not used 
	** so that read will return size of valid portion 
	** of table, not full size 
	*/
	valid_size = User_Mod_Index * (sizeof (return_vsc_mod_entry));
        uio->uio_resid = Cs_Get_Table.size - valid_size;
	break;
    case T_MEM_DATA_TABLE:
	/* 
	** number of valid bytes in table, always full size
	** of table ** so don't need to set uio_resid.
	*/
	break;
    }
    return(status);
}

/**************************************************************************
 * copy_mod_table()
 **************************************************************************
 *
 * Description:
 *	Copy the kernel's native module table to the user_mod buffer (in
 *	preparation for then copying it to user space).  This
 *	function will do a "preorder" (depth first) recursive descent of
 *	the vsc module table beginning at the first entry.
 *	We also save the user_mod array index (User_Mod_Index) for each
 *	user_mod entry in the corresponding kern_mod structure so that
 *	fix_mod_table() can fix up the array indices in User_Mod in a
 *	single additional pass through the vsc module table.
 *
 * Input Parameters: None
 *
 * Output Parameters: None
 *
 * Returns: None
 *
 * Globals Referenced:
 *	User_Mod	The kernel buffer to copy the data into before
 *			returning it to the user.
 *
 *	User_Mod_Index	The array index of the user_mod entry to copy
 *			into.  This is incremented here after each copy.
 *
 *	NOTE: Both User_Mod and User_Mod_Index are globals so they do not
 *	      need to be passed.
 *
 * External Calls: None
 *
 * Algorithm:
 *	For each vsc_mod_entry beginning with the first entry and
 *	continuing with the other modules on this bus, call
 *	copy_mod_entry() to copy the data into the User_Mod kernel
 *	buffer (which was previously allocated in config_read_table()),
 *	save the User_Mod_Index in the (so fix_mod_table() can
 *	fix up the array indices), and increment User_Mod_Index.
 *
 **************************************************************************/

void
copy_mod_table()
{
    int			mod_index, ba_index;
    vsc_mod_entry	*vsc_entry_ptr;
    vsc_mod_entry	*ftn_ptr;

    for (mod_index = 0; mod_index < NUM_SBUS_MODS; mod_index++ ) 
    {
	vsc_entry_ptr = vsc_mod_table[mod_index];
	if (vsc_entry_ptr != NULL)
	{
	  copy_mod_entry(&User_Mod[User_Mod_Index], vsc_entry_ptr);
	  vsc_entry_ptr->user_mod_index = User_Mod_Index++;
          switch (vsc_entry_ptr->iodc_type.type) {
	  case MOD_TYPE_BUS_ADAPTER:
            if ((vsc_entry_ptr->iodc_sversion.model == PA_CORE) ||
                (vsc_entry_ptr->iodc_sversion.model == EISA_BUS))
	    {
		ba_index = 0;
		/*************************************************************
		 * This section of code assumes that all the entries in the
		 * ba_mod_table will either be NULL or point to a vsc entry
		 * for an fio type card.  There are currently no ba type
		 * cards that have ba type cards attached.
  		 *************************************************************/
		while ((ba_index < NUM_SBUS_MODS) &&
		       (vsc_entry_ptr->type.ba.ba_mod_table[ba_index] != NULL))
                {
		    /*********************************************************
		     * Now that we know we have a valid vsc_entry_ptr in the
		     * ba_mod_table index we are interested in and that
		     * it is of type fio, we need to search all the possible
		     * next_ftn pointers for valid entries.  There will
		     * always be at least one copy done here for the first
		     * function in the ba_mod_table entry.  All fio entries
		     * have at least one function.
  		     *********************************************************/
		    for (ftn_ptr=vsc_entry_ptr->type.ba.ba_mod_table[ba_index];
		         ftn_ptr != NULL;ftn_ptr = ftn_ptr->type.fio.next_ftn)
		    {
	                copy_mod_entry(&User_Mod[User_Mod_Index], 
			               ftn_ptr);
	                ftn_ptr->user_mod_index = User_Mod_Index++;
		    }
		    ba_index++;
		}
	    }
	    break;
	  case MOD_TYPE_FOREIGN_IO:
	    switch (vsc_entry_ptr->iodc_sversion.model) {
	    case SCSI_SV_ID:
	    case LAN_SV_ID:
	    case HIL_SV_ID:
	    case CENT_SV_ID:
	    case SERIAL_SV_ID:
	    case SGC_SV_ID:
		for (ftn_ptr = vsc_entry_ptr->type.fio.next_ftn;
		     ftn_ptr != NULL; ftn_ptr = ftn_ptr->type.fio.next_ftn)
		{
	            copy_mod_entry(&User_Mod[User_Mod_Index], ftn_ptr);
	            ftn_ptr->user_mod_index = User_Mod_Index++;
		}
		break;
	    default:
		break;
	    }
	  default:
	    break;
	  }
	}
    }
}

/**************************************************************************
 * copy_mod_entry(user_mod, kern_mod)
 **************************************************************************
 *
 * Description:
 *	Copy the kernel's vsc_mod_entry (kern_mod) to the kernel
 *	user_mod buffer (pointed to by user_mod).  This function fills
 *	in all fields EXCEPT the next_ftn, ba_mod_table, isc_table.b_major
 *      and isc_table.c_major fields (which are all set to NONE).
 *
 * Input Parameters:
 *	user_mod	A pointer to the return_vsc_mod_entry to copy into.
 *
 *	kern_mod	A pointer to the vsc_mod_entry to copy from.
 *
 * Output Parameters: None
 *
 * Returns: None
 *
 * Globals Referenced:
 *	User_Mod_Index	The array index of the user_mod entry we are copying
 *			into.  This is saved here in the kern_mod.
 *
 * External Calls: None
 *
 * Algorithm:
 *	Copy the generic data from the kern_mod to the user_mod.
 *	Based on the iodc_sversion.model field, copy any additional fields 
 *      for that module type.
 *
 **************************************************************************/

void
copy_mod_entry(user_mod, kern_mod)
  return_vsc_mod_entry	*user_mod;
  vsc_mod_entry  	*kern_mod;
{
    int isc, i;
    struct isc_table_type *iscp;

    user_mod->iodc_hversion = kern_mod->iodc_hversion;
    user_mod->iodc_spa = kern_mod->iodc_spa;
    user_mod->iodc_type = kern_mod->iodc_type;
    user_mod->iodc_sversion = kern_mod->iodc_sversion;
    user_mod->iodc_reserved = kern_mod->iodc_reserved;
    user_mod->iodc_rev = kern_mod->iodc_rev;
    user_mod->iodc_dep = kern_mod->iodc_dep;
    user_mod->iodc_check = kern_mod->iodc_check;
    user_mod->iodc_length = kern_mod->iodc_length;
    user_mod->hpa = kern_mod->hpa;
    user_mod->more_pgs = kern_mod->more_pgs;

    switch (kern_mod->iodc_type.type) {
    case MOD_TYPE_BUS_ADAPTER:
      if ((kern_mod->iodc_sversion.model == PA_CORE) ||
          (kern_mod->iodc_sversion.model == EISA_BUS))
      {
	for (i = 0; i < NUM_SBUS_MODS; i++)
	{
	    user_mod->type.ba.ba_mod_table[i] = NONE;
	}
      }
      break;
    case MOD_TYPE_FOREIGN_IO:
      switch(kern_mod->iodc_sversion.model) {
      case SCSI_SV_ID:
      case LAN_SV_ID:
      case HIL_SV_ID:
      case CENT_SV_ID:
      case SERIAL_SV_ID:
      case SGC_SV_ID:
        user_mod->type.fio.next_ftn = NONE;
        user_mod->type.fio.ftn_no = kern_mod->type.fio.ftn_no;
	user_mod->type.fio.isc_table.b_major = NONE;
	user_mod->type.fio.isc_table.c_major = NONE;
        iscp = kern_mod->type.fio.isc_table;
        if (iscp != NULL)
        {

        	user_mod->type.fio.isc_table.my_isc = iscp->my_isc;
		user_mod->type.fio.isc_table.bus_type = iscp->bus_type;
		user_mod->type.fio.isc_table.if_id = iscp->if_id;
		user_mod->type.fio.isc_table.ftn_no = iscp->ftn_no;
		copy_hdw_path(user_mod->type.fio.isc_table.my_isc,
		      user_mod->type.fio.isc_table.ftn_no,
		      &user_mod->type.fio.isc_table.hdw_path);
	
        } else
        {
                /* This means that the driver is not configured into */
                /* the kernel, so hardcode information to indicate */
                /* no isc entry */
                user_mod->type.fio.isc_table.my_isc = NONE;
                user_mod->type.fio.isc_table.bus_type = NONE;
                user_mod->type.fio.isc_table.if_id = NONE;
                user_mod->type.fio.isc_table.ftn_no = NONE;
                user_mod->type.fio.isc_table.hdw_path.num_elements = 0;
                for (i = 0; i < MAX_IO_PATH_ELEMENTS; i++)
                {
                        user_mod->type.fio.isc_table.hdw_path.addr[i] = 0;
                }
        }
	break;
      default:
	break;
      }
      break;
    default:
      break;
    }
}

/**************************************************************************
* fix_mod_table()
 **************************************************************************
 *
 * Description:
 *	Fix up the next_ftn, ba_mod_table indices in user_mod so
 *	they contain the actual array indices of the appropriate user_mod
 *	entries. The function will do a "preorder" (depth first) recursive
 *	descent of the vsc module table beginning at the first entry.
 *
 * Input Parameters: None
 *
 * Output Parameters: None
 *
 * Returns: None
 *
 * Globals Referenced:
 *	User_Mod	The kernel buffer to do the fixing in.
 *
 *	NOTE: User_Mod is a global so it does not need to be passed to
 *	      each recursive invocation of the function.
 *
 * External Calls: None
 *
 * Algorithm:
 *	Traverse the vsc module table and use the
 *	user_mod_index values saved there by copy_mod_entry() to fix
 *	the User_Mod array indices, as follows:
 *	For each vsc_entry_ptr beginning with the first entry, and
 *	continuing  with the other modules on this bus, set the User_Mod
 *	"next_ftn" value to either NONE (if vsc_entry_ptr->fio.next_ftn is NULL)
 *      or to the user_mod_index from vsc_entry_ptr->fio.next_ftn.
 *	Set each entry in the User_Mod "ba_mod_table" array value to either 
 *      NONE (if the element of vsc_entry_ptr->ba.ba_mod_table is NULL)
 *      or to the user_mod_index from the element of 
 *      vsc_entry_ptr->ba.ba_mod_table.
 *
 **************************************************************************/

void
fix_mod_table()
{
    int			kern_index, ba_index;
    int			user_kern_index, user_ba_index, user_ftn_index;
    int			temp;
    vsc_mod_entry	*vsc_entry_ptr, *ftn_ptr, *ba_vsc_entry_ptr;

    for ( kern_index = 0; kern_index < NUM_SBUS_MODS; kern_index++)
    {
	vsc_entry_ptr = vsc_mod_table[kern_index];
	if (vsc_entry_ptr != NULL)
	{
	  user_kern_index = vsc_entry_ptr->user_mod_index;
          switch (vsc_entry_ptr->iodc_type.type) 
	  {
	  case MOD_TYPE_BUS_ADAPTER:
            if ((vsc_entry_ptr->iodc_sversion.model == PA_CORE) ||
                (vsc_entry_ptr->iodc_sversion.model == EISA_BUS))
	    {	
		ba_index = 0;
		/*************************************************************
		 * This section of code assumes that all the entries in the
		 * ba_mod_table will either be NULL or point to a vsc entry
		 * for an fio type card.  There are currently no ba type
		 * cards that have ba type cards attached.
  		 *************************************************************/
		while ((ba_index < NUM_SBUS_MODS) &&
		       (vsc_entry_ptr->type.ba.ba_mod_table[ba_index] != NULL))
                {
	            ba_vsc_entry_ptr = vsc_entry_ptr->type.ba.ba_mod_table[ba_index];
		    user_ba_index = ba_vsc_entry_ptr->user_mod_index;
		    User_Mod[user_kern_index].type.ba.ba_mod_table[ba_index] = user_ba_index;
		    /*********************************************************
		     * Now that we know we have a valid vsc_entry_ptr in the
		     * ba_mod_table index that we are interested in and that
		     * it is of type fio, we need to search all the possible
		     * next_ftn pointers for valid entries.  There will
		     * always be at least one copy done here for the first
		     * function in the ba_mod_table entry.  All fio entries
		     * have at least one function.
  		     *********************************************************/
		    for (ftn_ptr = ba_vsc_entry_ptr; ftn_ptr != NULL;
			 ftn_ptr = ftn_ptr->type.fio.next_ftn)
		    {
		        user_ftn_index = ftn_ptr->user_mod_index;
  		        /*****************************************************
			 * If there is a next function, set the User_Mod
			 * next function to the index of that function
			 * Otherwise, set it to -1 to indicate no next_ftn
  		         *****************************************************/
			if (ftn_ptr->type.fio.next_ftn != NULL) {
		            temp = ftn_ptr->type.fio.next_ftn->user_mod_index;
			}
			else {
		            temp = -1;
			}
		        User_Mod[user_ftn_index].type.fio.next_ftn = temp; 
		    }
		    ba_index++;
		}
	    }
	    break;
	  case MOD_TYPE_FOREIGN_IO:
	    switch(vsc_entry_ptr->iodc_sversion.model) {
	    case SCSI_SV_ID:
	    case LAN_SV_ID:
	    case HIL_SV_ID:
	    case CENT_SV_ID:
	    case SERIAL_SV_ID:
	    case SGC_SV_ID:
  		/*************************************************************
		 * Entry is an fio type card.  Search through next_ftn and
		 * fix User_Mod next_ftn indexes.  vsc_entry_ptr is valid
		 * but next_ftn may not be.
  		 *************************************************************/
		for (ftn_ptr = vsc_entry_ptr->type.fio.next_ftn;
		     ftn_ptr != NULL; ftn_ptr = ftn_ptr->type.fio.next_ftn)
		{
  		    /*****************************************************
		     * Set User_Mod current entry's next_ftn to 
		     * index of next_ftn
  		     *****************************************************/
		    user_ftn_index = ftn_ptr->user_mod_index;
		    User_Mod[user_kern_index].type.fio.next_ftn=user_ftn_index;

  		    /*****************************************************
		     * If there is a next function, set the User_Mod
		     * next_ftn to the index of that function
		     * Otherwise leave as is.
  		     *****************************************************/
		    if (ftn_ptr->type.fio.next_ftn != NULL) {
		        temp = ftn_ptr->type.fio.next_ftn->user_mod_index;
		        User_Mod[user_ftn_index].type.fio.next_ftn = temp;
		    }
		}
		break;
	    default:
	        break;
	    }
	    break;
	  default:
	    break;
	  }
	}
    }
}
#endif /* hp9000s700 */

/**************************************************************************
 * dconfig_write(dev, uio)
 **************************************************************************
 *
 * Description:
 *	There is no dconfig_write function. 
 *
 **************************************************************************/

/*
 * These three macros check "flag" against the appropriate access
 * permissions and call "func" if OK and return EPERM otherwise.
 * They are used below in dconfig_open().
 * Nobody actually uses if_w_perm today, but what the heck.
 */

#define if_r_perm(flag, func)	((flag & FREAD) ? func : EPERM)

#define if_w_perm(flag, func)	((flag & FWRITE) ? func : EPERM)

#define if_rw_perm(flag, func)	(((flag & FREAD) && (flag & FWRITE)) ? \
	    func : EPERM)

#ifdef IOSCAN

ioparser_stub() {return ENOSYS;}

int (*scsi_config_ident)() = ioparser_stub;
int (*hpib_config_ident)() = ioparser_stub;
int (*cs80_find_unit1)() = ioparser_stub;

#endif /* IOSCAN */

/**************************************************************************
 * dconfig_ioctl(dev, cmd, arg, flag)
 **************************************************************************
 *
 * Description:
 *	Perform control requests on the dconfig pseudo-driver.
 *
 * Input Parameters:
 *	dev	Device number of the file.  Ignored.
 *
 *	cmd	The commands, corresponding arg type, and required
 *		read/write access permissions are:
 *	    CONFIG_GET_TABLE		config_get_table	    r
 *	    CONFIG_SEARCH_ISC		config_search_isc	    r
 *
 *	arg	Pointer to the command arguments.  This is cast to a pointer
 *		of the appropriate type based on the command.
 *
 *	flag    Value corresponding to "oflag" in open(2).  Checked to make
 *		sure user has permission to perform a given ioctl.
 *
 * Output Parameters: None
 *
 * Returns:
 *	SUCCESS (0)	Success.
 *	ENOTTY		The "cmd" parameter is invalid.
 *	EPERM		Config is not open with the required read/write
 *			access permissions.
 *	other		Value returned by the sub-function.
 *
 * Globals Referenced:
 *	Config_Last_Cmd		Set to the "cmd" parameter if call ok.
 *
 * External Calls: None
 *
 * Algorithm:
 *	If "cmd" is invalid, return ENOTTY.
 *	Switch on "cmd" and check "flag" for neccessary access.
 *	If permission ok, call the appropriate sub-function, else return
 *	EPERM.
 *	If there was no error, save "cmd" in Config_Last_Cmd.
 *	Return the value we got back from the sub-function.
 *
 **************************************************************************/

/*ARGSUSED*/
dconfig_ioctl(dev, cmd, arg, flag)
  dev_t	    dev;
  int	    cmd;
  caddr_t   arg;
  int	    flag;
{
    int status = SUCCESS;
    int major_num;
#ifdef IOSCAN
    int busaddr;
    register struct ioctl_ident_hpib *hpibinfo;
    register struct driver_table_type *driver_table;
    int i;
#endif

    switch (cmd) {
#ifdef __hp9000s700
    case CONFIG_GET_TABLE:
	status = if_r_perm(flag, config_get_table((config_get_table_t *)arg));
	break;
#endif /* hp9000s700 */
    case CONFIG_SEARCH_ISC:
	status = if_r_perm(flag,
		config_search_isc((config_search_isc_t *)arg));
	break;

    case CONFIG_DRIVER_STATUS:
	major_num = ((config_drv_status *)(arg))->drv_major;
	if( (major_num >= 0) && (major_num < nchrdev) )
	{
	    if( cdevsw[major_num].d_open == nodev )
		 ((config_drv_status *)(arg))->drv_status = ENODEV;
	    else
		 ((config_drv_status *)(arg))->drv_status = SUCCESS;
	}
	else
		 ((config_drv_status *)(arg))->drv_status = ENODEV;
	
	break;
#ifdef IOSCAN
    case CONFIG_DRIVERS_PRESENT:
        driver_table = (struct driver_table_type *)arg;
        for (i=0; i<nchrdev; i++) {
            if ( cdevsw[i].d_open == nodev ) 
               driver_table->device_drivers_present[i] = 0;
            else driver_table->device_drivers_present[i] = 1;
        }
        for (i=0; i<=MAXISC; i++) {
            if ( isc_table[i] == NULL ) 
               driver_table->intf_drivers_present[i] = 0;
            else driver_table->intf_drivers_present[i] = 1;
        }
        break;
    case CONFIG_HPIB_IDENTIFY:
	status = (*hpib_config_ident)(arg);
        if (status) break;
        hpibinfo = (struct ioctl_ident_hpib *)arg;
           for (busaddr = 0; busaddr < 8; busaddr++) {
              switch (hpibinfo->dev_info[busaddr].ident) {
                 case 0x183:
                      sprintf(hpibinfo->dev_info[busaddr].class, 12, "tape_drive");
                      sprintf(hpibinfo->dev_info[busaddr].driver, 12, "tape");
                      hpibinfo->dev_info[busaddr].unit1_present = 0;
                      if ( cdevsw[TP_MAJOR].d_open == nodev ) 
                           hpibinfo->dev_info[busaddr].ident = -1;
                      break;
                 case 0x0174: 
                 case 0x0178:
                 case 0x0179: 
                 case 0x0180:
                 case 0x0190:
                      sprintf(hpibinfo->dev_info[busaddr].class, 12, "tape_drive");
                      sprintf(hpibinfo->dev_info[busaddr].driver, 12, "stape");
                      hpibinfo->dev_info[busaddr].unit1_present = 0;
                      if ( cdevsw[STP_MAJOR].d_open == nodev ) 
                           hpibinfo->dev_info[busaddr].ident = -1;
                      break;
                 case 0x0002:
                 case 0x0081:
                 case 0x0104:
                 case 0x0106:
                 case 0x010A:
                 case 0x010F:
                      sprintf(hpibinfo->dev_info[busaddr].class, 12, "disk");
                      sprintf(hpibinfo->dev_info[busaddr].driver, 12, "amigo");
                      hpibinfo->dev_info[busaddr].unit1_present = 0;
                      if ( cdevsw[AMIGO_MAJOR].d_open == nodev ) 
                          hpibinfo->dev_info[busaddr].ident = -1;
                      break;
                 case 0x260: 
                 case 0x268: 
                 case 0x270:
                      sprintf(hpibinfo->dev_info[busaddr].class, 12, "tape_drive");
                      sprintf(hpibinfo->dev_info[busaddr].driver, 12, "cs80");
                      if ( cdevsw[CS80_MAJOR].d_open == nodev )
                           hpibinfo->dev_info[busaddr].ident = -1;
                      else hpibinfo->dev_info[busaddr].unit1_present = 
                                                    (*cs80_find_unit1)(arg, busaddr);
                      break;
                 case -1: break;
                 default: 
                       if ((hpibinfo->dev_info[busaddr].ident >> 8) == 2) {    /* cs80 */
                           sprintf(hpibinfo->dev_info[busaddr].class, 12, "disk");
                           sprintf(hpibinfo->dev_info[busaddr].driver, 12, "cs80");
                           if ( cdevsw[CS80_MAJOR].d_open == nodev )
                                  hpibinfo->dev_info[busaddr].ident = -1;
                           else hpibinfo->dev_info[busaddr].unit1_present = 
                                                 (*cs80_find_unit1)(arg, busaddr);
                       }
                      /* Can anything be none of the above?  Should we do anything? */
                      break;
               } /* switch */
           } /* for */
	break;
    case CONFIG_SCSI_IDENTIFY:
	status = (*scsi_config_ident)(arg);
	break;
#endif /* IOSCAN */
    default:
	status = ENOTTY;
	break;
    }

    if (status == SUCCESS) {
	Config_Last_Cmd = cmd;
    }

    return(status);
}

#ifdef __hp9000s700
/**************************************************************************
 * config_get_table(arg)
 **************************************************************************
 *
 * Description:
 *      Get set up to return the I/O system table specified by which_table.
 *
 * Input Parameters:
 *	arg	Pointer to the command arguments.  The which_table field
 *		is an input parameter.
 *
 * Output Parameters:
 *	arg	Pointer to the command arguments.  The status field is set
 *		to one of:
 *	    >= 0    	    The number of elements in the requested table.
 *	    INVALID_TYPE    The which_table argument is invalid.
 *		The size field is set to the size of the requested table.
 *
 * Returns:
 *	SUCCESS (0)	Status is returned in arg.
 *
 * Globals Referenced:
 *	Cs_Get_Table  
 *
 * External Calls: None
 *
 * Algorithm:
 *      Check arg->which_table; set arg->status to INVALID_TYPE if not valid.
 *	Switch on which_table, look up the number of entries in the
 *	given table in ioheader, and set arg->status to this value.
 *	Set arg->size to the number of entries times the size of each one.
 *	Save a copy of the config_get_table_t in Cs_Get_Table so
 *	dconfig_read() can use it later.
 *	Return SUCCESS.
 *
 **************************************************************************/

config_get_table(arg)
  config_get_table_t	*arg;
{
    switch(arg->which_table) {
    case T_IO_MOD_TABLE:
	/*******************************************************************
	 * There are NUM_SBUS_MODS possible base entries, which can
	 * each have NUM_SBUS_MODS ba_mod_table entries, which can
	 * each have NUM_SBUS_MODS next_ftn entries.  Therefore, the
	 * total possible number of entries is NUM_SBUS_MODS^^3.
         *******************************************************************/
	arg->status = NUM_SBUS_MODS * NUM_SBUS_MODS * NUM_SBUS_MODS;
	arg->size = arg->status * sizeof(return_vsc_mod_entry);
	break;
    case T_MEM_DATA_TABLE:
	/*******************************************************************
	 * There are 32 entries in the memory configuration array where
	 * each entry is an integer in size.
         *******************************************************************/
	arg->status = 32;
	arg->size = 32 * 4;
	break;
    default:
	arg->status = INVALID_TYPE;
	break;
    }

    Cs_Get_Table = *arg;
    return(SUCCESS);
}
#endif /* hp9000s700 */


/**************************************************************************
 * config_search_isc(arg)
 **************************************************************************
 *
 * Description:
 *      Search the isc_table for entries matching the search criteria.
 *
 * Input Parameters:
 *	arg	Pointer to the command arguments.  The search_type,
 *		search_key, search_qual, and isc_entry fields are input
 *		parameters.
 *
 * Output Parameters:
 *	arg	Pointer to the command arguments.  The isc_entry field is
 *		filled in with the matched entry and the status field is
 *		set to one of:
 *	    >= 0    		The number of additional entries which match.
 *	    INVALID_KEY		The search_key argument is invalid.
 *	    INVALID_ENTRY_DATA	The search_key-specific input data in
 *				isc_entry is invalid.
 *	    INVALID_QUAL	The search_qual argument is invalid.
 *	    INVALID_TYPE	The search_type argument is invalid.
 *	    NO_MATCH		No isc_table entries match the search criteria.
 *	    NO_SEARCH_FIRST	SEARCH_NEXT was used before SEARCH_FIRST.
 *	    OUT_OF_MEMORY	No kernel memory available for search.
 *
 * Returns:
 *	SUCCESS (0)	Status is returned in arg.
 *
 * Globals Referenced:
 *	Search_Key       Search_Qual         Search_Type
 *	Search_Init      Search_Single_Node  Search_Count
 *
 * External Calls:  None.
 *
 * Algorithm:
 *	Save the search_type, search_key, search_qual, and, if the
 *	corresponding Search_Qual is set, save the init or ftn_no from 
 *      arg into the corresponding globals (so we don't have 
 *      to pass them to all the functions we call).
 *	Switch on the search_type:
 *	    SEARCH_FIRST:
 *		Free any previous search list.
 *		Check Search_Qual for validity; set arg->status to
 *		INVALID_QUAL and return if invalid bits are set.
 *		Call entry_search() to do the search and set up the
 *		list of matching entries which will be returned in
 *		SEARCH_NEXT.  If it returns any error, set arg->status
 *		to that value, free the search list, and return.
 *		Set Search_First to TRUE and CONTINUE into the SEARCH_NEXT
 *		case.
 *	    SEARCH_NEXT:
 *		If Search_First is FALSE, set arg->status to NO_SEARCH_FIRST
 *		and return.
 *		If Search_Count is 0, set arg->status to NO_MATCH and return.
 *		Call get_search_list_entry() to get the next entry from the
 *		Search_List and call copy_entry_data() to fill in the
 *		isc_entry structure.
 *		Set arg->status to Search_Count (the number of additional
 *		matching entries).
 *	    SEARCH_SINGLE:
 *		Check Search_Qual for validity; set arg->status to
 *		INVALID_QUAL and return if invalid bits are set.
 *		Call entry_search() to do the search and set up
 *		Search_Single_Node to point to the matched entry.  If it
 *		returns an error, set arg->status to that value, and return.
 *		If it didn't find a match, set arg->status to NO_MATCH and
 *		return.  Otherwise, call copy_entry_data() to fill in the
 *		isc_entry structure.
 *		Set arg->status to SUCCESS.
 *	    Anything else:
 *		Set arg->status to INVALID_TYPE.
 *	Return SUCCESS.
 *
 **************************************************************************/

config_search_isc(arg)
  config_search_isc_t	*arg;
{

    Search_Type = arg->search_type;
    Search_Key = arg->search_key;
    Search_Qual = arg->search_qual;

    if (Search_Qual & QUAL_INIT)
	Search_Init = TRUE;
    if (Search_Qual & QUAL_FTN_NO)
	Search_Ftn_No = arg->isc_entry.ftn_no;

    switch(Search_Type) {
    case SEARCH_FIRST:
	free_search_list();

	if ((Search_Qual & ~QUAL_MASK) != 0) {
	    arg->status = INVALID_QUAL;
	    break;
	}

	if ((arg->status = entry_search(&arg->isc_entry)) < 0) {
	    free_search_list();
	    break;
	}

	Search_First = TRUE;
	/* no break */
    case SEARCH_NEXT:
	if (Search_First == FALSE) {
	    arg->status = NO_SEARCH_FIRST;
	    break;
	}

	if (Search_Count == 0) {
	    arg->status = NO_MATCH;
	    break;
	}

	copy_entry_data(&arg->isc_entry, get_search_list_entry());
	arg->status = Search_Count;
	break;
    case SEARCH_SINGLE:
	Search_Single_Node = NULL;

	if ((Search_Qual & ~QUAL_MASK) != 0) {
	    arg->status = INVALID_QUAL;
	    break;
	}

	if ((arg->status = entry_search(&arg->isc_entry)) < 0)
	    break;

	if (Search_Single_Node == NULL) {
	    arg->status = NO_MATCH;
	    break;
	}

	copy_entry_data(&arg->isc_entry, Search_Single_Node);
	arg->status = SUCCESS;
	break;
    default:
	arg->status = INVALID_TYPE;
	break;
    }

    return(SUCCESS);
}

/**************************************************************************
 * entry_search(isc_entry)
 **************************************************************************
 *
 * Description:
 *      Search the isc_table, according to the search key, for entries matching
 *	the appropriate fields from isc_entry and set up a list of the entries
 *	found (SEARCH_FIRST) or a pointer to the entry (SEARCH_SINGLE).
 *
 * Input Parameters:
 *	isc_entry   Pointer to the input entry data.  Selected fields in
 *		    the structure are examined based on the search key and
 *		    search qual.
 *
 * Output Parameters: None.
 *
 * Returns:
 *	    SUCCESS (0)		Search worked.
 *	    INVALID_KEY		The Search_Key is invalid.
 *	    INVALID_ENTRY_DATA	The search_key-specific input data in
 *				isc_entry is invalid.
 *	    other		Value returned by add_search_candidate(),
 *				isc_table_search(), or hdw_path_search().
 *
 * Globals Referenced:
 *	Search_Key  Search_Qual 
 *
 * External Calls: None
 *
 * Algorithm:
 *	Use Search_Key to determine how to traverse the isc_table:
 *	    KEY_ALL:
 *		Call isc_table_search() to search the whole isc_table, and
 *		return its status.
 *	    KEY_MY_ISC:
 *		Call my_isc_search() to do the search, and
 *		return its status.
 *	    KEY_HDW_PATH:
 *		Check isc_entry->hdw_path to see if num_elements is within
 *		range and that each element is within range, and return 
 *              INVALID_ENTRY_DATA if not.
 *		Otherwise, call hdw_path_search() to do the search, and
 *		return its status.
 *	    KEY_BUS_TYPE:
 *		Call bus_type_search() to do the search passing in 
 *              isc_entry->bus_type, and return its status.
 *	    KEY_IF_ID:
 *		This is the same as KEY_BUS_TYPE, except we use isc_entry->if_id.
 *	    Anything else:
 *		Return INVALID_KEY.
 *	Finally, return SUCCESS (any error was returned earlier).
 *
 **************************************************************************/

entry_search(isc_entry)
  isc_entry_type	*isc_entry;
{
    int		    status;

    switch(Search_Key) {
    case KEY_ALL:
	return(isc_table_search());
	break;
    case KEY_MY_ISC:
	return(my_isc_search(isc_entry->my_isc));
	break;
    case KEY_HDW_PATH:
#ifdef __hp9000s700
	if ((isc_entry->hdw_path.num_elements < 3) ||
	    (isc_entry->hdw_path.num_elements > MAX_IO_PATH_ELEMENTS) ||
	    (isc_entry->hdw_path.addr[0] > 7) ||
	    (isc_entry->hdw_path.addr[1] > 15) ||
	    (isc_entry->hdw_path.addr[2] > 15))
#else
	if ((isc_entry->hdw_path.num_elements < 1) ||
	    (isc_entry->hdw_path.num_elements > MAX_IO_PATH_ELEMENTS) ||
	    (isc_entry->hdw_path.addr[0] >= ISC_TBL_SIZE))
#endif
	    return(INVALID_ENTRY_DATA);

	return(hdw_path_search(isc_entry->hdw_path));
	break;
    case KEY_IF_ID:
	return(if_id_search(isc_entry->if_id));
	break;
    case KEY_BUS_TYPE:
	return(bus_type_search(isc_entry->bus_type));
	break;
    default:
	return(INVALID_KEY);
    }

    return(SUCCESS);
}

/**************************************************************************
 * my_isc_search
 **************************************************************************
 *
 * Description:
 *      Search the isc_table for the entries that match the specified
 *      isc and add all the entries to the search list.
 *
 * Input Parameters: None
 *
 * Output Parameters: None.
 *
 * Returns:
 *	    SUCCESS (0)		Search worked.
 *	    other		Value returned by add_search_candidate().
 *
 * Globals Referenced:	None
 *
 * External Calls:	None
 *
 * Algorithm:
 *      Get the entry in the isc_table that matches this isc
 *      For this entry and each next_ftn entry that is valid
 *	    Call add_search_candidate() to add it to the search list.
 *	    If add_search_candidate() returns anything other then SUCCESS,
 *	    return that value.
 *	Finally, return SUCCESS (any error was returned earlier).
 *
 **************************************************************************/

int
my_isc_search(my_isc)
unsigned char	my_isc;
{
    int	i ,r;
    struct isc_table_type *isc_entry;

    for (isc_entry = isc_table[my_isc]; isc_entry != NULL; 
	 isc_entry = isc_entry->next_ftn)
    {
        if ((r = add_search_candidate(isc_entry)) != SUCCESS)
	    return(r);
    }

    return(SUCCESS);
}

/**************************************************************************
 * bus_type_search(bus_type) 
 **************************************************************************
 *
 * Description:
 *      Search the isc_table and add all the entries that match the
 *      specified bus_type to the search list.
 *
 * Input Parameters: 
 *          bus_type   The bus type we are looking for.
 *
 * Output Parameters: None.
 *
 * Returns:
 *	    SUCCESS (0)		Search worked.
 *	    other		Value returned by add_search_candidate().
 *
 * Globals Referenced:	None
 *
 * External Calls:	None
 *
 * Algorithm:
 *	For each entry in the isc_table and it's next_ftn entries
 *          If entry bus_type matches specified bus_type
 *	        Call add_search_candidate() to add it to the search list.
 *	        If add_search_candidate() returns anything other then SUCCESS,
 *	        return that value.
 *	Finally, return SUCCESS (any error was returned earlier).
 *
 **************************************************************************/

int
bus_type_search(bus_type)
char	bus_type;
{
    int	i, r;
    struct isc_table_type *isc_entry;

    for (i = 0; i < ISC_TBL_SIZE; i++)
    {
	for (isc_entry = isc_table[i]; isc_entry != NULL;
	     isc_entry = isc_entry->next_ftn)
	{
	    if (isc_entry->bus_type == bus_type)
	    {
	        if ((r = add_search_candidate(isc_entry)) != SUCCESS)
	    	    return(r);
	    }
	}
    }

    return(SUCCESS);
}

/**************************************************************************
 * if_id_search(if_id) 
 **************************************************************************
 *
 * Description:
 *      Search the isc_table and add all the entries that match the
 *      specified interface identifier to the search list.
 *
 * Input Parameters: 
 *          if_id   The interface identifier we are looking for.
 *
 * Output Parameters: None.
 *
 * Returns:
 *	    SUCCESS (0)		Search worked.
 *	    other		Value returned by add_search_candidate().
 *
 * Globals Referenced:	None
 *
 * External Calls:	None
 *
 * Algorithm:
 *	For each entry in the isc_table
 *          If entry if_id matches specified if_id
 *	        Call add_search_candidate() to add it to the search list.
 *	        If add_search_candidate() returns anything other then SUCCESS,
 *	        return that value.
 *	Finally, return SUCCESS (any error was returned earlier).
 *
 **************************************************************************/

int
if_id_search(if_id)
int	if_id;
{
    int	i, r;
    struct isc_table_type *isc_entry;

    for (i = 0; i < ISC_TBL_SIZE; i++)
    {
	for (isc_entry = isc_table[i]; isc_entry != NULL;
	     isc_entry = isc_entry->next_ftn)
	{
	    if (isc_entry->if_id == if_id)
	    {
	        if ((r = add_search_candidate(isc_entry)) != SUCCESS)
	    	    return(r);
	    }
	}
    }

    return(SUCCESS);
}

/**************************************************************************
 * hdw_path_search(hdw_path)
 **************************************************************************
 *
 * Description:
 *      Create a device minor number from the hardware path.
 *      Search the isc_table looking for entries that match the created
 *      device minor.  Add them to the search list.
 *
 * Input Parameters:
 *
 *	hdw_path    The hardware path we're looking for.  At least 3
 *		    elements must be valid (num_elements >= 3).
 *                  Only need the first 3 to get the isc entry.
 *
 * Output Parameters: None.
 *
 * Returns:
 *	    SUCCESS (0)		Search worked.
 *	    INVALID_ENTRY_DATA	The hdw_path is invalid.
 *          NO_MATCH            No entry matches hdw_path
 *	    other		Value returned by add_search_candidate().
 *
 * Globals Referenced:	None
 *
 * External Calls:	None
 *
 * Algorithm:
 *      Create the device minor from the hdw_path.
 *      Get the isc_table entry for that minor number
 *      If the minor number indicates it is multi-function
 *          Search the list of function entries for the matching function number
 *      If did not find any entry return NO_MATCH
 *	Finally, call add_search_candidate and return it's value
 *
 **************************************************************************/

int
hdw_path_search(hdw_path)
  hdw_path_type	    hdw_path;
{
    int dev_minor;
    struct isc_table_type	*isc_entry;

    /*****************************************************************
     * Initialize the dev.  Then using the known
     * hdw_path elements construct the dev minor number.
     *
     * For the 700:
     *      Pdev is vsc.slot.ftn_no
     *      If single function card then ftn_no will be 0.
     *      dev minor number is structured as follows:
     *        23    20 19    16 15     12 11             0
     *        ---------------------------------------------
     *        | vsc # | slot # |  ftn #  |   driver bits  |
     *        ---------------------------------------------
     *
     * For the 300/400:
     *      Pdev is selcode.ba
     *
     *        23              16 15                   0
     *        -----------------------------------------
     *        |    sel code     |    driver bits      |
     *        -----------------------------------------
     *
     *****************************************************************/

#ifdef __hp9000s700
    dev_minor = 0;
    dev_minor =  (hdw_path.addr[0] & 0x07) << 20;
    /*****************************************************************
     * Set slot bits to the value of path element 1.
     * If path element 2 is 0, then it is a single function card, leave
     * ftn # bits at 0.  Otherwise it is a multi function card, set
     * ftn # to value of element 2.
     *****************************************************************/
    dev_minor = dev_minor | (hdw_path.addr[1] << 16);
    if (hdw_path.addr[2] > 0)
    {
        dev_minor = dev_minor | (hdw_path.addr[2] << 12);
    }
#else
    dev_minor =  (hdw_path.addr[0] << 16);
#endif  /*  __hp9000s700 */

    /*****************************************************************
     * Now that have dev_minor search isc_table for matching entry
     * and add it to the list.
     *****************************************************************/
    isc_entry = isc_table[m_selcode(dev_minor)];
    if (isc_entry != NULL) 
    {
        if (isc_entry->ftn_no != -1) 
        {
            while ((isc_entry != NULL) &&
                   (isc_entry->ftn_no != m_function(dev_minor)))
                isc_entry = isc_entry->next_ftn;
        }
    }
    if (isc_entry == NULL)
    {
	return(NO_MATCH);
    }
    return(add_search_candidate(isc_entry));

}

/**************************************************************************
 * isc_table_search()
 **************************************************************************
 *
 * Description:
 *      Search the isc_table and add all the entries to the search list.
 *
 * Input Parameters: None
 *
 * Output Parameters: None.
 *
 * Returns:
 *	    SUCCESS (0)		Search worked.
 *	    other		Value returned by add_search_candidate().
 *
 * Globals Referenced:	None
 *
 * External Calls:	None
 *
 * Algorithm:
 *	For each entry in the 
 *	    Call add_search_candidate() to add it to the search list.
 *	    If add_search_candidate() returns anything other then SUCCESS,
 *	    return that value.
 *	Finally, return SUCCESS (any error was returned earlier).
 *
 **************************************************************************/

int
isc_table_search()
{
    int	i, r;
    struct isc_table_type *isc_entry;

    for (i = 0; i < ISC_TBL_SIZE; i++)
    {

	for (isc_entry = isc_table[i]; isc_entry != NULL;
	     isc_entry = isc_entry->next_ftn)
	{
	    if ((r = add_search_candidate(isc_entry)) != SUCCESS)
		return(r);
	}
    }

    return(SUCCESS);
}

/**************************************************************************
 * add_search_candidate(entry)
 **************************************************************************
 *
 * Description:
 *	Add a candidate search entry to the appropriate search list (either
 *	SEARCH_FIRST or SEARCH_SINGLE) if it
 *	matches the search qualifiers specified by the user.
 *
 * Input Parameters:
 *	entry	A pointer to the entry to add to the list.
 *
 * Output Parameters: None.
 *
 * Returns:
 *	SUCCESS (0)	It worked.
 *  Warnings (>0):
 *	END_OF_SEARCH	The search_type is SEARCH_SINGLE and we've added
 *			the first (and only) entry to the search list.
 *  Errors (<0):
 *	other		Value returned by add_search_list_entry()
 *			
 *
 * Globals Referenced:
 *	Search_Type  Search_Qual  Search_Single_Node
 *
 * External Calls:	None.
 *
 * Algorithm:
 *	Switch on Search_Type:
 *	SEARCH_FIRST:
 *	    If match_search_qual() says the entry matches the search
 *	    qualifiers, call add_search_list_entry() to add the entry to the
 *	    list, and return its return value if it fails.
 *	SEARCH_SINGLE:
 *	    Call match_search_qual() to see if the entry matches the search
 *	    qualifiers.  If it does, set Search_Single_Node to point to the
 *	    entry and return END_OF_SEARCH.
 *	Return SUCCESS.
 *
 **************************************************************************/

int
add_search_candidate(entry)
  struct isc_table_type	*entry;
{
    int r;

    switch (Search_Type) {
    case SEARCH_FIRST:
	if (match_search_qual(entry)) {
	    if ((r = add_search_list_entry(entry)) != SUCCESS)
		return(r);
	}
	break;
    case SEARCH_SINGLE:
	if (match_search_qual(entry)) {
	    Search_Single_Node = entry;
	    return(END_OF_SEARCH);
	}
	break;
    }

    return(SUCCESS);
}

/**************************************************************************
 * match_search_qual(entry)
 **************************************************************************
 *
 * Description:
 *	Determine whether a candidate search entry matches the user's search
 *	qualifiers.
 *
 * Input Parameters:
 *	entry	A pointer to the entry in the isc_table to check.
 *
 * Output Parameters: None.
 *
 * Returns:
 *	TRUE	The entry matches the qualifiers.
 *	FALSE	It doesn't.
 *
 * Globals Referenced:
 *	Search_Qual  Search_Init  Search_Ftn_No
 *
 * External Calls: None.
 *
 * Algorithm:
 *	If Search_Qual is QUAL_NONE, immediately return TRUE.  This makes
 *	no qualifiers (the usual case) as fast as possible and only slows
 *	down the others a little bit.
 *	Otherwise, the basic strategy is to AND Search_Qual with each
 *	QUAL bit in turn, and for each one which is set, check the
 *	appropriate fields in the entry and return FALSE if the check fails.
 *	If we make it to the end of the function, return TRUE.
 *
 **************************************************************************/

int
match_search_qual(entry)
  struct isc_table_type	*entry;
{
  struct eisa_if_info 	*if_info;

    if (Search_Qual == QUAL_NONE)
    {
#ifdef __hp9000s700
	return(TRUE);
#else
	if( entry->card_type > NO_ID )
	     return(TRUE);
	else
	     return(FALSE);
#endif
    }

#ifdef __hp9000s700
    if (Search_Qual & QUAL_INIT)
    {
        if_info = (struct eisa_if_info *) entry->if_info;
	if (if_info != NULL) 
	{
	    if ((if_info->flags & INITIALIZED) == FALSE)
	    return(FALSE);
	}
    }
#endif  /* __hp9000s700 */
    if ((Search_Qual & QUAL_FTN_NO) && entry->ftn_no != Search_Ftn_No)
	return(FALSE);

    return(TRUE);
}

/**************************************************************************
 * add_search_list_entry(entry)
 **************************************************************************
 *
 * Description:
 *	Add a entry to the search list.
 *
 * Input Parameters:
 *	entry	A pointer to the entry in the isc_table to add to the list.
 *
 * Output Parameters: None.
 *
 * Returns:
 *	SUCCESS (0)	It worked.
 *	OUT_OF_MEMORY	No kernel memory available to add to the list.
 *
 * Globals Referenced:
 *	Search_List  Search_List_End  Search_Count
 *
 * External Calls:
 *	MALLOC()
 *
 * Algorithm:
 *	The search list is maintained as a linked list of
 *	config_search_list_t structures, each with room for
 *	SEARCH_ENTRIES_PER_ENTRY entry pointers.  Each entry also has a count
 *	of the number of valid entries (entry_count) and the index of the
 *	first valid entry (first_entry).
 *	If the list is completely empty (Search_list is NULL), try to
 *	allocate a new config_search_list_t.  If MALLOC() fails, return
 *	OUT_OF_MEMORY.  Otherwise, set Search_List, and Search_List_End
 *	to point to the new entry, set the entry's entry_count and
 *	first_entry values to 0, and its next pointer to NULL.
 *	Else (list not empty), if the last entry on the list is full,
 *	allocate another one (as above) and link it onto the end of the
 *	search list.
 *	By this point, we have an entry with room for the new entry pointer,
 *	so fill it in and increment the entry's entry_count and the global
 *	Search_Count, and return SUCCESS.
 *
 **************************************************************************/

int
add_search_list_entry(entry)
  struct isc_table_type	*entry;
{
    if (Search_List == NULL_SEARCH_LIST_PTR) {
	/* might sleep here */
	MALLOC(Search_List, config_search_list_t *,
	    sizeof(config_search_list_t), M_TEMP, M_WAITOK);
	if (Search_List == NULL_SEARCH_LIST_PTR)
	    return(OUT_OF_MEMORY);
	Search_List_End = Search_List;
	Search_List->entry_count = 0;
	Search_List->first_entry = 0;
	Search_List->next = NULL_SEARCH_LIST_PTR;
    } else if (Search_List_End->entry_count == SEARCH_ENTRIES_PER_ENTRY) {
	/* might sleep here */
	MALLOC(Search_List_End->next, config_search_list_t *,
	    sizeof(config_search_list_t), M_TEMP, M_WAITOK);
	if (Search_List_End->next == NULL_SEARCH_LIST_PTR)
	    return(OUT_OF_MEMORY);
	Search_List_End = Search_List_End->next;
	Search_List_End->entry_count = 0;
	Search_List_End->first_entry = 0;
	Search_List_End->next = NULL_SEARCH_LIST_PTR;
    }

    /* Search_List_End now points to a list entry with space for a entry */
    Search_List_End->entry[Search_List_End->entry_count++] = entry;
    Search_Count++;
    return(SUCCESS);
}


/**************************************************************************
 * free_search_list()
 **************************************************************************
 *
 * Description:
 *	Free the search list.
 *
 * Input Parameters:	None.
 *
 * Output Parameters:	None.
 *
 * Returns:		None.
 *
 * Globals Referenced:
 *	Search_List  Search_List_End  Search_Count  Search_First
 *
 * External Calls:
 *	FREE()
 *
 * Algorithm:
 *	For each entry on the search list, call FREE().
 *	Then set Search_List and Search_List_End to NULL, Search_Count
 *	to 0, and Search_First to FALSE.
 *
 **************************************************************************/

void
free_search_list()
{
    config_search_list_t *n;

    while (Search_List != NULL_SEARCH_LIST_PTR) {
	n = Search_List->next;
	FREE((caddr_t)Search_List, M_TEMP);
	Search_List = n;
    }

    /* Search_List is NULL at this point */
    Search_List_End = NULL_SEARCH_LIST_PTR;
    Search_Count = 0;
    Search_First = FALSE;
}

/**************************************************************************
 * get_search_list_entry()
 **************************************************************************
 *
 * Description:
 *	Get the next entry from the search list.  Also, update Search_Count
 *	and Search_First as appropriate.
 *
 * Input Parameters:	None.
 *
 * Output Parameters: None.
 *
 * Returns:
 *	A pointer to the entry.
 *
 * Globals Referenced:
 *	Search_List  Search_Count  Search_First
 *
 * External Calls:
 *	FREE()
 *
 * Algorithm:
 *	The search list is maintained as a linked list of
 *	config_search_list_t structures, each with room for
 *	SEARCH_ENTRIES_PER_ENTRY entry pointers.  Each entry also has a count
 *	of the number of valid entries (entry_count) and the index of the
 *	first valid entry (first_entry).
 *	Save the entry pointer at the "first_entry" location from the first
 *	entry on the search list.
 *	Advance first_entry and decrement the entry_count in the entry.
 *	If this is the last entry in the entry, advance Search_List and
 *	FREE() the entry.
 *	Decrement the global Search_Count and if it's now 0, set
 *	Search_First to FALSE (so any additional SEARCH_NEXT calls will
 *	return NO_SEARCH_FIRST).
 *	Return the entry pointer.
 *
 **************************************************************************/

struct isc_table_type *
get_search_list_entry()
{
    struct isc_table_type	    *entry;
    config_search_list_t    *n;

    entry = Search_List->entry[Search_List->first_entry++];

    if (--Search_List->entry_count == 0) {
	n = Search_List;
	Search_List = Search_List->next;
	FREE((caddr_t)n, M_TEMP);
    }

    if (--Search_Count == 0)
	Search_First = FALSE;

    return(entry);
}

/**************************************************************************
 * copy_entry_data(isc_entry, table_entry)
 **************************************************************************
 *
 * Description:
 *	Copy entry data from an isc_table entry to an isc_entry_type.
 *
 * Input Parameters:
 *	isc_entry    Pointer to the isc_entry_type to copy into.
 *
 *	table_entry  Pointer to the struct isc_table_type to copy from.
 *
 * Output Parameters:	None.
 *
 * Returns:		None.
 *
 * Globals Referenced:	None.
 *
 * External Calls:
 *	bcopy()  bzero()
 *
 * Algorithm:
 *	First, use bzero() to zero out the isc_entry.  After which,
 *	this is pretty much a simple field-by-field copy of data from the
 *	table_entry (and the tables it points to) into the isc_entry_type.
 *	The only tricky field is the hdw_path, for which we call
 *	copy_hardware_path() (see definition below).
 *	One other weirdness is the c_major and b_major fields are set to
 *      NONE.  It is up to the calling library routine to fill these in.
 *
 **************************************************************************/

void
copy_entry_data(isc_entry, table_entry)
  isc_entry_type	*isc_entry;
  struct isc_table_type	*table_entry;
{
    bzero((caddr_t)isc_entry, sizeof(isc_entry_type));
    isc_entry->my_isc = table_entry->my_isc;
    isc_entry->bus_type = table_entry->bus_type;
    isc_entry->ftn_no = table_entry->ftn_no;
    isc_entry->if_id = table_entry->if_id;

#ifdef __hp9000s700
    isc_entry->if_id = table_entry->if_id;
#else
    isc_entry->if_id = table_entry->card_type;
#endif

    isc_entry->b_major = NONE;
    isc_entry->c_major = NONE;
    copy_hdw_path(isc_entry->my_isc, isc_entry->ftn_no, &isc_entry->hdw_path);
}

/**************************************************************************
 *copy_hdw_path(my_isc, ftn_no, hdw_path)
 **************************************************************************
 *
 * Description:
 *	Copy the hardware path associated with a my_isc, ftn_no into
 *	hdw_path.
 *
 * Input Parameters:
 *	my_isc    Interface select code to use
 *	ftn_no    Function code to use
 *
 * Output Parameters:	None.
 *	Hdw_path  Pointer to the hdw_path structure to fill in.
 *
 * Returns:		None.
 *
 * Globals Referenced:  None.
 *
 * External Calls:	None.
 *
 * Algorithm:
 *    Initialize hdw_path structure
 *    Get first element using vsc number in my_isc
 *    Get second element using slot number in my_isc
 *    Get third element ftn_no
 *    Set num_elements to 3
 *
 **************************************************************************/

void
copy_hdw_path(my_isc, ftn_no, hdw_path)
unsigned char	my_isc;
int		ftn_no;
hdw_path_type	*hdw_path;
{
    int i;

    /*****************************************************************
     * Initialize the hdw_path structure.  Then using the known
     * my_isc value and the ftn_no construct a pdev.
     * Pdev is vsc.slot.ftn_no
     * my_isc is structures as follows:
     *
     * For the 700:		For the 300 400
     *
     *    7 6 5 4 3 2 1 0	 7	       0
     *   -----------------	-----------------
     *   | vsc # |slot # |	|  select code  |
     *   -----------------	-----------------
     *****************************************************************/
    hdw_path->num_elements = 0;
    for (i = 0; i < MAX_IO_PATH_ELEMENTS; i++)
    {
        hdw_path->addr[i] = 0;
    }
#ifdef __hp9000s700
    hdw_path->addr[0] = ((int)my_isc & 0x000000f0) >> 4;
    /*****************************************************************
     * Set element 1 to value of slot.
     * If ftn_no is -1 then is a single function card and hdw_path
     * element 2 will be 0. Otherwise it is the ftn_no.
     *****************************************************************/
    hdw_path->addr[1] = (int)my_isc & 0x0000000f;

    if (ftn_no != -1)
    {
    	hdw_path->addr[2] = (int)ftn_no;
    }

    hdw_path->num_elements = 3;
#else
    hdw_path->addr[0] = (int)my_isc;
    hdw_path->num_elements = 1;

#endif
}


#ifdef __hp9000s700
/* ERROR - Stub functions for Snakes I/O using it in error */


io_tree_entry *find_child(conn, parent, hdw_address)
  io_conn_entry *conn;
  io_tree_entry *parent;
  int           hdw_address;
{
}
int
mod_conn_check(mod_path, level, conn_path, valid_elements)
  module_path_type  *mod_path;
  int               level;
  io_conn_entry     *conn_path[];
  int               *valid_elements;
{
}
#endif /* hp9000s700 */
