/*
 * @(#)gr_hyper.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 16:54:27 $
 * $Locker:  $
 */

/*
 * Definitions for monochrome Hyperion/Frodo.
 *
 * The board is configurable in the following DIO-II address spaces:
 *
 * address space #     CSR/PROM space	    display memory space
 *	1	    100 0000 - 100 ffff	    120 0000 - 123 ffff
 *	2	    140 0000 - 140 ffff	    160 0000 - 163 ffff
 *	3	    180 0000 - 180 ffff	    1A0 0000 - 1A3 ffff
 *	4	    1C0 0000 - 1C0 ffff	    1E0 0000 - 1E3 ffff
 *
 * Within the CSR/PROM address space, the CSR space is in the range
 * 0x4000 to 0x5fff from the base of CSR/PROM space.
 */

/*
 * CSR/PROM address space structure for mono Hyperion.
 */
struct hyper_ctlspace {
    char foo[0x4000];
    char control;		    /* control register (1) */
    char bar[7];
    char status;		    /* status  register (2) */
};

/*
 * Hyperion control register bit definitions.
 */
#define	HYPERION_CR_MTRSEL_MASK	0x03	    /* monitor select -- 2-bit field */
#   define HYPERION_VIDEO_RESET	    0x00
#   define HYPERION_19_IN	    0x01
#   define HYPERION_15_IN	    0x02
#   define HYPERION_DIAGCLK_ENB	    0x03
#define	HYPERION_CR_VIDENB	0x04		  /* video enable (1=enable) */
#define	HYPERION_CR_INVVID	0x08			    /* inverse video */
#define HYPERION_CR_UNUSED1	0x10				   /* unused */
#define HYPERION_CR_UNUSED2	0x20				   /* unused */
#define HYPERION_CR_DIAGCLOCK	0x40			 /* diagnostic clock */
#define HYPERION_CR_UNUSED3	0x80				   /* unused */

/*
 * Frodo status register bit definitions.
 */
#define HYPERION_SR_MTRSNS_MASK	0x03	     /* monitor sense -- 2-bit field */
#   define HYPERION_15_ATTCHD	    0x00
#   define HYPERION_19_ATTCHD	    0x01
#   define HYPERION_NO_ATTCHD	    0x03
#define HYPERION_SR_VSYNC	0x04			    /* vertical sync */
#define HYPERION_SR_HSYNC	0x08			  /* horizontal sync */
#define HYPERION_SR_VBLANK	0x10			   /* vertical blank */
#define HYPERION_SR_HBLANK	0x20			 /* horizontal blank */
#define HYPERION_SR_DIAGDATA	0x40		      /* diagnostic data bit */
#define HYPERION_SR_UNUSED1	0x80				   /* unused */
