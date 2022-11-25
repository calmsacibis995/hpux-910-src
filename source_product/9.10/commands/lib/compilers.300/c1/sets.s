# file sets.s
# @(#) $Revision: 66.6 $
# KLEENIX_ID @(#)sets.s	16.1 90/11/05 */
#======================================================================
#
# Procedures: Sets.s                           (Optimizer Set Routines)
# By: Greg Lindhorst, July 1988 
# Where: Colorado Language Lab, HP Fort Collins
#
# This is an assembly version of 'sets.c'.
#
# Description:
#   This is a rewrite of the old set routines which were used to do
#   set manipulations in the global optimizer for the Series 300.  
#   This version assumes that almost all of our sets are sparse, and
#   thus we use a linked list representation for the sets, where only
#   the elements which are set are represented.
#
# Refrences:
#   Old Series 300 'sets.s' code, by Scott Omdahl (COL), written 2/10/88.
#   "Bit Vectors: The Set Utilities", by Karl Pettis (CAL),
#                                     Version 1.0, 12/1/86.  
#

#
# defines:
#   DEBUG1 (or PROFILE) -- Causes profiling calls to be made.
#   DEBUG2 (or LINK) -- Causes links/unlinks to be made for stack dumps.
#   NUM_BLOCKS -- number of blocks to allocate at a time (default 512)
#         *** WARNING *** This number should not exceed 32,767 (since a
#         'dbra' instruction is used to iterate through them).
#
ifdef(`PROFILE',`define(`DEBUG1')')       # Alternate forms
ifdef(`LINK',`define(`DEBUG2')')

# Default for NUM_BLOCKS (Must be less than 32,767)
ifdef(`NUM_BLOCKS',,define(`NUM_BLOCKS',512))

# Profiling information
ifdef(`DEBUG1',`define(`PROFILE',`
		mov.l	&$1,%a0					 	# Location for information
		jsr		mcount						# Make the call
		')',`define(`PROFILE')')

# Definitions for the LINK and UNLK macros
ifdef(`DEBUG2',`define(`LINK',`
		link.l	%a6, &0						# Link for stack trace
		')',`define(`LINK')')
ifdef(`DEBUG2',`define(UNLK,`
		unlk	%a6							# Recover the stack frame
		')',`define(UNLK)')

# Argument locations on the stack (based on doing a LINK/UNLK 
ifdef(`DEBUG2',`define(STACK_ADD,4)',`define(STACK_ADD,0)')

# Definitions for the Stack Arguments
define(ARG1,`eval(STACK_ADD+4)(%sp)')
define(ARG2,`eval(STACK_ADD+8)(%sp)')
define(ARG3,`eval(STACK_ADD+12)(%sp)')

#
# Offsets off of a pointer (for our data structure)
#  Our C structure would look like this:
#    struct set_entry {
#             unsigned int num;
#             unsigned int data;
#             struct set_entry *next;
#             } ;
#
define(NUM,`($*)')     
define(DATA,`4($*)')	
define(NEXT,`8($*)')

# Sizeof that structure == sizeof( struct set_entry )
define(STRUCT_SIZE,12)

# The definitions for PUSH and POP
define(PUSH,`pushdef(`STACK_ADD',eval(STACK_ADD+4)) mov.l	$1,-(%sp)')
define(POP,`popdef(`STACK_ADD') mov.l	(%sp)+,$1')

#
# 'FREELIST'
#   This macro tells 'getablock' that the 'free_list' is currently being
#   stored in a register, and that this register should be used.  It
#   also moves the contents of 'free_list' into the register named.
#
# 'UNFREELIST'
#   'unfreelist' will tell getablock that the free_list is now really
#   in the variable 'free_list', and that all further action should come
#   from that variable.  It also places the old register back into the
#   variable.
#
define(FREE_REG,`free_list')
define(FREELIST,`
				mov.l	free_list, $*
				define(`FREE_REG',$*)
				')
define(UN_FREELIST,`
				mov.l	$*, free_list
				define(`FREE_REG',free_list)
				')

#
# 'GETABLOCK'
#   This macro is used to get a new block, either from the new block
#   pool, or to call 'get_sets' for some more.  It will return it's
#   block in %a0, and will fix the 'free_list' before returning.  This
#   macro is effected by the current setting of 'free_reg' (see above).
#
define(L_NUM,1)
define(GETABLOCK,`
				mov.l	FREE_REG, %a0       
				tst.l	%a0
				bne		`getb_'L_NUM
				jsr		get_sets
`getb_'L_NUM:	mov.l	NEXT(%a0), FREE_REG
				define(`L_NUM',incr(L_NUM))
	 			 ')

#
# 'DESTLIST'
#    There are many times in this code when the destination list is
#    used as the list to draw blocks from for a new set, until it is
#    exhausted.  At that point, we need to start taking blocks off the
#    free_list.  If it should run dry, we need to call get_sets for some
#    more. 
#
#    All of these are handled by this macro.  The first argument is a
#    temporary address register which will be erased, but need not be
#    saved after this macro.  The second argument is a data register
#    for which a bit is to be set (bit 30) to indicate that the
#    free_list is now in use.  %a0 MUST BE PRESERVED until a call to
#    'DESTLIST_DONE', at which time the free_list in memory is updated.
#
define(DESTLIST,`
			mov.l	%a0, $1				# Move down the destination list
			mov.l	NEXT(%a0), %a0
			tst.l	%a0
			bne		`dest_'incr(incr(L_NUM))
			btst	&30, $2				# Check flag (on free_list)
			bne		`dest_'L_NUM
			bset	&30, $2				# Set the flag
			mov.l	free_list, %a0		# Get the free list
			tst.l	%a0
			bne		`dest_'incr(L_NUM)
`dest_'L_NUM:			
			jsr		get_sets			# must be an empty free_set, fill it
`dest_'incr(L_NUM):
			mov.l	%a0, NEXT($1)		# fix that last blocks pointer
`dest_'incr(incr(L_NUM)):
			define(`L_NUM',eval(L_NUM+3))
			   ')

#
# 'DESTLIST_DONE'
#   This macro will save the free_list back into memory if need be.
#   It checks the bit 30 of the data register specified.  If it is set,
#   then we need to save the free_list back out of %a0.  Otherwise, we
#   need to trim the destination list, by placing extras on the free_list.
#   The block in question will be capped, and the remainder 'trim()ed'
#   (see the C version, the function 'trim' did something like this).
#
#   The first argument is a temporary address register.  The second
#   argument is the register which contains the flag, set by DESTLIST.
#   Since this is always one of the last things to be done in a 
#   procedure, ARGUMENT 2's VALUE MAY CHANGE!, but that should not
#   affect much.
#
define(DESTLIST_DONE,`
			btst	&30, $2				# did we take the free_list?
			beq		`ddest_'L_NUM
			mov.l	NEXT(%a0), free_list  # Store the free_list
			clr.l	NEXT(%a0)			# cap the list 
			bra		`ddest_'incr(L_NUM)
`ddest_'L_NUM:
			mov.l	%a0, $1 			# Keep the head
			mov.l	NEXT(%a0), %a0		# Get to the next block
			tst.l	%a0
			beq		`ddest_'incr(L_NUM) # No blocks, dont bother
			clr.l	NEXT($1)			# cap the list
			mov.l	%a0, $2				# save the head
`ddest_'incr(incr(L_NUM)):
			mov.l	%a0, $1				# Scan for the end
			mov.l	NEXT(%a0), %a0
			tst.l	%a0
			bne		`ddest_'incr(incr(L_NUM))
			mov.l	free_list, NEXT($1) # Patch into the free_list
			mov.l	$2, free_list		# Save the free_list
`ddest_'incr(L_NUM):
			define(`L_NUM',incr(incr(L_NUM)))
			')

# 
# 'free_list'
#   This pointer points to the top of the free list.  If it is empty, 
#   then 'get_sets()' is called to replentish it.  
#
			comm	free_list, 4           # The Free Linked List

# Memory for the various p_variables
ifdef(`DEBUG1',`
			comm	p_new_set,4
			comm	p_free_set,4
			comm	p_new_set_n,4
			comm	p_free_set_n,4
			comm	p_set_elem_vector,4
			comm	p_xin,4
			comm    p_nextel,4
			comm	p_adelement,4
			comm	p_delelement,4
			comm	p_setassign,4
			comm	p_clearset,4
			comm	p_addsetrange,4
			comm	p_setunion,4
			comm	p_intersect,4
			comm	p_difference,4
			comm	p_setcompare,4
			comm	p_differentsets,4
			comm	p_isemptyset,4
			comm	p_set_size,4
			comm	p_change_set_size,4
			   ')

			text
#----------------------------------------------------------------------
#
# Procedure: new_set                               (Allocate a New Set)
# 
# Description:
#   This procedure allocates a new_set.  All that is done is that 
#   a header block is allocated, and filled in with the appropriate 
#   maximum number of elements.
#
# Calling:
#	struct set *new_set( max_elems )
#	int max_elems;
#
			global	_new_set
_new_set:	LINK
			PROFILE( p_new_set )
			GETABLOCK
			mov.l	ARG1,NUM(%a0)			# set->num = max_elems
			clr.l	NEXT(%a0)				# set->next = NULL
			mov.l	%a0, %d0				# prepare for return
			UNLK
			rts			

#----------------------------------------------------------------------
#
# Procedure: free_set                                      (free a set)
#
# Description:
#   Free a set which has been allocated with 'new_set'.
#
# Calling:
#   void free_set( set )
#   struct set_entry *set;
#
			global	_free_set
_free_set:	LINK
			PROFILE( p_free_set )
			mov.l	ARG1,%a0				# scan = set
			mov.l	%a0, %d0				# copy a copy, set Z flag
			beq		set_error				# Null pointer check
free_001:	mov.l	%a0, %a1                # { temp = scan
			mov.l	NEXT(%a0),%a0			#   scan = scan->next
			tst.l	%a0
			bne		free_001				# } while( scan != NULL )
			mov.l	free_list,NEXT(%a1)	 	# temp->next = free_list
			mov.l	%d0,free_list			# free_list = set
			UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: new_set_n                             (Multiple 'new_set')
#
# Description:
#   This routine does the same thing as new_set does, except that it
#   returns 'num_sets' number of sets, in an array of vectors.  
#
# Calling:
#   struct set_entry **new_set_n( max_elems, num_sets )
#   unsigned int max_elems, num_sets;
#
			global	_new_set_n
_new_set_n:	LINK
			PROFILE( p_new_set_n )
			PUSH( %a2 )

# Malloc up the vector
			mov.l	ARG2,%d0				# Get number of sets
			mov.l	%d0, %a2				# Save the value
			addq.l	&1,%d0					# Add one for the NULL at the end
			lsl.l	&2,%d0					# Multiply by 4 (for pointers)
			mov.l	%d0, -(%sp)		        # Push for call
			jsr		_malloc					# Call Malloc
			tst.l	%d0						# Check malloc return code
			beq		set_malloc_bad
			addq.l	&4, %sp					# Recover from the call

# Get ready for the loop through the vectors
			mov.l	%d0,%a1					# Place in an address register
			PUSH( %d0 )						# Save for the return
			mov.l	%a2,%d0					# %d0 = num_sets (as a counter)
			subq.l	&1, %d0					# Since dbra jumps on 0
			mov.l	ARG1,%d1				# %d1 = max_elems (as storage)
			FREELIST( %a2 )					# We want the list in a reg (fast)

# The loop: get a block and fill it in
newn_002:	GETABLOCK						# { Get a block (off list)
			mov.l	%a0,(%a1)+				#   Store in the vectors array
			mov.l	%d1,NUM(%a0)			#   set->num = max_elems
			clr.l	NEXT(%a0)				#   set->next = NULL
			dbra	%d0,newn_002			# } while( num_sets-- )
# These three instructions are needed since 'dbra' is a 16 bit decrement,
# and we require a full 32 bit decrement.
			clr.w   %d0
			subq.l  &1, %d0           
			bpl     newn_002

# Store a NULL at the end of the last one
			clr.l	(%a1)

# Restore the free_list
			UN_FREELIST( %a2 )

			POP( %d0 )						# Get return value
			POP( %a2 )
			UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: free_set_n                           (multiple 'free_set')
#
# Description:
#   This procedure will free n sets, which were allocated with the
#   new_set_n call, in the same way that free_set works.
#
# Calling:
#   void free_set_n( sets )
#   struct set_entry **sets;
#
			global	_free_set_n
_free_set_n:	
			LINK		
			PROFILE( p_free_set_n )
			PUSH( %a2 )
			
# Preparation for loop
			mov.l	ARG1,%a0				# Get the head of the vectors
			mov.l	%a0, -(%sp)             # Push for the call to _free
			beq		set_error				# Null check
			mov.l	(%a0)+,%d1				# Need to keep the head
			beq		freen_020

# Loop, scaning down each linked list, tying to next guy
			mov.l	%d1, %a1				# Get first entry
freen_002:	mov.l	%a1, %a2				# { temp = scan
			mov.l	NEXT(%a1),%a1			#   scan = scan->next
			tst.l	%a1
			bne		freen_002				# } while( scan )
			mov.l	(%a0)+,%a1				# Get the next vector
			tst.l	%a1
			beq		freen_010				# if zero, we out of here
			mov.l	%a1, NEXT(%a2)			# Place in that last guys list
			bra		freen_002
			
# Complete the list by sticking it in the free_list
freen_010:	mov.l	free_list, NEXT(%a2)	# Patch in one end 
			mov.l	%d1, free_list			#   and the other
			
# Free the list
freen_020:									# ARG1 is already pushed
			jsr		_free					# Free the vectors 
			addq.l	&4, %sp					# fix the stack
			
			POP( %a2 )
			UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: set_elem_vector		(Convert from sets to element vector)
#
# Description: 
#   This procedure will fill a vector with element #'s for a set.  Vector
#   of element #'s is terminated with -1.
#
# Calling:
#   void set_elem_vector( set, vector )
#   struct set_entry *set;
#   long *vector;
#
			global	_set_elem_vector
_set_elem_vector:	LINK
			PROFILE( p_set_elem_vector )
			PUSH( %d2 )
			
# Get stuff off the stack, do some error checking
			mov.l	ARG1,%a0		# Get the set pointer
			tst.l	%a0
			beq	set_error		# Null Check
			mov.l	ARG2, %a1		# Vector pointer

# Loop through all the blocks in the set
sev_002:
			mov.l	NEXT(%a0),%a0		# Update Pointer
			tst.l	%a0
			beq	sev_001			# End of list

# Find all set members in this block and add to vector
			clr.l	%d0			# offset
			mov.l	DATA(%a0),%d2		# Get the data
sev_003:
			mov.l	%d0,%d1			# width
			neg.b	%d1			# subtract from 32
			bfffo	%d2{%d0:%d1},%d0	# find first element
			beq	sev_002			# no more in this block
			mov.l	NUM(%a0),%d1		# block number
			lsl.l	&5,%d1			# mult by 32
			add.l	%d0,%d1			# element number
			mov.l	%d1,(%a1)+		# store in vector
			addq	&1,%d0			# start at next pos
			and.l	&0x1f,%d0
			bne	sev_003			# more bits in word
			bra	sev_002			# end of word -- do next

# Set -1 terminator in vector and return
sev_001:
			movq	&-1,%d0
			mov.l	%d0,(%a1)+		# store in vector
			POP( %d2 )
			UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: xin                                     (Check an element)
#
# Description: 
#   This procedure will return the value of a particular element in the
#   given set.
#
# Calling:
#   int xin( set, elem )
#   struct set_entry *set;
#   unsigned int elem;						
#
			global	_xin
_xin:		LINK
			PROFILE( p_xin )
			
# Get stuff off the stack, do some error checking
			mov.l	ARG1, %a0				# Get the set pointer
			tst.l	%a0
			beq		set_error				# Null Check
			mov.l	ARG2, %d0				# Element Number
			cmp.l	%d0, NUM(%a0)			# Check bounds on 'elem' number
			bhs		set_error

# Loop Prep: calculate block number
			mov.l	%d0, %d1				# Save for bit number (later)
			lsr.l	&5, %d0                 # Divide by 32

# Loop: search for the block we need
xin_002:
			mov.l	NEXT(%a0), %a0			# Update Pointer
			tst.l	%a0
			beq		xin_001					# End of list
			cmp.l	%d0, NUM(%a0)			# Check the block num
			bhi		xin_002					# Too Low, got to next one
			bne		xin_001					# Wrong block, return 0

# We have the proper block, test the bit
			mov.l	DATA(%a0), %d0			# Get the data
			not.b	%d1						# Need to get reverse offset
			btst	%d1, %d0				# Test the bit
			beq		xin_001					# No Bit, Return 0
			movq.l	&1, %d0					# Return 1
			bra		xin_010					# Leave

# Clear return, and leave
xin_001:
			clr.l	%d0
xin_010:
			UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: nextel                  (find the next element in the set)
#
# Description:
#   Finds the next element which is set.  If -1 is sent in, then search
#   begins at the beginning, otherwise the search begins AFTER the 
#   element specified.  If no elements are left, -1 is returned.
#
#   This version is a very crude attempt, which will no doubt be very,
#   very slow.  It uses a very niave approach.
#
# Calling:
#   int nextel( elem, set )
#   int elem;
#   struct set_entry *set;
#
			global	_nextel
_nextel:	LINK
			PROFILE( p_nextel )

# Get arguments, error check
			mov.l	ARG2, %a0			# set
			tst.l	%a0
			beq		set_error			# Null Check
			mov.l	ARG1, %d0			# elem

# increment %d0 since want to start search there, error check		
			addq.l	&1, %d0				# elem++
			mov.l	%d0, %d1			# Store a copy of it
			lsr.l	&5, %d1				# divide by 32 (block number)

			cmp.l	%d0, NUM(%a0)		# Error check on number of elements
			bhi		set_error			# can be equal to number of elems
			
# Loop, looking a higher or equal block
nextel_1:	mov.l	NEXT(%a0), %a0		# Go to the next block
			tst.l	%a0
			beq		nextel_2			# We done looking
			cmp.l	%d1, NUM(%a0)		# Check block nums
			bhi		nextel_1			# Continue the scan
			bne		nextel_3

# If we are equal, then see if any more bits are in this one
			and.l	&0x1f, %d0			# Just get the low order bits
			mov.b	%d0, %d1			# Need width
			neg.b	%d1					# Negate (only use 0x1f worth)
			bfffo   DATA(%a0){%d0:%d1}, %d0   # Get the first high bit off %d1
			bne		nextel_9			# Good numbers, exit

# If we were equal, but no bits left, we need another block
			mov.l	NEXT(%a0), %a0		# We need to go to the next block
			tst.l	%a0
			beq		nextel_2			# No more blocks, leave
			
# Else, we were not equal.  Get the next block and find the bit
nextel_3:	bfffo   DATA(%a0){&0:&32}, %d0	   # Get the bit number
nextel_9:	mov.l	NUM(%a0), %d1		# Get the new block number

# Add in the block number (* 32)
			lsl.l	&5, %d1				# Multiply by 32
			add.l	%d1, %d0			# Add in to the bit offset

# The return code
nextel_8:	UNLK						# Exit
			rts			

# The error code
nextel_2:	movq.l	&-1, %d0			# Error Code
			bra		nextel_8			# Exit (up above)

#----------------------------------------------------------------------
#
# Procedure: adelement                       (Add an Element to a set)
#
# Description: 
#   Adds an element to a set, by copying the set to another set, 
#   and then either simply setting a bit in a currnet block, or
#   adding a new block.
#
# Calling:
#   void adelement( elem, src_set, dest_set )
#   unsigned int elem;
#   struct set_entry *src_set, *dest_set;
#
			global	_adelement
_adelement:	LINK
			PROFILE( p_adelement )
			PUSH( %a2 )

# Preliminary: get Block Number, Sets pointers, error checking
			mov.l	ARG1, %d0			# elem
			mov.l	%d0, %d1
			lsr.l	&5, %d0				# divide by 32 -> block_num
			mov.l	ARG2, %a2			# src_set
			tst.l	%a2
			beq		set_error			# Null check
			cmp.l	%d1, NUM(%a2)		# check sizes 
			bhs		set_error
			and.l	&0x1f, %d1			# want just the offset
			not.b	%d1					# subtract 32 for reverse numbering
			mov.l	ARG3, %a1			# dest_set
			tst.l	%a1
			beq		set_error			# Null check
			cmp.l	%a2, %a1			# src_set == dest_set ?
			bne		adel_1
			
# Sets are the same: simply modify the src_set
adel_3:
			mov.l	%a2, %a1			# temp = src_scan
			mov.l	NEXT(%a2), %a2		# src_scan = src_scan->next
			tst.l	%a2
			beq		adel_2              # src_scan == NULL ?
			cmp.l	%d0, NUM(%a2)		# src_scan->num < block_num ?
			bhi		adel_3

# If this is the correct block, set the bit only
			bne		adel_2
			mov.l	DATA(%a2), %d0		# get the data
			bset	%d1, %d0			# or in the bit
			mov.l	%d0, DATA(%a2)		# store it back
			bra		adel_out

# If this is the wrong block, get a new block, fill it in
adel_2:
			GETABLOCK
			mov.l	%a0, NEXT(%a1)		# Patch into our list
			mov.l	%a2, NEXT(%a0)
			mov.l	%d0, NUM(%a0)		# dest_scan->num = block_num
			clr.l	%d0					# clear it
			bset	%d1, %d0			# set the bit (in the register)
			mov.l	%d0, DATA(%a0)		# dest_scan->data = bit_masks[...]
			bra		adel_out

# Sets are different: copy to the destination set, maybe insert a block
adel_1:
			mov.l	NUM(%a2), %a0		# %a0 is a temporary here 
			cmp.l	%a0, NUM(%a1)		# Check Sizes
			bne		set_error

# Loop, finding blocks to copy, and looking for the one to insert
			mov.l	%a1, %a0			# get ready for loop
			mov.l	NEXT(%a2), %a2
adel_30:
			tst.l	%a2
			beq		adel_9				# Z set by routines below
adel_31:
			DESTLIST( %a1, %d1 )		# Get a block

			cmp.l	%d0, NUM(%a2)		# Block numbers
			bhi		adel_21				# lower than, copy contents
			beq		adel_20				# equal, set bit
adel_22:
			mov.l	%d0, NUM(%a0)		# copy over block numbers
			clr.l	%d0					# clear before the set
			bset	%d1, %d0			# set the bit
			mov.l	%d0, DATA(%a0)		# save the word
			movq.l	&-1, %d0			# since we can never be greater than
			bset	&29, %d1  	 		# set flag that we did it 
			bra		adel_31				# (%a0 != NULL, don't check)

# If we have the proper block, or in the bit
adel_20:
			mov.l	%d0, NUM(%a0)		# copy over block numbers
			mov.l	DATA(%a2), %d0		# get the data
			bset	%d1, %d0			# set the bit
			mov.l	%d0, DATA(%a0)		# store it
			movq.l	&-1, %d0			# we will never be greater
			bset 	&29, %d1  	 		# set flag that we did it
			mov.l	NEXT(%a2), %a2		# update the source pointer 
			bra		adel_30

# Otherwise, just copy over the block
adel_21:
			mov.l	NUM(%a2), NUM(%a0)	# copy over block numbers
			mov.l	DATA(%a2),DATA(%a0) # copy over the data block
			mov.l	NEXT(%a2), %a2		# update the source pointer 
			bra		adel_30

# Common point: done scanning, did we end up adding the needed bit?
adel_9:
			btst	&29, %d1			# our flag
			bne		adel_91				
			DESTLIST( %a1, %d1 )		# Get a block
			mov.l	%d0, NUM(%a0)		# Same code as adel_22 -> adel_20
			clr.l	%d0					# clear before the set
			bset	%d1, %d0			# ste the bit
			mov.l	%d0, DATA(%a0)		# store it back

# Strip off any remaining blocks
adel_91:
			DESTLIST_DONE( %a1, %d1 )

# Exit stuff
adel_out:
			POP( %a2 )
			UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: delelement                  (Delete an Element from a set)
#
# Description: 
#   Deletes an element from a set, by copying the set to another set, 
#   and then either simply clearing a bit in a currnet block, or
#   deleting a block.
#
# Calling:
#   void delelement( elem, src_set, dest_set )
#   unsigned int elem;
#   struct set_entry *src_set, *dest_set;
#
			global	_delelement
_delelement:
			LINK
			PROFILE( p_delelement )
			PUSH( %a2 )		
			
# Get arguments
			mov.l	ARG1, %d0			# Element number
			mov.l	%d0, %d1
			lsr.l	&5, %d0				# Block number
			mov.l	ARG2, %a2			# src_set
			tst.l	%a2					# null check
			beq		set_error			
			cmp.l	%d1, NUM(%a2)		# Check sizes
			bhs		set_error
			mov.l	ARG3, %a1			# dest_set
			tst.l	%a1					# null check
			beq		set_error
			cmp.l	%a2, %a1			# src_set == dest_set ?
			bne		del_1

# Sets are the same: simply modify the src_set
del_3:
			mov.l	%a2, %a1			# temp = src_scan
			mov.l	NEXT(%a2), %a2		# src_scan = src_scan->next
			tst.l	%a2
			beq		del_out				# src_scan == NULL ?
			cmp.l	%d0, NUM(%a2)		# src_scan->num < block_num ?
			bhi		del_3

# If this be the block, zero the bit
			bne		del_out				# If wrong block, get out
			mov.l	DATA(%a2), %d0		# Get the data
			not.b	%d1					# Reverse the offset
			bclr	%d1, %d0			# Clear the bit
			tst.l	%d0					# Did we lose this block?
			beq		del_4			
			mov.l	%d0, DATA(%a2)		# Store the bits
			bra		del_out

# We zeroed the bit and found that this block is now empty. 
# It needs to be deleted (pushed onto free_list).
del_4:
			mov.l	NEXT(%a2), NEXT(%a1)	# Splice the old list
			mov.l	free_list, NEXT(%a2)	# Place on the free_list
			mov.l	%a2, free_list
			bra		del_out

# Sets are not the same: copy over blocks until we find ours
del_1:		PUSH( %d2 )					# Need a temporary
			mov.l	NUM(%a2), %a0		# a0 is a temporary here
			cmp.l	%a0, NUM(%a1)		# Check sizes
			bne		set_error
			mov.l	%a1, %a0			# List to place results

# Loop, finding blocks to copy, and looking for the one to delete
del_30:		mov.l	NEXT(%a2), %a2
			tst.l	%a2
			beq		del_9				# End of source
			mov.l	DATA(%a2), %d2		# Get Data
			cmp.l	%d0, NUM(%a2)		# Check block num
			bne		del_31

# equal blocks: do the mask, check for zero
			not.b	%d1					# Reverse order
			bclr	%d1, %d2			# Clear the bit
			tst.l	%d2					# Are we zero ?
			beq		del_30

# either unequal blocks, or there was residual stuff in the block
del_31:
			DESTLIST( %a1, %d1 )		# Get a block
			mov.l	%d2, DATA(%a0)		# copy over the bits
			mov.l	NUM(%a2), NUM(%a0)	# Copy over the block number
			bra		del_30

# End of source
del_9:
			DESTLIST_DONE( %a1, %d1 )	# Restore the various lists
			POP( %d2 )					# Get back our temporary

del_out:
			POP( %a2 )					# Return
			UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: setassign                                (copy sets)
#
# Description: 
#   Copies the source set to the destination set.
#
# Calling:
#   void setassign( src, dest )
#   struct set_entry *src, *dest;		
#
			global	_setassign
_setassign:	LINK
			PROFILE( p_setassign )
			PUSH( %a2 )

# Get pointers: error check
			mov.l	ARG1, %a2		# Get Source
			tst.l	%a2				# source == NULL ?						
			beq		set_error
			mov.l	ARG2, %a0		# Get Destination
			cmp.l	%a2, %a0		# source == destination ?
			beq		seta_out
			tst.l	%a0				# destination == NULL ?
			beq		set_error
			mov.l	NUM(%a2), %d1	# d1 is used here as purely a temporary
			cmp.l	%d1, NUM(%a0)	# Check sizes
			bne		set_error

# While there is source, copy to destination
			clr.l	%d1				# Flag for DESTLIST macros clearing
seta_1:		mov.l	NEXT(%a2), %a2	# move to the next block
			tst.l	%a2				# destination == NULL ?
			beq		seta_2			
			DESTLIST( %a1, %d1 )	# Get a block
			mov.l	DATA(%a2), DATA(%a0)	# Copy the block
			mov.l	NUM(%a2), NUM(%a0)
			bra		seta_1

# End of source
seta_2:		DESTLIST_DONE( %a1, %d1 )	# Cap the lists
			
seta_out:	POP( %a2 )
			UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: clearset                                    (Clear a set)
#
# Description:
#   Clears all data from a set.  The set header block is still there.
#
# Calling:
#   void clearset( src )
#   struct set_entry *src;
#
			global	_clearset
_clearset:	LINK
			PROFILE( p_clearset )

# Error check, get set
			mov.l	ARG1, %a0		# Get set
			tst.l	%a0				# set == NULL ?
			beq		set_error
			mov.l	%a0, %a1		# Get a copy of the header
			mov.l	NEXT(%a0), %a0	# Move to the first block
			clr.l	NEXT(%a1)		# Cap the header
			mov.l	%a0, %d0		# Get a copy, set Z flag
			beq		clear_out
			
# Scan set, look for end
clear_1:	mov.l	%a0, %a1		# { temp = scan
			mov.l	NEXT(%a0), %a0	#   scan = scan->next
			tst.l	%a0				
			bne		clear_1			# } while( scan )
		
			mov.l	free_list, NEXT(%a1)	# Place on free_list
			mov.l	%d0, free_list

clear_out:	UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: addsetrange                        (add range of elements)
#
# Description:
#   This procedure will add a range of elements to a set.  Note that this
#   procedure does not have a destination, the source is modified.
#
# Calling:
#   void addsetrange( low, high, set )
#   unsigned int low, high;
#   struct set_entry *set;
#
			global	_addsetrange
_addsetrange:
			LINK
			PROFILE( p_addsetrange )
			PUSH( %d2 )				# Need some temporaries
			PUSH( %d3 )

# Get sets, element numnbers, SOME error checking (some to come below)
			mov.l	ARG3, %a0		# Get the set
			tst.l	%a0				# Error Checking
			beq		set_error

			mov.l	ARG1, %d0		# Get Low element number
			mov.l	%d0, %d1		# Make a copy
			lsr.l	&5, %d0			# Get block number

			mov.l	ARG2, %d2		# Get High element number
			mov.l	%d2, %d3		# Make a copy
			lsr.l	&5, %d2			# Check element range

			cmp.l	%d1, %d3		# low < high ?		
			bhi		set_error
			cmp.l	%d3, NUM(%a0)	# Check size bounds
			bhs		set_error
			
			and.l	&0x1f, %d1		# Get lower bits (for offsets)
			and.l	&0x1f, %d3		
			
# Search for the block needed
adds_1:
			mov.l	%a0, %a1		# Update the pointers
			mov.l	NEXT(%a0), %a0	
			tst.l	%a0				# scan == NULL ?
			beq		adds_2		
			cmp.l	%d0, NUM(%a0)	# scan->num < block_num ?
			bhi		adds_1			# continue the scan

# If it is not equal, get a block for our use
			beq		adds_3	
adds_2:
			GETABLOCK
			mov.l	NEXT(%a1), NEXT(%a0)	# Place in our list
			mov.l	%a0, NEXT(%a1)
			mov.l	%d0, NUM(%a0)	# Copy over information
			clr.l	DATA(%a0)		# Clear the data area (for or'ing later)

# We have the block, we need to add the correct bits
adds_3:
			cmp.l	%d0, %d2		# same block for high and low ?
			bne		adds_4
			sub.l	%d1, %d3		# subtract off the low offset
			addq.l	&1, %d3			# need one more
			bfset	DATA(%a0){%d1:%d3}	# set the bits
			bra		adds_out
adds_4:
			mov.l	%d4, %a1		# One more temporary
			mov.b	%d1, %d4		# Calculate the width of the word
			neg.b	%d4				# negate (subtract from 32)
			bfset	DATA(%a0){%d1:%d4}    # Set the bits
			mov.l	%a1, %d4		# Get back our temporary

# Loop through, filling in the interim blocks
adds_10:
			addq.l	&1, %d0			# ++block_low
			cmp.l	%d0, %d2		# block_low < block_high
			bhs		adds_21

			mov.l	%a0, %a1		# Save the old one
			mov.l	NEXT(%a0), %a0	# Move down the list
			tst.l	%a0				# Do we need a new block ?
			beq		adds_12			
			cmp.l	%d0, NUM(%a0)	# Are we on the correct block ?
			beq		adds_13

# Not the correct block number: insert a block
adds_12:
			GETABLOCK
			mov.l	NEXT(%a1), NEXT(%a0)	# Put in our list
			mov.l	%a0, NEXT(%a1)
			
			mov.l	%d0, NUM(%a0)			# Fill in some info
adds_13:
			mov.l	&0xffffffff, DATA(%a0)	# Fill in the data (all 1's)
			bra		adds_10
			
# Now, we are on the last block, find out if we have this block present
adds_21:
			mov.l	%a0, %a1		# Move down the list
			mov.l	NEXT(%a0), %a0	
			tst.l	%a0				# End of list ?
			beq		adds_22							
			cmp.l	%d2, NUM(%a0)	# Did we find our block ?
			beq		adds_23

adds_22:	GETABLOCK				# We need a block
			mov.l	NEXT(%a1), NEXT(%a0)	# Put in our current list
			mov.l	%a0, NEXT(%a1)
			mov.l	%d2, NUM(%a0)	# Fill in block number
			clr.l	DATA(%a0)		# Clear the data 

# We have a block, set the high bits
adds_23:	addq.l	&1, %d3			# need one more
			bfset	DATA(%a0){&0:%d3}	# set the bits

adds_out:
			POP( %d3 )
			POP( %d2 )
			UNLK
			rts
			
#----------------------------------------------------------------------
#
# Procedure: setunion                               (union of two sets)	
#
# Description:
#   This procedure will take the union of two sets, and return this
#   in the second set. i.e., dest <-- set1 U dest.
#
# Calling:
#   void setunion( set1, dest )
#   struct set_entry *set1, *dest;						
#
			global	_setunion
_setunion:	LINK
			PROFILE( p_setunion )
			PUSH( %a2 )

# Get the pointers, do some error checking, trivial case stuff
			mov.l	ARG1, %a1		# Set 1
			tst.l	%a1				# Null Check
			beq		set_error
			mov.l	ARG2, %a0		# Set 2
			tst.l	%a0				# Null Check		
			beq		set_error
			cmp.l	%a1, %a0		# Trivial case: set1 == dest
			beq	setu_out

			mov.l	NUM(%a0), %d0	# Check sizes
			cmp.l	%d0, NUM(%a1)	
			bne		set_error
			
# Scan down set1/destination, adding blocks for set2 as needed
			mov.l	%a0, %a2		# old = dest
			mov.l	NEXT(%a0), %a0		# dest = dest->next			
setu_10:		mov.l	NEXT(%a1), %a1		# Move down set1
			tst.l	%a1			# done ?
			beq	setu_out

			mov.l	NUM(%a1), %d0		# Get the block number
			bra	setu_15			# Don't increment %a0 just yet

# loop throuh set1, searching for that block
setu_13:		mov.l	%a0, %a2		# Move down the list
			mov.l	NEXT(%a0), %a0	

setu_15:		tst.l	%a0
			beq	setu_12
setu_16:		cmp.l	%d0, NUM(%a0)		# Correct block number ?
			bhi	setu_13

# If we are the correct block, or in the data
			bne	setu_12
			mov.l	DATA(%a1), %d1		# Or the data in
			or.l	%d1, DATA(%a0)
			bra	setu_10

# Wrong block, add a block
setu_12:		mov.l	%a0, %d1		# Save where we are now
			GETABLOCK				
			mov.l	%d1, NEXT(%a0)	# stick in our list
			mov.l	%a0, NEXT(%a2) 
			mov.l	%d0, NUM(%a0)	# fill in information (block_num)
			mov.l	DATA(%a1), DATA(%a0)  # and data
			bra	setu_10

# Exit
setu_out:		POP( %a2 )
			UNLK
			rts
#----------------------------------------------------------------------
#
# Procedure: intersect                       (intersection of two sets)
#
# Description:
#   This code will take the intersect of two sets, and return the 
#   result in dest.
#		
# Calling:
#   void intersect( set1, set2, dest )
#   struct set_entry *set1, *set2, *dest;			
#
			global	_intersect
_intersect: LINK
			PROFILE( p_intersect )
			PUSH( %a2 )
			PUSH( %a3 )

# Get the pointers, do some error checking, trivial case stuff
			mov.l	ARG1, %a0		# Set 1
			tst.l	%a0				# Null Check
			beq		set_error
			mov.l	ARG2, %a1		# Set 2
			tst.l	%a1				# Null Check		
			beq		set_error
			mov.l	ARG3, %a2		# Set 3 : Destination
			tst.l	%a2				# Null Check
			beq		set_error
			cmp.l	%a0, %a1		# Trivial case: set1 == set2
			bne		inter_1
			mov.l	%a2, -(%sp)		# Push arguments on stack, call setassign
			mov.l	%a1, -(%sp)
			jsr		_setassign
			addq.l	&8, %sp			# remove paramters
			bra		inter_out			

# If %a1 == %a2, then swap %a0 and %a1
inter_1:	cmp.l	%a1, %a2
			bne		inter_2			
			exg		%a0, %a1		# swap them
			bra		inter_3

# Source == Destination:
inter_2:	cmp.l	%a0, %a2		# Does set1 == destination set ?
			bne		inter_50
inter_3:	mov.l	NUM(%a0), %d0	# Check sizes
			cmp.l	%d0, NUM(%a1)	
			bne		set_error

# Scan down set2, looking for set1/destination blocks
			mov.l	%a0, %a3		# old = set1
			mov.l	NEXT(%a0), %a0	# scan1 = scan1->next

inter_10:	mov.l	%a3, %a2		# replentish
			tst.l	%a0				# if done with set1, we are finished
			beq		inter_out
			mov.l	NEXT(%a1), %a1	# Move down set2
			tst.l	%a1				# At the end ?
			beq		inter_20
			mov.l	NUM(%a1), %d0	# Get block_number
			mov.l	%a2, %d1		# Save where to start deleting
			mov.l	%a2, %a3		# Default place to stick %a2 afterward
			bra		inter_15

# loop through set1, deleting blocks
inter_13:	mov.l	%a0, %a2
			mov.l	NEXT(%a0), %a0	# Move down the list
inter_15:	tst.l	%a0				# End of list
			beq		inter_18
			cmp.l	%d0, NUM(%a0)	# Compare block numbers
			bhi		inter_13
			bne		inter_18

# We have the same block: check the data, see if we need to delete this
			mov.l	DATA(%a1), %d0	# Get the data
			and.l	%d0, DATA(%a0)	# Store it
			bne		inter_19

# We don't need this block any more, delete it
			mov.l	%a0, %a2		# Delete %a2's block
			mov.l	NEXT(%a0), %a0
			mov.l	%a0, %d0		# Get ready to jump in after inter_18
			bra		inter_18b

# Up the pointer quietly, if %a2 lags, then delete
inter_19:	mov.l	%a0, %a3		# Change the back pointer (only case)
			mov.l	NEXT(%a0), %a0	# Update scan

# Do we have blocks to kill ?
inter_18:	cmp.l	%d1, %a2		# Did we go over any blocks ?
			beq		inter_10		
			mov.l	NEXT(%a2), %d0			# Splice the list/free_list
inter_18b:	mov.l	free_list, NEXT(%a2)
			mov.l	%d1, %a2		
			mov.l	NEXT(%a2), free_list
			mov.l	%d0, NEXT(%a2)
			bra		inter_10
			
# Done with the loop: must be extra blocks, free them.
inter_20:	clr.l	NEXT(%a2)		# cap off the list
			tst.l	%a0				# anything left to free?
			beq		inter_out
			mov.l	%a0, %d0		# save the head of the list
inter_21:	mov.l	%a0, %a2		# move down the list
			mov.l	NEXT(%a0), %a0	
			tst.l	%a0	
			bne		inter_21
			mov.l	free_list, NEXT(%a2)  	# place on the free list
			mov.l	%d0, free_list
			bra		inter_out		# out of here...

# Source != Destination
inter_50:	mov.l	NUM(%a0), %d0	# Check sizes
			cmp.l	%d0, NUM(%a1)	# against one set	
			bne		set_error
			cmp.l	%d0, NUM(%a2)	# and against the other set.
			bne		set_error

			mov.l	NEXT(%a0), %a0	# Move to the data blocks
			mov.l	NEXT(%a1), %a1

			exg		%a0, %a2		# a0 is our destination
			clr.l	%d1				# clear our flag

inter_59:	tst.l	%a1				# Check against null
			beq		inter_60
			tst.l	%a2
			beq		inter_60		

# Check the blocks.  If same (and some bits remain) then store in a block.
			mov.l	NUM(%a2), %d0	# Get block number
			cmp.l	%d0, NUM(%a1)	
			bhi		inter_51		# too high, up %a1
			blo		inter_52 		# too low, up %a2

			mov.l	DATA(%a2), %d0	# Get the data and do the 'and'
			and.l	DATA(%a1), %d0
			beq		inter_53		# Result == 0, up both pointers

			DESTLIST( %a3, %d1 )	# Get a block
			mov.l	%d0, DATA(%a0)		# Fill in the information
			mov.l	NUM(%a2), NUM(%a0)

# Up the pointers, based on which route we took
inter_53:	mov.l	NEXT(%a2), %a2	# Up both pointers
inter_51:	mov.l	NEXT(%a1), %a1	
			bra		inter_59
inter_52:	mov.l	NEXT(%a2), %a2	
			bra		inter_59

# All done, fix lists
inter_60:	DESTLIST_DONE( %a3, %d1 )

inter_out:	POP( %a3 )
			POP( %a2 )
			UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: difference               (find the difference of two sets)
#
# Description: 
#    Destination set = set1 - set2.  Note that the numbers are switched
#    in the calling sequence.
#
# Calling:
#    void difference( set2, set1, dest )
#    struct set_entry *set1, *set2, *dest;
#
			global	_difference
_difference:
			LINK
			PROFILE( p_difference )
			PUSH( %a2 )
			PUSH( %a3 )
			PUSH( %a4 )

# Get the pointers, do some error checking, trivial case stuff
# The set numbers are reversed, since the calling sequence is reversed
# (that is, ARG1->%a1, ARG2->%a0)
			mov.l	ARG1, %a1		# Set 1
			tst.l	%a1				# Null Check
			beq		set_error
			mov.l	ARG2, %a0		# Set 2
			tst.l	%a0				# Null Check		
			beq		set_error
			mov.l	ARG3, %a2		# Set 3 : Destination
			tst.l	%a2				# Null Check
			beq		set_error
			cmp.l	%a0, %a1		# Trivial case: set1 == set2
			bne		diff_1
			mov.l	%a2, -(%sp)		# Push argument on stack, call clearset
			jsr		_clearset
			addq.l	&4, %sp			# remove paramters
			bra		diff_out

# set1 == destination, set2 != destination
diff_1:		cmp.l	%a0, %a2		# check pointers
			bne		diff_2
			mov.l	NUM(%a0), %d0	# check sizes
			cmp.l	%d0, NUM(%a1)
			bne		set_error

# initialize before the loop
			mov.l	NEXT(%a0), %a0	# scan1 = set1->next
			tst.l	%a0				# Null ?
			beq		diff_out

# loop: go down set1, looking for blocks in set2 that match
diff_11:
			mov.l	NEXT(%a1), %a1	# move down set2
			tst.l	%a1				# Null ?
			beq		diff_out

# chcek blocks: scan if too low, finish if too high
diff_14:	mov.l	NUM(%a1), %d0	# check block numbers
			cmp.l	%d0, NUM(%a0)
			blo		diff_11			# high, we are finished
			beq		diff_15			# equal, do some work
			mov.l	%a0, %a2
			mov.l	NEXT(%a0), %a0	# move down set1
			tst.l	%a0				# hit null?
			bne		diff_14	
			bra		diff_out

# equal blocks: do the operation, if no remaining bits, delete block
diff_15:	mov.l	DATA(%a1), %d0	# Bits that we don't want
			not.l	%d0				# Bits that we DO want
			and.l	DATA(%a0), %d0	# and with set1
			beq		diff_16
			mov.l	%d0, DATA(%a0)	# store the bits
			mov.l	%a0, %a2		# update the pointer
			mov.l	NEXT(%a0), %a0	
			tst.l	%a0
			bne		diff_11			
			bra		diff_out		# finished

# equal blocks, but no bits left.  delete the block
diff_16:	mov.l	NEXT(%a0), %d0			# put on the free list
			mov.l	%d0, NEXT(%a2)
			mov.l	free_list, NEXT(%a0)
			mov.l	%a0, free_list
			mov.l	%d0, %a0				# update the forward pointer
			tst.l	%a0						# Null ?
			bne		diff_11
			bra		diff_out

# set2 == destination, set1 != destination
# This code does not follow the C, simply bacuse it is way too complex.
# Instead, we build a new set, and run through the code for
# dest1,2 != destination, and then fix the pointers.
diff_2:		cmp.l	%a1, %a2		# Does set2 == destination set ?
			bne		diff_3
			mov.l	NUM(%a0), %d0	# Check sizes
			cmp.l	%d0, NUM(%a1)	
			bne		set_error
			mov.l	&0x40000000, %d1	# Set the flag for DESTLIST (bit 30)

# Move registers around: a0 = dest, a1 = set1, a2 = set2->next, a4 = set2
			exg		%a0, %a1		# Move register
			mov.l	NEXT(%a2), %a2	# Move down the list
			mov.l	free_list, NEXT(%a0)	# Get the free list
			tst.l	%a2				# If nothing, we are finished
			beq		diff_31
			mov.l	%a2, free_list	# Store the new top of list
			bset 	&27, %d1 		# our special flag
									# Don't worry about %a4, must set below
			bra		diff_31

# set1/set2 != destination
diff_3:		mov.l	NUM(%a0), %d0	# Check sizes
			cmp.l	%d0, NUM(%a1)	# against one set	
			bne		set_error
			cmp.l	%d0, NUM(%a2)	# and against the other set.
			bne		set_error

			exg		%a0, %a1		# a0 is our destination
			exg		%a0, %a2 		# need to keep order correct
			mov.l	NEXT(%a2), %a2	# Move to the data on scan2
			clr.l	%d1				# clear our flag

# loop, looking for blocks to add from set1, after checking with set2
diff_31:	mov.l	NEXT(%a1), %a1
			tst.l	%a1				# scan1
			beq		diff_39
			mov.l	NUM(%a1), %d0	# get the block_number
			bra		diff_32

# look for a matching set2 block
diff_33:	mov.l	%a2, %a4		# Store the backward pointer
			mov.l	NEXT(%a2), %a2	# move down our list
diff_32:	tst.l	%a2				# scan2
			beq		diff_34
			cmp.l	%d0, NUM(%a2)	# found out block?
			bhi		diff_33

# if equal blocks, check for any bits remaining
			bne		diff_34
			mov.l	DATA(%a2), %d0	# get bits we don't want
			mov.l	%a2, %a4		# Save the back pointer
			mov.l	NEXT(%a2), %a2	# we are done with this block 
			not.l	%d0				# bits we do want
			and.l	DATA(%a1), %d0	
			bne		diff_35			# go ahead and add the block
			bra		diff_31			# continue the scan

# unequal, we need a block
diff_34:	mov.l	DATA(%a1), %d0	# get data
diff_35:	DESTLIST( %a3, %d1 )	# Get a block for it
			mov.l	%d0, DATA(%a0)	# store data
			mov.l	NUM(%a1), NUM(%a0)	# store block_num
			bra		diff_31
			
# store extra blocks, exit
diff_39:	btst	&27, %d1		# Check our special bit (from diff_2)
			bne		diff_40

# We are the continuation of diff_3
			DESTLIST_DONE( %a3, %d1 )
			bra		diff_out
			
# We are the continuation of diff_2
diff_41:	mov.l	%a2, %a4
			mov.l	NEXT(%a2), %a2
diff_40:	tst.l	%a2				# Did we hit the end ?
			bne		diff_41
			mov.l	NEXT(%a0), NEXT(%a4)	# The free_list is now complete
			clr.l	NEXT(%a0)

# We are leaving...			
diff_out:	POP( %a4 )
			POP( %a3 )
			POP( %a2 )
			UNLK
			rts		

#----------------------------------------------------------------------
#
# Procedure: setcompare                                  (compare sets)
#
# Description:
#   This procedure returns an integer (0-4) which indicates the following:
#                 0 -- disjoint sets
#                 1 -- set1 IN set2
#                 2 -- set2 IN set1
#                 3 -- set1 == set2
#                 4 -- set1 and set2 overlap, but no containment
#
# Calling:
#   int setcompare( set1, set2 )
#   struct set_entry *set1, *set2;
#
			global	_setcompare
_setcompare:
			LINK
			PROFILE( p_setcompare )
			PUSH( %d2 )
			PUSH( %d3 )

# Startup, get pointers, error checking
			mov.l	ARG1, %a0			# set1
			tst.l	%a0					# null check
			beq		set_error
			mov.l	ARG2, %a1			# set2
			tst.l	%a1					# null check
			beq		set_error
			mov.l	NUM(%a0), %d1		# check set sizes
			cmp.l	%d1, NUM(%a1)	
			bne		set_error
			movq.l	&3, %d0				# start off with all bits set

# Move to the data
			mov.l	NEXT(%a0), %a0
			mov.l	NEXT(%a1), %a1

# Loop, looking for matching blocks
setcmp_1:	tst.l	%a0					# set1 finished ?
			beq		setcmp_30
setcmp_1b:	tst.l	%a1					# is set2 finished ?
			beq		setcmp_40
			
			mov.l	NUM(%a0), %d1		# Matching Block ?
			cmp.l	%d1, NUM(%a1)			
			blo		setcmp_10
			bhi		setcmp_20
			
# equal blocks, compare data
# This section of code is almost verbatim from the old set routines
# (assembly version, by Scott Omdahl)
			mov.l	DATA(%a0), %d1		# Get the Data
			mov.l	DATA(%a1), %d2
		
			mov.l	%d1, %d3			# Any bits in common ?
			and.l	%d2, %d3
			beq		setcmp_null			# No overlap, look for only's
			cmp.l	%d3, %d1			# *set1 == NULL ?
			beq		setcmp_i2
			and.b	&2, %d0				# no, so set1 IN set2 not possible
			beq		setcmp_overlap
setcmp_i2:	
			cmp.l	%d3, %d2			# *set2 == NULL ?
			beq		setcmp_i3
			and.b	&1, %d0				# no, so set2 IN set1 not possible
			beq		setcmp_overlap
setcmp_i3:
			or.b	&4, %d0				# non-empty intersection -> overlap
			bra		setcmp_2

# No intersection, find which one is null (if any)
setcmp_null:
			tst.l	%d1					# was *set1 empty?
			beq		setcmp_e2
            and.b   &6, %d0             # no, set1 IN set2 not possible
setcmp_e2:	
			tst.l	%d2					# was *set2 empty?
			beq		setcmp_2
			and.b	&5, %d0				# no, set2 IN set1 not possible

# Update pointers, continue on
setcmp_2:	mov.l	NEXT(%a0), %a0		# move down the lists
			mov.l	NEXT(%a1), %a1
			bra		setcmp_1			

# set1 < set2: set1 IN set2 not possible
setcmp_10:	and.b	&6, %d0				# as above
			mov.l	NEXT(%a0), %a0		
			bra		setcmp_1

# set1 > set2: set2 IN set1 not possible
setcmp_20:	and.b	&5, %d0				# as above
			mov.l	NEXT(%a1), %a1		
			bra		setcmp_1b

# overlap: we have overlap and disjoint
setcmp_overlap:
			movq.l	&4, %d0
			bra		setcmp_out_1
			
# After the loop: any blocks remaining?
setcmp_30:	tst.l	%a1
			beq		setcmp_out
			and.b	&5, %d0				# set2 IN set1 not possible
			bra		setcmp_out

setcmp_40:	and.b	&6, %d0				# we know %a0 is not empty
										# set1 IN set2 not possible
			
# finito: check result == overlap flag (again, see Scott's code)
setcmp_out:
			cmp.l	%d0, &4				# result == overlap ?
			beq		setcmp_out_1
			and.b	&3, %d0				# no, strip overlap bit
setcmp_out_1:
			POP( %d3 )
			POP( %d2 )
			UNLK
			rts
			
#----------------------------------------------------------------------
# 
# Procedure: differentsets        (compares two sets for equality only)
#
# Description:
#   This is a modified version of setcompare, which checks only for
#   two sets equality.  1 = not equal, 0 = equal.
#
# Calling:
#   int differentsets( set1, set2 )
#   struct set_entry *set1, *set2;
#
			global	_differentsets
_differentsets:
			LINK
			PROFILE( p_differentsets )
			
# get the pointers, do error checking
			mov.l	ARG1, %a0			# set1
			tst.l	%a0					# Null check
			beq		set_error
			mov.l	ARG2, %a1			# set2
			tst.l	%a1					# Null check
			beq		set_error
			clr.l	%d0					# Exit code for trivial case
			cmp.l	%a0, %a1			# Trivial case...
			beq		diffs_out
			mov.l	NUM(%a0), %d0		# check sizes
			cmp.l	%d0, NUM(%a1)
			bne		set_error
			movq.l	&1, %d0				# assume they are different
	
# scan down both
diffs_1:	mov.l	NEXT(%a0), %a0		# move down both lists
			mov.l	NEXT(%a1), %a1
			tst.l	%a0					# check for ends of lists
			beq		diffs_2
			tst.l	%a1
			beq		diffs_out

			mov.l	NUM(%a0), %d1		# Check block numbers
			cmp.l	%d1, NUM(%a1)
			bne		diffs_out

			mov.l	DATA(%a0), %d1		# Check data
			cmp.l	%d1, DATA(%a1)		
			bne		diffs_out
			bra		diffs_1

# %a0 == NULL, check %a1
diffs_2:	tst.l	%a1
			bne		diffs_out
			clr.l	%d0				# %a0 == %a1 == NULL, same

diffs_out:	UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: isemptyset                             (is the set empty?)
#
# Description:
#   Simply checks to see if a set has been empty.
#
# Calling:
#   int isemptyset( set )
#   struct set_entry *set;
#
			global	_isemptyset
_isemptyset:
			LINK
			PROFILE( p_isemptyset )

			mov.l	ARG1, %a0		# Get the set
			tst.l	%a0				# Null check
			beq		set_error
			tst.l	NEXT(%a0)		# Check the next pointer
			seq		%d0				# Set accordingly
			and.l	&1, %d0			# Get just the last bit
			
			UNLK
			rts

#----------------------------------------------------------------------
#
# Procedure: set_size                        (return max size of a set)
#
# Description:
#   Returns the size of a set, right out of the header.
#
# Calling:
#   int set_size( set )
#   struct set_entry *set;
#
			global	_set_size
_set_size:	LINK
			PROFILE( p_set_size )

			mov.l	ARG1, %a0		# Get the set
			tst.l	%a0				# Null check
			beq		set_error
			mov.l	NUM(%a0), %d0	# Get the size

			UNLK
			rts

#----------------------------------------------------------------------
# 
# Procedure: change_set_size             (modify the max size of a set)
#
# Description:
#    Will change the maximum size of a set.  Note that no scan is made
#    to insure that there are no blocks which would be invalid under
#    the new size (when reducing the size).  Care should be taken.
#
# Calling:
#    void change_set_size( set, newsize )
#    struct set_entry *set;
#    int newsize;
#
			global	_change_set_size
_change_set_size:
			LINK
			PROFILE( p_change_set_size )

			mov.l	ARG1, %a0			# Get the set
			tst.l	%a0					# Null Check
			beq		set_error
			mov.l	ARG2, %d0			# Get the new size, >= 0
			bmi		set_error
			mov.l	%d0, NUM(%a0)		# Store the new size
	
			UNLK
			rts
			
#-----------------------------------------------------------------------
# 
# Internal Procedure: get_sets					(malloc more set blocks)
#
# Description:
#    This procedure is called when the 'free_list' is empty, to
#    replentish it.  The M4 Define "NUM_BLOCKS" determines the number
#    of blocks which are allocated at a time.
#
#    All registers are preserved, except 'a0' which will hold the value
#    of the free_list upon return.  BE SURE TO FILL IN THE VALUE OF THE
#    FREE_LIST, presumably after using the top element.
#
get_sets:	mov.l	%a1, -(%sp)			# Save Registers
			mov.l	%d0, -(%sp)
			mov.l	%d1, -(%sp)

			mov.l	&eval(NUM_BLOCKS*STRUCT_SIZE), %d0 	   # Number of blocks
			mov.l	%d0, -(%sp)					# Push for call
			jsr		_malloc			
			tst.l	%d0					# Null pointer returned ?
			beq		set_malloc_bad
			addq.l	&4, %sp

			mov.l	%d0, %a1				# For scan
			mov.l	%a1, %a0				# For return

# Initialize everything (make the block into a list)
			mov.l	&eval(NUM_BLOCKS-2), %d1  # Number of times to iterate
get_1:		add.l	&STRUCT_SIZE, %a1		# Up the pointer
			mov.l	%a1, -4(%a1)			# last->next = current
			dbra	%d1, get_1

# Put a NULL capper on the end
			clr.l	NEXT(%a1)
						
# pop registers and return
			mov.l	(%sp)+, %d1
			mov.l	(%sp)+, %d0
			mov.l	(%sp)+, %a1
			rts

#-----------------------------------------------------------------------
#
# Internal Procedure: set_error                          (bitch and die)
#
# Description:
#    This routine is called from anywhere that a problem was seen.  A return
#    from this routine is not expected.
#
set_malloc_bad:
			mov.l	&error_msg2, -(%sp)
			jsr		_uerror
# NEVER RETURNS
			trap	&8						# .. just in case it does

set_error:	
			mov.l	&error_msg, -(%sp)		# Get message
			jsr		_cerror					# NEVER RETURNS
			trap	&8						# .. just in case it does

			data
error_msg:	byte	"Set Error Detected\n",0
error_msg2: byte    "Out of Memory\n",0
			text

# <<<<<<<<<<<<<<<<<<<<<<<< End of Sets.s >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
