/* @(#) $Revision: 70.1 $ */   

#ifdef _NAMESPACE_CLEAN
#define cvtnum _cvtnum
#endif /* _NAMESPACE_CLEAN */

#include "cvtnum.h"

#define	C_NUMBER	0
#define	C_NAN		1
#define	C_INFINITY	2

typedef unsigned short int32[2];
typedef unsigned short int64[4];
typedef unsigned short int80[5];
typedef unsigned short int128[8];
typedef unsigned short int160[10];

typedef struct {
	short exp;
	unsigned m_sign : 1;
	unsigned round : 1;
	unsigned sticky : 1;
	unsigned : 13;
	int32 m;
} float32;

typedef struct {
	short exp;
	unsigned m_sign : 1;
	unsigned round : 1;
	unsigned sticky : 1;
	unsigned : 13;
	int64 m;
} float64;

typedef struct {
	short exp;
	unsigned m_sign : 1;
	unsigned round : 1;
	unsigned sticky : 1;
	unsigned : 13;
	int80 m;
} float80;

static int m_sign,e_sign,m_len,e_len,decpt;
static unsigned char mantissa[21],exponent[4];

enum ctype {ot,ws,dp,pl,mi,zr,nz,a_,hx,e_,f_,i_,n_,lp,rp};
enum stype {init,mdpt,msgn,mzro,fzro,mdgt,fdgt,eexp,esgn,ezro,edgt,
            nan1,nan2,nan3,nnlp,ndgt,inf1,inf2,texp,tnxp,eror};

static short max_mant_size[4] = {9,17,21,17};
static short max_exp_size[4] = {2,3,4,4};
static short max_nan_size[4] = {6,13,16,16};

static enum ctype chars[111] =
{ 
	ot,ot,ot,ot,ot,ot,ot,ot,ot,ws,ws,ws,ws,ws,ot,ot,
	ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,
	ws,ot,ot,ot,ot,ot,ot,ot,lp,rp,ot,pl,ot,mi,dp,ot,
	zr,nz,nz,nz,nz,nz,nz,nz,nz,nz,ot,ot,ot,ot,ot,ot,
	ot,a_,hx,hx,hx,e_,f_,ot,ot,i_,ot,ot,ot,ot,n_,ot,
	ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,ot,
	ot,a_,hx,hx,hx,e_,f_,ot,ot,i_,ot,ot,ot,ot,n_ 
};

static enum stype transition[18][15] =
{
{eror,init,mdpt,msgn,msgn,mzro,mdgt,eror,eror,eror,eror,inf1,nan1,eror,eror},
{eror,eror,eror,eror,eror,fzro,fdgt,eror,eror,eror,eror,eror,eror,eror,eror},
{eror,eror,mdpt,eror,eror,mzro,mdgt,eror,eror,eror,eror,inf1,nan1,eror,eror},
{tnxp,tnxp,fzro,tnxp,tnxp,mzro,mdgt,tnxp,tnxp,eexp,tnxp,tnxp,tnxp,tnxp,tnxp},
{tnxp,tnxp,tnxp,tnxp,tnxp,fzro,fdgt,tnxp,tnxp,eexp,tnxp,tnxp,tnxp,tnxp,tnxp},
{tnxp,tnxp,fdgt,tnxp,tnxp,mdgt,mdgt,tnxp,tnxp,eexp,tnxp,tnxp,tnxp,tnxp,tnxp},
{tnxp,tnxp,tnxp,tnxp,tnxp,fdgt,fdgt,tnxp,tnxp,eexp,tnxp,tnxp,tnxp,tnxp,tnxp},
{tnxp,tnxp,tnxp,esgn,esgn,ezro,edgt,tnxp,tnxp,tnxp,tnxp,tnxp,tnxp,tnxp,tnxp},
{tnxp,tnxp,tnxp,tnxp,tnxp,ezro,edgt,tnxp,tnxp,tnxp,tnxp,tnxp,tnxp,tnxp,tnxp},
{texp,texp,texp,texp,texp,ezro,edgt,texp,texp,texp,texp,texp,texp,texp,texp},
{texp,texp,texp,texp,texp,edgt,edgt,texp,texp,texp,texp,texp,texp,texp,texp},
{eror,eror,eror,eror,eror,eror,eror,nan2,eror,eror,eror,eror,eror,eror,eror},
{eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,nan3,eror,eror},
{eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,nnlp,eror},
{eror,eror,eror,eror,eror,ndgt,ndgt,ndgt,ndgt,ndgt,ndgt,eror,eror,eror,eror},
{eror,eror,eror,eror,eror,ndgt,ndgt,ndgt,ndgt,ndgt,ndgt,eror,eror,eror,tnxp},
{eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,inf2,eror,eror},
{eror,eror,eror,eror,eror,eror,eror,eror,eror,eror,tnxp,eror,eror,eror,eror}
};

static int msb32[10] = {0,3,6,9,13,16,19,23,26,29};
static int msb64[18] = {0,3,6,9,13,16,19,23,26,29,0,36,39,43,46,49,53,56};
static int msb80[22] = {0,3,6,9,13,-1,19,23,26,29,33,36,
			39,43,46,0,53,56,59,63,66,69};

static unsigned short table32a[14][2] = {
	{0x0000,0x0000},
	{0xA000,0x0000},
	{0xC800,0x0000},
	{0xFA00,0x0000},
	{0x9C40,0x0000},
	{0xC350,0x0000},
	{0xF424,0x0000},
	{0x9896,0x8000},
	{0xBEBC,0x2000},
	{0xEE6B,0x2800},
	{0x9502,0xF900},
	{0xBA43,0xB740},
	{0xE8D4,0xA510},
	{0x9184,0xE72A}
};

static unsigned short table32b[4][2] = {
	{0x0000,0x0000},
	{0xB5E6,0x20F4},
	{0x813F,0x3979},
	{0xB7AB,0xC627}
};

static short table32ae[14] = {0,4,7,10,14,17,20,24,27,30,34,37,40,44};

static short table32be[4] = {0,47,94,140};

static unsigned short table64a[28][4] = {
	{0x0000,0x0000,0x0000,0x0000},
	{0xA000,0x0000,0x0000,0x0000},
	{0xC800,0x0000,0x0000,0x0000},
	{0xFA00,0x0000,0x0000,0x0000},
	{0x9C40,0x0000,0x0000,0x0000},
	{0xC350,0x0000,0x0000,0x0000},
	{0xF424,0x0000,0x0000,0x0000},
	{0x9896,0x8000,0x0000,0x0000},
	{0xBEBC,0x2000,0x0000,0x0000},
	{0xEE6B,0x2800,0x0000,0x0000},
	{0x9502,0xF900,0x0000,0x0000},
	{0xBA43,0xB740,0x0000,0x0000},
	{0xE8D4,0xA510,0x0000,0x0000},
	{0x9184,0xE72A,0x0000,0x0000},
	{0xB5E6,0x20F4,0x8000,0x0000},
	{0xE35F,0xA931,0xA000,0x0000},
	{0x8E1B,0xC9BF,0x0400,0x0000},
	{0xB1A2,0xBC2E,0xC500,0x0000},
	{0xDE0B,0x6B3A,0x7640,0x0000},
	{0x8AC7,0x2304,0x89E8,0x0000},
	{0xAD78,0xEBC5,0xAC62,0x0000},
	{0xD8D7,0x26B7,0x177A,0x8000},
	{0x8786,0x7832,0x6EAC,0x9000},
	{0xA968,0x163F,0x0A57,0xB400},
	{0xD3C2,0x1BCE,0xCCED,0xA100},
	{0x8459,0x5161,0x4014,0x84A0},
	{0xA56F,0xA5B9,0x9019,0xA5C8},
	{0xCECB,0x8F27,0xF420,0x0F3A}
};

static unsigned short table64b[13][4] = {
	{0x0000,0x0000,0x0000,0x0000},
	{0x813F,0x3978,0xF894,0x0984},
	{0x8281,0x8F12,0x81ED,0x44A0},
	{0x83C7,0x088E,0x1AAB,0x65DB},
	{0x850F,0xADC0,0x9923,0x329E},
	{0x865B,0x8692,0x5B9B,0xC5C2},
	{0x87AA,0x9AFF,0x7904,0x2287},
	{0x88FC,0xF317,0xF222,0x41E2},
	{0x8A52,0x96FF,0xE33C,0xC930},
	{0x8BAB,0x8EEF,0xB640,0x9C1A},
	{0x8D07,0xE334,0x5563,0x7EB3},
	{0x8E67,0x9C2F,0x5E44,0xFF8F},
	{0x8FCA,0xC257,0x558E,0xE4E6}
};

static short table64ae[28] = {0,4,7,10,14,17,20,24,27,30,34,37,40,44,47,
			      50,54,57,60,64,67,70,74,77,80,84,87,90};

static short table64be[13] = {0,94,187,280,373,466,559,
			      652,745,838,931,1024,1117};

static unsigned short table80a[35][5] = {
	{0x0000,0x0000,0x0000,0x0000,0x0000},
	{0xA000,0x0000,0x0000,0x0000,0x0000},
	{0xC800,0x0000,0x0000,0x0000,0x0000},
	{0xFA00,0x0000,0x0000,0x0000,0x0000},
	{0x9C40,0x0000,0x0000,0x0000,0x0000},
	{0xC350,0x0000,0x0000,0x0000,0x0000},
	{0xF424,0x0000,0x0000,0x0000,0x0000},
	{0x9896,0x8000,0x0000,0x0000,0x0000},
	{0xBEBC,0x2000,0x0000,0x0000,0x0000},
	{0xEE6B,0x2800,0x0000,0x0000,0x0000},
	{0x9502,0xF900,0x0000,0x0000,0x0000},
	{0xBA43,0xB740,0x0000,0x0000,0x0000},
	{0xE8D4,0xA510,0x0000,0x0000,0x0000},
	{0x9184,0xE72A,0x0000,0x0000,0x0000},
	{0xB5E6,0x20F4,0x8000,0x0000,0x0000},
	{0xE35F,0xA931,0xA000,0x0000,0x0000},
	{0x8E1B,0xC9BF,0x0400,0x0000,0x0000},
	{0xB1A2,0xBC2E,0xC500,0x0000,0x0000},
	{0xDE0B,0x6B3A,0x7640,0x0000,0x0000},
	{0x8AC7,0x2304,0x89E8,0x0000,0x0000},
	{0xAD78,0xEBC5,0xAC62,0x0000,0x0000},
	{0xD8D7,0x26B7,0x177A,0x8000,0x0000},
	{0x8786,0x7832,0x6EAC,0x9000,0x0000},
	{0xA968,0x163F,0x0A57,0xB400,0x0000},
	{0xD3C2,0x1BCE,0xCCED,0xA100,0x0000},
	{0x8459,0x5161,0x4014,0x84A0,0x0000},
	{0xA56F,0xA5B9,0x9019,0xA5C8,0x0000},
	{0xCECB,0x8F27,0xF420,0x0F3A,0x0000},
	{0x813F,0x3978,0xF894,0x0984,0x4000},
	{0xA18F,0x07D7,0x36B9,0x0BE5,0x5000},
	{0xC9F2,0xC9CD,0x0467,0x4EDE,0xA400},
	{0xFC6F,0x7C40,0x4581,0x2296,0x4D00},
	{0x9DC5,0xADA8,0x2B70,0xB59D,0xF020},
	{0xC537,0x1912,0x364C,0xE305,0x6C28},
	{0xF684,0xDF56,0xC3E0,0x1BC6,0xC732}
};

static unsigned short table80b[142][5] = {
	{0x0000,0x0000,0x0000,0x0000,0x0000},
	{0x9A13,0x0B96,0x3A6C,0x115C,0x3C7F},
	{0xB975,0xD6B6,0xEE39,0xE436,0xB3E3},
	{0xDF3D,0x5E9B,0xC0F6,0x53E1,0x2F29},
	{0x865B,0x8692,0x5B9B,0xC5C2,0x0B8A},
	{0xA1BA,0x1BA7,0x9E16,0x32DC,0x6463},
	{0xC2AB,0xF989,0x935D,0xDBFE,0x6AD0},
	{0xEA53,0xDF5F,0xD18D,0x5513,0x84C8},
	{0x8D07,0xE334,0x5563,0x7EB2,0xDB0B},
	{0xA9C2,0x794A,0xE3A3,0xC69A,0xB2EB},
	{0xCC57,0x3C2A,0x0ECC,0xDAA6,0xDFAD},
	{0xF5F7,0x5BD4,0xDAB6,0xD49C,0xD432},
	{0x9409,0x19BB,0xD462,0x0B6D,0x2505},
	{0xB230,0xF9B6,0x53D8,0xE415,0x0743},
	{0xD67D,0x6FD5,0xC620,0x541E,0x460C},
	{0x8117,0x6A4B,0x284D,0xD493,0x2835},
	{0x9B63,0x610B,0xB924,0x3E46,0x6555},
	{0xBB0A,0xAF93,0x6C36,0x0ADA,0x61BB},
	{0xE124,0xAFC1,0xDF0E,0x4314,0xF315},
	{0x8780,0xD1A4,0x5DFF,0xCD28,0x8469},
	{0xA31B,0x259C,0xFA50,0x498F,0x7479},
	{0xC454,0xEE0C,0x35F3,0x53F5,0xC14D},
	{0xEC53,0x64C7,0xC051,0x043B,0x699D},
	{0x8E3B,0xBF82,0xE8A4,0x4CE7,0xADF2},
	{0xAB35,0x0C27,0xFEB9,0x0ACC,0x3C4A},
	{0xCE15,0x4BFF,0x6BC0,0xB4B4,0x8674},
	{0xF810,0x4940,0x491A,0x7EF4,0x0B0F},
	{0x954C,0x4080,0x610C,0x746F,0x20C4},
	{0xB3B5,0xF46F,0xCEDC,0x9C88,0x16C0},
	{0xD851,0xA75D,0x5BBC,0x5C88,0xE9CF},
	{0x8231,0x3688,0x0884,0x0353,0x31E7},
	{0x9CB6,0x94B2,0xCA37,0xE7A1,0xEA29},
	{0xBCA2,0xFC30,0xCC19,0xF090,0x9EB6},
	{0xE310,0x28AF,0x8B92,0x5961,0x96F0},
	{0x88A8,0x9CF3,0x9006,0x4AEB,0x3A7B},
	{0xA47F,0x323B,0x36E3,0x96CB,0x1C7B},
	{0xC601,0x8234,0xB148,0x6FB5,0x46C1},
	{0xEE57,0x46CD,0x3E72,0x6F43,0x4746},
	{0x8F72,0x3BDB,0x565F,0x2A64,0x46D4},
	{0xACAA,0xC7F4,0xF2F5,0x2E8C,0xC352},
	{0xCFD7,0x298D,0xB6CB,0x9672,0xDCE4},
	{0xFA2D,0xCABF,0x1789,0xEB91,0xDB1E},
	{0x9692,0x28AF,0xC7FA,0xB524,0xF391},
	{0xB53E,0x4046,0xCE31,0x7483,0x2271},
	{0xDA29,0xDCFA,0xCBC8,0xBE72,0x22FC},
	{0x834D,0x69EA,0x1803,0xB8B1,0x4192},
	{0x9E0C,0xACCE,0x1F64,0x3645,0x799A},
	{0xBE3E,0xC418,0x3A34,0xA0D9,0xD19D},
	{0xE4FF,0xD276,0xEEDC,0xE658,0x87E9},
	{0x89D2,0xEDF5,0x8AB7,0xAC0D,0x08FB},
	{0xA5E6,0x4814,0x9FE1,0x786A,0xEF26},
	{0xC7B1,0xBDEC,0x0331,0xA7B7,0x8F84},
	{0xF05F,0x8EF5,0xCAA2,0x331E,0x7275},
	{0x90AB,0x5DF8,0xA221,0x0F84,0xC297},
	{0xAE23,0xB397,0x9AE5,0x1FAB,0xD32D},
	{0xD19C,0xDD22,0x819D,0x5835,0x16B9},
	{0xFC4F,0xEA4F,0xD590,0xB40A,0x7A38},
	{0x97DA,0xD84D,0xE9DF,0x3CE9,0x2BCB},
	{0xB6C9,0xE478,0xE14C,0xD915,0x21A6},
	{0xDC06,0x1965,0x3AA9,0xD644,0x5A42},
	{0x846C,0x09B0,0x28AE,0x0395,0x04F6},
	{0x9F65,0xAFAE,0x14F8,0xA594,0x36EA},
	{0xBFDE,0x0EE3,0x5615,0xC0F2,0x51D2},
	{0xE6F3,0xB63D,0xFE64,0x8B4E,0x4192},
	{0x8AFF,0xCA2B,0xD1F8,0x8549,0x1E34},
	{0xA750,0x6DC9,0xD9B2,0x1CED,0xBE65},
	{0xC965,0xA92C,0x6DEE,0x50CC,0x17FE},
	{0xF26C,0x46DB,0xAC73,0x35D8,0x5967},
	{0x91E7,0x2BA2,0x51DA,0xEE3D,0x564F},
	{0xAF9F,0xD604,0xDFD2,0x90FE,0x1AEF},
	{0xD366,0x6F1D,0x7DDE,0x7C4E,0x3330},
	{0xFE76,0xB206,0xE3E4,0x0168,0x36D1},
	{0x9926,0x556B,0xC8DE,0xFE43,0x5A85},
	{0xB858,0xE853,0x65D6,0x2467,0xB351},
	{0xDDE6,0x6566,0xD332,0x1B5C,0x5F9C},
	{0x858D,0x1B24,0x7FAC,0x85B8,0xD31B},
	{0xA0C1,0xA3B0,0xCFAC,0x27B5,0x13E1},
	{0xC180,0xE43C,0x5676,0x6FBB,0xB423},
	{0xE8EB,0xDD3E,0xA7F5,0xC090,0x637B},
	{0x8C2F,0x3723,0xEE8D,0x646F,0x3DD1},
	{0xA8BD,0xAA0A,0x0064,0xFA44,0x8B23},
	{0xCB1D,0x4C01,0x9DDA,0x13CF,0x8AB0},
	{0xF47D,0x782E,0x21B9,0xC65F,0x7AF8},
	{0x9325,0xAAAC,0x8930,0x4B57,0x7A63},
	{0xB11F,0x3640,0xDAA2,0x9ADE,0x9255},
	{0xD533,0xE7F0,0xA4C0,0xEC31,0xE619},
	{0x8051,0x1607,0x5201,0x4964,0x0199},
	{0x9A74,0xA627,0xA53B,0x3DEA,0x8FC5},
	{0xB9EB,0x5333,0xAA27,0x2E9B,0x11C5},
	{0xDFCA,0xC9DC,0xF029,0xF087,0x1712},
	{0x86B0,0xA39C,0xEE70,0x3E0A,0xF49A},
	{0xA220,0x8F42,0x5AB3,0x6925,0x2195},
	{0xC327,0x4BDE,0x2D70,0x8910,0x1556},
	{0xEAE8,0x50C6,0xFD4A,0xB5E1,0x2653},
	{0x8D61,0x3A77,0x8857,0x6BAC,0xCA0E},
	{0xAA2E,0x0392,0xC745,0xA005,0x718D},
	{0xCCD8,0xAE88,0xCF70,0xAD84,0x12E3},
	{0xF693,0x2CB1,0x8C4D,0xC755,0x1473},
	{0x9466,0xE0F8,0x2501,0x73AE,0x3893},
	{0xB2A1,0xDB5E,0xF4FC,0x1C3D,0x21AC},
	{0xD705,0x5020,0x5EE7,0x13EC,0xD67B},
	{0x8169,0x3153,0xD41E,0x5D28,0x730E},
	{0x9BC5,0xD0AD,0x1A39,0xF3B6,0x8230},
	{0xBB81,0x2C87,0x1018,0x2F84,0x8009},
	{0xE1B3,0x4FB8,0x4632,0x1D04,0x72C5},
	{0x87D6,0xA87A,0xEBE6,0xE32D,0xB8B2},
	{0xA382,0x78DC,0xC618,0xC1CD,0x4491},
	{0xC4D1,0x4D94,0xAD04,0xF79D,0x2BC7},
	{0xECE9,0x1A39,0x6002,0x5C31,0x7CB5},
	{0x8E95,0xD9CC,0x80CA,0x32B9,0xDE7E},
	{0xABA1,0x8130,0x98B5,0x7A3A,0x8984},
	{0xCE97,0xD8F0,0xF5A4,0xAA27,0x45DF},
	{0xF8AD,0x6E3F,0xA030,0xBD15,0xC9B1},
	{0x95AA,0xD472,0xD731,0xCF89,0x232E},
	{0xB427,0xCC82,0x0AB6,0x78DC,0x1709},
	{0xD8DA,0xB043,0xACA2,0x187D,0x8E18},
	{0x8283,0xB014,0x7212,0x99BB,0xD008},
	{0x9D19,0xDB35,0x3B4D,0x4615,0x6DD6},
	{0xBD1A,0x7BCB,0x3017,0x95B8,0x09A4},
	{0xE39F,0xFFFD,0x0E01,0xB3F6,0x5582},
	{0x88FF,0x2F2B,0xADE7,0x4531,0xC9AC},
	{0xA4E7,0x6708,0x4556,0x62FA,0x0B14},
	{0xC67E,0xF13C,0xABF1,0xC1DA,0x7286},
	{0xEEEE,0x430C,0xADF7,0x6854,0xCC61},
	{0x8FCD,0x1AD5,0x0D9B,0x6AF0,0x62FE},
	{0xAD18,0x29BE,0xB64B,0x1B4A,0x6EFB},
	{0xD05A,0xD37A,0xE089,0xD1EB,0x432C},
	{0xFACC,0x46C7,0x9218,0x9907,0x808D},
	{0x96F1,0x8B17,0x42AA,0xD751,0x888D},
	{0xB5B1,0x10DC,0x8B91,0x52B3,0x9C5B},
	{0xDAB4,0x1104,0x4E87,0xE733,0x04E3},
	{0x83A0,0x977F,0xEEB5,0xDBB7,0x9D6D},
	{0x9E70,0xCC06,0xB17A,0xA9C6,0xDE86},
	{0xBEB7,0x488D,0xFC8E,0x78AD,0x52B3},
	{0xE590,0xE3C3,0x2F00,0x31D1,0xFA4E},
	{0x8A2A,0x3D28,0x42D5,0x2EAA,0x33DA},
	{0xA64F,0x605B,0x4E33,0x52CD,0x5B84},
	{0xC830,0x3EC4,0x2AD8,0x8026,0x78C7},
	{0xF0F7,0xD4CC,0x6DF8,0x202E,0xA24D},
	{0x9107,0x034F,0xD3AC,0xC463,0xE82A},
	{0xAE92,0x0427,0x5937,0xA4C0,0xA8C9},
	{0xD221,0xA679,0x6453,0xFD04,0x1AAB}
};

static short table80ae[35] = {0,4,7,10,14,17,20,24,27,30,34,37,40,44,47,
			      50,54,57,60,64,67,70,74,77,80,84,87,90,94,
			      97,100,103,107,110,113};

static short table80be[142] = {0,117,233,349,466,582,698,814,931,1047,1163,
			       1279,1396,1512,1628,1745,1861,1977,2093,2210,
			       2326,2442,2558,2675,2791,2907,3023,3140,3256,
			       3372,3489,3605,3721,3837,3954,4070,4186,4302,
			       4419,4535,4651,4767,4884,5000,5116,5233,5349,
			       5465,5581,5698,5814,5930,6046,6163,6279,6395,
			       6511,6628,6744,6860,6977,7093,7209,7325,7442,
			       7558,7674,7790,7907,8023,8139,8255,8372,8488,
			       8604,8721,8837,8953,9069,9186,9302,9418,9534,
			       9651,9767,9883,10000,10116,10232,10348,10465,
			       10581,10697,10813,10930,11046,11162,11278,
			       11395,11511,11627,11744,11860,11976,12092,
			       12209,12325,12441,12557,12674,12790,12906,
			       13022,13139,13255,13371,13488,13604,13720,
			       13836,13953,14069,14185,14301,14418,14534,
			       14650,14766,14883,14999,15115,15232,15348,
			       15464,15580,15697,15813,15929,16045,16162,
			       16278,16394};

#ifdef _NAMESPACE_CLEAN
#undef cvtnum
#pragma _HP_SECONDARY_DEF _cvtnum cvtnum
#define cvtnum _cvtnum
#endif /* _NAMESPACE_CLEAN */

int cvtnum(src,dst,typ,rnd,ptr,inx) unsigned char *src,*dst,**ptr;
int typ,rnd,*inx;
{
	int scan(),notanumber(),sngl(),dble(),extend(),pack();
	void infinity(),zero();
	int inexact,tempinx,numtype,returncode;
	unsigned char *scanptr;

	inexact = 0;
	scanptr = src;
	returncode = scan(&scanptr,typ,&numtype,&tempinx);
	if (ptr != (unsigned char **)0) *ptr = scanptr;
	if (returncode) {
		if (inx != (int *)0) *inx = 0;
		return C_BADCHAR;
	};
	inexact |= tempinx;
	if (numtype == C_NAN) {
		returncode = notanumber(typ,dst,&tempinx);
		inexact |= tempinx;
		if (inx != (int *)0) *inx = inexact;
		return returncode;
	};
	if (numtype == C_INFINITY) {
		infinity(m_sign,typ,dst);
		if (inx != (int *)0) *inx = 0;
		return C_INF;
	};
	if (!m_len) {
		zero(m_sign,typ,dst);
		if (inx != (int *)0) *inx = 0;
		return 0;
	};
	switch (typ) {
	case C_SNGL:
		returncode = sngl(rnd,dst,&tempinx);
		break;
	case C_DBLE:
		returncode = dble(rnd,dst,&tempinx);
		break;
	case C_EXT:
		returncode = extend(rnd,dst,&tempinx);
		break;
	case C_DPACK:
		returncode = pack(rnd,dst,&tempinx);
		break;
	};
	inexact |= tempinx;
	if (inx != (int *)0) *inx = inexact;
	return returncode;
}

static int scan(ptr,typ,numtype,inx) unsigned char **ptr; 
int typ,*numtype,*inx;
{
	enum stype state;
	enum ctype type;
	short max_mant,max_exp,max_nan;
	register unsigned char c, *sptr = *ptr, *dptr = mantissa;
	unsigned char *eptr;

	m_sign = 0;
	m_len = 0;
	e_sign = 0;
	e_len = 0;
	decpt = 0;
	*inx = 0;
	*numtype = C_NUMBER;
	eptr = 0;
	max_mant = max_mant_size[typ];

	state = init;

	for (;;) {
		type = ((c = *sptr++) < 111) ? chars[c] : ot;
		switch (state = transition[(int) state][(int) type]) {
		case msgn: 
			if (type == mi) m_sign = 1;
			break;
		case fzro: 
			if (type == dp) break;
			decpt--;
			break;
		case mdgt: 
			decpt++;
		case fdgt:	
			if (type == dp) break;
			if (m_len < max_mant) {
				m_len++;
				*dptr++ = c;
			} 
			else if (type != zr) *inx = 1;
			break;
		case eexp: 
			eptr = sptr - 1;
			max_exp = max_exp_size[typ];
			dptr = exponent;
			break;
		case esgn: 
			if (type == mi) e_sign = 1;
			break;
		case edgt: 
			if (e_len >= max_exp) return 1;
			e_len++;
			*dptr++ = c;
			break;
		case nan1: 
			*numtype = C_NAN;
			max_nan = max_nan_size[typ];
			break;
		case ndgt: 
			if (m_len < max_nan) {
				m_len++;
				*dptr++ = c;
			} 
			else if (type != zr) *inx = 1;
			break;
		case inf1: 
			*numtype = C_INFINITY;
			break;
		case texp: 
			if (!e_len) {
				*exponent = '0';
				e_len = 1;
			};
			*ptr = sptr - 1;
			return 0;
		case tnxp: 
			*exponent = '0';
			e_len = 1;
			e_sign = 0;
			if (eptr) *ptr = eptr;
			else *ptr = *numtype == C_NUMBER ? sptr - 1 : sptr;
			return 0;
		case eror: 
			return 1;
		};
	};
}

static int sngl(rnd,dst,inx) int rnd,*inx; unsigned char *dst;
{
	int t1,t2,exp,p10,inexact,returncode;
	unsigned char *ptr;
	float32 value,scale;
	int get_exp(),cvt32();
	void mant32(),scale32(),__mult32(),div32();

	*inx = 0;
	exp = get_exp();
	if (exp < -44) {
		value.exp = -32768;
		value.m_sign = m_sign;
		returncode = cvt32(rnd,&value,dst,&inexact);
		*inx = 1;
		return returncode;
	};
	if (exp > 39) {
		value.exp = 32767;
		value.m_sign = m_sign;
		returncode = cvt32(rnd,&value,dst,&inexact);
		*inx = 1;
		return returncode;
	};
	mant32(&value);
	p10 = exp - m_len;
	if (p10 > 0) {
		scale32(p10,&scale,&inexact);
		*inx |= inexact;
		__mult32(&value,&scale);
	} else if (p10 < 0) {
		scale32(-p10,&scale,&inexact);
		*inx |= inexact;
		div32(&value,&scale);
	};
	value.m_sign = m_sign;
	returncode = cvt32(rnd,&value,dst,&inexact);
	*inx |= inexact;
	return returncode;
}

static int dble(rnd,dst,inx) int rnd,*inx; unsigned char *dst;
{
	int t1,t2,exp,p10,inexact,returncode;
	unsigned char *ptr;
	float64 value,scale;
	int get_exp(),cvt64();
	void mant64(),scale64(),__mult64(),div64();

	*inx = 0;
	exp = get_exp();
	if (exp < -323) {
		value.exp = -32768;
		value.m_sign = m_sign;
		returncode = cvt64(rnd,&value,dst,&inexact);
		*inx = 1;
		return returncode;
	};
	if (exp > 309) {
		value.exp = 32767;
		value.m_sign = m_sign;
		returncode = cvt64(rnd,&value,dst,&inexact);
		*inx = 1;
		return returncode;
	};
	mant64(&value);
	p10 = exp - m_len;
	if (p10 > 0) {
		scale64(p10,&scale,&inexact);
		*inx |= inexact;
		__mult64(&value,&scale);
	} else if (p10 < 0) {
		scale64(-p10,&scale,&inexact);
		*inx |= inexact;
		div64(&value,&scale);
	};
	value.m_sign = m_sign;
	returncode = cvt64(rnd,&value,dst,&inexact);
	*inx |= inexact;
	return returncode;
}

static int extend(rnd,dst,inx) int rnd,*inx; unsigned char *dst;
{
	int t1,t2,exp,p10,inexact,returncode;
	unsigned char *ptr;
	float80 value,scale;
	int get_exp(),cvt80();
	void mant80(),scale80(),__mult80(),div80();

	*inx = 0;
	exp = get_exp();
	if (exp < -4951) {
		value.exp = -32768;
		value.m_sign = m_sign;
		returncode = cvt80(rnd,&value,dst,&inexact);
		*inx = 1;
		return returncode;
	};
	if (exp > 4932) {
		value.exp = 32767;
		value.m_sign = m_sign;
		returncode = cvt80(rnd,&value,dst,&inexact);
		*inx = 1;
		return returncode;
	};
	mant80(&value);
	p10 = exp - m_len;
	if (p10 > 0) {
		scale80(p10,&scale,&inexact);
		*inx |= inexact;
		__mult80(&value,&scale);
	} else if (p10 < 0) {
		scale80(-p10,&scale,&inexact);
		*inx |= inexact;
		div80(&value,&scale);
	};
	value.m_sign = m_sign;
	returncode = cvt80(rnd,&value,dst,&inexact);
	*inx |= inexact;
	return returncode;
}

static int pack(rnd,dst,inx) int rnd,*inx; 
register unsigned char *dst;
{
	register unsigned long t1,t2;
	register short ctr;
	unsigned long t3;
	int exp,shift,errorcode;
	short round,carry;
	int get_exp();
	void infinity(),zero(),max(),hextobin80(),lsr80(),lsl80();
	void bintobcd();
	union {
		int80 i;
		unsigned char c[10];
	} value;
	unsigned char bcdexp[2];

	exp = get_exp() - 1;
	if (exp > 999) {
		switch (rnd) {
		case C_NEAR:
			infinity(m_sign,C_DPACK,dst);
			break;
		case C_POS_INF:
			if (m_sign) max(m_sign,C_DPACK,dst);
			else infinity(m_sign,C_DPACK,dst);
			break;
		case C_NEG_INF:
			if (m_sign) infinity(m_sign,C_DPACK,dst);
			else max(m_sign,C_DPACK,dst);
			break;
		case C_TOZERO:
			max(m_sign,C_DPACK,dst);
			break;
		};
		*inx = 1;
		return C_OVER;
	};
	if (exp < -999) {
		if (exp < -1016) {
			zero(m_sign,C_DPACK,dst);
			if (rnd == C_POS_INF && !m_sign ||
			    rnd == C_NEG_INF && m_sign) dst[11] = 1;
			*inx = 1;
			return C_UNDER;
		};
		hextobin80(mantissa,m_len,value.i);
		if (exp - m_len < -1016) {
			shift = (m_len - exp - 1016) << 2;
			*inx = 0;
			t1 = 4 - (shift >> 4);
			for (ctr = 4; ctr > t1; ctr--) {
				if (value.i[ctr]) {
					*inx = 1;
					break;
				};
			};
			if (t2 = shift & 0xF)
				if (value.i[t1] & (1 << t2) - 1) *inx = 1;
			if (!*inx) errorcode = 0;
			else {
				switch (rnd) {
				case C_NEAR:
					if (t2) t2 -= 4;
					else {
						t2 = 12;
						t1++;
					};
					t3 = (value.i[t1] >> t2) & 0xF;
					if (t3 < 5) round = 0;
					else if (t3 > 5) round = 1;
					else {
						round = 0;
						for (ctr = 4; ctr > t1; ctr--) {
							if (value.i[ctr]) {
								round = 1;
								break;
							};
						};
						if (value.i[t1] & (1 << t2) - 1) round = 1;
						if (!round) {
							if (t2 < 12) t2 += 4;
							else {
								t2 = 0;
								t1--;
							};
							round = (value.i[t1] >> t2) & 1;
						};
					};
					break;
				case C_POS_INF:
					round = 1 - m_sign;
					break;
				case C_NEG_INF:
					round = m_sign;
					break;
				case C_TOZERO:
					round = 0;
					break;
				};
				if (round) {
					carry = 1;
					while (carry) {
						if (((value.i[t1] >> t2) & 0xF) == 9) {
							value.i[t1] &= ~(0xF << t2);
							if (t2 < 12) t2 += 4;
							else {
								t2 = 0;
								t1--;
							};
						} else {
							value.i[t1] += 1 << t2;
							carry = 0;
						};
					};
					errorcode = shift == 4 && !t1 && t2 == 4 ? 0 : C_UNDER;
				} else {
					if (exp == -1016) {
						zero(m_sign,C_DPACK,dst);
						if (rnd == C_POS_INF && !m_sign 
						  || rnd == C_NEG_INF && m_sign)
							dst[11] = 1;
						*inx = 1;
						return C_UNDER;
					};
					errorcode = C_UNDER;
				};
			};
			if (shift) lsr80(value.i,shift);
		} else {
			*inx = 0;
			errorcode = 0;
			shift = (1016 + exp - m_len) << 2;
			if (shift) lsl80(value.i,shift);
		};
		exp = -999;
	} else {
		hextobin80(mantissa,m_len,value.i);
		*inx = 0;
		errorcode = 0;
		shift = (17 - m_len) << 2;
		if (shift) lsl80(value.i,shift);
	};
	bintobcd((unsigned long) (exp >= 0 ? exp : -exp),bcdexp);
	if (m_sign) bcdexp[0] |= 0x80;
	if (exp < 0) bcdexp[0] |= 0x40;
	*dst++ = bcdexp[0];
	*dst++ = bcdexp[1];
	for (ctr = 0; ctr < 10; ctr++) *dst++ = value.c[ctr];
	return errorcode;
}

static void infinity(sign,typ,dst) int sign,typ; register unsigned char *dst;
{
	register short ctr;

	*dst++ = sign ? 0xFF : 0x7F;
	switch (typ) {
	case C_SNGL:
		*dst++ = 0x80;
		ctr = 2;
		break;
	case C_DBLE:
		*dst++ = 0xF0;
		ctr = 6;
		break;
	default:
		*dst++ = 0xFF;
		ctr = 10;
		break;
	};	
	for (;ctr > 0; ctr--) *dst++ = 0;
	return;
}

static int notanumber(typ,dst,inx) int typ,*inx; register unsigned char *dst;
{
	void hextobin80(),lsr80(),lsl80();
	register short ctr;
	register unsigned char *src;
	union {
		int80 i;
		unsigned char c[10];
	} value;

	hextobin80(mantissa,m_len,value.i);
	switch (typ) {
	case C_SNGL:
		if (ctr = 6 - m_len) {
			*inx = 0;
			lsl80(value.i,(ctr << 2) - 1);
		} else {
			*inx = value.c[9] & 1;
			lsr80(value.i,1);
		};
		*dst++ = m_sign ? 0xFF : 0x7F;
		*dst++ = value.c[7] | 0x80;
		*dst++ = value.c[8];
		*dst++ = value.c[9];
		return value.c[7] & 0x40 ? C_QNAN : C_SNAN;
	case C_DBLE:
		*inx = 0;
		if (ctr = 13 - m_len) lsl80(value.i,ctr << 2);
		value.c[2] |= m_sign ? 0xFF : 0x7F;
		value.c[3] |= 0xF0;
		src = &value.c[2];
		for (ctr = 8; ctr > 0; ctr--) *dst++ = *src++;
		return value.c[3] & 0x8 ? C_QNAN : C_SNAN;
	default:
		if (ctr = 16 - m_len) {
			*inx = 0;
			lsl80(value.i,(ctr << 2) - 1);
		} else {
			*inx = value.c[9] & 1;
			lsr80(value.i,1);
		};
		*dst++ = m_sign ? 0xFF : 0x7F;
		*dst++ = 0xFF;
		*dst++ = 0;
		*dst++ = 0;
		src = &value.c[2];
		for (ctr = 8; ctr > 0; ctr--) *dst++ = *src++;
		return value.c[2] & 0x40 ? C_QNAN : C_SNAN;
	};
}

static void zero(sign,typ,dst) int sign,typ; register unsigned char *dst;
{
	register short ctr;

	*dst++ = sign ? 0x80 : 0;
	switch (typ) {
	case C_SNGL:
		ctr = 3;
		break;
	case C_DBLE:
		ctr = 7;
		break;
	case C_EXT:
		ctr = 11;
		break;
	case C_DPACK:
		ctr = 11;
		break;
	};
	for (;ctr > 0; ctr--) *dst++ = 0;
	return;
}

static void max(sign,typ,dst) int sign,typ; register unsigned char *dst;
{
	register short ctr;

	if (typ == C_DPACK) {
		*dst++ = sign ? 0x89 : 0x09;
		*dst++ = 0x99;
		*dst++ = 0x0;
		*dst++ = 0x9;
		for (ctr = 8; ctr > 0; ctr--) *dst++ = 0x99;
	} else {
		*dst++ = sign ? 0xFF : 0x7F;
		switch (typ) {
		case C_SNGL:
			*dst++ = 0x7F;
			ctr = 2;
			break;
		case C_DBLE:
			*dst++ = 0xEF;
			ctr = 6;
			break;
		case C_EXT:
			*dst++ = 0xFE;
			*dst++ = 0x0;
			*dst++ = 0x0;
			ctr = 8;
			break;
		};
		for (;ctr > 0; ctr--) *dst++ = 0xFF;
	};
	return;
}

static int cvt32(rnd,value,dst,inx) int rnd,*inx; float32 *value;
register unsigned char *dst;
{
	int inexact,returncode;
	union {
		int32 i;
		unsigned char c[4];
	} result;
	register unsigned char *src;
	void infinity(),max(),zero(),round32(),lsr32();

	round32(value,rnd,24,inx);
	if (value->exp > 128) {
		*inx = 1;
		switch (rnd) {
		case C_NEAR:
			infinity((int) value->m_sign,C_SNGL,dst);
			break;
		case C_POS_INF:
			if (value->m_sign) max((int) value->m_sign,C_SNGL,dst);
			else infinity((int) value->m_sign,C_SNGL,dst);
			break;
		case C_NEG_INF:
			if (value->m_sign) infinity((int) value->m_sign,C_SNGL,dst);
			else max((int) value->m_sign,C_SNGL,dst);
			break;
		case C_TOZERO:
			max((int) value->m_sign,C_SNGL,dst);
			break;
		};	
		return C_OVER;
	};
	returncode = 0;
	if (value->exp < -125) {
		if (value->exp >= -149)
			round32(value,rnd,value->exp + 149,&inexact);
		if (value->exp < -148) {
			*inx = 1;
			zero((int) value->m_sign,C_SNGL,dst);
			if (rnd == C_POS_INF && !value->m_sign ||
			    rnd == C_NEG_INF && value->m_sign) dst[3] = 1;
			return C_UNDER;
		};
		if (inexact && value->exp < -125) returncode = C_UNDER;
		*inx |= inexact;
	};
	result.i[0] = value->m[0];
	result.i[1] = value->m[1];
	if (value->exp >= -125) {
		result.c[0] &= 0x7F;
		lsr32(result.i,8);
		result.i[0] |= (value->exp + 126) << 7;
	} else lsr32(result.i,-117 - value->exp);
	if (value->m_sign) result.c[0] |= 0x80;
	src = result.c;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst = *src;
	return returncode;
}

static int cvt64(rnd,value,dst,inx) int rnd,*inx; float64 *value;
register unsigned char *dst;
{
	int inexact,returncode;
	union {
		int64 i;
		unsigned char c[8];
	} result;
	register unsigned char *src;
	register short ctr;
	void infinity(),max(),zero(),round64(),lsr64();

	round64(value,rnd,53,inx);
	if (value->exp > 1024) {
		*inx = 1;
		switch (rnd) {
		case C_NEAR:
			infinity((int) value->m_sign,C_DBLE,dst);
			break;
		case C_POS_INF:
			if (value->m_sign) max((int) value->m_sign,C_DBLE,dst);
			else infinity((int) value->m_sign,C_DBLE,dst);
			break;
		case C_NEG_INF:
			if (value->m_sign) infinity((int) value->m_sign,C_DBLE,dst);
			else max((int) value->m_sign,C_DBLE,dst);
			break;
		case C_TOZERO:
			max((int) value->m_sign,C_DBLE,dst);
			break;
		};	
		return C_OVER;
	};
	returncode = 0;
	if (value->exp < -1021) {
		if (value->exp >= -1074)
			round64(value,rnd,value->exp + 1074,&inexact);
		if (value->exp < -1073) {
			*inx = 1;
			zero((int) value->m_sign,C_DBLE,dst);
			if (rnd == C_POS_INF && !value->m_sign ||
			    rnd == C_NEG_INF && value->m_sign) dst[7] = 1;
			return C_UNDER;
		};
		if (inexact && value->exp < -1021) returncode = C_UNDER;
		*inx |= inexact;
	};
	result.i[0] = value->m[0];
	result.i[1] = value->m[1];
	result.i[2] = value->m[2];
	result.i[3] = value->m[3];
	if (value->exp >= -1021) {
		result.c[0] &= 0x7F;
		lsr64(result.i,11);
		result.i[0] |= (value->exp + 1022) << 4;
	} else lsr64(result.i,-1010 - value->exp);
	if (value->m_sign) result.c[0] |= 0x80;
	src = result.c;
	for (ctr = 0; ctr < 8; ctr++) *dst++ = *src++;
	return returncode;
}

static int cvt80(rnd,value,dst,inx) int rnd,*inx; float80 *value;
register unsigned char *dst;
{
	int inexact,returncode;
	union {
		int80 i;
		unsigned char c[10];
	} result;
	union {
		unsigned short s[2];
		unsigned char c[4];
	} sc;
	register unsigned char *src;
	register short ctr;
	void infinity(),max(),zero(),round80(),lsr80();

	round80(value,rnd,64,inx);
	if (value->exp > 16384) {
		*inx = 1;
		switch (rnd) {
		case C_NEAR:
			infinity((int) value->m_sign,C_EXT,dst);
			break;
		case C_POS_INF:
			if (value->m_sign) max((int) value->m_sign,C_EXT,dst);
			else infinity((int) value->m_sign,C_EXT,dst);
			break;
		case C_NEG_INF:
			if (value->m_sign) infinity((int) value->m_sign,C_EXT,dst);
			else max((int) value->m_sign,C_EXT,dst);
			break;
		case C_TOZERO:
			max((int) value->m_sign,C_EXT,dst);
			break;
		};	
		return C_OVER;
	};
	returncode = 0;
	if (value->exp < -16381) {
		if (value->exp >= -16445)
			round80(value,rnd,value->exp + 16445,&inexact);
		if (value->exp < -16444) {
			*inx = 1;
			zero((int) value->m_sign,C_EXT,dst);
			if (rnd == C_POS_INF && !value->m_sign ||
			    rnd == C_NEG_INF && value->m_sign) dst[11] = 1;
			return C_UNDER;
		};
		if (inexact && value->exp < -16381) returncode = C_UNDER;
		*inx |= inexact;
	};
	result.i[0] = value->m[0];
	result.i[1] = value->m[1];
	result.i[2] = value->m[2];
	result.i[3] = value->m[3];
	result.i[4] = value->m[4];
	if (value->exp < -16381) {
		lsr80(result.i,-16381 - value->exp);
		value->exp = -16382;
	};
	sc.s[0] = value->exp + 16382;
	if (value->m_sign) sc.c[0] |= 0x80;
	sc.s[1] = 0;
	src = sc.c;
	for (ctr = 0; ctr < 4; ctr++) *dst++ = *src++;
	src = result.c;
	for (ctr = 0; ctr < 8; ctr++) *dst++ = *src++;
	return returncode;
}

static int get_exp()
{
	void dectobin32();
	union {
		int32 i;
		int exp;
	} value;

	dectobin32(exponent,e_len,value.i);
	if (e_sign) value.exp = -value.exp;
	return value.exp + decpt;
}

static void mant32(value) float32 *value;
{
	register int bit;
	register unsigned long mask;
	void dectobin32(),lsl32();
	union {
		int32 i;
		unsigned long l;
	} x;

	dectobin32(mantissa,m_len,x.i);
	bit = msb32[m_len];
	mask = 1 << bit;
	while (!(x.l & mask)) {
		mask >>= 1;
		bit--;
	};
	lsl32(x.i,31 - bit);
	value->exp = bit + 1;
	value->m_sign = m_sign;
	value->round = 0;
	value->sticky = 0;
	value->m[0] = x.i[0];
	value->m[1] = x.i[1];
	return;
}

static void mant64(value) float64 *value;
{
	register int bit;
	register unsigned long temp,mask;
	void dectobin64(),lsl64();
	union {
		int64 i;
		unsigned long l[2];
	} x;

	dectobin64(mantissa,m_len,x.i);
	if (!(bit = msb64[m_len])) bit = x.l[0] ? 33 : 31;
	if (bit > 31) {
		mask = 1 << bit - 32;
		temp = x.l[0];
	} else {
		mask = 1 << bit;
		temp = x.l[1];
	};
	while (!(temp & mask)) {
		mask >>= 1;
		bit--;
	};
	lsl64(x.i,63 - bit);
	value->exp = bit + 1;
	value->m_sign = m_sign;
	value->round = 0;
	value->sticky = 0;
	value->m[0] = x.i[0];
	value->m[1] = x.i[1];
	value->m[2] = x.i[2];
	value->m[3] = x.i[3];
	return;
}

static void mant80(value) float80 *value;
{
	register int bit;
	register unsigned long temp,mask;
	void dectobin80(),lsl80();
	union {
		int80 i;
		unsigned long l[2];
	} x;

	dectobin80(mantissa,m_len,x.i);
	if ((bit = msb80[m_len]) <= 0) 
		bit = bit ? (x.l[1] ? 16 : 15) : (x.l[0] ? 49 : 47);
	if (bit > 47) {
		mask = 1 << bit - 48;
		temp = x.l[0];
	} else if (bit > 15) {
		mask = 1 << bit - 16;
		temp = x.l[1];
	} else {
		mask = 1 << bit;
		temp = (unsigned long) x.i[4];
	};
	while (!(temp & mask)) {
		mask >>= 1;
		bit--;
	};
	lsl80(x.i,79 - bit);
	value->exp = bit + 1;
	value->m_sign = m_sign;
	value->round = 0;
	value->sticky = 0;
	value->m[0] = x.i[0];
	value->m[1] = x.i[1];
	value->m[2] = x.i[2];
	value->m[3] = x.i[3];
	value->m[4] = x.i[4];
	return;
}

static void bintobcd(value,ptr) unsigned long value;
register unsigned char *ptr;
{
	register unsigned short arg = value;
	*ptr++ = arg / 100;
	arg %= 100;
	*ptr++ = ((arg / 10) << 4) | (arg % 10);
	return;
}

static void dectobin32(ptr,len,value) register unsigned char *ptr;
int len; int32 value;
{
	register unsigned long result;
	register short ctr;
	union {
		int32 i;
		unsigned long l;
	} res;

	result = 0;
	for (ctr = len; ctr > 0; ctr--) {
		result += result;
		result += result << 2;
		result += *ptr++ - '0';
	};
	res.l = result;
	value[0] = res.i[0];
	value[1] = res.i[1];
	return;
}

static void dectobin64(ptr,len,value) register unsigned char *ptr;
int len; int64 value;
{
	register unsigned long result1,result0,temp1,temp0;
	register short ctr;
	union {
		int64 i;
		unsigned long l[2];
	} res;

	result1 = 0;
	result0 = 0;
	if ((ctr = len) > 9) ctr = 9;
	len -= ctr;
	while (ctr-- > 0) {
		result0 += result0;
		result0 += result0 << 2;
		result0 += *ptr++ - '0';
	};
	for (ctr = len; ctr > 0; ctr--) {
		result1 += result1;
		if ((long) result0 < 0) ++result1;
		result0 += result0;
		temp1 = result1 << 2;
		if ((long) (temp0 = result0) < 0) temp1 += 2;
		if ((long) (temp0 <<= 1) < 0) ++temp1;
		temp0 += temp0;
		if ((result0 += temp0) < temp0) ++result1;
		result1 += temp1;
		temp0 = (unsigned long) (*ptr++ - '0');
		if ((result0 += temp0) < temp0) ++result1;
	};
	res.l[0] = result1;
	res.l[1] = result0;
	value[0] = res.i[0];
	value[1] = res.i[1];
	value[2] = res.i[2];
	value[3] = res.i[3];
	return;
}

static void dectobin80(ptr,len,value) register unsigned char *ptr;
int len; int80 value;
{
	register unsigned long result2,result1,result0,temp2,temp1,temp0;
	register short ctr;
	union {
		unsigned short s[6];
		unsigned long l[3];
	} res;

	result2 = 0;
	result1 = 0;
	result0 = 0;
	if ((ctr = len) > 9) ctr = 9;
	len -= ctr;
	while (ctr-- > 0) {
		result0 += result0;
		result0 += result0 << 2;
		result0 += (unsigned long) *ptr++ - '0';
	};
	ctr = len;
	if ((ctr = len) > 10) ctr = 10;
	len -= ctr;
	while (ctr-- > 0) {
		result1 += result1;
		if ((long) result0 < 0) ++result1;
		result0 += result0;
		temp1 = result1 << 2;
		if ((long) (temp0 = result0) < 0) temp1 += 2;
		if ((long) (temp0 <<= 1) < 0) ++temp1;
		temp0 += temp0;
		if ((result0 += temp0) < temp0) ++result1;
		result1 += temp1;
		temp0 = (unsigned long) (*ptr++ - '0');
		if ((result0 += temp0) < temp0) ++result1;
	};
	for (ctr = len; ctr > 0; ctr--) {
		result2 += result2;
		if ((long) result1 < 0) ++result2;
		result1 += result1;
		if ((long) result0 < 0) ++result1;
		result0 += result0;
		temp2 = result2 << 2;
		if ((long) (temp1 = result1) < 0) temp2 += 2;
		if ((long) (temp1 <<= 1) < 0) ++temp2;
		temp1 += temp1;
		if ((long) (temp0 = result0) < 0) temp1 += 2;
		if ((long) (temp0 <<= 1) < 0) ++temp1;
		temp0 += temp0;
		result2 += temp2;
		if ((result1 += temp1) < temp1) ++result2;
		if ((result0 += temp0) < temp0) if (!++result1) ++result2;
		temp0 = (unsigned long) (*ptr++ - '0');
		if ((result0 += temp0) < temp0) if (!++result1) ++result2;
	};
	res.l[0] = result2;
	res.l[1] = result1;
	res.l[2] = result0;
	value[0] = res.s[1];
	value[1] = res.s[2];
	value[2] = res.s[3];
	value[3] = res.s[4];
	value[4] = res.s[5];
	return;
}

static void hextobin80(ptr,len,res) register unsigned char *ptr; 
int len; int80 res;
{
	register short ctr;
	register unsigned char *resptr,c1,c2;
	union {
		int80 i;
		unsigned char c[10];
	} value;

	for (ctr = 0; ctr < 5; ctr++) value.i[ctr] = 0;
	resptr = &value.c[10 - ((len + 1) >> 1)];
	c2 = 0;
	for (ctr = len; ctr > 0; ctr--) {
		c1 = *ptr++;
		if ((c1 -= '0') > 9) c1 = (c1 & 0x1F) - 7;
		if (ctr & 1) *resptr++ = c1 | c2;
		else c2 = c1 << 4;
	};
	for (ctr = 0; ctr < 5; ctr++) res[ctr] = value.i[ctr];
	return;
}

static void scale32(p10,value,inx) int p10,*inx; float32 *value;
{
	int mple14,inexact;
	float32 temp;
	void __mult32(),round32();

	value->m_sign = 0;
	value->round = 0;
	value->sticky = 0;
	if (p10 < 14) {
		*inx = 0;
		value->exp = table32ae[p10];
		value->m[0] = table32a[p10][0];
		value->m[1] = table32a[p10][1];
	} else {
		*inx = 1;
		mple14 = p10 / 14;
		value->exp = table32be[mple14];
		value->m[0] = table32b[mple14][0];
		value->m[1] = table32b[mple14][1];
		if (p10 %= 14) {
			temp.m_sign = 0;
			temp.exp = table32ae[p10];
			temp.m[0] = table32a[p10][0];
			temp.m[1] = table32a[p10][1];
			__mult32(value,&temp);
			round32(value,C_NEAR,32,&inexact);
		};
	};
	return;
}

static void scale64(p10,value,inx) int p10,*inx; float64 *value;
{
	int mple28,inexact;
	float64 temp;
	void __mult64(),round64();

	value->m_sign = 0;
	value->round = 0;
	value->sticky = 0;
	if (p10 < 28) {
		*inx = 0;
		value->exp = table64ae[p10];
		value->m[0] = table64a[p10][0];
		value->m[1] = table64a[p10][1];
		value->m[2] = table64a[p10][2];
		value->m[3] = table64a[p10][3];
	} else {
		*inx = 1;
		mple28 = p10 / 28;
		value->exp = table64be[mple28];
		value->m[0] = table64b[mple28][0];
		value->m[1] = table64b[mple28][1];
		value->m[2] = table64b[mple28][2];
		value->m[3] = table64b[mple28][3];
		if (p10 %= 28) {
			temp.m_sign = 0;
			temp.exp = table64ae[p10];
			temp.m[0] = table64a[p10][0];
			temp.m[1] = table64a[p10][1];
			temp.m[2] = table64a[p10][2];
			temp.m[3] = table64a[p10][3];
			__mult64(value,&temp);
			round64(value,C_NEAR,64,&inexact);
		};
	};
	return;
}

static void scale80(p10,value,inx) int p10,*inx; float80 *value;
{
	int mple35,inexact;
	float80 temp;
	void __mult80(),round80();

	value->m_sign = 0;
	value->round = 0;
	value->sticky = 0;
	if (p10 < 35) {
		*inx = 0;
		value->exp = table80ae[p10];
		value->m[0] = table80a[p10][0];
		value->m[1] = table80a[p10][1];
		value->m[2] = table80a[p10][2];
		value->m[3] = table80a[p10][3];
		value->m[4] = table80a[p10][4];
	} else {
		*inx = 1;
		mple35 = p10 / 35;
		value->exp = table80be[mple35];
		value->m[0] = table80b[mple35][0];
		value->m[1] = table80b[mple35][1];
		value->m[2] = table80b[mple35][2];
		value->m[3] = table80b[mple35][3];
		value->m[4] = table80b[mple35][4];
		if (p10 %= 35) {
			temp.m_sign = 0;
			temp.exp = table80ae[p10];
			temp.m[0] = table80a[p10][0];
			temp.m[1] = table80a[p10][1];
			temp.m[2] = table80a[p10][2];
			temp.m[3] = table80a[p10][3];
			temp.m[4] = table80a[p10][4];
			__mult80(value,&temp);
			round80(value,C_NEAR,80,&inexact);
		};
	};
	return;
}

static void lsl32(value,count) int32 value; int count;
{
	union {
		int32 i;
		unsigned long l;
	} x;

	if (count < 32) {
		x.i[0] = value[0];
		x.i[1] = value[1];
		x.l <<= count;
	} else x.l = 0;

	value[0] = x.i[0];
	value[1] = x.i[1];
	return;
}

static void lsl64(x,count) int64 x; int count;
{
	int offset;
	register unsigned short *c1,*c2,ctr;

	if (count < 64) {
		if (offset = count >> 4) {
			c1 = x;
			c2 = c1 + offset;
			for (ctr = 4 - offset; ctr > 0; ctr--) 
				*c1++ = *c2++;
			for (ctr = offset; ctr > 0; ctr--) 
				*c1++ = 0;
		};
		if (count &= 0xF) {
			for (ctr = 0; ctr < 3; ctr++) {
				x[ctr] <<= count;
				x[ctr] |= x[ctr+1] >> (16 - count);
			};
			x[3] <<= count;
		};
	} else for (ctr = 0; ctr < 4; ctr++) x[ctr] = 0;
	return;
}

static void lsl80(x,count) int80 x; int count;
{
	int offset;
	register unsigned short *c1,*c2,ctr;

	if (count < 80) {
		if (offset = count >> 4) {
			c1 = x;
			c2 = c1 + offset;
			for (ctr = 5 - offset; ctr > 0; ctr--) 
				*c1++ = *c2++;
			for (ctr = offset; ctr > 0; ctr--) 
				*c1++ = 0;
		};
		if (count &= 0xF) {
			for (ctr = 0; ctr < 4; ctr++) {
				x[ctr] <<= count;
				x[ctr] |= x[ctr+1] >> (16 - count);
			};
			x[4] <<= count;
		};
	} else for (ctr = 0; ctr < 5; ctr++) x[ctr] = 0;
	return;
}

static void lsr32(value,count) int32 value; int count;
{
	union {
		int32 i;
		unsigned long l;
	} x;

	if (count < 32) {
		x.i[0] = value[0];
		x.i[1] = value[1];
		x.l >>= count;
	} else x.l = 0;

	value[0] = x.i[0];
	value[1] = x.i[1];
	return;
}

static void lsr64(x,count) int64 x; int count;
{
	int offset;
	register unsigned short *c1,*c2,ctr;

	if (count < 64) {
		if (offset = count >> 4) {
			c1 = &x[3];
			c2 = c1 - offset;
			for (ctr = 4 - offset; ctr > 0; ctr--) 
				*c1-- = *c2--;
			for (ctr = offset; ctr > 0; ctr--) 
				*c1-- = 0;
		};
		if (count &= 0xF) {
			for (ctr = 3; ctr > 0; ctr--) {
				x[ctr] >>= count;
				x[ctr] |= x[ctr-1] << (16 - count);
			};
			x[0] >>= count;
		};
	} else for (ctr = 0; ctr < 4; ctr++) x[ctr] = 0;
	return;
}

static void lsr80(x,count) int80 x; int count;
{
	int offset;
	register unsigned short *c1,*c2,ctr;

	if (count < 80) {
		if (offset = count >> 4) {
			c1 = &x[4];
			c2 = c1 - offset;
			for (ctr = 5 - offset; ctr > 0; ctr--) 
				*c1-- = *c2--;
			for (ctr = offset; ctr > 0; ctr--) 
				*c1-- = 0;
		};
		if (count &= 0xF) {
			for (ctr = 4; ctr > 0; ctr--) {
				x[ctr] >>= count;
				x[ctr] |= x[ctr-1] << (16 - count);
			};
			x[0] >>= count;
		};
	} else for (ctr = 0; ctr < 5; ctr++) x[ctr] = 0;
	return;
}

static void round32(value,rnd,bits,inx) float32 *value; int rnd,bits,*inx;
{
	short round,r,s;
	union {
		int32 i;
		unsigned long l;
	} x;

	x.i[0] = value->m[0];
	x.i[1] = value->m[1];
	if (value->round || value->sticky || x.l << bits) {
		*inx = 1;
		round = 0;
		switch (rnd) {
		case C_NEAR:
			if (bits == 32) {
				r = value->round;
				s = value->sticky;
			} else {
				r = (long) (x.l << bits) < 0;
				s = value->round || value->sticky ||
					x.l << bits + 1;
			};
			if (r)
				if (s) round = 1;
				else if (bits) 
					round = (long) (x.l << bits - 1) < 0;
			break;
		case C_POS_INF:
			round = 1 - value->m_sign;
			break;
		case C_NEG_INF:
			round = value->m_sign;
			break;
		};
		value->round = 0;
		value->sticky = 0;
		x.l &= ~0 << 32 - bits;
		if (round)
			if (bits ? ((x.l += 1 << 32 - bits) ? 0 : 1) : 1) {
				x.i[0] = 0x8000;
				value->exp++;
			};
		value->m[0] = x.i[0];
		value->m[1] = x.i[1];
	} else *inx = 0;
	return;
}

static void round64(value,rnd,bits,inx) float64 *value; int rnd,bits,*inx;
{
	short round,r,s,c;
	union {
		int64 i;
		unsigned long l[2];
	} x;

	x.i[0] = value->m[0];
	x.i[1] = value->m[1];
	x.i[2] = value->m[2];
	x.i[3] = value->m[3];
	if (value->round || value->sticky || (bits >= 32 ? x.l[1] << bits - 32 :
	    x.l[1] || x.l[0] << bits)) {
		*inx = 1;
		round = 0;
		switch (rnd) {
		case C_NEAR:
			if (bits == 64) {
				r = value->round;
				s = value->sticky;
			} else {
				s = value->round || value->sticky;
				if (bits >= 32) {
					r = (long) (x.l[1] << bits - 32) < 0;
					s = s || x.l[1] << bits - 31;
				} else {
					r = (long) (x.l[0] << bits) < 0 ? 1 : 0;
					s = s || x.l[1] || x.l[0] << bits + 1;
				};
			};
			if (r)
				if (s) round = 1;
				else if (bits) 
					if (bits > 32)
						round = (long) (x.l[1] << bits - 33) < 0;
					else
						round = (long) (x.l[0] << bits - 1) < 0;
			break;
		case C_POS_INF:
			round = 1 - value->m_sign;
			break;
		case C_NEG_INF:
			round = value->m_sign;
			break;
		};
		value->round = 0;
		value->sticky = 0;
		if (bits > 32) x.l[1] &= ~0 << 64 - bits;
		else {
			x.l[1] = 0;
			x.l[0] &= ~0 << 32 - bits;
		};
		if (round) {
			c = 0;
			if (bits > 32) {
				if (!(x.l[1] += 1 << 64 - bits)) 
					if (!++x.l[0]) c++;
			} else if (!(x.l[0] += 1 << 32 - bits)) c++;
			if (c) {
				value->exp++;
				x.i[0] = 0x8000;
			};
		};
		value->m[0] = x.i[0];
		value->m[1] = x.i[1];
		value->m[2] = x.i[2];
		value->m[3] = x.i[3];
	} else *inx = 0;
	return;
}

static void round80(value,rnd,bits,inx) float80 *value; int rnd,bits,*inx;
{
	short round,r,s,c;
	union {
		int80 i;
		unsigned long l[2];
	} x;

	x.i[0] = value->m[0];
	x.i[1] = value->m[1];
	x.i[2] = value->m[2];
	x.i[3] = value->m[3];
	x.i[4] = value->m[4];
	if (value->round || value->sticky || (bits >= 64 ? x.i[4] << bits - 64 :
	    x.i[4] || (bits >= 32 ? x.l[1] << bits - 32 : x.l[1] || 
	    x.l[0] << bits))) {
		*inx = 1;
		round = 0;
		switch (rnd) {
		case C_NEAR:
			if (bits == 80) {
				r = value->round;
				s = value->sticky;
			} else {
				s = value->round || value->sticky;
				if (bits >= 64) {
					r = (short) (x.i[4] << bits - 64) < 0;
					s = s || x.i[4] << bits - 63;
				} else if (bits >= 32) {
					r = (long) (x.l[1] << bits - 32) < 0;
					s = s || x.i[4] || x.l[1] << bits - 31;
				} else {
					r = (long) (x.l[0] << bits) < 0;
					s = s || x.i[4] || x.l[1] || x.l[0] << bits + 1;
				};
			};
			if (r)
				if (s) round = 1;
				else if (bits) 
					if (bits > 64)
						round = (short) (x.i[4] << bits - 65) < 0;
					else if (bits > 32)
						round = (long) (x.l[1] << bits - 33) < 0;
					else
						round = (long) (x.l[0] << bits - 1) < 0;
			break;
		case C_POS_INF:
			round = 1 - value->m_sign;
			break;
		case C_NEG_INF:
			round = value->m_sign;
			break;
		};
		value->round = 0;
		value->sticky = 0;
		if (bits > 64) x.i[4] &= ~0 << 80 - bits;
		else {
			x.i[4] = 0;
			if (bits > 32) x.l[1] &= ~0 << 64 - bits;
			else {
				x.l[1] = 0;
				x.l[0] &= ~0 << 32 - bits;
			};
		};
		if (round) {
			c = 0;
			if (bits > 64) {
				if (!(x.i[4] += 1 << 80 - bits)) 
					if (!++x.l[1]) if (!++x.l[0]) c++;
			} else if (bits > 32) {
				if (!(x.l[1] += 1 << 64 - bits)) 
					if (!++x.l[0]) c++;
			} else if (!(x.l[0] += 1 << 32 - bits)) c++;
			if (c) {
				value->exp++;
				x.i[0] = 0x8000;
			};
		};
		value->m[0] = x.i[0];
		value->m[1] = x.i[1];
		value->m[2] = x.i[2];
		value->m[3] = x.i[3];
		value->m[4] = x.i[4];
	} else *inx = 0;
	return;
}

#ifndef __hp9000s300
static void mult32(x,y) float32 *x,*y;
{
	register unsigned long t0,t1,t2;
	union {
		unsigned long l[2];
		int64 i;
	} res;

	x->exp += y->exp;
	x->m_sign ^= y->m_sign;

	t0 = 0;
	t1 = x->m[0] * y->m[1];
	t2 = x->m[1] * y->m[0];
	if ((t1 += t2) < t2) ++t0;

	res.l[0] = t0;
	res.l[1] = t1;

	res.i[0] = res.i[1];
	res.i[1] = res.i[2];
	res.i[2] = res.i[3];
	res.i[3] = 0;
	
	t0 = x->m[0] * y->m[0] + res.l[0];
	t1 = x->m[1] * y->m[1];
	if ((t1 += res.l[1]) < res.l[1]) ++t0;

	if ((long) t0 > 0) {
		x->exp--;
		t0 <<= 1;
		if ((long) t1 < 0) ++t0;
		t1 <<= 1;
	};

	res.l[0] = t0;
	x->m[0] = res.i[0];
	x->m[1] = res.i[1];
	x->round = (long) t1 < 0;
	x->sticky = t1 & 0x7FFFFFFF ? 1 : 0;

	return;
}
#endif /* ! __hp9000s300 */

#ifndef __hp9000s300
static void mult64(x,y) float64 *x,*y;
{
	register unsigned long t0,t1,t2,t3,t4;
	short ctr;
	union {
		unsigned long l[4];
		int128 i;
	} res;

	x->exp += y->exp;
	x->m_sign ^= y->m_sign;

	t0 = 0;
	t1 = x->m[1] * y->m[0];
	t2 = x->m[0] * y->m[1];
	if ((t1 += t2) < t2) ++t0;
	t2 = x->m[3] * y->m[0];
	t3 = x->m[2] * y->m[1];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	t3 = x->m[1] * y->m[2];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	t3 = x->m[0] * y->m[3];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	t3 = x->m[3] * y->m[2];
	t4 = x->m[2] * y->m[3];
	if ((t3 += t4) < t4) if (!++t2) if (!++t1) ++t0;

	res.l[0] = t0;
	res.l[1] = t1;
	res.l[2] = t2;
	res.l[3] = t3;

	for (ctr = 0; ctr < 7; ctr++) res.i[ctr] = res.i[ctr+1];
	res.i[7] = 0;

	t0 = x->m[0] * y->m[0] + res.l[0];
	t1 = x->m[2] * y->m[0];
	t2 = x->m[1] * y->m[1];
	if ((t1 += t2) < t2) ++t0;
	t2 = x->m[0] * y->m[2];
	if ((t1 += t2) < t2) ++t0;
	if ((t1 += res.l[1]) < res.l[1]) ++t0;
	t2 = x->m[3] * y->m[1];
	t3 = x->m[2] * y->m[2];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	t3 = x->m[1] * y->m[3];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	if ((t2 += res.l[2]) < res.l[2]) if (!++t1) ++t0;
	t3 = x->m[3] * y->m[3];
	if ((t3 += res.l[3]) < res.l[3]) if (!++t2) if (!++t1) ++t0;

	if ((long) t0 > 0) {
		x->exp--;
		t0 <<= 1;
		if ((long) t1 < 0) ++t0;
		t1 <<= 1;
		if ((long) t2 < 0) ++t1;
		t2 <<= 1;
	};

	res.l[0] = t0;
	res.l[1] = t1;
	x->m[0] = res.i[0];
	x->m[1] = res.i[1];
	x->m[2] = res.i[2];
	x->m[3] = res.i[3];
	x->round = (long) t2 < 0;
	x->sticky = t3 || t2 & 0x7FFFFFFF;

	return;
}
#endif /* ! __hp9000s300 */

#ifndef __hp9000s300
static void mult80(x,y) float80 *x,*y;
{
	register unsigned long t0,t1,t2,t3,t4,t5;
	short ctr;
	union {
		unsigned long l[5];
		int160 s;
	} res;

	x->exp += y->exp;
	x->m_sign ^= y->m_sign;

	t0 = 0;
	t1 = x->m[1] * y->m[0];
	t2 = x->m[0] * y->m[1];
	if ((t1 += t2) < t2) ++t0;
	t2 = x->m[2] * y->m[1];
	t3 = x->m[1] * y->m[2];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	t3 = x->m[3] * y->m[0];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	t3 = x->m[0] * y->m[3];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	t3 = x->m[3] * y->m[2];
	t4 = x->m[2] * y->m[3];
	if ((t3 += t4) < t4) if (!++t2) if (!++t1) ++t0;
	t4 = x->m[4] * y->m[1];
	if ((t3 += t4) < t4) if (!++t2) if (!++t1) ++t0;
	t4 = x->m[1] * y->m[4];
	if ((t3 += t4) < t4) if (!++t2) if (!++t1) ++t0;
	t4 = x->m[4] * y->m[3];
	t5 = x->m[3] * y->m[4];
	if ((t4 += t5) < t5) if (!++t3) if (!++t2) if (!++t1) ++t0;

	res.l[0] = t0;
	res.l[1] = t1;
	res.l[2] = t2;
	res.l[3] = t3;
	res.l[4] = t4;

	for (ctr = 0; ctr < 9; ctr++) res.s[ctr] = res.s[ctr+1];
	res.s[9] = 0;

	t0 = x->m[0] * y->m[0] + res.l[0];
	t1 = x->m[2] * y->m[0];
	t2 = x->m[1] * y->m[1];
	if ((t1 += t2) < t2) ++t0;
	t2 = x->m[0] * y->m[2];
	if ((t1 += t2) < t2) ++t0;
	if ((t1 += res.l[1]) < res.l[1]) ++t0;
	t2 = x->m[4] * y->m[0];
	t3 = x->m[3] * y->m[1];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	t3 = x->m[2] * y->m[2];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	t3 = x->m[1] * y->m[3];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	t3 = x->m[0] * y->m[4];
	if ((t2 += t3) < t3) if (!++t1) ++t0;
	if ((t2 += res.l[2]) < res.l[2]) if (!++t1) ++t0;
	t3 = x->m[4] * y->m[2];
	t4 = x->m[3] * y->m[3];
	if ((t3 += t4) < t4) if (!++t2) if (!++t1) ++t0;
	t4 = x->m[2] * y->m[4];
	if ((t3 += t4) < t4) if (!++t2) if (!++t1) ++t0;
	if ((t3 += res.l[3]) < res.l[3]) if (!++t2) if (!++t1) ++t0;
	t4 = x->m[4] * y->m[4];
	if ((t4 += res.l[4]) < res.l[4]) if (!++t3) if (!++t2) if (!++t1) ++t0;

	if ((long) t0 > 0) {
		x->exp--;
		t0 <<= 1;
		if ((long) t1 < 0) ++t0;
		t1 <<= 1;
		if ((long) t2 < 0) ++t1;
		t2 <<= 1;
	};

	res.l[0] = t0;
	res.l[1] = t1;
	res.l[2] = t2;
	for (ctr = 0; ctr < 5; ctr++) x->m[ctr] = res.s[ctr];
	x->round = (short) res.s[5] < 0;
	x->sticky = t3 || t4 || res.s[5] & 0x7FFF;

	return;
}
#endif /* ! __hp9000s300 */

static void div32(x,y) float32 *x,*y;
{
	short ctr,msb = 0,borrow;
	union {
		unsigned long l;
		unsigned short s[2];
	} q,w,t0,t1;
	union {
		unsigned long l[2];
		unsigned short s[4];
	} d,p;
	unsigned short r[3];

	x->m_sign ^= y->m_sign;
	x->exp -= y->exp;
	w.s[0] = y->m[0];
	w.s[1] = y->m[1];
	d.s[0] = x->m[0];
	d.s[1] = x->m[1];
	d.l[1] = 0;
	for (ctr = 0; ctr < 3; ctr++) {
		q.l = d.l[0] / w.s[0];
		t0.l = w.s[0] * q.s[1];
		t1.l = w.s[1] * q.s[0];
		p.s[0] = (t0.l += t1.l) < t1.l;
		p.s[1] = t0.s[0];
		p.s[2] = t0.s[1];
		p.s[3] = 0;
		t0.l = w.s[0] * q.s[0];
		t1.l = w.s[1] * q.s[1];
		if ((p.l[1] += t1.l) < t1.l) ++p.l[0];
		p.l[0] += t0.l;
		d.s[3] = d.s[2];
		d.s[2] = d.s[1];
		d.s[1] = d.s[0];
		d.s[0] = 0;
		borrow = 0;
		if (d.l[1] < p.l[1]) if (!d.l[0]--) borrow = 1;
		d.l[1] -= p.l[1];
		if (d.l[0] < p.l[0]) borrow = 1;
		d.l[0] -= p.l[0];
		while (borrow) {
			if ((d.l[1] += w.l) < w.l) if (!++d.l[0]) borrow = 0;
			q.l--;
		};
		if (q.s[0]) msb = 1;
		r[ctr] = q.s[1];
		d.l[0] = d.l[1];
		d.l[1] = 0;
	};
	if (d.l[0] || d.l[1]) r[2] |= 2;
	if (msb) {
		x->exp++;
		if (r[2] & 1) r[2] |= 2;
		r[2] >>= 1;
		if (r[1] & 1) r[2] |= 0x8000;
		r[1] >>= 1;
		if (r[0] & 1) r[1] |= 0x8000;
		r[0] >>= 1;
		r[0] |= 0x8000;
	};
	x->m[0] = r[0];
	x->m[1] = r[1];
	x->round = (short) r[2] < 0;
	x->sticky = r[2] & 0x7FFF ? 1 : 0;
	return;
}

static void div64(x,y) float64 *x,*y;
{
	short ctr1,ctr2,ctr3;
	union {
		unsigned short s[7];
		unsigned long l[3];
	} a;
	union {
		unsigned short s[4];
		unsigned long l[2];
	} b;
	union {
		unsigned short s[6];
		unsigned long l[3];
	} p;
	union {
		unsigned short s[2];
		unsigned long l;
	} q;
	unsigned short r[6];
	unsigned long t1;

	x->exp -= y->exp;
	x->m_sign ^= y->m_sign;
	for (ctr1 = 0; ctr1 < 4; ctr1++) {
		a.s[ctr1 + 2] = x->m[ctr1];
		b.s[ctr1] = y->m[ctr1];
	};
	if (!(r[0] = a.l[1] > b.l[0]))
		if (a.l[1] == b.l[0]) r[0] = a.l[2] >= b.l[1];
	if (r[0]) {
		if (b.l[1] > a.l[2]) a.l[1]--;
		a.l[2] -= b.l[1];
		a.l[1] -= b.l[0];
	};
	a.l[0] = 0;
	for (ctr1 = 1; ctr1 < 6; ctr1++) {
		if ((q.l = a.l[1] / (unsigned long) b.s[0]) > 0xFFFF)
			q.l = 0xFFFF;
		p.l[0] = (unsigned long) q.s[1] * (unsigned long) b.s[1];
		p.l[1] = (unsigned long) q.s[1] * (unsigned long) b.s[3];
		for (ctr2 = 4; ctr2 > 0; ctr2--) p.s[ctr2] = p.s[ctr2 - 1];
		p.s[0] = 0;
		t1 = (unsigned long) q.s[1] * (unsigned long) b.s[2];
		if ((p.l[1] += t1) < t1) ++p.l[0];
		p.l[0] += (unsigned long) q.s[1] * (unsigned long) b.s[0];
		if (p.s[4]) if (!a.l[2]--) if (!a.l[1]--) a.l[0]--;
		a.s[6] = -p.s[4];
		if (p.l[1] > a.l[2]) if (!a.l[1]--) a.l[0]--;
		a.l[2] -= p.l[1];
		if (p.l[0] > a.l[1]) a.l[0]--;
		a.l[1] -= p.l[0];
		while (a.l[0]) {
			q.l--;
			for (ctr2 = 3; ctr2 >= 0; ctr2--)
				if ((a.s[ctr2 + 3] += b.s[ctr2]) < b.s[ctr2])
					for (ctr3 = ctr2 + 2; ctr3 >= 0; ctr3--)
						if (++a.s[ctr3]) break;
		};
		r[ctr1] = q.s[1];
		for (ctr2 = 2; ctr2 < 6; ctr2++) a.s[ctr2] = a.s[ctr2 + 1];
	};
	if (a.l[1] || a.l[2] || r[5] & 1) r[5] |= 2;
	if (r[0]) {
		x->exp++;
		for (ctr1 = 5; ctr1 > 0; ctr1--) {
			r[ctr1] >>= 1;
			if (r[ctr1 - 1] & 1) r[ctr1] |= 0x8000;
		};
	};
	for (ctr1 = 0; ctr1 < 4; ctr1++) x->m[ctr1] = r[ctr1 + 1];
	x->round = (short) r[5] < 0;
	x->sticky = r[5] & 0x7FFF ? 1 : 0;
	return;
};

static void div80(x,y) float80 *x,*y;
{
	short ctr1,ctr2,ctr3;
	union {
		unsigned short s[8];
		unsigned long l[4];
	} a;
	union {
		unsigned short s[5];
		unsigned long l[2];
	} b;
	union {
		unsigned short s[6];
		unsigned long l[3];
	} p;
	union {
		unsigned short s[2];
		unsigned long l;
	} q;
	unsigned short r[7];
	unsigned long t1;

	x->exp -= y->exp;
	x->m_sign ^= y->m_sign;
	for (ctr1 = 0; ctr1 < 5; ctr1++) {
		a.s[ctr1 + 2] = x->m[ctr1];
		b.s[ctr1] = y->m[ctr1];
	};
	if (!(r[0] = a.l[1] > b.l[0]))
		if (a.l[1] == b.l[0]) 
			if (!(r[0] = a.l[2] > b.l[1]))
				if (a.l[2] == b.l[1]) 
					r[0] = a.s[6] >= b.s[4];
	if (r[0]) {
		if (b.s[4] > a.s[6]) if (!a.l[2]--) a.l[1]--;
		a.s[6] -= b.s[4];
		if (b.l[1] > a.l[2]) a.l[1]--;
		a.l[2] -= b.l[1];
		a.l[1] -= b.l[0];
	};
	a.l[0] = 0;
	for (ctr1 = 1; ctr1 < 7; ctr1++) {
		a.s[7] = 0;
		if ((q.l = a.l[1] / (unsigned long) b.s[0]) > 0xFFFF)
			q.l = 0xFFFF;
		p.l[1] = (unsigned long) q.s[1] * (unsigned long) b.s[1];
		p.l[2] = (unsigned long) q.s[1] * (unsigned long) b.s[3];
		for (ctr2 = 1; ctr2 <= 4; ctr2++) p.s[ctr2] = p.s[ctr2 + 1];
		p.s[0] = 0;
		p.s[5] = 0;
		t1 = (unsigned long) q.s[1] * (unsigned long) b.s[4];
		if ((p.l[2] += t1) < t1) if (!++p.l[1]) ++p.l[0];
		t1 = (unsigned long) q.s[1] * (unsigned long) b.s[2];
		if ((p.l[1] += t1) < t1) ++p.l[0];
		p.l[0] += (unsigned long) q.s[1] * (unsigned long) b.s[0];
		if (p.l[2] > a.l[3]) if (!a.l[2]--) if (!a.l[1]--) a.l[0]--;
		a.l[3] -= p.l[2];
		if (p.l[1] > a.l[2]) if (!a.l[1]--) a.l[0]--;
		a.l[2] -= p.l[1];
		if (p.l[0] > a.l[1]) a.l[0]--;
		a.l[1] -= p.l[0];
		while (a.l[0]) {
			q.l--;
			for (ctr2 = 4; ctr2 >= 0; ctr2--)
				if ((a.s[ctr2 + 3] += b.s[ctr2]) < b.s[ctr2])
					for (ctr3 = ctr2 + 2; ctr3 >= 0; ctr3--)
						if (++a.s[ctr3]) break;
		};
		r[ctr1] = q.s[1];
		for (ctr2 = 2; ctr2 < 7; ctr2++) a.s[ctr2] = a.s[ctr2 + 1];
	};
	if (a.l[1] || a.l[2] || a.l[3] || r[6] & 1) r[6] |= 2;
	if (r[0]) {
		x->exp++;
		for (ctr1 = 6; ctr1 > 0; ctr1--) {
			r[ctr1] >>= 1;
			if (r[ctr1 - 1] & 1) r[ctr1] |= 0x8000;
		};
	};
	for (ctr1 = 0; ctr1 < 5; ctr1++) x->m[ctr1] = r[ctr1 + 1];
	x->round = (short) r[6] < 0;
	x->sticky = r[6] & 0x7FFF ? 1 : 0;
	return;
};
