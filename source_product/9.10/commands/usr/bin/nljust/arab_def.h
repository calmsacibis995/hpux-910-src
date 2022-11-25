/* @(#) $Revision: 62.1 $ */   

/* General constants */

#define		FALSE			0
#define		TRUE			1

/* LAAM ALIF group constants (table offsets) */

#define		ALF_AFTER_LAM		0
#define		ALF_HAM_LAM		1
#define		ALF_MAD_LAM		2
#define		HAMZA_ALIF_LAM		3

/* Old ALIF following LAAM group fonts */

#define		F_ALF_AFTER_LAM		212
#define		F_ALF_HAM_LAM		209
#define		F_ALF_MAD_LAM		210
#define		F_HAMZA_ALIF_LAM	211

/* New inital & medial ALIF following LAAM group fonts */

#define		I_ALF_AFTER_LAM		77
#define		M_ALF_AFTER_LAM		73
#define		I_ALF_HAM_LAM		78
#define		M_ALF_HAM_LAM		74
#define		I_ALF_MAD_LAM		79
#define		M_ALF_MAD_LAM		75
#define		I_HAMZA_ALIF_LAM	80
#define		M_HAMZA_ALIF_LAM	76

/* New LAAM font */

#define		I_LAAM			66
#define		M_LAAM			65

/* Table offset constants */

#define		OFF_AR			193
#define		OFF_DN			235
#define		OFF_DM			238
#define		OFF_MS			161
#define		OFF_S1			219
#define		OFF_S2			251

/* Context shape constants */

#define		INITIAL			0
#define		MEDIAL			1
#define		FINAL			2
#define		ISOLATED		3
#define		SPECIAL			4
#define		MID_NUM			5
#define		LAT_CHR			6

/* CODAR U/FD ARAB8 constants */ 

#define		LATIN_CHAR_BOUNDARY	160
#define		ALF_HAMZA_U		195
#define		ALF_MADDA_U		194
#define		ALF_MAKSURA_U		233
#define		ALF_U			199
#define		ARAB_SPACE_U		160
#define		BAA_U			200
#define		COMMERCIAL_AT_U		192
#define		DAAL_U			207
#define		DHAAD_U			214
#define		FAA_U			225
#define		GHAYN_U			218
#define		HAMZA_C_U		198
#define		HAMZA_U			193
#define		HAMZA_U_ALIF_U		197
#define		INDIA_NINE_U		185
#define		INDIA_ZERO_U		176
#define		LAAM_U			228
#define		LATIN_SPACE_U		32
#define		RAA_U			209
#define		SEEN_U			211
#define		SHADDA_FAT_U		246
#define		SHADDA_KAS_U		248
#define		SUKUN_U			242
#define		TAA_MARBUTA_U		201
#define		TAA_U			202
#define		TAMDEED_U		224
#define		TAN_DHAMMA_U		236
#define		TAN_FATHA_U		235
#define		TAN_KASRA_U		237
#define		THAAL_U			208
#define		THAA_U			203
#define		UNDERLINE_U		223
#define		OVERLINE_U		254
#define		OPEN_BRACE_U		251
#define		WAW_HAMZA_U		196
#define		WAW_U			232
#define		YA_U			234
#define		ZAA_U			210

/* Offsets for console and printer font arrays */

#define		DIAC_NORMAL	00
#define		DIAC_MEDIAL	00+14
#define		DIAC_BAA_Q	00+14+11
#define		DIAC_SEEN_Q	00+14+11+14
#define		MISC_CHAR	00+14+11+14+14
#define		SPEC_1		00+14+11+14+14+32
#define		SPEC_2		00+14+11+14+14+32+5
#define		ARAB_MAP	00+14+11+14+14+32+5+4
#define		LAAM_ALIF	00+14+11+14+14+32+5+4+168
#define		ALF_TANWEEN	00+14+11+14+14+32+5+4+168+4
#define		BA_Q		00+14+11+14+14+32+5+4+168+4+1
#define		SEEN_Q		00+14+11+14+14+32+5+4+168+4+1+1
#define		BUG_CHAR	00+14+11+14+14+32+5+4+168+4+1+1+1
#define		SPACE_CHAR	00+14+11+14+14+32+5+4+168+4+1+1+1+1
#define		TAMDEED		00+14+11+14+14+32+5+4+168+4+1+1+1+1+1
#define		TAN_FAT_BQ	00+14+11+14+14+32+5+4+168+4+1+1+1+1+1+1
