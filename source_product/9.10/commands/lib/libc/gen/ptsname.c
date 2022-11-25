
#ifdef _NAMESPACE_CLEAN
#    undef ptsname
#    pragma _HP_SECONDARY_DEF _ptsname ptsname
#    define ptsname _ptsname

#    define stat _stat
#    define fstat _fstat
#    define sprintf _sprintf
#endif


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <string.h>

#define MPTYMAJOR 16    		/* major number for master pty's */
#define SPTYMAJOR 17    		/* major number for slave  pty's */
#define SPTYDIR   "/dev/pty/"  		
#define SPTYNAME  "tty"

#define PTYNAMLEN     128         /* Maximum absolute path length for a pty */
#define ERROR ((char *) 0)

static char alpha[] = {'p','q','r','s','t','u','v','w','x','y',
		       'z','a','b','c','e','f','g','h','i','j',
		       'k','l','m','n','o'};

static struct {
	int size;
	int modval;
	char *format;
       }   range_spec[] = {
			    {   400,   16, "%c%x"  },
			    {  2500,  100, "%c%02d"},
			    { 25000, 1000, "%c%03d"},
			    {     0,    0,       0 }
		          };

/*-----------------------------------------------------------------------------
 |	FUNCTION -	Given an open master pty file-descriptor return the
 |			name of the corresponding slave pty.
 |
 |	Input    -	file descriptor of an open master pty.
 |
 |	Output   -	pointer to path of slave pty.
 |			on error return null char pointer.
 |
 |	Name-Space      It is a union of three name-spaces, the earlier two
 |			being supported for compatibility. The name comprises
 |			a generic name (/dev/pty/tty) followed by an alphabet
 |			followed by one to three numerals. The alphabet is one
 |			of the 25 in alpha[], which has 'p' thro' 'o' excluding
 |			'd'. The numeral is either hex or decimal.
 |	                ------------------------------------
 |	                | minor | name  |    Remarks       |
 |	                |----------------------------------|
 |                      |      0|  ttyp0|  Modulo 16 hex   |
 |	                |      :|      :|  representation. |
 |	                |     15|  ttypf|                  |
 |	                |     16|  ttyq0|                  |
 |	                |      :|      :|                  |
 |	                |    175|  ttyzf|                  |
 |	                |    176|  ttya0|                  |
 |	                |      :|      :|                  |
 |	                |    223|  ttycf|                  |
 |	                |    224|  ttye0|                  |
 |	                |      :|      :|                  |
 |	                |    399|  ttyof|  Total 400       |
 |	                |----------------------------------|
 |	                |    400| ttyp00|  Modulo hundred  |
 |	                |      :|      :|  decimal repr.   |
 |	                |    499| ttyp99|                  |
 |	                |    500| ttyq00|                  |
 |	                |      :|      :|                  |
 |	                |   1499| ttyz99|                  |
 |	                |   1500| ttya00|                  |
 |	                |      :|      :|                  |
 |	                |   1799| ttyc99|                  |
 |	                |   1800| ttye00|                  |
 |	                |      :|      :|                  |
 |	                |   2899| ttyo99|  Total 2500      |
 |	                |----------------------------------|
 |	                |   2900|ttyp000|  Modulo thousand |
 |	                |      :|      :|  decimal repr.   |
 |	                |   3899|ttyp999|                  |
 |	                |   4900|ttyq000|                  |
 |	                |      :|      :|                  |
 |	                |  12899|ttyz999|                  |
 |	                |  13900|ttya000|                  |
 |	                |      :|      :|                  |
 |	                |  16899|ttyc999|                  |
 |	                |  16900|ttye000|                  |
 |	                |      :|      :|                  |
 |	                |  27899|ttyo999|  Total 25000     |
 |	                |  27900|	|     invalid      |
 |	                ------------------------------------
 ----------------------------------------------------------------------------*/

char *
ptsname( fdm )
int fdm;				/* master pty fd */
{
	dev_t dev;
	static char path[ PTYNAMLEN ];
	char tail[5];
	struct stat status;
	u_long offset;
	int i;

	if( fstat( fdm, &status ) == -1) return ERROR;
	dev = status.st_rdev;
	if( major( dev ) != MPTYMAJOR) return ERROR;
	dev = minor( dev );

	/* generate path */

	offset = dev;
	for(i = 0; range_spec[i].size && (offset >= range_spec[i].size); i++)
			offset -= range_spec[i].size;

	if(range_spec[i].size == 0) return ERROR;
	sprintf(tail, range_spec[i].format, 
				alpha[offset/range_spec[i].modval] ,
				(offset%range_spec[i].modval));

	sprintf( path, "%s%s%s", SPTYDIR, SPTYNAME, tail );

	/* confirm validity of path */

	if(( stat( path, &status ) == -1 ) ||
		( major( status.st_rdev ) != SPTYMAJOR ) ||
		( minor( status.st_rdev ) != dev ))
		return ERROR;
	return( path );
}
