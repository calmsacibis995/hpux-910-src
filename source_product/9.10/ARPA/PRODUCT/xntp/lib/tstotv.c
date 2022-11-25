/* $Header: tstotv.c,v 1.2.109.2 94/10/31 10:55:01 mike Exp $
 * tstotv - tables for converting from NTP time stamps to struct timeval
 */

# include "ntp_types.h"

/*
 * Tables to convert from a time stamp fraction to usecs.  Note that
 * the units of these tables are actually (usec<<3).  We carry three
 * guard bits so that the result can be properly truncated (or rounded)
 * to be correct to the least significant bit.
 *
 * These tables are rounded.
 */

LONG    tstoushi[256] =
{
    0x000000, 0x007a12, 0x00f424, 0x016e36,
	0x01e848, 0x02625a, 0x02dc6c, 0x03567e,
	0x03d090, 0x044aa2, 0x04c4b4, 0x053ec6,
	0x05b8d8, 0x0632ea, 0x06acfc, 0x07270e,
	0x07a120, 0x081b32, 0x089544, 0x090f56,
	0x098968, 0x0a037a, 0x0a7d8c, 0x0af79e,
	0x0b71b0, 0x0bebc2, 0x0c65d4, 0x0cdfe6,
	0x0d59f8, 0x0dd40a, 0x0e4e1c, 0x0ec82e,
	0x0f4240, 0x0fbc52, 0x103664, 0x10b076,
	0x112a88, 0x11a49a, 0x121eac, 0x1298be,
	0x1312d0, 0x138ce2, 0x1406f4, 0x148106,
	0x14fb18, 0x15752a, 0x15ef3c, 0x16694e,
	0x16e360, 0x175d72, 0x17d784, 0x185196,
	0x18cba8, 0x1945ba, 0x19bfcc, 0x1a39de,
	0x1ab3f0, 0x1b2e02, 0x1ba814, 0x1c2226,
	0x1c9c38, 0x1d164a, 0x1d905c, 0x1e0a6e,
	0x1e8480, 0x1efe92, 0x1f78a4, 0x1ff2b6,
	0x206cc8, 0x20e6da, 0x2160ec, 0x21dafe,
	0x225510, 0x22cf22, 0x234934, 0x23c346,
	0x243d58, 0x24b76a, 0x25317c, 0x25ab8e,
	0x2625a0, 0x269fb2, 0x2719c4, 0x2793d6,
	0x280de8, 0x2887fa, 0x29020c, 0x297c1e,
	0x29f630, 0x2a7042, 0x2aea54, 0x2b6466,
	0x2bde78, 0x2c588a, 0x2cd29c, 0x2d4cae,
	0x2dc6c0, 0x2e40d2, 0x2ebae4, 0x2f34f6,
	0x2faf08, 0x30291a, 0x30a32c, 0x311d3e,
	0x319750, 0x321162, 0x328b74, 0x330586,
	0x337f98, 0x33f9aa, 0x3473bc, 0x34edce,
	0x3567e0, 0x35e1f2, 0x365c04, 0x36d616,
	0x375028, 0x37ca3a, 0x38444c, 0x38be5e,
	0x393870, 0x39b282, 0x3a2c94, 0x3aa6a6,
	0x3b20b8, 0x3b9aca, 0x3c14dc, 0x3c8eee,
	0x3d0900, 0x3d8312, 0x3dfd24, 0x3e7736,
	0x3ef148, 0x3f6b5a, 0x3fe56c, 0x405f7e,
	0x40d990, 0x4153a2, 0x41cdb4, 0x4247c6,
	0x42c1d8, 0x433bea, 0x43b5fc, 0x44300e,
	0x44aa20, 0x452432, 0x459e44, 0x461856,
	0x469268, 0x470c7a, 0x47868c, 0x48009e,
	0x487ab0, 0x48f4c2, 0x496ed4, 0x49e8e6,
	0x4a62f8, 0x4add0a, 0x4b571c, 0x4bd12e,
	0x4c4b40, 0x4cc552, 0x4d3f64, 0x4db976,
	0x4e3388, 0x4ead9a, 0x4f27ac, 0x4fa1be,
	0x501bd0, 0x5095e2, 0x510ff4, 0x518a06,
	0x520418, 0x527e2a, 0x52f83c, 0x53724e,
	0x53ec60, 0x546672, 0x54e084, 0x555a96,
	0x55d4a8, 0x564eba, 0x56c8cc, 0x5742de,
	0x57bcf0, 0x583702, 0x58b114, 0x592b26,
	0x59a538, 0x5a1f4a, 0x5a995c, 0x5b136e,
	0x5b8d80, 0x5c0792, 0x5c81a4, 0x5cfbb6,
	0x5d75c8, 0x5defda, 0x5e69ec, 0x5ee3fe,
	0x5f5e10, 0x5fd822, 0x605234, 0x60cc46,
	0x614658, 0x61c06a, 0x623a7c, 0x62b48e,
	0x632ea0, 0x63a8b2, 0x6422c4, 0x649cd6,
	0x6516e8, 0x6590fa, 0x660b0c, 0x66851e,
	0x66ff30, 0x677942, 0x67f354, 0x686d66,
	0x68e778, 0x69618a, 0x69db9c, 0x6a55ae,
	0x6acfc0, 0x6b49d2, 0x6bc3e4, 0x6c3df6,
	0x6cb808, 0x6d321a, 0x6dac2c, 0x6e263e,
	0x6ea050, 0x6f1a62, 0x6f9474, 0x700e86,
	0x708898, 0x7102aa, 0x717cbc, 0x71f6ce,
	0x7270e0, 0x72eaf2, 0x736504, 0x73df16,
	0x745928, 0x74d33a, 0x754d4c, 0x75c75e,
	0x764170, 0x76bb82, 0x773594, 0x77afa6,
	0x7829b8, 0x78a3ca, 0x791ddc, 0x7997ee
};

LONG    tstousmid[256] =
{
    0x0000, 0x007a, 0x00f4, 0x016e, 0x01e8, 0x0262, 0x02dc, 0x0356,
	0x03d1, 0x044b, 0x04c5, 0x053f, 0x05b9, 0x0633, 0x06ad, 0x0727,
	0x07a1, 0x081b, 0x0895, 0x090f, 0x0989, 0x0a03, 0x0a7e, 0x0af8,
	0x0b72, 0x0bec, 0x0c66, 0x0ce0, 0x0d5a, 0x0dd4, 0x0e4e, 0x0ec8,
	0x0f42, 0x0fbc, 0x1036, 0x10b0, 0x112b, 0x11a5, 0x121f, 0x1299,
	0x1313, 0x138d, 0x1407, 0x1481, 0x14fb, 0x1575, 0x15ef, 0x1669,
	0x16e3, 0x175d, 0x17d8, 0x1852, 0x18cc, 0x1946, 0x19c0, 0x1a3a,
	0x1ab4, 0x1b2e, 0x1ba8, 0x1c22, 0x1c9c, 0x1d16, 0x1d90, 0x1e0a,
	0x1e84, 0x1eff, 0x1f79, 0x1ff3, 0x206d, 0x20e7, 0x2161, 0x21db,
	0x2255, 0x22cf, 0x2349, 0x23c3, 0x243d, 0x24b7, 0x2531, 0x25ac,
	0x2626, 0x26a0, 0x271a, 0x2794, 0x280e, 0x2888, 0x2902, 0x297c,
	0x29f6, 0x2a70, 0x2aea, 0x2b64, 0x2bde, 0x2c59, 0x2cd3, 0x2d4d,
	0x2dc7, 0x2e41, 0x2ebb, 0x2f35, 0x2faf, 0x3029, 0x30a3, 0x311d,
	0x3197, 0x3211, 0x328b, 0x3306, 0x3380, 0x33fa, 0x3474, 0x34ee,
	0x3568, 0x35e2, 0x365c, 0x36d6, 0x3750, 0x37ca, 0x3844, 0x38be,
	0x3938, 0x39b3, 0x3a2d, 0x3aa7, 0x3b21, 0x3b9b, 0x3c15, 0x3c8f,
	0x3d09, 0x3d83, 0x3dfd, 0x3e77, 0x3ef1, 0x3f6b, 0x3fe5, 0x405f,
	0x40da, 0x4154, 0x41ce, 0x4248, 0x42c2, 0x433c, 0x43b6, 0x4430,
	0x44aa, 0x4524, 0x459e, 0x4618, 0x4692, 0x470c, 0x4787, 0x4801,
	0x487b, 0x48f5, 0x496f, 0x49e9, 0x4a63, 0x4add, 0x4b57, 0x4bd1,
	0x4c4b, 0x4cc5, 0x4d3f, 0x4db9, 0x4e34, 0x4eae, 0x4f28, 0x4fa2,
	0x501c, 0x5096, 0x5110, 0x518a, 0x5204, 0x527e, 0x52f8, 0x5372,
	0x53ec, 0x5466, 0x54e1, 0x555b, 0x55d5, 0x564f, 0x56c9, 0x5743,
	0x57bd, 0x5837, 0x58b1, 0x592b, 0x59a5, 0x5a1f, 0x5a99, 0x5b13,
	0x5b8d, 0x5c08, 0x5c82, 0x5cfc, 0x5d76, 0x5df0, 0x5e6a, 0x5ee4,
	0x5f5e, 0x5fd8, 0x6052, 0x60cc, 0x6146, 0x61c0, 0x623a, 0x62b5,
	0x632f, 0x63a9, 0x6423, 0x649d, 0x6517, 0x6591, 0x660b, 0x6685,
	0x66ff, 0x6779, 0x67f3, 0x686d, 0x68e7, 0x6962, 0x69dc, 0x6a56,
	0x6ad0, 0x6b4a, 0x6bc4, 0x6c3e, 0x6cb8, 0x6d32, 0x6dac, 0x6e26,
	0x6ea0, 0x6f1a, 0x6f94, 0x700f, 0x7089, 0x7103, 0x717d, 0x71f7,
	0x7271, 0x72eb, 0x7365, 0x73df, 0x7459, 0x74d3, 0x754d, 0x75c7,
	0x7641, 0x76bc, 0x7736, 0x77b0, 0x782a, 0x78a4, 0x791e, 0x7998
};

LONG    tstouslo[128] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e,
	0x1f, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
	0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
	0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x33, 0x34,
	0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c,
	0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44,
	0x45, 0x46, 0x47, 0x48, 0x48, 0x49, 0x4a, 0x4b,
	0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53,
	0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b,
	0x5c, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62,
	0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
	0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x71,
	0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79
};
