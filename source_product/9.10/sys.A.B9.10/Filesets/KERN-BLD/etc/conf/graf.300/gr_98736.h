/*
 * @(#)gr_98736.h: $Revision: 1.3.84.4 $ $Date: 93/12/08 12:07:28 $
 * $Locker:  $
 */

#define gen_work_buf_0_ptr   g_union.genesis_info.work_buffer_0_ptr
#define gen_work_buf_1_ptr   g_union.genesis_info.work_buffer_1_ptr
#define gen_work_buf_0_uvptr g_union.genesis_info.work_buffer_0_uvptr
#define gen_work_buf_1_uvptr g_union.genesis_info.work_buffer_1_uvptr
#define gen_root_ptr         g_union.genesis_info.root_ptr
#define gen_segment_table    g_union.genesis_info.segment_table
#define gen_spt_block        g_union.genesis_info.spt_block

#define GEN_SSTE_PTPTR 0xfffff000
#define GEN_SSTE_VALID 0x00000001 /* Segment table entry is valid */

#define GEN_SPTE_PFRAME 0xfffff000
#define GEN_SPTE_WRITE  0x00000020 /* Page frame write enable */
#define GEN_SPTE_REF    0x00000004 /* Page frame reference */
#define GEN_SPTE_DIRTY  0x00000002 /* Page frame dirty */
#define GEN_SPTE_VALID  0x00000001 /* Page table entry is valid */
/* Macros for determining interface type */

#define is_genesis(x) ((x)->desc.crt_id == S9000_ID_98736 ? 1 : 0 )


#define is_blackfoot(x) ((x)->desc.crt_id == S9000_ID_98736 && \
			 (x)->desc.crt_attributes & CRT_VDMA_HARDWARE ? 1 : 0 )

/* Genesis Address Space Offsets */

#define GEN_PRIV_PAGE     0x3E000  /* Beginning of priv. page (1 Page)     */
#define GEN_NON_PRIV_PAGE 0x3F000  /* Beginning of non priv. page (1 page) */
#define GEN_GBUS_CONTROL  0x40000  /* Beginning of GBUS Space              */

/* Genesis Register Offsets */

#define GEN_INT        0x00003 /* Blackfoot Only */
#define GEN_CMD        0x3E000 /* Blackfoot Only */
#define GEN_DATA1      0x3E004 /* Blackfoot Only */
#define GEN_DATA2      0x3E008 /* Blackfoot Only */
#define GEN_FAULT_ADDR 0x3E00C /* Blackfoot Only */
#define GEN_FAULT_CNT  0x3E01C /* Blackfoot Only */
#define GEN_ROOT_PTR   0x3E010 /* Blackfoot Only */
#define GEN_TLB_V1     0x3E044 /* Blackfoot Only */
#define GEN_IOCR       0x3E810 /* Blackfoot Only */
#define GEN_WBUF0      0x3E804
#define GEN_WBUF1      0x3E808
#define GEN_BUSCTL     0x3F800
#define GEN_IOSTAT     0x3F810 /* Blackfoot Only */

/* Bits in the Bus Control Register */

#define BC_START        0x80000000

/* Bits in the IO Status Register (Blackfoot Only) */

#define IOST_INTPND     0x80000000
#define IOST_CMDRDY     0x08000000
#define IOST_DMABUSY    0x04000000
#define IOST_DMASUSP    0x02000000
#define IOST_DMA_R_W    0x01000000
#define IOST_DMA_PG_FLT 0x00800000
#define IOST_DMA_AC_FLT 0x00400000
#define IOST_DMA_INT    0x00200000
#define IOST_DMA_STATUS 0x001e0000

/* Bits in the IO Control Register (Blackfoot Only) */

#define IOCR_RESET_DMA  0x20000000

/* Bits in Interrupt Register (Blackfoot Only) */

#define VDMA_INT_ENABLE  0x80
#define VDMA_INT_REQUEST 0x40

#define VDMA_INT_LEVEL  6 /* Interrupt Level to use for VDMA */
#define VDMA_INT_OFFSET 3 /* Interrupt Level is offset by this in register */

#define VDMA_MAX_ISR_FAULT 4 /* Max # of pages we will pre-page under interrupt */
#define VDMA_FAULT_THRESH 16 /* Schedule vdmad if fault count >= this value     */


/* Blackfoot DMA Processor Commands */

#define GEN_SUSPEND_DMA          0x00260000
#define GEN_RESUME_DMA           0x00280000
#define GEN_RESTART_FAULT        0x00410000
#define GEN_UPDATE_ROOT_POINTER  0x00420000
#define GEN_PURGE_TLB_ENTRY      0x00440000
