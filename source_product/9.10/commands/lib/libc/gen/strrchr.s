# @(#) $Revision: 66.3 $
#
# ptr = strrchr(str, c);
#
# Return the ptr in sp at which the character c last appears;
# (char *)0 if not found
#
# NOTE: It is legal to search for a \0, in which case we return a
#       pointer to the \0 in the string (rather than NULL)
#
# ALGORITHM:
#     char *
#     strrchr(str, c)
#     register char *str;
#     register char c;
#     {
#         register char *spot;
#
#         if (str == (char *)0)
#             return (char *)0;
#
#	  if (c == '\0')
#	  {
#             while (*str++)
#                 continue;
#             return str - 1;
#         }
#
#         spot = (char *)1;
#         while ((c2 = *str++) != '\0')
#             if (c2 == c)
#                 spot = str;
#         return  spot - 1;
#     }
	version	2
ifdef(`_NAMESPACE_CLEAN',`
	global	__strrchr
	sglobal	_strrchr
__strrchr:',`
	global	_strrchr
')
_strrchr:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_strrchr(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_strrchr,%a0
	jsr	mcount
    ')
')
ifdef(`NO_NULL_DEREFERENCE',`
	mov.l	4(%sp),%d0	# d0 = str
#
# If there is not a 0 byte at address 0, so we must explictly check
# for a NULL pointer.  This is not required in the current s300
# implementation, so this code is currently disabled.
#
	beq.b	L3		# if (str == NULL) return 0
	mov.l	%a0,%d0		# a0 = str',`
	mov.l	4(%sp),%a0	# a0 = str
')
	mov.b	11(%sp),%d1	# d1 = character to find
	beq.b	L4		# special case to find end of string

	movq.l	&1,%d0		# make bits 9-16 of d0 0 for dbeq
	mov.l	%d0,%a1		# spot = 1
	bra.b	L2
L1:
	mov.l	%a0,%a1		# spot = str
L2:
	mov.b	(%a0)+,%d0	# d0 = *str++
	cmp.b	%d0,%d1		# char matched?
	dbeq	%d0,L2		# continue if no match or !end-of-str
	beq.b	L1		# matched character, save spot

	movq.l	&-1,%d0
	add.l	%a1,%d0		# return str - 1
L3:
	rts
L4:
	tst.b	(%a0)+		# search for '\0'
	bne.b	L4
	movq.l	&-1,%d0
	add.l	%a0,%d0		# return str - 1
	rts
ifdef(`PROFILE',`
	data
p_strrchr:
	long	0
')
