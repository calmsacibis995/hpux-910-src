#include "quad.h"

/***	The constants of specifying exponential region:		      ***/
 
int _e[NSR] = { 2560, 1280, 640, 320, 160, 80, 40, 20, 10, 9,
		  8,    7,   6,   5,   4,  3,  2,  1,  1
	     };

/***	Quad precision floating point constants:		      ***/
/***	1E2560, 1E1280, 1E640, 1E320, 1E160, 1E80, 1E40, 1E20, 1E10,  ***/
/***	1E9,    1E8,    1E7,   1E6,   1E5,   1E4,  1E3,  1E2,  1E1,   ***/
/***	1E1							      ***/

QUIN _q0[NSR] =
        { { 0x6137194A, 0xA9804143, 0xE14CBBFD, 0xA13244D0, 0x140D5DF0 },
          { 0x509B0C59, 0x181DD70A, 0xDD9610B7, 0x99AB80BD, 0xD3F3D9FE },
          { 0x484D0619, 0xEF23CA95, 0x3A392DC9, 0x5915C5E1, 0x4C3A5D38 },
          { 0x44260308, 0x5E53E599, 0xC6EBCD42, 0x2B0601A8, 0xCC80439D },
          { 0x42126C2D, 0x4256FFCC, 0x2F54AEF7, 0x30D6629A, 0xC0124703 },
          { 0x4108AFCE, 0xF51F0FB5, 0xEFF7B866, 0xE8BD92F7, 0xD20C6531 },
          { 0x4083D632, 0x9F1C35CA, 0x4BFABB9F, 0x56100000,          0 },
          { 0x40415AF1, 0xD78B58C4,          0,          0,          0 },
          { 0x40202A05, 0xF2000000,          0,          0,          0 },
          { 0x401CDCD6, 0x50000000,          0,          0,          0 },
          { 0x40197D78, 0x40000000,          0,          0,          0 },
          { 0x4016312D,          0,          0,          0,          0 },
          { 0x4012E848,          0,          0,          0,          0 },
          { 0x400F86A0,          0,          0,          0,          0 },
          { 0x400C3880,          0,          0,          0,          0 },
          { 0x4008F400,          0,          0,          0,          0 },
          { 0x40059000,          0,          0,          0,          0 },
          { 0x40024000,          0,          0,          0,          0 },
          { 0x40024000,          0,          0,          0,          0 }
        };

/***	The other specific QUIN precision floating point conatants:   ***/
/***	1E-2560, 1E-1280, 1E-640, 1E-320, 1E-160, 1E-80, 1E-40,       ***/
/***	1E-20,   1E-10,   1E-9,   1E-8,   1E-7,   1E-6,  1E-5,        ***/
/***	1E-4,    1E-3,    1E-2,   1E-1,   1E-1			      ***/

QUIN _q1[NSR] =
        { { 0x1EC6D1F6, 0xFB85BD81, 0x4808B31D, 0xD8FA82EA, 0x9812B5A0 },
          { 0x2F62E870, 0xBA12EBDB, 0x757C3E97, 0xECCA79A7, 0x72EFBBDC },
          { 0x37B0F414, 0xD9B710E2, 0x68F9A09D, 0xCBDF8C1A, 0x7C92225D },
          { 0x3BD7FA01, 0x712E8F04, 0x71A11241, 0x61312AAA, 0x4569573F },
          { 0x3DEB67E9, 0xC127B6E7, 0x4126B3DA, 0x42CECAD2, 0x1EAD1FCB },
          { 0x3EF52F8A, 0xC174D612, 0x334BB99B, 0x0F3F92CF, 0xA834043B },
          { 0x3F7A16C2, 0x62777579, 0xC58C4647, 0x5896767B, 0x402A76C5 },
          { 0x3FBC79CA, 0x10C92422, 0x35D511E9, 0x76394D79, 0xEB08303D },
          { 0x3FDDB7CD, 0xFD9D7BDB, 0xAB7D6AE6, 0x881CB510, 0x9A365F7E },
          { 0x3FE112E0, 0xBE826D69, 0x4B2E62D0, 0x1511F12A, 0x6061FBAE },
          { 0x3FE45798, 0xEE2308C3, 0x9DF9FB84, 0x1A566D74, 0xF87A7A9A },
          { 0x3FE7AD7F, 0x29ABCAF4, 0x85787A65, 0x20EC08D2, 0x36991941 },
          { 0x3FEB0C6F, 0x7A0B5ED8, 0xD36B4C7F, 0x34938583, 0x621FAFC8 },
          { 0x3FEE4F8B, 0x588E368F, 0x08461F9F, 0x01B866E4, 0x3AA79BBA },
          { 0x3FF1A36E, 0x2EB1C432, 0xCA57A786, 0xC226809D, 0x495182A9 },
          { 0x3FF50624, 0xDD2F1A9F, 0xBE76C8B4, 0x39581062, 0x4DD2F1A9 },
          { 0x3FF847AE, 0x147AE147, 0xAE147AE1, 0x47AE147A, 0xE147AE14 },
          { 0x3FFB9999, 0x99999999, 0x99999999, 0x99999999, 0x99999999 },
          { 0x3FFB9999, 0x99999999, 0x99999999, 0x99999999, 0x99999999 }
        };

/***	The patterns for masking specific bits			      ***/

unsigned  _a[LENGTH] = { 0x00000001, 0x00000002, 0x00000004, 0x00000008,
		        0x00000010, 0x00000020, 0x00000040, 0x00000080,
		        0x00000100, 0x00000200, 0x00000400, 0x00000800,
		        0x00001000, 0x00002000, 0x00004000, 0x00008000,
		        0x00010000, 0x00020000, 0x00040000, 0x00080000,
		        0x00100000, 0x00200000, 0x00400000, 0x00800000,
		        0x01000000, 0x02000000, 0x04000000, 0x08000000,
		        0x10000000, 0x20000000, 0x40000000, 0x80000000
		      };

/***	The constants being used for left shifting		      ***/

unsigned _ls[LENGTH] = { 0x80000000, 0xc0000000, 0xe0000000, 0xf0000000,
		        0xf8000000, 0xfc000000, 0xfe000000, 0xff000000,
		        0xff800000, 0xffc00000, 0xffe00000, 0xfff00000,
		        0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000,
		        0xffff8000, 0xffffc000, 0xffffe000, 0xfffff000,
		        0xfffff800, 0xfffffc00, 0xfffffe00, 0xffffff00,
		        0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0,
		        0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff
		      };


/***	Error messages						      ***/
/***  	Not currently used.  Commented out until needed	- ets 2/89    ***/

#ifdef __FUTURE__
char _err0[] = "INVALID OPERATION -- ";
char _err1[] = "Input illegal character !";
char _err2[] = "Illegal floating point format( INFINITY or NaN ) !";
#endif __FUTURE__
