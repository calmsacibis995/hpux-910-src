# @(#) $Revision: 66.5 $
#
# ptr = strchr(str, c);
#
# Return the ptr in sp at which the character c appears;
# (char *)0 if not found
#
# NOTE: It is legal to search for a \0, in which case we return a
#       pointer to the \0 in the string (rather than NULL)
#
# ALGORITHM:
#     char *
#     strchr(str, c)
#     register char *str;
#     register char c;
#     {
#         if (str == (char *)0)
#             return (char *)0;
#
#         while ((c2 = *str++) != '\0')
#             if (c2 == c)
#                 return str - 1;
#
#         if (c == '\0')
#             return  str - 1;
#         return (char *)0;
#     }
	version	2
ifdef(`_NAMESPACE_CLEAN',`
	global	__strchr
	sglobal	_strchr
__strchr:',`
	global	_strchr
')
_strchr:
ifdef(`PROFILE',`
    ifdef(`PIC',`
	mov.l	&DLT,%a0
	lea.l	-6(%pc,%a0.l),%a0
	mov.l	p_strchr(%a0),%a0
	bsr.l	mcount',`
	mov.l	&p_strchr,%a0
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
	mov.l	%d0,%a0		# a0 = str',`
	mov.l	4(%sp),%a0	# a0 = str
')
	mov.b	11(%sp),%d1	# d1 = character to find
	movq.l	&0,%d0		# make bits 9-16 of d0 0 for dbeq
L1:
	mov.b	(%a0)+,%d0	# d0 = *str++
	cmp.b	%d0,%d1		# if (d0 != d1 &&
	dbeq	%d0,L1		#     d0 != 0) continue
	bne.b	L4		# didn't find it, return NULL
L2:
	movq.l  &-1,%d0
	add.l   %a0,%d0         # return ptr to first matching char
L3:
	rts
L4:
	movq.l	&0,%d0		# return 0
	rts
ifdef(`PROFILE',`
	data
p_strchr:
	long	0
')
