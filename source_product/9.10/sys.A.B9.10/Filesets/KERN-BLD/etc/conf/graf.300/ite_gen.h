/*
 * @(#)ite_gen.h: $Revision: 1.3.84.3 $ $Date: 93/09/17 20:59:28 $
 * $Locker:  $
 */


#ifndef __ITE_GEN_H_INCLUDED
#define __ITE_GEN_H_INCLUDED

struct gen_rgb {		 	 /* A Genesis overlay colormap entry */
    char dummy;
    char blue;
    char green;
    char red;
};


struct disp_reg_set {
    int  d01[(0x0c>>2)]; 
    int  OPE_P;					/*  window base + 0x0c	*/
    int  OPE_S;					/*  window base + 0x10	*/
    int  OIA_P;					/*  window base + 0x14	*/
    int  OIA_S;					/*  window base + 0x18	*/
    int  d02[(0x40>>2)-(0x18>>2)-1];
};


struct genesis {
    int	 reset;					/*  0x00000	*/

/* pixel cache */
    int  x10[(0x41000>>2)-(0x00000>>2)-1];
    int  PXC_PWE;				/*  0x41000	*/
    int	 PXC_PRE;				/*  0x41004 	*/
    int	 PXC_AROP;				/*  0x41008 	*/
    int  PXC_DATAPATH;				/*  0x4100c 	*/
    int  PXE_COMBMODE;				/*  0x41010 	*/
    int  PXC_GAMMA;				/*  0x41014 	*/
    int  PXC_DITHER;				/*  0x41018 	*/
    int  PXC_BPP;				/*  0x4101c 	*/
    int  PXC_DISPMODE;				/*  0x41020 	*/
    int  PXC_ZCOMP;				/*  0x41024 	*/
    int  x11[(0x41030>>2)-(0x41024>>2)-1];
    int  PXC_SRCCOMP;				/*  0x41030 	*/
    int  PXC_DESTCOMP;				/*  0x41034 	*/
    int  x12[(0x41080>>2)-(0x41034>>2)-1];
    int  PXC_RR0;				/*  0x41080	*/
    int  PXC_RR1;				/*  0x41084	*/
    int  PXC_RR2;				/*  0x41088	*/
    int  PXC_RR3;				/*  0x4108c	*/
    int  PXC_RR4;				/*  0x41090	*/
    int  PXC_RR5;				/*  0x41094	*/
    int  PXC_RR6;				/*  0x41098	*/
    int  PXC_RR7;				/*  0x4109c	*/
    int	 PXC_BROP;				/*  0x410a0 	*/
    int  x13[(0x410fe>>2)-(0x410a0>>2)-1];
    int  PXC_NOP;				/*  0x410fe     */

/* framebuffer controller */
    int  x20[(0x44000>>2)-(0x410fe>>2)-1]; 
    int  FBCStatus;				/*  0x44000 	*/
    int	 PipePlugRequest;			/*  0x44004	*/
    int  x21[(0x4401c>>2)-(0x44004>>2)-1];
    int	 FrameBufferAccess;			/*  0x4401c	*/
    int  x22[(0x44088>>2)-(0x4401c>>2)-1];
    int  BlockMoveStart;		      	/*  0x44088	*/
    int  BROPConfig;				/*  0x4408c	*/
    int	 TMConfig;				/*  0x44090	*/
    int  x23[(0x44098>>2)-(0x44090>>2)-1];
    int	 ZConfig;				/*  0x44098	*/
    int	 WindowID;				/*  0x4409c	*/

/* framebuffer controller (register set #0) */
    int  x30[(0x44200>>2)-(0x4409c>>2)-1]; 
    int  WindowOffsetX;				/*  0x44200	*/
    int  WindowOffsetY;				/*  0x44204	*/
    int  x31[(0x44220>>2)-(0x44204>>2)-1]; 
    int  SourceX;				/*  0x44220	*/
    int  SourceY;				/*  0x44224	*/
    int  DestX;					/*  0x44228	*/
    int  DestY;					/*  0x4422c	*/
    int  Width;					/*  0x44230	*/
    int  Height;				/*  0x44234	*/
    int  x32[(0x44240>>2)-(0x44234>>2)-1]; 
    int  TConfig;				/*  0x44240	*/
    int  x33[(0x442c0>>2)-(0x44240>>2)-1]; 
    int  GbusWidth;	    		 	/*  0x442c0	*/

/* display board, overlay colormaps */
    int  x40[(0x83c00>>2)-(0x442c0>>2)-1]; 
    struct gen_rgb oprimary[15][16]; 		/*  0x83c00	*/
    struct gen_rgb oiprimary[8]; 		/*  0x83fc0	*/
    int  x41[(0x8bc00>>2)-(0x83fc0>>2)-(sizeof(struct gen_rgb)/sizeof(int))*8];
    struct gen_rgb osecondary[15][16];		/*  0x8bc00	*/
    struct gen_rgb oisecondary[8];		/*  0x8bfc0	*/
    int  x42[(0x94000>>2)-(0x8bfc0>>2)-(sizeof(struct gen_rgb)/sizeof(int))*8];

/* display board window register sets (0 - 15) */
    struct disp_reg_set window_regs[16];	/*  0x94000	*/
    int  x43[(0x94404>>2)-(0x94000>>2)-
	     (sizeof(struct disp_reg_set)/sizeof(int))*16];
    int  DISPEN;				/*  0x94404	*/
};

#endif /* __ITE_GEN_H_INCLUDED */
