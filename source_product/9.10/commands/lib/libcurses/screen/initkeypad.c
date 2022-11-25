/* @(#) $Revision: 72.1 $ */      
#include "curses.ext"

#ifdef	 	KEYPAD
static struct map *_addone();
/*
 * Make up the needed array of map structures for dealing with the keypad.
 */
#define MAXKEYS 100	/* number of keys we understand */

struct map *
_init_keypad()
{
	struct map *r, *p;

	r = (struct map *) calloc(MAXKEYS, sizeof (struct map));
	if (r == (struct map *)0)
	    return r;
	p = r;

	/* If down arrow key sends \n, don't map it. */
	if (key_down && strcmp(key_down, "\n"))
		p = _addone(p, key_down,	KEY_DOWN,	"down");
	p = _addone(p, key_up,		KEY_UP,		"up");
	/* If left arrow key sends \b, don't map it. */
	if (key_left && strcmp(key_left, "\b"))
		p = _addone(p, key_left,	KEY_LEFT,	"left");
	p = _addone(p, key_right,	KEY_RIGHT,	"right");
	p = _addone(p, key_home,	KEY_HOME,	"home");
	/* If backspace key sends \b, don't map it. */
	if (key_backspace && strcmp(key_backspace, "\b"))
		p = _addone(p, key_backspace,	KEY_BACKSPACE,	"backspace");
	p = _addone(p, key_f0,		KEY_F(0),	lab_f0?lab_f0:"f0");
	p = _addone(p, key_f1,		KEY_F(1),	lab_f1?lab_f1:"f1");
	p = _addone(p, key_f2,		KEY_F(2),	lab_f2?lab_f2:"f2");
	p = _addone(p, key_f3,		KEY_F(3),	lab_f3?lab_f3:"f3");
	p = _addone(p, key_f4,		KEY_F(4),	lab_f4?lab_f4:"f4");
	p = _addone(p, key_f5,		KEY_F(5),	lab_f5?lab_f5:"f5");
	p = _addone(p, key_f6,		KEY_F(6),	lab_f6?lab_f6:"f6");
	p = _addone(p, key_f7,		KEY_F(7),	lab_f7?lab_f7:"f7");
	p = _addone(p, key_f8,		KEY_F(8),	lab_f8?lab_f8:"f8");
	p = _addone(p, key_f9,		KEY_F(9),	lab_f9?lab_f9:"f9");
	p = _addone(p, key_f10,         KEY_F(10),      "f10");
	p = _addone(p, key_f11,		KEY_F(11),	"f11");
	p = _addone(p, key_f12,		KEY_F(12),	"f12");
	p = _addone(p, key_f13,		KEY_F(13),	"f13");
	p = _addone(p, key_f14,		KEY_F(14),	"f14");
	p = _addone(p, key_f15,		KEY_F(15),	"f15");
	p = _addone(p, key_f16,		KEY_F(16),	"f16");
	p = _addone(p, key_f17,		KEY_F(17),	"f17");
	p = _addone(p, key_f18,		KEY_F(18),	"f18");
	p = _addone(p, key_f19,		KEY_F(19),	"f19");
	p = _addone(p, key_f20,		KEY_F(20),      "f20");
	p = _addone(p, key_f21,		KEY_F(21),	"f21");
	p = _addone(p, key_f22,		KEY_F(22),	"f22");
	p = _addone(p, key_f23,		KEY_F(23),	"f23");
	p = _addone(p, key_f24,		KEY_F(24),	"f24");
	p = _addone(p, key_f25,		KEY_F(25),	"f25");
	p = _addone(p, key_f26,		KEY_F(26),	"f26");
	p = _addone(p, key_f27,		KEY_F(27),	"f27");
	p = _addone(p, key_f28,		KEY_F(28),	"f28");
	p = _addone(p, key_f29,		KEY_F(29),	"f29");
	p = _addone(p, key_f30,		KEY_F(30),      "f30");
	p = _addone(p, key_f31,		KEY_F(31),	"f31");
	p = _addone(p, key_f32,		KEY_F(32),	"f32");
	p = _addone(p, key_f33,		KEY_F(33),	"f33");
	p = _addone(p, key_f34,		KEY_F(34),	"f34");
	p = _addone(p, key_f35,		KEY_F(35),	"f35");
	p = _addone(p, key_f36,		KEY_F(36),	"f36");
	p = _addone(p, key_f37,		KEY_F(37),	"f37");
	p = _addone(p, key_f38,		KEY_F(38),	"f38");
	p = _addone(p, key_f39,		KEY_F(39),	"f39");
	p = _addone(p, key_f40,		KEY_F(40),      "f40");
	p = _addone(p, key_f41,		KEY_F(41),	"f41");
	p = _addone(p, key_f42,		KEY_F(42),	"f42");
	p = _addone(p, key_f43,		KEY_F(43),	"f43");
	p = _addone(p, key_f44,		KEY_F(44),	"f44");
	p = _addone(p, key_f45,		KEY_F(45),	"f45");
	p = _addone(p, key_f46,		KEY_F(46),	"f46");
	p = _addone(p, key_f47,		KEY_F(47),	"f47");
	p = _addone(p, key_f48,		KEY_F(48),	"f48");
	p = _addone(p, key_f49,		KEY_F(49),	"f49");
	p = _addone(p, key_f50,		KEY_F(50),      "f50");
	p = _addone(p, key_f51,		KEY_F(51),	"f51");
	p = _addone(p, key_f52,		KEY_F(52),	"f52");
	p = _addone(p, key_f53,		KEY_F(53),	"f53");
	p = _addone(p, key_f54,		KEY_F(54),	"f54");
	p = _addone(p, key_f55,		KEY_F(55),	"f55");
	p = _addone(p, key_f56,		KEY_F(56),	"f56");
	p = _addone(p, key_f57,		KEY_F(57),	"f57");
	p = _addone(p, key_f58,		KEY_F(58),	"f58");
	p = _addone(p, key_f59,		KEY_F(59),	"f59");
	p = _addone(p, key_f60,		KEY_F(60),      "f60");
	p = _addone(p, key_f61,		KEY_F(61),	"f61");
	p = _addone(p, key_f62,		KEY_F(62),	"f62");
	p = _addone(p, key_f63,		KEY_F(63),	"f63");
	p = _addone(p, key_dl,		KEY_DL,		"dl");
	p = _addone(p, key_il,		KEY_IL,		"il");
	p = _addone(p, key_dc,		KEY_DC,		"dc");
	p = _addone(p, key_ic,		KEY_IC,		"ic");
	p = _addone(p, key_eic,		KEY_EIC,	"eic");
	p = _addone(p, key_clear,	KEY_CLEAR,	"clear");
	p = _addone(p, key_eos,		KEY_EOS,	"eos");
	p = _addone(p, key_eol,		KEY_EOL,	"eol");
	p = _addone(p, key_sf,		KEY_SF,		"sf");
	p = _addone(p, key_sr,		KEY_SR,		"sr");
	p = _addone(p, key_npage,	KEY_NPAGE,	"npage");
	p = _addone(p, key_ppage,	KEY_PPAGE,	"ppage");
	p = _addone(p, key_stab,	KEY_STAB,	"stab");
	p = _addone(p, key_ctab,	KEY_CTAB,	"ctab");
	p = _addone(p, key_catab,	KEY_CATAB,	"catab");
	p = _addone(p, key_ll,		KEY_LL,		"ll");
	p = _addone(p, key_a1,		KEY_A1,		"a1");
	p = _addone(p, key_a3,		KEY_A3,		"a3");
	p = _addone(p, key_b2,		KEY_B2,		"b2");
	p = _addone(p, key_c1,		KEY_C1,		"c1");
	p = _addone(p, key_c3,		KEY_C3,		"c3");
#ifdef KEY_BTAB
	p = _addone(p, back_tab,	KEY_BTAB,	"cbt");
#endif
	p -> keynum = 0;		/* termination convention */
#ifdef DEBUG
	if(outf) fprintf(outf, "return key structure %x, ending at %x\n", r, p);
#endif
	return r;
}

/*
 * Map text into num, updating the map structure p.
 * label is currently unused, but is an English description
 * of what the key is labelled, similar to the name in vi.
 */
static
struct map *
_addone(p, text, num, label)
struct map *p;
char *text;
int num;
char *label;
{
	if (text) {
		strcpy(p->label, label);
		strcpy(p->sends, text);
		p->keynum = num;
#ifdef DEBUG
		if(outf) fprintf(outf, "have key label %s, sends '%s', value %o\n", p->label, p->sends, p->keynum);
#endif
		p++;
	}
	return p;
}

#endif		KEYPAD
