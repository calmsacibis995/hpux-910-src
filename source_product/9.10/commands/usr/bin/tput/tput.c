#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 70.5 $";
#endif

/* tput - print terminal attribute
 *
 * return codes:
 *  2 usage error
 *  3 bad terminal type given
 *  4 unknown capname
 *  5 error reading from stdin
 *  0 ok (if boolean capname -> TRUE)
 *  1 (for boolean capname  -> FALSE)
 *
 * tput printfs (a value) if an INT capname was given (e.g. cols),
 * putp's (a string) if a STRING capname was given (e.g. clear), and
 * for BOOLEAN capnames (e.g. hard-copy) just returns the boolean value.
 */

#include <curses.h>	/* includes <signal.h> <stdio.h> <unctrl.h> <sgtty.h> */
#include <term.h>	/* needed for curses capability */
#include <ctype.h>	/* function prototype for isalpha() */
#include <locale.h>	/* function prototype for setlocale() */
#include <nl_types.h>	/* function prototype for catopen(), catgets() */
#include <stdlib.h>	/* function prototype for getenv(), atoi() */
#include <string.h>	/* function prototype for strcmp() */

/* The sequencing of the #defines is important.  These must in in alphabetical
 * order and the numbers must increment contiguously starting from 0 since
 * these #defines are indices into a sorted array.  If any additional #defines
 * are added, all #defines must be resequenced.
 */
#define AM	0
#define BEL	1
#define BLINK	2
#define BOLD	3
#define BW	4
#define CBT	5
#define CIVIS	6
#define CLEAR	7
#define CMDCH	8
#define CNORM	9
#define COLZ	10	/* COLS -> COLZ becaues of a conflict in <curses.h> */
#define CR	11
#define CSR	12
#define CUB	13
#define CUB1	14
#define CUD	15
#define CUD1	16
#define CUF	17
#define CUF1	18
#define CUP	19
#define CUU	20
#define CUU1	21
#define CVVIS	22
#define DA	23
#define DB	24
#define DCH	25
#define DCH1	26
#define DIM	27
#define DL	28
#define DL1	29
#define DSL	30
#define ECH	31
#define ED	32
#define EL	33
#define EO	34
#define ESLOK	35
#define FF	36
#define FLASH	37
#define FSL	38
#define GN	39
#define HC	40
#define HD	41
#define HOME	42
#define HPA	43
#define HS	44
#define HT	45
#define HTS	46
#define HU	47
#define HZ	48
#define ICH	49
#define ICH1	50
#define IF	51
#define IL	52
#define IL1	53
#define IN	54
#define IND	55
#define INDN	56
#define INIT	57
#define INVIS	58
#define IP	59
#define IPROG	60
#define IS1	61
#define IS2	62
#define IS3	63
#define IT	64
#define KA1	65
#define KA3	66
#define KB2	67
#define KBS	68
#define KC1	69
#define KC3	70
#define KCLR	71
#define KCTAB	72
#define KCUB1	73
#define KCUD1	74
#define KCUF1	75
#define KCUU1	76
#define KDCH1	77
#define KDL1	78
#define KED	79
#define KEL	80
#define KF0	81
#define KF1	82
#define KF10	83
#define KF11	84
#define KF12	85
#define KF13	86
#define KF14	87
#define KF15	88
#define KF16	89
#define KF17	90
#define KF18	91
#define KF19	92
#define KF2	93
#define KF20	94
#define KF21	95
#define KF22	96
#define KF23	97
#define KF24	98
#define KF25	99
#define KF26	100
#define KF27	101
#define KF28	102
#define KF29	103
#define KF3	104
#define KF30	105
#define KF31	106
#define KF32	107
#define KF33	108
#define KF34	109
#define KF35	110
#define KF36	111
#define KF37	112
#define KF38	113
#define KF39	114
#define KF4	115
#define KF40	116
#define KF41	117
#define KF42	118
#define KF43	119
#define KF44	120
#define KF45	121
#define KF46	122
#define KF47	123
#define KF48	124
#define KF49	125
#define KF5	126
#define KF50	127
#define KF51	128
#define KF52	129
#define KF53	130
#define KF54	131
#define KF55	132
#define KF56	133
#define KF57	134
#define KF58	135
#define KF59	136
#define KF6	137
#define KF60	138
#define KF61	139
#define KF62	140
#define KF63	141
#define KF7	142
#define KF8	143
#define KF9	144
#define KHOME	145
#define KHTS	146
#define KICH1	147
#define KIL1	148
#define KIND	149
#define KLL	150
#define KM	151
#define KNP	152
#define KPP	153
#define KRI	154
#define KRMIR	155
#define KTBC	156
#define LF0	157
#define LF1	158
#define LF10	159
#define LF2	160
#define LF3	161
#define LF4	162
#define LF5	163
#define LF6	164
#define LF7	165
#define LF8	166
#define LF9	167
#define LH	168
#define LINEZ	169	/* LINES -> LINEZ becaues of a conflict in <curses.h> */
#define LL	170
#define LM	171
#define LW	172
#define MC0	173
#define MC4	174
#define MC5	175
#define MC5P	176
#define MEML	177
#define MEMU	178
#define MIR	179
#define MRCUP	180
#define MSGR	181
#define NEL	182
#define NLAB	183
#define OS	184
#define PAD	185
#define PB	186
#define PFKEY	187
#define PFLOC	188
#define PFX	189
#define PLN	190
#define PROT	191
#define RC	192
#define REP	193
#define RESET	194
#define REV	195
#define RF	196
#define RI	197
#define RIN	198
#define RMACS	199
#define RMCUP	200
#define RMDC	201
#define RMIR	202
#define RMKX	203
#define RMLN	204
#define RMM	205
#define RMSO	206
#define RMUL	207
#define RS1	208
#define RS2	209
#define RS3	210
#define SC	211
#define SGR	212
#define SGR0	213
#define SMACS	214
#define SMCUP	215
#define SMDC	216
#define SMIR	217
#define SMKX	218
#define SMLN	219
#define SMM	220
#define SMSO	221
#define SMUL	222
#define TBC	223
#define TSL	224
#define UC	225
#define UL	226
#define VPA	227
#define VT	228
#define WIND	229
#define WSL	230
#define XENL	231
#define XHP	232
#define XMC	233
#define XON	234
#define XSB	235
#define XT	236

#define NCOLS	237	/* number of elements in the array */

struct node {		/* artifact to get bsearch to work correctly */
	char *cp;
};

/* this list must stay sorted, otherwise bsearch() may fail to find
 * the corresponding capname
 */
struct node table[NCOLS] = {
"am",
"bel",
"blink",
"bold",
"bw",
"cbt",
"civis",
"clear",
"cmdch",
"cnorm",
"cols",
"cr",
"csr",
"cub",
"cub1",
"cud",
"cud1",
"cuf",
"cuf1",
"cup",
"cuu",
"cuu1",
"cvvis",
"da",
"db",
"dch",
"dch1",
"dim",
"dl",
"dl1",
"dsl",
"ech",
"ed",
"el",
"eo",
"eslok",
"ff",
"flash",
"fsl",
"gn",
"hc",
"hd",
"home",
"hpa",
"hs",
"ht",
"hts",
"hu",
"hz",
"ich",
"ich1",
"if",
"il",
"il1",
"in",
"ind",
"indn",
"init",
"invis",
"ip",
"iprog",
"is1",
"is2",
"is3",
"it",
"ka1",
"ka3",
"kb2",
"kbs",
"kc1",
"kc3",
"kclr",
"kctab",
"kcub1",
"kcud1",
"kcuf1",
"kcuu1",
"kdch1",
"kdl1",
"ked",
"kel",
"kf0",
"kf1",
"kf10",
"kf11",
"kf12",
"kf13",
"kf14",
"kf15",
"kf16",
"kf17",
"kf18",
"kf19",
"kf2",
"kf20",
"kf21",
"kf22",
"kf23",
"kf24",
"kf25",
"kf26",
"kf27",
"kf28",
"kf29",
"kf3",
"kf30",
"kf31",
"kf32",
"kf33",
"kf34",
"kf35",
"kf36",
"kf37",
"kf38",
"kf39",
"kf4",
"kf40",
"kf41",
"kf42",
"kf43",
"kf44",
"kf45",
"kf46",
"kf47",
"kf48",
"kf49",
"kf5",
"kf50",
"kf51",
"kf52",
"kf53",
"kf54",
"kf55",
"kf56",
"kf57",
"kf58",
"kf59",
"kf6",
"kf60",
"kf61",
"kf62",
"kf63",
"kf7",
"kf8",
"kf9",
"khome",
"khts",
"kich1",
"kil1",
"kind",
"kll",
"km",
"knp",
"kpp",
"kri",
"krmir",
"ktbc",
"lf0",
"lf1",
"lf10",
"lf2",
"lf3",
"lf4",
"lf5",
"lf6",
"lf7",
"lf8",
"lf9",
"lh",
"lines",
"ll",
"lm",
"lw",
"mc0",
"mc4",
"mc5",
"mc5p",
"meml",
"memu",
"mir",
"mrcup",
"msgr",
"nel",
"nlab",
"os",
"pad",
"pb",
"pfkey",
"pfloc",
"pfx",
"pln",
"prot",
"rc",
"rep",
"reset",
"rev",
"rf",
"ri",
"rin",
"rmacs",
"rmcup",
"rmdc",
"rmir",
"rmkx",
"rmln",
"rmm",
"rmso",
"rmul",
"rs1",
"rs2",
"rs3",
"sc",
"sgr",
"sgr0",
"smacs",
"smcup",
"smdc",
"smir",
"smkx",
"smln",
"smm",
"smso",
"smul",
"tbc",
"tsl",
"uc",
"ul",
"vpa",
"vt",
"wind",
"wsl",
"xenl",
"xhp",
"xmc",
"xon",
"xsb",
"xt",
};

#define NUMBER	2	/* indicates a INT capname was given */
#define STRING	3	/* indicates a STRING capname was given */

/* Global defs */
#define NL_SETN 1
nl_catd catd;


int
nodecmp(struct node *s, struct node *t)
{
	return strcmp(s->cp,t->cp);
}


/* capindex "returns" either an INT, STRING or BOOLEAN return value
 * depending on the index.
 *
 * If an INT capname was specified, *number is assigned the corresponding
 * terminfo value and capname returns an INT return value.
 *
 * If a STRING capname was specified, *string is assigned to point to the
 * corresponding terminfo string and capname specifies a STRING return value.
 *
 * If a BOOLEAN capname was specified, capname returns the boolean value.
 */
int
capindex (int index, int *number, char **string)
{
	switch (index) {
	case AM:	return(1-auto_right_margin);
	case BEL:	{ *string = bell;  return(STRING); }
	case BLINK:	{ *string = enter_blink_mode;  return(STRING); }
	case BOLD:	{ *string = enter_bold_mode;   return(STRING); }
	case BW:	return(1-auto_left_margin);
	case CBT:	{ *string = back_tab; return(STRING); }
	case CIVIS:	{ *string = cursor_invisible;  return(STRING); }
	case CLEAR:	{ *string = clear_screen;   return(STRING); }
	case CMDCH:	{ *string = command_character; return(STRING); }
	case CNORM:	{ *string = cursor_normal;  return(STRING); }
	case COLZ:	{ *number = columns; return(NUMBER); }
	case CR:	{ *string = carriage_return;   return(STRING); }
	case CSR:	{ *string = change_scroll_region; return(STRING); }
	case CUB1:	{ *string = cursor_left; return(STRING); }
	case CUB:	{ *string = parm_left_cursor;  return(STRING); }
	case CUD1:	{ *string = cursor_down; return(STRING); }
	case CUD:	{ *string = parm_down_cursor;  return(STRING); }
	case CUF1:	{ *string = cursor_right;   return(STRING); }
	case CUF:	{ *string = parm_right_cursor; return(STRING); }
	case CUP:	{ *string = cursor_address; return(STRING); }
	case CUU1:	{ *string = cursor_up;   return(STRING); }
	case CUU:	{ *string = parm_up_cursor; return(STRING); }
	case CVVIS:	{ *string = cursor_visible; return(STRING); }
	case DA:	return(1-memory_above);
	case DB:	return(1-memory_below);
	case DCH1:	{ *string = delete_character;  return(STRING); }
	case DCH:	{ *string = parm_dch; return(STRING); }
	case DIM:	{ *string = enter_dim_mode; return(STRING); }
	case DL1:	{ *string = delete_line; return(STRING); }
	case DL:	{ *string = parm_delete_line;  return(STRING); }
	case DSL:	{ *string = dis_status_line;   return(STRING); }
	case ECH:	{ *string = erase_chars; return(STRING); }
	case ED:	{ *string = clr_eos;  return(STRING); }
	case EL:	{ *string = clr_eol;  return(STRING); }
	case EO:	return(1-erase_overstrike);
	case ESLOK:	return(1-status_line_esc_ok);
	case FF:	{ *string = form_feed;   return(STRING); }
	case FLASH:	{ *string = flash_screen;   return(STRING); }
	case FSL:	{ *string = from_status_line;  return(STRING); }
	case GN:	return(1-generic_type);
	case HC:	return(1-hard_copy);
	case HD:	{ *string = down_half_line; return(STRING); }
	case HOME:	{ *string = cursor_home; return(STRING); }
	case HPA:	{ *string = column_address; return(STRING); }
	case HS:	return(1-has_status_line);
	case HT:	{ *string = tab;   return(STRING); }
	case HTS:	{ *string = set_tab;  return(STRING); }
	case HU:	{ *string = up_half_line;   return(STRING); }
	case HZ:	return(1-tilde_glitch);
	case ICH1:	{ *string = insert_character;  return(STRING); }
	case ICH:	{ *string = parm_ich; return(STRING); }
	case IF:	{ *string = init_file;   return(STRING); }
	case IL1:	{ *string = insert_line; return(STRING); }
	case IL:	{ *string = parm_insert_line;  return(STRING); }
	case IN:	return(1-insert_null_glitch);
	case IND:	{ *string = scroll_forward; return(STRING); }
	case INDN:	{ *string = parm_index;  return(STRING); }
	case INIT:	{ (void)putp(init_1string);
			  (void)putp(init_2string);
			  (void)putp(init_3string);
			  return(0);
			}
	case INVIS:	{ *string = enter_secure_mode; return(STRING); }
	case IP:	{ *string = insert_padding; return(STRING); }
	case IPROG:	{ *string = init_prog;   return(STRING); }
	case IS1:	{ *string = init_1string;   return(STRING); }
	case IS2:	{ *string = init_2string;   return(STRING); }
	case IS3:	{ *string = init_3string;   return(STRING); }
	case IT:	{ *number = init_tabs; return(NUMBER); }
	case KA1:	{ *string = key_a1;   return(STRING); }
	case KA3:	{ *string = key_a3;   return(STRING); }
	case KB2:	{ *string = key_b2;   return(STRING); }
	case KBS:	{ *string = key_backspace;  return(STRING); }
	case KC1:	{ *string = key_c1;   return(STRING); }
	case KC3:	{ *string = key_c3;   return(STRING); }
	case KCLR:	{ *string = key_clear;   return(STRING); }
	case KCTAB:	{ *string = key_ctab; return(STRING); }
	case KCUB1:	{ *string = key_left; return(STRING); }
	case KCUD1:	{ *string = key_down; return(STRING); }
	case KCUF1:	{ *string = key_right;   return(STRING); }
	case KCUU1:	{ *string = key_up;   return(STRING); }
	case KDCH1:	{ *string = key_dc;   return(STRING); }
	case KDL1:	{ *string = key_dl;   return(STRING); }
	case KED:	{ *string = key_eos;  return(STRING); }
	case KEL:	{ *string = key_eol;  return(STRING); }
	case KF0:	{ *string = key_f0;   return(STRING); }
	case KF1:	{ *string = key_f1;   return(STRING); }
	case KF10:	{ *string = key_f10;  return(STRING); }
	case KF11:	{ *string = key_f11;   return(STRING); }
	case KF12:	{ *string = key_f12;   return(STRING); }
	case KF13:	{ *string = key_f13;   return(STRING); }
	case KF14:	{ *string = key_f14;   return(STRING); }
	case KF15:	{ *string = key_f15;   return(STRING); }
	case KF16:	{ *string = key_f16;   return(STRING); }
	case KF17:	{ *string = key_f17;   return(STRING); }
	case KF18:	{ *string = key_f18;   return(STRING); }
	case KF19:	{ *string = key_f19;   return(STRING); }
	case KF2:	{ *string = key_f2;   return(STRING); }
	case KF20:	{ *string = key_f20;   return(STRING); }
	case KF21:	{ *string = key_f21;   return(STRING); }
	case KF22:	{ *string = key_f22;   return(STRING); }
	case KF23:	{ *string = key_f23;   return(STRING); }
	case KF24:	{ *string = key_f24;   return(STRING); }
	case KF25:	{ *string = key_f25;   return(STRING); }
	case KF26:	{ *string = key_f26;   return(STRING); }
	case KF27:	{ *string = key_f27;   return(STRING); }
	case KF28:	{ *string = key_f28;   return(STRING); }
	case KF29:	{ *string = key_f29;   return(STRING); }
	case KF3:	{ *string = key_f3;   return(STRING); }
	case KF30:	{ *string = key_f30;   return(STRING); }
	case KF31:	{ *string = key_f31;   return(STRING); }
	case KF32:	{ *string = key_f32;   return(STRING); }
	case KF33:	{ *string = key_f33;   return(STRING); }
	case KF34:	{ *string = key_f34;   return(STRING); }
	case KF35:	{ *string = key_f35;   return(STRING); }
	case KF36:	{ *string = key_f36;   return(STRING); }
	case KF37:	{ *string = key_f37;   return(STRING); }
	case KF38:	{ *string = key_f38;   return(STRING); }
	case KF39:	{ *string = key_f39;   return(STRING); }
	case KF4:	{ *string = key_f4;   return(STRING); }
	case KF40:	{ *string = key_f40;   return(STRING); }
	case KF41:	{ *string = key_f41;   return(STRING); }
	case KF42:	{ *string = key_f42;   return(STRING); }
	case KF43:	{ *string = key_f43;   return(STRING); }
	case KF44:	{ *string = key_f44;   return(STRING); }
	case KF45:	{ *string = key_f45;   return(STRING); }
	case KF46:	{ *string = key_f46;   return(STRING); }
	case KF47:	{ *string = key_f47;   return(STRING); }
	case KF48:	{ *string = key_f48;   return(STRING); }
	case KF49:	{ *string = key_f49;   return(STRING); }
	case KF5:	{ *string = key_f5;   return(STRING); }
	case KF50:	{ *string = key_f50;   return(STRING); }
	case KF51:	{ *string = key_f51;   return(STRING); }
	case KF52:	{ *string = key_f52;   return(STRING); }
	case KF53:	{ *string = key_f53;   return(STRING); }
	case KF54:	{ *string = key_f54;   return(STRING); }
	case KF55:	{ *string = key_f55;   return(STRING); }
	case KF56:	{ *string = key_f56;   return(STRING); }
	case KF57:	{ *string = key_f57;   return(STRING); }
	case KF58:	{ *string = key_f58;   return(STRING); }
	case KF59:	{ *string = key_f59;   return(STRING); }
	case KF6:	{ *string = key_f6;   return(STRING); }
	case KF60:	{ *string = key_f60;   return(STRING); }
	case KF61:	{ *string = key_f61;   return(STRING); }
	case KF62:	{ *string = key_f62;   return(STRING); }
	case KF63:	{ *string = key_f63;   return(STRING); }
	case KF7:	{ *string = key_f7;   return(STRING); }
	case KF8:	{ *string = key_f8;   return(STRING); }
	case KF9:	{ *string = key_f9;   return(STRING); }
	case KHOME:	{ *string = key_home; return(STRING); }
	case KHTS:	{ *string = key_stab; return(STRING); }
	case KICH1:	{ *string = key_ic;   return(STRING); }
	case KIL1:	{ *string = key_il;   return(STRING); }
	case KIND:	{ *string = key_sf;   return(STRING); }
	case KLL:	{ *string = key_ll;   return(STRING); }
	case KM:	return(1-has_meta_key);
	case KNP:	{ *string = key_npage;   return(STRING); }
	case KPP:	{ *string = key_ppage;   return(STRING); }
	case KRI:	{ *string = key_sr;   return(STRING); }
	case KRMIR:	{ *string = key_eic;  return(STRING); }
	case KTBC:	{ *string = key_catab;   return(STRING); }
	case LF0:	{ *string = lab_f0;   return(STRING); }
	case LF1:	{ *string = lab_f1;   return(STRING); }
	case LF10:	{ *string = lab_f10;  return(STRING); }
	case LF2:	{ *string = lab_f2;   return(STRING); }
	case LF3:	{ *string = lab_f3;   return(STRING); }
	case LF4:	{ *string = lab_f4;   return(STRING); }
	case LF5:	{ *string = lab_f5;   return(STRING); }
	case LF6:	{ *string = lab_f6;   return(STRING); }
	case LF7:	{ *string = lab_f7;   return(STRING); }
	case LF8:	{ *string = lab_f8;   return(STRING); }
	case LF9:	{ *string = lab_f9;   return(STRING); }
	case LH:	{ *number = label_height;   return(NUMBER); }
	case LINEZ:	{ *number = lines; return(NUMBER); }
	case LL:	{ *string = cursor_to_ll;   return(STRING); }
	case LM:	{ *number = lines_of_memory; return(NUMBER); }
	case LW:	{ *number = label_width;   return(NUMBER); }
	case MC0:	{ *string = print_screen;   return(STRING); }
	case MC4:	{ *string = prtr_off; return(STRING); }
	case MC5:	{ *string = prtr_on;  return(STRING); }
	case MC5P:	{ *string = prtr_non; return(STRING); }
	case MEML:	{ *string = memory_lock; return(STRING); }
	case MEMU:	{ *string = memory_unlock;  return(STRING); }
	case MIR:	return(1-move_insert_mode);
	case MRCUP:	{ *string = cursor_mem_address;   return(STRING); }
	case MSGR:	return(1-move_standout_mode);
	case NEL:	{ *string = newline;  return(STRING); }
	case NLAB:	{ *number = num_labels;   return(NUMBER); }
	case OS:	return(1-over_strike);
	case PAD:	{ *string = pad_char; return(STRING); }
	case PB:	{ *number = padding_baud_rate; return(NUMBER); }
	case PFKEY:	{ *string = pkey_key; return(STRING); }
	case PFLOC:	{ *string = pkey_local;  return(STRING); }
	case PFX:	{ *string = pkey_xmit;   return(STRING); }
	case PLN:	{ *string = plab_norm;   return(STRING); }
	case PROT:	{ *string = enter_protected_mode; return(STRING); }
	case RC:	{ *string = restore_cursor; return(STRING); }
	case REP:	{ *string = repeat_char; return(STRING); }
	case RESET:	{ (void)putp(reset_1string);
			  (void)putp(reset_2string);
			  (void)putp(reset_3string);
			  return(0);
			}
	case REV:	{ *string = enter_reverse_mode;   return(STRING); }
	case RF:	{ *string = reset_file;  return(STRING); }
	case RI:	{ *string = scroll_reverse; return(STRING); }
	case RIN:	{ *string = parm_rindex; return(STRING); }
	case RMACS:	{ *string = exit_alt_charset_mode;   return(STRING); }
	case RMCUP:	{ *string = exit_ca_mode;   return(STRING); }
	case RMDC:	{ *string = exit_delete_mode;  return(STRING); }
	case RMIR:	{ *string = exit_insert_mode;  return(STRING); }
	case RMKX:	{ *string = keypad_local;   return(STRING); }
	case RMLN:	{ *string = label_off;   return(STRING); }
	case RMM:	{ *string = meta_off; return(STRING); }
	case RMSO:	{ *string = exit_standout_mode;   return(STRING); }
	case RMUL:	{ *string = exit_underline_mode;  return(STRING); }
	case RS1:	{ *string = reset_1string;  return(STRING); }
	case RS2:	{ *string = reset_2string;  return(STRING); }
	case RS3:	{ *string = reset_3string;  return(STRING); }
	case SC:	{ *string = save_cursor; return(STRING); }
	case SGR0:	{ *string = exit_attribute_mode;  return(STRING); }
	case SGR:	{ *string = set_attributes; return(STRING); }
	case SMACS:	{ *string = enter_alt_charset_mode;  return(STRING); }
	case SMCUP:	{ *string = enter_ca_mode;  return(STRING); }
	case SMDC:	{ *string = enter_delete_mode; return(STRING); }
	case SMIR:	{ *string = enter_insert_mode; return(STRING); }
	case SMKX:	{ *string = keypad_xmit; return(STRING); }
	case SMLN:	{ *string = label_on;   return(STRING); }
	case SMM:	{ *string = meta_on;  return(STRING); }
	case SMSO:	{ *string = enter_standout_mode;  return(STRING); }
	case SMUL:	{ *string = enter_underline_mode; return(STRING); }
	case TBC:	{ *string = clear_all_tabs; return(STRING); }
	case TSL:	{ *string = to_status_line; return(STRING); }
	case UC:	{ *string = underline_char; return(STRING); }
	case UL:	return(1-transparent_underline);
	case VPA:	{ *string = row_address; return(STRING); }
	case VT:	{ *number = virtual_terminal; return(NUMBER); }
	case WIND:	{ *string = set_window;  return(STRING); }
	case WSL:	{ *number = width_status_line; return(NUMBER); }
	case XENL:	return(1-eat_newline_glitch);
	case XHP:	return(1-ceol_standout_glitch);
	case XMC:	{ *number = magic_cookie_glitch; return(NUMBER); }
	case XON:	return(1-xon_xoff);
	case XSB:	return(1-beehive_glitch);
	case XT:	return(1-teleray_glitch);
	default:	return(4);
	}
} /* capindex */


main (int argc, char *argv[])
{
    int err;			/* stores setupterm() error return value */
    int xcode;			/* program's exit code */
    int stdin_flag = 0;		/* set to 1 if we are reading from stdin */
    int end_of_input = 0;	/* set to 1 to stop reading/parsing input */

    extern opterr, optind;
    extern char *optarg;

    char *termtype_ptr = NULL;	/* pointer to termtype string */
    int optchar;
    opterr = 0;

    if (!setlocale(LC_ALL, "")) {	/* get language & initialize environment table */
	(void)fputs(_errlocale("tput"), stderr);
	catd = (nl_catd) -1;
	(void)putenv("LC_ALL=");
    } else {			/* initialization okay, open message catalog */
	catd = catopen ( "tput", 0 );
    }

    while (( optchar = getopt ( argc, argv, "ST:" )) != EOF )
	switch ( optchar ) {
	case 'S':
	    stdin_flag++;
	    break;
	case 'T':
	    termtype_ptr = optarg;
	    break;
	case '?':
	    (void)fprintf(stderr,(catgets(catd,NL_SETN,1,
			"tput: usage: \"tput [-STterm] capname [parms ...]\"\n")));
	    exit(2);
	}

    if ( ! stdin_flag && optind >= argc ) {
	(void)fprintf(stderr,(catgets(catd,NL_SETN,1,
		"tput: usage: \"tput [-STterm] capname [parms ...]\"\n")));
	exit(2);
    }

    if ( termtype_ptr == NULL )
	(void)setupterm(0,1,&err);	/* use TERM from environment */
    else
	(void)setupterm(termtype_ptr,1,&err);

    if ( err <= 0 )
    {
	(void)fprintf(stderr,(catgets(catd,NL_SETN,2,
	    "tput: unknown terminal \"%s\"\n")),
	    (termtype_ptr != NULL) ? termtype_ptr : getenv("TERM"));
	exit(3);
    }

    /* process arguments from either the arg list or stdin */
    do {
	struct node tbl;
	struct node *tbl_ptr;
#define TPARM_MAX 9
	int p[TPARM_MAX], i;
	char buf[BUFSIZ];		/* buffer to hold input line */
	char cap[BUFSIZ];		/* buffer to capname */
	int num_conv = 1;		/* capname counts as one conversion */

	for(i=0; i<TPARM_MAX; i++) p[i] = 0;	/* zero out the array */

	if ( ! stdin_flag ) {		/* arguments come from the argument list */
	    tbl.cp = argv[optind++];

	    /* This handles multiple capnames in the argument list.
	     * atoi() will return a 0 if the argument starts with a character.
	     * if it is a character then it's the next capname, so break out
	     * of the for-loop 
	     */
	    for (i=0;  optind < argc && i < TPARM_MAX; optind++, num_conv++)
		if ((p[i++] = atoi(argv[optind])) == 0 && isalpha(*argv[optind]))
			break;
	    end_of_input = (optind >= argc) ? 1 : 0;

	} else {				/* arguments come from stdin */

	    tbl.cp = cap;
	    if ( gets(buf) != NULL ) {
		num_conv = sscanf(buf,"%s %d %d %d %d %d %d %d %d %d", tbl.cp, &p[0],
			&p[1],&p[2],&p[3],&p[4],&p[5],&p[6],&p[7],&p[TPARM_MAX-1]);
		if ( num_conv == EOF )
		    continue;		/* skip blank lines */
	    } else {
		if ( feof(stdin) ) {
		    break;		/* exit do-while loop */
		} else if ( ferror(stdin) ) {
		    (void)fprintf(stderr,(catgets(catd,NL_SETN,4,"tput: read error from stdin")));
		    exit(5);
		} else {
		    break;		/* exit do-while loop */
		}
	    }
	}

	tbl_ptr = (struct node *)bsearch((void *)(&tbl), (void *)table, NCOLS,
			    sizeof(struct node), (int (*)())nodecmp);
	if (tbl_ptr != 0) {
	    char *string;
	    int number;
	    int index = tbl_ptr - table;
	    xcode = capindex(index, &number, &string);

	    switch (xcode) {
	    case 0:		/* BOOLEAN return values */
	    case 1:
		break;
	    case NUMBER:
		(void)printf("%d\n", number);
		xcode = 0;
		break;
	    case STRING:
		if ( num_conv > 1 ) {
		    char *bp;
		    bp = tparm(string,p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[TPARM_MAX-1]);
		    (void)putp(bp);
		} else
		    (void)putp(string);
		xcode = 0;
		break;
	    default:
		(void)fprintf(stderr,(catgets(catd,NL_SETN,3,"tput: unknown capname %s\n")),tbl.cp);
		xcode = 4;
		break;
	    }
	} else {
	    (void)fprintf(stderr,(catgets(catd,NL_SETN,3,"tput: unknown capname %s\n")),tbl.cp);
	    xcode = 4;
	}
    } while ( ! end_of_input );

    (void)resetterm();
    (void)catclose(catd);
    return(xcode);
}
