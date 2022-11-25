/*
 * Signals using sigvector() instead of SysV signal().
 *
 */

#include <signal.h>

/*VARARGS*/
void (*signal(sig, action))()
int sig;
void (*action)();
{

	struct sigvec vec, ovec;

	vec.sv_handler = action;
	vec.sv_mask = 0;
	vec.sv_flags = 0;

	if (sigvector(sig, &vec, &ovec) < 0)
		return((void (*)())-1);
	return(ovec.sv_handler);
}

int sigvec(sig, vec, ovec)
int sig;
struct sigvec *vec, *ovec;
{
	return(sigvector(sig, vec, ovec));
}
