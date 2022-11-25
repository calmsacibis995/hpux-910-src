#include <signal.h>

sigvec(sig, vec, ovec)
int sig;
struct sigvec *vec, *ovec;
{
	sigvector(sig,vec,ovec);
}
