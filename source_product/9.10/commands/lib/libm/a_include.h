
define(`FP040')   # Turn on support for 040 test code 

# set constants for matherr error types
# THESE VALUES MUST CORRESPOND WITH /usr/include/math.h

	set	DOMAIN,1
	set	SING,2
	set	OVERFLOW,3
	set	UNDERFLOW,4
	set	TLOSS,5
	set	PLOSS,6

# set constants for errno types
# THESE VALUES MUST CORRESPOND WITH /usr/include/errno.h

	set	EDOM,33		# errno value for DOMAIN error
	set	ERANGE,34	# errno value for RANGE error

# set constants for operand type (and return type?)

	set	FLOAT_TYPE,1
	set	DOUBLE_TYPE,2

# set constants for return types

	set	NAN_RET,0	# Return Not-a-number
	set	ZERO_RET,1	# Return zero
	set	INF_RET,2	# Return infinity
	set	HUGE_RET,3	# Return HUGE
	set	NHUGE_RET,4	# Return minus HUGE
	set	MAXFLOAT_RET,5	# Return MAXFLOAT
	set	NMAXFLOAT_RET,6	# Return minus MAXFLOAT
	set	OP1_RET,7	# Return operand 1
	set	ONE_RET,8	# Return 1

	set	PI,0

# pop_double_value() - pops a double value off the stack

	define(`pop_double_value',`
	addq.w	&8,%sp
	')

# call_error(name,addr1,addr2,func_type,return_value,error_type)

	define(`call_error',`
	mov.l	&$6,-(%sp)
	mov.l	&$5,-(%sp)
	mov.l	&$4,-(%sp)
	mov.l	$3,-(%sp)
	mov.l	$2,-(%sp)
ifdef(`PIC',`
	mov.l	&DLT,%a1
	lea.l	-6(%pc,%a1.l),%a1
	mov.l	$1_name(%a1),-(%sp)
	bsr.l	__error_handler
	',`
	pea	$1_name
	jsr	__error_handler
	')
	lea	(24,%sp),%sp

	ifdef(`$1_name_defined',`',`
	data
$1_name:	byte	"$1\0"

	text

	define(`$1_name_defined',`x')
	')
	')

# init_code(name) defines the appropriate global and sglobal values
# for a function and also sets up the needed PROFILE code for the
# profiled version of the library.

	define(`init_code',`

	version 2

	text

	ifdef(`_NAMESPACE_CLEAN',`
	global __$1
	sglobal _$1',`
	global _$1
	')

ifdef(`_NAMESPACE_CLEAN',`
__$1:
')
_$1:
	ifdef(`PROFILE',`
	mov.l	&p_$1,%a0
	jsr	mcount

	data
p_$1:	long	0

	text
	')
	')


define(`CR_PI_X',`0x40000000c90fdaa22168c235')   # Taken from Motorola FPSP

