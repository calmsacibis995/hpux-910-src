/* @(#) $Revision: 70.2 $ */      

/*
 * This structure describes the format of the STI rom.
 * STI (Standard Text Interface) is a means by which graphics cards
 * contain information telling the OS how to deal with them.
 *
 * See Conwell Dickey for information on the STI specification.
 */

struct sti_rom {
	char pad1[3];
	unsigned char device_type;
	char pad3[7];
	unsigned char global_rom_rev;
	char pad4[3];
	unsigned char local_rom_rev;
	int graphics_id[8];
	int font_start[4];
	int max_state_storage[4];
	int last_addr_of_rom[4];
	int dev_region_list[4];
	int pad5[100];

	/* These entry points are for the 68000 routines. */
	int init_graph[4];
	int state_mgmt[4];
	int font_unpmv[4];
	int block_move[4];
	int self_test[4];
	int excep_hdlr[4];
	int inq_conf[4];
};



struct sti_font {
	unsigned int first_char[2];		/* first character in font */
	unsigned int last_char[2];		/* Last character in font */
	char pad1[3];
	unsigned char width;			/* width of characters */
	char pad2[3];
	unsigned char height;			/* height of characters */
	char pad3[3];
	unsigned char font_type;
	char pad4[3];
	unsigned char bytes_per_char;
	int offset[4];
	char pad5[3];
	unsigned char underline_height;
	char pad6[3];
	unsigned char underline_offset;
};


/* Font stuff
       _______________________________________________________________
      | Rel Addr	bytes			Definition	      |
      |_______________________________________________________________|
      | 0x03		  2	     ASCII Code for first char in font|
      | 0x07		  -					      |
      | 0x0b		  2	     ASCII Code for last char in font |
      | 0x0f		  -					      |
      |_______________________________________________________________|
      | 0x13		  1	     Width of Font in pixels (W)      |
      | 0x17		  1	     Height of Font in pixels (H)     |
      | 0x1b		  1	     Font Type			      |
      | 0x1f		  1	     bytes/char (int((W+7)/8))*H      |
      |_______________________________________________________________|
      | 0x23		  4	     offset to start of next font     |
      | 0x27		  -					      |
      | 0x2b		  -					      |
      | 0x2f		  -					      |
      |_______________________________________________________________|
      | 0x33		  1	     Height of underline in pixels    |
      | 0x37		  1	     Offset of underline	      |
      | 0x3b		  1	     Unused			      |
      | 0x3f		  1	     Unused			      |
      |_______________________________________________________________|
      | 0x43	    (int(W+7)/8)*H   First character		      |
      | ---	    (int(W+7)/8)*H   Second character		      |
      |_______________________________________________________________|
      | thru							      |
      |_______________________________________________________________|
      | ---	    (int(W+7)/8)*H   last character		      |
      |_______________________________________________________________|
*/

/* sti module return parameters */
#define PASS			0
#define FAIL			-1
#define NOT_RDY			1

/* other defs */
#define REGION_MAX     		8
#define	DEV_NAME_LENGTH 	32

/****************************************************************
 *
 * global config structures
 *
 ****************************************************************/

typedef struct {
   int	 text_planes;			/* number of planes used for text */
   short int onscreen_x;		/* screen width in pixels */
   short int onscreen_y;		/* screen height in pixels */
   short int offscreen_x;		/* offscreen width in pixels */
   short int offscreen_y;		/* offscreen height in pixels */
   short int total_x;			/* frame buffer width in pixels	*/
   short int total_y;			/* frame buffer height in pixels */
   int   region_ptrs[REGION_MAX];	/* region pointers */
   int   reent_lvl;			/* storage for reentry level value */
   int   save_addr;         		/* where to save or restore state */
   int   *future_ptr;			/* pointer to future data structure */
} glob_cfg;

/****************************************************************
 *
 * region descriptor structure
 *
 ****************************************************************/

typedef struct {
   unsigned  offset	      : 14;	/* offset in 4kbyte page */ 
   unsigned  sys_only	      : 1;	/* don't map to user space */ 
   unsigned  cache	      : 1;	/* map to data cache */ 
   unsigned  pad	      : 1;	/* not used */ 
   unsigned  last	      : 1;	/* last region in list */ 
   unsigned  length	      : 14;	/* length in 4kbyte page */ 
} region_desc;

/****************************************************************
 *
 * init_graph structures
 *
 *	init_graph(&init_flags,&init_inptr,&init_outptr,&glob_cfg)
 *
 ****************************************************************/

typedef struct {
   unsigned   wait	      : 1;	/* should routine idle wait or not */
   unsigned   reset	      : 1;	/* hard reset the device? */
   unsigned   text	      : 1;	/* turn on text display planes?	*/
   unsigned   nontext	      : 1;	/* turn on non-text display planes? */
   unsigned   clear           : 1;	/* clear text display planes? */
   unsigned   cmap_blk        : 1;	/* non-text planes cmap black? */
   unsigned   enable_be_timer : 1;	/* enable bus error timer */
   unsigned   enable_be_int   : 1;	/* enable bus error timer interrupt */
   unsigned   no_chg_tx	      : 1;	/* don't change text settings */
   unsigned   no_chg_ntx      : 1;	/* don't change non-text settings */
   unsigned   no_chg_bet      : 1;	/* don't change berr timer settings */
   unsigned   no_chg_bei      : 1;	/* don't change berr int settings */
   unsigned   init_cmap_tx    : 1;	/* initialize cmap for text planes */
   unsigned   pad	      :19;	/* pad to word boundary */
   int	      *future_ptr;		/* pointer to future data */
} init_flags;

typedef struct {
   int        text_planes;		/* number of planes to use for text */
   int	      *future_ptr;		/* pointer to future data */
} init_inptr;

typedef struct {
   int	      errno;			/* error number on failure */
   int	      text_planes;		/* number of planes used for text */
   int	      *future_ptr;		/* pointer to future data */
} init_outptr;

/*****************************************************************
 *
 * state_mgmt structures
 *
 *	state_mgmt(&state_flags,&state_inptr,&state_outptr,&glob_cfg)
 *
 ****************************************************************/

typedef struct {
   unsigned   wait	      : 1;	/* should routine idle wait or not */
   unsigned   save	      : 1;	/* save (1) or restore (0) state */
   unsigned   pad	      :30;	/* pad to word boundary */
   int	      *future_ptr;		/* pointer to future data */
} state_flags;

typedef struct {
   int	      save_addr;		/* where to save or restore state */
   int	      *future_ptr;		/* pointer to future data */
} state_inptr;

typedef struct {
   int	      errno;			/* error number on failure */
   int	      *future_ptr;		/* pointer to future data */
} state_outptr;

/****************************************************************
 *
 * font_unpmv structures
 *
 *	font_flags(&font_flags,&font_inptr,&font_outptr,&glob_cfg)
 *
 ****************************************************************/

typedef struct {
   unsigned   wait	      : 1;	/* should routine idle wait or not */
   unsigned   pad	      :31;	/* pad to word boundary */
   int	      *future_ptr;		/* pointer to future data */
} font_flags;

typedef struct {
   int	      font_start_addr;		/* address of font start */
   short int  index;			/* index into font table of character */
   char	      fg_color;			/* foreground color of character */
   char	      bg_color;			/* background color of character */
   short int  dest_x;			/* X location of character upper left */
   short int  dest_y;			/* Y location of character upper left */
   int	      *future_ptr;		/* pointer to future data */
} font_inptr;

typedef struct {
   int	      errno;			/* error number on failure */
   int	      *future_ptr;		/* pointer to future data */
} font_outptr;

/****************************************************************
 *
 * block_move structures
 *
 *	block_move(&blkmv_flags,&blkmv_inptr,&blkmv_outptr,&glob_cfg)
 *
 ****************************************************************/

typedef struct {
   unsigned   wait	      : 1;	/* should routine idle wait or not */
   unsigned   color	      : 1;	/* change color during move? */
   unsigned   clear	      : 1;	/* clear during move? */
   unsigned   pad	      :29;	/* pad to word boundary */
   int	      *future_ptr;		/* pointer to future data */
} blkmv_flags;

typedef struct {
   char	      fg_color;			/* foreground color after move */
   char	      bg_color;			/* background color after move */
   short int  src_x;			/* source upper left pixel x location */
   short int  src_y;			/* source upper left pixel y location */
   short int  dest_x;			/* dest upper left pixel x location */
   short int  dest_y;			/* dest upper left pixel y location */
   short int  width;			/* block width in pixels */
   short int  height;			/* block height in pixels */
   int	      *future_ptr;		/* pointer to future data */
} blkmv_inptr;

typedef struct {
   int	      errno;			/* error number on failure */
   int	      *future_ptr;		/* pointer to future data */
} blkmv_outptr;

/****************************************************************
 *
 * self_test structures
 *
 *	self_test(&test_flags,&test_inptr,&test_outptr,&glob_cfg)
 *
 ****************************************************************/

typedef struct {
   unsigned   wait	      : 1;	/* should routine idle wait or not */
   unsigned   pad	      :31;	/* pad to word boundary	*/
   int	      *future_ptr;		/* pointer to future data */
} test_flags;

typedef struct {
   int	      *future_ptr;		/* pointer to future data */
} test_inptr;

typedef struct {
   int	      errno;			/* error number on failure */
   int 	      result;			/* result of the self test */
   int	      *future_ptr;		/* pointer to future data */
} test_outptr;

/****************************************************************
 *
 * excep_hdlr structures
 *
 *	excep_hdlr(&excep_flags,&excep_inptr,&excep_outptr,&glob_cfg)
 *
 ****************************************************************/

typedef struct {
   unsigned   wait	      : 1;	/* should routine idle wait or not */
   unsigned   clr_int	      : 1;	/* should routine clr int or not */
   unsigned   clr_be          : 1;	/* should routine clr be stat or not */
   unsigned   save_int        : 1;	/* should int state be saved or not */
   unsigned   restore_int     : 1;	/* should int state be restored or not*/
   unsigned   pad	      :27;	/* pad to word boundary	*/
   int	      *future_ptr;		/* pointer to future data */
} excep_flags;

typedef struct {
   int	      save_addr;		/* where to save or restore int state */
   int	      *future_ptr;		/* pointer to future data */
} excep_inptr;

typedef struct {
   int	      errno;			/* error number on failure */
   unsigned   be	      : 1;	/* was be intercepted or not */
   unsigned   int_pend	      : 1;	/* is there an existing int or not */
   unsigned   pad	      :30;	/* pad to word boundary	*/
   int	      *future_ptr;		/* pointer to future data */
} excep_outptr;

/****************************************************************
 *
 * inq_conf structures
 *
 *	inq_conf(&conf_flags,&conf_inptr,&conf_outptr,&glob_cfg)
 *
 ****************************************************************/

typedef struct {
   unsigned   wait	      : 1;	/* should routine idle wait or not */
   unsigned   pad	      :31;	/* pad to word boundary	*/
   int	      *future_ptr;		/* pointer to future data */
} conf_flags;

typedef struct {
   int	      *future_ptr;		/* pointer to future data */
} conf_inptr;

typedef struct {
   int	errno;				/* error number on failure */
   short int onscreen_x;		/* screen width in pixels */
   short int onscreen_y;		/* screen height in pixels */
   short int offscreen_x;		/* offscreen width in pixels */
   short int offscreen_y;		/* offscreen height in pixels */
   short int total_x;			/* frame buffer width in pixels	*/
   short int total_y;			/* frame buffer height in pixels */
   int  bits_per_pixel;			/* bits/pixel device has configured */
   int  bits_used;			/* bits which can be accessed */
   int  planes;				/* number of fb planes in system */
   char dev_name[DEV_NAME_LENGTH];	/* null terminated product name	*/
   unsigned int attributes;		/* flags denoting attributes */
   int	*future_ptr;			/* pointer to future data */
} conf_outptr;
