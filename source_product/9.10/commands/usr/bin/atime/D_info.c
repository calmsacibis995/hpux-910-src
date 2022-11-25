/*

    Dprint_info_code()

    Create code to print information about name, machine, date,
    size, and instructions.

		data
	info:	byte	"name",9,9
		byte	....		# name
		byte	10,"machine:",9
		byte	....		# machine
		byte	10,"date:",9,9
		byte	....		# date
		byte	10

		text
		mov.l	file,-(%sp)
		pea	{length}	# total length of name, machine & date
		pea	1.w
		pea	info
		jsr	_fwrite		# fwrite(info,1,{length},file)
		lea	16(%sp),%sp

    #if mode is performance analysis
		data
	info1:	byte	"size:",9,9,"%1u bytes",10
		byte	"instructions:"
		byte	{instruction count}	# ASCII representation
		byte	10,0

		text
		lea	___Zovhd,%a0
		sub.l	&___Ztime,%a0	# length of time section
		pea	(%a0)
		pea	info1
		mov.l	file,-(%sp)
		jsr	_fprintf	# fprintf(file,"size:\t\t%1u bytes\n
					# instructions:\t{instruction count}\n",
					# ___Zovhd-___Ztime)
		lea	12(%sp),%sp
    #endif

*/

#include "fizz.h"
#include <sys/param.h>

void Dprint_info_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    unsigned long length;
    char name[L_cuserid + 1];
    char machine[MAXHOSTNAMELEN];
    char date[100], *dateptr;
    long clock;
    char *count;

    length = 26;

    /* get user name */
    (void) cuserid(name);
    length += strlen(name);

    /* get machine name */
    gethostname(machine, sizeof(machine));
    length += strlen(machine);

    /* get date */
    clock = time((long *) 0);
    (void) strncpy(date, ctime(&clock), sizeof(date) - 1);
    dateptr = strchr(date, 10);
    if (dateptr != NULL) *dateptr = '\0';
    else date[sizeof(date) - 1] = '\0';
    length += strlen(date);

    print(output, "\tdata\n");
    print(output, "info:\tbyte\t\"name:\",9,9\n");
    print_byte_info(name);
    print(output, "\tbyte\t10,\"machine:\",9\n");
    print_byte_info(machine);
    print(output, "\tbyte\t10,\"date:\",9,9\n");
    print_byte_info(date);
    print(output, "\tbyte\t10\n");
    print(output, "\ttext\n");
    print(output, "\tmov.l\tfile,-(%%sp)\n");
    print(output, "\tpea\t%d\n", length);
    print(output, "\tpea\t1.w\n");
    print(output, "\tpea\tinfo\n");
    print(output, "\tjsr\t_fwrite\n");
    print(output, "\tlea\t16(%%sp),%%sp\n");

    if (gMode == PERFORMANCE_ANALYSIS) {
	print(output, "\tdata\n");
	print(output, "info1:\tbyte\t\"size:\",9,9,\"%%1u bytes\",10\n");
	print(output, "\tbyte\t\"instructions:\",9\n");
	CREATE_STRING(count, 11);
	sprintf(count, "%1u", get_instruction_count());
	print_byte_info(count);
	free(count);
	print(output, "\tbyte\t10,0\n");
	print(output, "\ttext\n");
	print(output, "\tlea\t___Zovhd,%%a0\n");
	print(output, "\tsub.l\t&___Ztime,%%a0\n");
	print(output, "\tpea\t(%%a0)\n");
	print(output, "\tpea\tinfo1\n");
	print(output, "\tmov.l\tfile,-(%%sp)\n");
	print(output, "\tjsr\t_fprintf\n");
	print(output, "\tlea\t12(%%sp),%%sp\n");
    };

    return;
}
