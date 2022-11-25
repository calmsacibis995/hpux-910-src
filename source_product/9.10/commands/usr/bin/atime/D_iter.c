/*

    Dprint_iterations_code()

    Create code to print the iteration count.

		data
	iterate:byte	"iterations:",9
		byte	....		# ASCII representation of iterations

		text
		mov.l	file,-(%sp)
		pea	{length}	# iteration string length + 13
		pea	1.w
		pea	iterate
		jsr	_fwrite		# fwrite(iterate,1,{length},file)
		lea	16(%sp),%sp

*/

#include "fizz.h"

void Dprint_iterations_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    char iterations[11];

    sprintf(iterations, "%u", gIterations);

    print(output, "\tdata\n");
    print(output, "iterate:\tbyte\t\"iterations:\",9\n");
    print_byte_info(iterations);
    print(output, "\tbyte\t10\n");
    print(output, "\ttext\n");
    print(output, "\tmov.l\tfile,-(%%sp)\n");
    print(output, "\tpea\t%d\n", strlen(iterations) + 13);
    print(output, "\tpea\t1.w\n");
    print(output, "\tpea\titerate\n");
    print(output, "\tjsr\t_fwrite\n");
    print(output, "\tlea\t16(%%sp),%%sp\n");

    return;
}
