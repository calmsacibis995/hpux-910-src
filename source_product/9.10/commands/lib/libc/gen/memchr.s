# @(#) $Revision: 66.6 $
#
# ptr = memchr(s, c, n);
#
# Scan memory pointed to by 's' for the byte 'c' for at most
# 'n' bytes.
# Returns NULL if 'c' does not occur, otherwise a pointer to the
# matched character.
#
# char *
# memchr(s, c, n)
# unsigned char *s;
# unsigned char c;
# int n; /* signed */
# {
#     if (n == 0)
#	  return 0;
#
#     --n;
#     do
#     {
#	  if (*s++ == c)
#	      return s - 1;
#     } while (--n >= 0);
#     return 0;
# }
#
	version 2
ifdef(`_NAMESPACE_CLEAN',`
	global	__memchr
	sglobal _memchr
__memchr:',`
	global	_memchr
')
_memchr:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_memchr(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_memchr,%a0
	jsr	mcount
    ')
')
	mov.l	4(%sp),%a0		#(2)  a0 = s
	mov.b	11(%sp),%d1		#(2)  d1 = char
	mov.l	12(%sp),%d0		#(2)  d0 = count
	subq.l	&1,%d0			# 1   check was 0 and fix dbeq
	bcs.b	Lzero			#2/3  count != 0, begin loop

Linner:
	cmp.b	%d1,(%a0)+		#(1.3) match byte?
	dbeq	%d0,Linner		#3/4   no, continue

	bne.b	Lnotfound		#2/3  found match, return a0-1

	mov.l	%a0,%d0			# 1   ptr in d0
	subq.l	&1,%d0			# 1   return a0-1
	rts

Lnotfound:
	subi.l	&0x10000,%d0
	bcc.b	Linner			#2/3  another 65k bytes?
Lzero:
	movq.l	&0,%d0			# 1   count expired, return 0
	rts
ifdef(`PROFILE',`
	data
p_memchr:
	long	0
')
