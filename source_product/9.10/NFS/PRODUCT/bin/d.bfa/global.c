#include <stdio.h>
#include "defines.h"

FILE	*outfile = NULL;

STACK_ELEMENT	*stack = NULL;
int		tos = -1;
int		stack_elements = DEFAULT_STACK_MAX;

char	*stream_string = NULL;
int	stream_pos = 0;
int	stream_len = DEFAULT_STREAM_LEN;

char	*else_string = NULL;
int	else_pos  = 0;
int	else_len  = DEFAULT_ELSE_LEN;

int	quest_count= 0;
int	curl_count = 0;
int	semi_cr	   = 0;
int	no_update  = 0;
int	specifier  = 0;


int	line_number = 1;

char  *input_files[MAXFILES];         	/* Files to process pointers 	*/
char  cur_output[MAXPATH];              /* Current output file name     */
char  dbase_file[MAXPATH];              /* Database file name 		*/
int   cur_input = 0;			/* Current file pointer index 	*/
int   num_input = 0;			/* Number of files to process	*/

char  cur_func[MAXPATH];                /* Current Function Name        */


char  *main_ids[MAXMAINS];		/* Main procedure names		*/
int   main_cnt     = 0;			/* Main procedure name count	*/


int    h_flag  = 0;   /* Record only loop entries not iterations        */
int    a_flag  = 1;   /* Record only loop interation not entries        */
int    q_flag  = 0;   /* Suppress echoing of file names                 */
int    m_flag  = 0;   /* Dynamic allocation of branch flow data flag    */
int    i_flag  = 0;   /* Include file code generation flag              */


int    e_flag  = 1;   /* Exit substitution flag				*/

#if defined(FORK)
int    f_flag  = 1;   /* Fork substitution flag				*/
#endif

#if defined(EXEC)
int    x_flag  = 1;   /* Exec substitution flag				*/
#endif

#if defined(BFA_KERNEL)
int    k_flag  = 0;   /* Generate BFA for kernel code.                  */
#endif

#if defined(TSR)
int    r_flag  = 0;   /* Terminate and stay resident flag 		*/
int    vector  = INTERRUPT_VECTOR;
#endif

#if defined(BFAWIN)
int	w_flag = 0;   /* Generate MS-Windows bfa code			*/
#endif

char  c_args[MAXLINE];			/* Buffer to hold command args  */
char  cpp[MAXLINE];			/* C Preprocessor command       */
char  cpp_end[MAXLINE];			/* C Preprocessor command ending*/

int    bfa_ret = 0;			/* Return value			*/

