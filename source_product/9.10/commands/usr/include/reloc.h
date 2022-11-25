/* @(#) $Revision: 66.1 $ */
/*
 *  HP 9000 Series 800
 *  Object File Format Definitions
 *
 *  Copyright Hewlett-Packard Co. 1985
 *
 */
#ifndef _RELOC_INCLUDED /* allow multiple inclusions */
#define _RELOC_INCLUDED

#ifdef __hp9000s800

/*
 * The following declarations are for relocation entries in new-format
 * relocatable object files, produced by compilers on HP-UX Release 3.0
 * and later.  A relocatable object file with this format will have
 * a version number in the file header of NEW_VERSION_ID (see filehdr.h).
 *
 * Relocation entries are a stream of bytes; each subspace (see scnhdr.h)
 * contains a byte offset from the beginning of the fixup table, and a
 * length in bytes of the relocation entries for that subspace.
 * The first byte of each relocation entry is the opcode as described
 * below.  Entries can be from 1 to 12 (currently) bytes long, and
 * describe 0, 1, or more words of data to be relocated.
 */

#define R_NO_RELOCATION   0x00	/* 00-1f:  n words, not relocatable */
#define R_ZEROES          0x20	/* 20-21:  n words, all zero */
#define R_UNINIT          0x22	/* 22-23:  n words, uninitialized */
#define R_RELOCATION      0x24	/* 24:     1 word, relocatable data */
#define R_DATA_ONE_SYMBOL 0x25	/* 25-26:  1 word, data external reference */
#define R_DATA_PLABEL     0x27	/* 27-28:  1 word, data plabel reference */
#define R_SPACE_REF       0x29	/* 29:     1 word, initialized space id */
#define R_REPEATED_INIT   0x2a  /* 2a-2d:  n words, repeated pattern */
				/* 2e-2f:  reserved */
#define R_PCREL_CALL      0x30	/* 30-3d:  1 word, pc-relative call */
				/* 3e-3f:  reserved */
#define R_ABS_CALL        0x40	/* 40-4d:  1 word, absolute call */
				/* 4e-4f:  reserved */
#define R_DP_RELATIVE     0x50	/* 50-72:  1 word, dp-relative load/store */
				/* 73-77:  reserved */
#define R_DLT_REL         0x78	/* 78-79:  1 word, dlt-relative load/store */
#define R_CODE_ONE_SYMBOL 0x80  /* 80-a2:  1 word, relocatable code */
				/* a3-ad:  reserved */
#define R_MILLI_REL       0xae  /* ae-af:  1 word, millicode-relative branch */
#define R_CODE_PLABEL     0xb0  /* b0-b1:  1 word, code plabel reference */
#define R_BREAKPOINT      0xb2	/* b2:     1 word, statement breakpoint */
#define R_ENTRY           0xb3	/* b3-b4:  procedure entry */
#define R_ALT_ENTRY       0xb5	/* b5:     alternate exit */
#define R_EXIT            0xb6	/* b6:     procedure exit */
#define R_BEGIN_TRY       0xb7	/* b7:     start of try block */
#define R_END_TRY         0xb8	/* b8-ba:  end of try block */
#define R_BEGIN_BRTAB     0xbb	/* bb:     start of branch table */
#define R_END_BRTAB       0xbc	/* bc:     end of branch table */
#define R_STATEMENT       0xbd	/* bd-bf:  statement number */
#define R_DATA_EXPR       0xc0	/* c0:     1 word, relocatable data expr */
#define R_CODE_EXPR       0xc1	/* c1:     1 word, relocatable code expr */
#define R_FSEL            0xc2	/* c2:     F' override */
#define R_LSEL            0xc3	/* c3:     L'/LD'/LS'/LR' override */
#define R_RSEL            0xc4	/* c4:     R'/RD'/RS'/RR' override */
#define R_N_MODE          0xc5	/* c5:     set L'/R' mode */
#define R_S_MODE          0xc6	/* c6:     set LS'/RS' mode */
#define R_D_MODE          0xc7	/* c7:     set LD'/RD' mode */
#define R_R_MODE          0xc8	/* c8:     set LR'/RR' mode */
#define R_DATA_OVERRIDE   0xc9  /* c9-cd:  get data from fixup area */
#define R_TRANSLATED      0xce  /* ce:     toggle translated mode */
#define R_AUX_UNWIND      0xcf  /* cf:     auxiliary unwind (proc begin) */
#define R_COMP1           0xd0	/* d0:     arbitrary expression */

#define     R_PUSH_PCON1      0x00      /* 00-3f:  positive constant */
#define     R_PUSH_DOT        0x40
#define     R_MAX             0x41
#define     R_MIN             0x42
#define     R_ADD             0x43
#define     R_SUB             0x44
#define     R_MULT            0x45
#define     R_DIV             0x46
#define     R_MOD             0x47
#define     R_AND             0x48
#define     R_OR              0x49
#define     R_XOR             0x4a
#define     R_NOT             0x4b
                                        /* 4c-5f:  reserved */
#define     R_LSHIFT          0x60      /* 60-7f:  shift count (0-variable) */
#define     R_ARITH_RSHIFT    0x80      /* 80-9f:  shift count (0-variable) */
#define     R_LOGIC_RSHIFT    0xa0      /* a0-bf:  shift count (0-variable) */
#define     R_PUSH_NCON1      0xc0      /* c0-ff:  negative constant */

#define R_COMP2           0xd1	/* d1:     arbitrary expression */
#define     R_PUSH_PCON2      0x00      /* 00-7f:  positive constant */
#define     R_PUSH_SYM        0x80
                                        /* 81:     reserved */
#define     R_PUSH_PLABEL     0x82      /* 82-83:  plabel */
                                        /* 84-bf:  reserved */
#define     R_PUSH_NCON2      0xc0      /* c0-ff:  negative constant */

#define R_COMP3           0xd2	/* d2:     arbitrary expression */
#define     R_PUSH_PROC       0x00      /* 00-01:  procedure */
#define     R_PUSH_CONST      0x02      /* 02:     constant */
                                        /* 03-ff:  reserved */

#define R_PREV_FIXUP      0xd3	/* d3-d6:  apply previous fixup again */
#define R_SEC_STMT        0xd7	/* d7:     secondary statement number */
				/* d8-df:  reserved */
#define R_RESERVED        0xe0	/* e0-ff:  reserved for compiler/linker */


/*
 * Loader fixups
 *
 * These are produced by the linker only when the -HK option is used.
 * They indicate initialized pointers in the data space that contain
 * the addresses of other locations in the data space, so that those
 * pointers can be relocated at load time if the data is loaded at
 * some address other than that specified at link time.  (References
 * from code to data are always relative to the dp register (r27),
 * and will not need to be relocated.)  These are needed primarily
 * for the MPE XL operating system, but may be useful for some
 * dynamic loading applications on HP-UX.
 */

struct loader_fixup {
        unsigned int fixup_type;
        unsigned int space_index;   /* index of space to fix up         */
        unsigned int space_offset;  /* offset at which to patch         */
        int 	     constant;	    /* constant used to patch the space */
};

/* loader fixup types */

#define LD_SPACE_REF 0  /* space reference fixup */
#define LD_DATA_REF  1  /* data reference fixup  */
#define LD_XRT_REF   2  /* XRT reference fixup  */

#define LDRFIX struct loader_fixup
#define LDRFIXSZ sizeof(LDRFIX)


/*
 * The following definitions are for old-format relocatable object
 * files, and for absolute fixups (e_abs) in all executable files.
 * Old relocatable object files were produced by compilers before
 * HP-UX Release 3.0 on the Series 800, and the file header contains
 * the version number VERSION_ID (see filehdr.h).
 *
 * Absolute fixups are produced by the linker only when the -HF
 * option is specified.  They record the locations of all absolute
 * branches (LDIL/BLE) in the code, so that code can be relocated
 * without being completely relinked.  They are primarily useful
 * only on MPE XL, but may be useful in dynamic loading applications
 * on HP-UX.
 */

struct fixup_request_record {
        unsigned int need_data_ref  : 1;
        unsigned int arg_reloc      : 10;
        unsigned int expression_type: 5;  /* type of expr. providing value    */
	unsigned int exec_level     : 2;  /* execution level at point of call */
        unsigned int fixup_format   : 6;  /* inst. or data format             */
        unsigned int fixup_field    : 8;  /* field from fixup value           */
        unsigned int subspace_offset;     /* offset of word to be fixed up    */
        unsigned int symbol_index_one;    /* sequence # of symbol             */
        unsigned int symbol_index_two;    /* sequence # of second symbol      */
        int          fixup_constant;      /* constant part           	      */
};

/* field selectors */

#define e_fsel  0    /* F'  : no change           	     */
#define e_lssel 1    /* LS' : if (bit 21) then add 0x800
                              arithmetic shift right 11 bits */
#define e_rssel 2    /* RS' : Sign extend from bit 21        */
#define e_lsel  3    /* L'  : Arithmetic shift right 11 bits */
#define e_rsel  4    /* R'  : Set bits 0-20 to zero          */
#define e_ldsel 5    /* LD' : Add 0x800, arithmetic shift
                              right 11 bits          	     */
#define e_rdsel 6    /* RD' : Set bits 0-20 to one           */
#define e_lrsel 7    /* LR' : L' with "rounded" constant     */
#define e_rrsel 8    /* RR' : R' with "rounded" constant     */

/* expression types */

#define e_one         0      /* label + constant                   */
#define e_two         1      /* label - label + constant           */
#define e_pcrel       2      /* label - subspace offset + constant */
#define e_con         3      /* constant                           */
#define e_plabel      7      /* plabel for label                   */
#define e_abs	     18      /* absolute, 1st sym index is address */

/* fixup formats */

#define i_exp14 0      /* 14 bit signed long displacement     */
#define i_exp21 1      /* 21 bit signed immediate             */
#define i_exp11 2      /* signed 11 bit immediate             */
#define i_rel17 3      /* 19 bit signed displacement, discard */
                       /* 2 low order bits to make 17 bits    */
#define i_rel12 4      /* 14 bit signed displacement, discard */
                       /* 2 low order bits to make 12 bits    */
#define i_data  5      /* whole word                          */
#define i_none  6      /* no expression in this word          */
#define i_abs17 7      /* same as i_rel17                     */
#define i_milli 8      /* i_abs17 plus SR bits                */
#define i_break 9      /* conditionally replace NOP w/ BREAK  */

#define RELOC struct fixup_request_record
#define RELSZ sizeof(RELOC)

#endif /* __hp9000s800 */
#endif /* _RELOC_INCLUDED */
