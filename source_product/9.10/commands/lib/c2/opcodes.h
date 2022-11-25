/* @(#) $Revision: 70.1 $ */     

/* 68020 opcodes */

#define ADD	2
#define ADDA	3
#define ADDQ	4
#define AND	5
#define ASL	6
#define ASR	7
#define BCC	8
#define BCHG	9
#define BCLR	10
#define BCS	11
#define BEQ	12
#define BFCHG	13	/* order of bit fld instructions is impt */
#define BFCLR	14
#define BFEXTS	15
#define BFEXTU	16
#define BFFFO	17
#define BFINS	18
#define BFSET	19
#define BFTST	20
#define BGE	21
#define BGT	22
#define BHI	23
#define BLE	24
#define BLS	25
#define BLT	26
#define BMI	27
#define BNE	28
#define BPL	29
#define BRA	30
#define BSET	31
#define BSR	32
#define BTST	33
#define BVC	34
#define BVS	35
#define CLR	36
#define CMP	37
#define DBCC	38
#define DBCS	39
#define DBEQ	40
#define DBF	41
#define DBGE	42
#define DBGT	43
#define DBHI	44
#define DBLE	45
#define DBLS	46
#define DBLT	47
#define DBMI	48
#define DBNE	49
#define DBPL	50
#define DBRA	51
#define DBT	52
#define DBVC	53
#define DBVS	54
#define DIVS	55
#define DIVSL	56
#define DIVU	57
#define DIVUL	58
#define EOR	59
#define EXG	60
#define EXT	61
#define EXTB	62
#define JMP	63
#define JSR	64
#define LEA	65
#define LINK	66
#define LSL	67
#define LSR	68
#define MOVE	69
#define MOVEM	70
#define MOVEQ	71
#define MULS	72
#define MULU	73
#define NEG	74
#define NOT	75
#define OR	76
#define PEA	77
#define ROL	78
#define ROR	79
#define RTS	80
#define SCC	81
#define SCS	82
#define SEQ	83
#define SF	84
#define SGE	85
#define SGT	86
#define SHI	87
#define SLE	88
#define SLS	89
#define SLT	90
#define SMI	91
#define SNE	92
#define SPL	93
#define ST	94
#define SUB	95
#define SUBA	96
#define SUBQ	97
#define SVC	98
#define SVS	99
#define SWAP	100
#define TRAP	101
#define TST	102
#define UNLK	103

/* Pseudo-ops and others */

#define ASM	104
#define ASCIZ	105
#define BSS	106
#define CBR	107
#define COMM	108
#define COMMENT	109
#define DATA	110
#define DC	111
#define DC_B	112
#define DC_W	113
#define DC_L	114
#define DLABEL	115
#define DS	116
#define END	117
#define GLOBAL	118
#define JSW	119
#define LABEL	120
#define LALIGN	121
#define LCOMM	122
#define SET	123
#define TEXT	124
#define VERSION	125
#define SGLOBAL	126
#define SHLIB_VERSION	127

/* 68881 opcodes */

#define	FLOW	130	/* lower-bound for 881 opcodes */
#define FABS	130
#define FACOS	131
#define FADD	132
#define FASIN	133
#define FATAN	134
#define FBEQ	135
#define FBGE	136
#define FBGL	137
#define FBGLE	138
#define FBGT	139
#define FBLE	140
#define FBLT	141
#define FBNEQ	142
#define FBNGE	143
#define FBNGL	144
#define FBNGLE	145
#define FBNGT	146
#define FBNLE	147
#define FBNLT	148
#define FCMP	149
#define FCOS	150
#define FCOSH	151
#define FDIV	152
#define FETOX	153
#define FINTRZ	154
#define FLOG10	155
#define FLOGN	156
#define FMOD	157
#define FMOV	158
#define FMOVM	159
#define FMUL	160
#define FNEG	161
#define FSGLMUL	162
#define FSGLDIV	163
#define FSIN	164
#define FSINH	165
#define FSQRT	166
#define FSUB	167
#define FTAN	168
#define FTANH	169
#define FTEST	170
#define	FHIGH	170	/* upper-bound for 881 opcodes */

#ifdef DRAGON
/* Dragon opcodes */

#define	FPLOW	171	/* lower-bound for dragon opcodes */
#define FPABS	171
#define FPADD	172
#define FPBEQ	173
#define FPBGE	174
#define FPBGL	175
#define FPBGLE	176
#define FPBGT	177
#define FPBLE	178
#define FPBLT	179
#define FPBNE	180
#define FPBNGE	181
#define FPBNGL	182
#define FPBNGLE	183
#define FPBNGT	184
#define FPBNLE	185
#define FPBNLT	186
#define FPCMP	187
#define FPCVD	188
#define FPCVL	189
#define FPCVS	190
#define FPDIV	191
#define FPINTRZ	192
#define FPMOV	193
#define FPMUL	194
#define FPNEG	195
#define FPSUB	196
#define FPTEST	197
#define FPMABS	198
#define FPMADD	199
#define FPMCMP	200
#define FPMCVD	201
#define FPMCVL	202
#define FPMCVS	203
#define FPMDIV	204
#define FPMINTRZ	205
#define FPMMOV	206
#define FPMMUL	207
#define FPMNEG	208
#define FPMSUB	209
#define FPMTEST	210
#define FPM2ADD	211
#define FPM2CMP	212
#define FPM2DIV	213
#define FPM2MUL	214
#define FPM2SUB	215
#define	FPHIGH	215	/* upper-bound for dragon opcodes */
#endif DRAGON
