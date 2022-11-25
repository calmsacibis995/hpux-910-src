/* @(#) $Revision: 64.2 $ */    
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define srand _srand
#define rand _rand
#endif

static long randx=1;

#ifdef _NAMESPACE_CLEAN
#undef srand
#pragma _HP_SECONDARY_DEF _srand srand
#define srand _srand
#endif

void
srand(x)
unsigned x;
{
	randx = x;
}

#ifdef _NAMESPACE_CLEAN
#undef rand
#pragma _HP_SECONDARY_DEF _rand rand
#define rand _rand
#endif

int
rand()
{
#ifdef hp9000s500
	/* disable user traps so rand does not go through uke_prog_trap */
	asm("
		pshr 4		# TOS = status register
		lni 0x800001	# load mask
		and		# clear user trap bit
		setr 4		# restore status register
	");

#endif hp9000s500
	return(((randx = randx * 1103515245L + 12345)>>16) & 0x7fff);
}
