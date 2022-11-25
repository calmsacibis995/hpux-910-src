/* @(#) $Revision: 70.4 $ */      

#include "Ktty.h"
#include "basic.h"

extern char kdb_interrupt;

/*
 * This table translates a cooked HIL keycode to ASCII.
 * Note that break translates as ^C (the 3,3 entry) which is not really right.
 *
 * A zero entry means that there is no translation.
 */
struct hil_codes {
	unsigned char unshifted, shifted;
} codes[] = {
	  0,0,   '`','~','\\','|',  27,127,   0,0,     3,3,     0,0,     0,0,
	  0,0,  '\t','\t',  0,0,     0,0,     0,0,     0,0,     0,0,     0,0,
	  0,0,     0,0,     0,0,     0,0,     0,0,     0,0,     0,0,     0,0,
	  0,0,  '\t','\t',  0,0,     0,0,     0,0,     0,0,     0,0,     0,0,
	  0,0,     0,0,     0,0,     0,0,     0,0,     0,0,     0,0,     0,0,
	  0,0,     0,0,     0,0,     0,0,     0,0,     0,0,  '\b','\b',  0,0,
	  0,0,     0,0,     0,0,     0,0,     0,0,     0,0,     0,0,     0,0,
	  0,0,  '\r','\r',  0,0,     0,0,   '0','0', '.','.', ',',',', '+','+',
	'1','1', '2','2', '3','3', '-','-', '4','4', '5','5', '6','6', '*','*',
	'7','7', '8','8', '9','9', '/','/',   0,0,     0,0,     0,0,     0,0,
	'1','!', '2','@', '3','#', '4','$', '5','%', '6','^', '7','&', '8','*',
	'9','(', '0',')', '-','_', '=','+', '[','{', ']','}', ';',':', '\'','"',
	',','<', '.','>', '/','?', ' ',' ', 'o','O', 'p','P', 'k','K', 'l','L',
	'q','Q', 'w','W', 'e','E', 'r','R', 't','T', 'y','Y', 'u','U', 'i','I',
	'a','A', 's','S', 'd','D', 'f','F', 'g','G', 'h','H', 'j','J', 'm','M',
	'z','Z', 'x','X', 'c','C', 'v','V', 'b','B', 'n','N',   0,0,     0,0,
};

/*
 * This table translates raw HIL keycodes to cooked keycodes.
 * Use it like this: raw_to_cooked[raw>>1] because the low-order bit
 * of the raw keycode indicates up/down, and cooked keycodes correspond
 * only to the down stroke.
 *
 * An entry of -1 means that there is no translation.
 */
int raw_to_cooked[128] = {
	0x82, -001, -001, -001, -001, -001, -001, 0x05,
	0x44, 0x49, 0x45, 0x4a, 0x46, 0x48, 0x3e, 0x08,
	0x40, 0x4b, 0x41, 0x3f, 0x42, 0x47, 0x3c, 0x43,
	0x7c, 0x7b, 0x7a, 0x79, 0x78, -001, -001, 0x03,
	-001, 0x0b, -001, 0x0c, 0x3d, 0x0a, 0x09, 0x0d,
	0x75, 0x74, 0x73, 0x72, 0x71, 0x70, -001, 0x18,
	0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x69, 0x68, 0x19,
	0x56, 0x55, 0x54, 0x53, 0x52, 0x51, 0x50, 0x01,
	-001, -001, -001, -001, -001, -001, -001, -001,
	0x15, 0x21, 0x20, 0x1c, 0x1b, -001, 0x06, 0x11,
	0x14, 0x1d, 0x1e, 0x1f, 0x24, -001, 0x16, 0x17,
	0x57, 0x58, 0x59, 0x5a, 0x5b, 0x2e, 0x28, 0x29,
	0x6f, 0x64, 0x65, 0x5c, 0x5d, 0x02, 0x2b, 0x2c,
	0x76, 0x66, 0x67, 0x5e, 0x5f, 0x39, 0x0e, 0x0f,
	0x77, 0x60, 0x61, 0x62, 0xaa, 0x07, 0xab, 0x10,
	0x7d, 0x63, -001, -001, 0x26, 0x22, 0x23, 0x27,
};


/*
 * Read from the HIL keyboard.
 * Return an ASCII key, or 0 if no key was pressed.
 */

int
hil_read()
{
	extern int current_io_base;
	register unsigned char *hil_status, *hil_data;
	register int status, data, key;
	register struct hil_codes *h;
	static int initialized=0;
	static int raw_key=0, shift, control, buflen=0;
	static char inbuf[10];

	/* Read status first, because reading data clears the interrupt. */
	hil_status = (unsigned char *) current_io_base+0x428003;
	hil_data   = (unsigned char *) current_io_base+0x428001;

	if (!initialized) {
		int i;

		while (*hil_status & 0x02);	/* wait for old commands */
		*hil_status = 0xeb;		/* write control register */
		while (*hil_status & 0x02);	/* wait for old commands */
		*hil_data = 0x93;		/* Put this in cntrl reg */
		/* wait for reconfig data */
		for (i = 200000; i > 0  && !(*hil_status & 0x01); i--);
		data = *hil_data;

		while (*hil_status & 0x02);	/* wait for old commands */
		*hil_status = 0x5c;		/* interrupts on */

		initialized=1;
	}

	status = *hil_status;
	if ((status & 01)==0)
		return 0;		/* nothing to read */
	data = *hil_data;
	switch (status>>4) {
	case 5:				/* HIL status */
		if (data != 0x18) {
			buflen=0;
			return 0;
		}

		if (buflen!=2 || inbuf[0] != 0x40)
			return 0;
		raw_key = inbuf[1];

		switch (raw_key) {
		case 0x08: case 0x0a: shift=1; return 0;     /* left shift */
		case 0x09: case 0x0b: shift=0; return 0;     /* right shift */
		case 0x00: case 0x0c: control=1; return 0;   /* left control */
		case 0x01: case 0x0d: control=0; return 0;   /* right control */
		default:
			if (raw_key & 01)	/* up-stroke? */
				return 0;	/* ignore it. */
			data=raw_to_cooked[(raw_key>>1) & 0x7f];
			if (data == -1)		/* invalid key */
				return 0;
		}
		break;
	case 6:				/* HIL data from a raw keyboard */
		if (buflen < sizeof(inbuf)-1)		/* does it fit? */
			inbuf[buflen++] = data;		/* save the data */
		return 0;
	case 8:		/* Cooked key - shift and control */
	case 9:		/* Cooked key - control only */
	case 10:	/* Cooked key - shift only */
	case 11:	/* Cooked key - no shift, no control */
		shift = !(status & 0x10);
		control = !(status & 0x20);
		break;
	default:
		/* Strange keycode */
		printf("unexpected hil: status=%x data=%x\n", status, data);
		return 0;
	}


	h = &codes[data & 0x7f];		/* get unshift/shifted ASCII */
	key = shift ? h->shifted : h->unshifted;
	if (control)
		key &= 0x1f;
	if (key=='\03')				/* map ^C to interrupt */
		kdb_interrupt = true;
	return key;
}
