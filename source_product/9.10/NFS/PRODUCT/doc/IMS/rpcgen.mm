
.\ Leave the first line of the file blank as the .TL and .AS
.\ macros do not work otherwise.  Don't ask me why.

.ce 4
.TL
COLORADO NETWORKS DIVISION
IMS FOR RPCGEN



.ce 3
Mike Shipley
CND 1-229-2131 
m_shipley@hpcnd




.AS
The following is the Internal Maintenance Specification for the
rpcgen program.  Rpcgen is part of the NFS product as of the 6.5
release on the Series 300 and 7.0 for the Series 800.

.AE








.ce
\*(DT
.nr P 1
.PH " 'RPCGEN IMS' 'Introduction' "
.bp
.SA 1
.H 1 "Introduction"
.PF " 'HP Internal Use Only' ' \\\\n(H1-\\\\nP'  "


.nf

     Project Name    : SMURFS

.H 2 "Personnel"
     Project Manager : Dave Matthews      (CND)
    
     Project Engineer: Mike Shipley       (CND)

.H 2 "What is rpcgen?"
Rpcgen is a part of NFS3.2 that can take a specification written
in a C-like language for a RPC
(Remote Procedure Call) client and server pair and produce a
series of files that ease the implementation of the RPC client
and server. 
These files consist of C code and header files.
The specification language is stored in a ".x"
file (similar to the .c files of C).  The files that are produced
have their names derived from the main stem of the .x file.
For example if rpcgen is given a file named quick.x, then rpcgen
will produce the following files--quick.h, quick_xdr.x, quick_svc.c
and quick_clnt.c.  The quick.h file contains the data definitions
that tie together the other files.  The quick_xdr.x file contains
the XDR (eXternal Data Representation) routines needed by the
client and server to get data back and forth.  The quick_svc.c file contains
the server program that contains calls to user supplied routines
which will do the actual remote computing.  The quick_clnt.c
has the client side interface routines to make the connection from the
client program to the remote procedures.  

For more details on the functions of rpcgen, see the ERS for rpcgen.  Also
look at the rpcgen chapter of the NFS Programming manual. 

.PH " 'Document Title ' 'History ' "
.bp

.nr P 1
.H 1 "Revision History"
.nf
First Version.....................................October, 1988



.PH " 'RPCGEN IMS' 'List of Files' "
.fi
.bp
.nr P 1
.H 1 "The files that comprise rpcgen"
This section will give a brief description of the files that make
up rpcgen.  All of the files are prefixed with "rpc_" and not
"rpcgen_".  This was done in order to fit the file names within
the 12 character limit that rcs is currently setting.

.H 2 "rpc_cltout.c"
This file contains the routines that produce the stub routines for the
_clnt.c file.

.H 2 "rpc_cout.c"
The xdr routines are produced by this file which will be
placed in the _xdr.c file.

.H 2 "rpc_hout.c"
The .h file is written by the routines in this file.

.H 2 "rpc_main.c"
The main file contains the code that parses the command line options.  It
also has the top level routines that access the code to write the four
output files.

.H 2 "rpc_parse.c"
This file has the input language parsing routines.

.H 2 "rpc_parse.h"
This file contains the data definitions and structures used by the routines
of rpc_parse.c.

.H 2 "rpc_scan.c"
The routines that look at the input characters and turn them into 
symbols meaningful to rpcgen are in rpc_scan.c

.H 2 "rpc_scan.h"
This file contains the data definitions and structures used by the routines
of rpc_scan.c.

.H 2 "rpc_svcout.c"
The server side program is produced by routines in this file.

.H 2 "rpc_util.c"
Various useful functions that are used by the other files of rpcgen
are collected in this file.

.H 2 "rpc_util.h"
Global data definitions that are used by the other files of
rpcgen are defined here.

.PH " 'RPCGEN IMS' 'Major Data Structures ' " 
.fi
.bp
.nr P 1
.H 1 "Names and Connections"

The intermediate text representation of the input given to rpcgen is
maintained as a linked list declared to be of type "list".  A global
pointer "defined" points to this list and it is accessed by many
routines in rpcgen.

Each element of this list points to a "definition" struct.  The
"definition" struct is a union made up of the six struct's that
are used to represent the six constructs that make up the input
language for rpcgen.

The six struct's are "typedef_def", "enum_def", "const_def",
"struct_def", "union_def" and "program_def".  Because each of the
constructs are different from each other, their respective struct's
are different from each other, but they do have some common characteristics.
They will have a pointer to the name of each definition of a construct
(for example, the program name given in a program definition).  
If the construct
is able to have multiple items declared inside it such as a program
definition having multiple versions, its struct will have a list
of those versions.  In this specific case, a version of a program can
have multiple procedures and these are represented as a list pointed
to by each version struct.

Basically the data definitions found in the rpc_parse.h file are used
to describe the client/server pair in a fashion understandable by
the rest of rpcgen.

.PH " 'RPCGEN IMS' 'Program Flow' "
.fi
.bp
.nr P 1
.H 1 "4 basic pieces"

The main program of rpcgen starts by parsing the command options found
on the execution line of rpcgen.  These options can force rpcgen to
generate just one of the possible four output files, or to produce
all four of the files.  If all four files are to be produced, then
they are produced independently of each other.  Each time a new file
is written, the input text is reparsed.  For each type of output file,
there exists a top-level function that controls the creation of the
output file.  The basic form of these functions is to open the input 
and outfile files, write some preliminary C-code to the output file,
call the language parser and then use the results of the parse to 
create their specific output file. 

.PH " 'RPCGEN IMS' 'Parser' "
.fi
.bp
.nr P 1
.H 1 "Parsing section"

The parser for rpcgen is very straight forward and easy to follow by
reading the code and looking at the specification of the input
language for rpcgen.  It is a recursive descent parser.
The formal specification of the rpcgen input language is such that
the parser does not require recursion.

The parser is accessed at the highest level through a call to get_definition().
Before it is called, the input text is passed through the C-preprocessor so
that the writer of the .x file can take advantage of the preprocessor.
For example, rpcgen allows the use ofinclude files.
As each input token is accepted and classified, the parser uses one of
three routines (scan, scan2, scan3) to determine if the next input 
token legally completes the next portion of the formal language of rpcgen.
These routines check for one, two or three possible legal inputs.
If the input language contains a syntax error, the offending line
along with an error message pointing to the area of the line where the
error is detected is printed and then rpcgen terminates.  There is no
attempt to try and do any error recovery.

As the input language is parsed, the "list" list is constructed of the
legally formed constructs of the input.  This list is then used by the
output routines when they generate code.

.PH " 'RPCGEN IMS' 'The Output Section' "
.fi
.bp
.nr P 1
.H 1 "s_output"
The file "rpc_svcout.c" contains the routines that generate the 
server side program.  This program will register the programs defined
by the .x file and then wait for an incoming request to call the
procedure desired by the request.  
This server program will be linked with the user
supplied procedures that do the actual work required by the
incoming requests.
The routines write_most(),
write_rest() and write_programs() are the top level routines that
take the appropriate portions of the "list" list and produce the 
"_svc.c" file.  

.H 1 "h_output"
The h_output() routine loops through the input file calling get_definition()
to parse the input.  For each legal definition, print_datadef() will
write to the ".h" a C legal version of the definition.

.H 1 "l_output"
The client stub routines are produced by the l_output() routine.  These
stubs are what are called by the user supplied client side program
when the program wants to make a remote procedure call.  The stubs
take a parameter, do the correct RPC call to reach the server and
then hand back the result to the client program.
The routine write_stubs() is the top level procedure that takes the
"list" and generates the client stubs in the "_clnt.c" file.

.H 1 "c_output"
Finally the xdr routines are generated by the c_output() routine (not
to be confused with client generation).  These xdr routines are used
to convert arguments and results from internal machine representation
to machine independent format suitable for transmission over to 
a remote machine.  These routines are used by both the client and
server side functions and can be found in the "_xdr.c" file.


.PH " 'RPCGEN IMS' 'HP Additions' "
.fi
.bp
.nr P 1
.H 1 "Improvements"

The differences between the original code and the current code are labeled with
the symbol "HPNFS" in a comment.  In the case of adding NLS (Natural Language
System) to the code, all of these changes are not labeled.

The first differences are the changes to make rpcgen generate HP-UX
compatible C code.  There is code that generates a call to the memset()
function.  It is in the format of BSD syntax which has just two parameters
while HP-UX has three parameters.  So code is generated with "#ifdef hpux"
to allow both forms of the memset call.  This can be found in rpc_cltout.c
and in rpc_svcout.c.
Another change was to generate code that would cause a definition of
NULL to 0 if it is not already defined.  Apparently in SunOS, this symbol
is always defined.  This was done in rpc_main.c

If an array is declared using [] (fixed length array) inside a union
definition in a ".x" file, the code emitted by rpcgen would look something
like this
.nf

	(char *)&objp->the_union_name_u.array_varient

.fi
This will cause the C compiler, when it attempts to compile such a line,
to issue a warning about taking the address
of an array.  A change was made so that the "&" is no longer emitted
which makes the C compiler much happier.  This was done in rpc_cout.c.

In rpc_hout.c, when code that describes an enum construct is generated,
it had a "," after the last element of the enum.
.nf

       enum colors {
	  RED = 0,
	  BLUE = 1,
	  YELLOW = 2,
       };

.fi
While the C compiler does not reject such a construct,
changes were made to eliminate that last "," from the output file.

A new option (-u) was added to rpcgen.  This tells rpcgen to generate
code which will unregister a server program 
from the port mapper if it receives a signal
before terminating.  The rpc_main.c file is where most of these changes
were made.

When the internal list describing the different programs defined in
the ".x" file was created, the last element did not have a NULL assigned
to its tail pointer.  This would cause a segmentation violation
in somewhat unpredictable circumstances.  This was changed in rpc_parse.c.

There were two major changes made to NLSize rpcgen.  The first was the
standard replacement of the message strings that are printed by rpcgen 
with function calls to message catalog files to obtain the proper text.
The second change was to allow rpcgen to handle 16-bit characters inside
comments and quoted strings.  This is needed since the second byte of 16-bit
character could be a double quote which would terminate a quoted string
prematurely.  The changes for the message strings can be found all over
the rpcgen code while the changes for the 16-bit data are in rpc_scan.c.

In the original specification of the rpcgen input language, it would only
allow a numerical constant to be the matching value in a "case" portion
of a union declaration.  A change was made that would allow character
constants and octal constants in the "case".  This change was made in
rpc_scan.c.
.nf

	union read_result  switch (char error) {
	case 'n': char data[1024];
	case 'y': int errno;
	default : void
	};
		
.fi

Another change made to the parsing of unions was in the area of what type
of value could be in the "switch" portion of the union.  The original rpcgen
allowed simple variables and arrays.  It makes no sense to have
an array as the switch variable, so a change was made to reject such
an attempt.  The change can be found in rpc_parse.c.
.nf
 
	This is NO longer allowed:

	union read_result  switch (int an_array[5]) {
	case  1 : char data[1024];
	case  2 : int errno;
	default : void
	};
		
.fi


.PH " 'RPCGEN IMS' 'End' "
.fi
.bp
.nr P 1
.H 1 "Wrap-up"

This IMS is not loaded with lots of details of all of the algorithms of
all of the routines making up rpcgen.  This is mostly because with an
understanding of compilers and the specification of the input
language, the parsing section of the program is easy to understand.
Then by examining the code in the four output files and example code
generated by rpcgen, one can understand specific parts of the code 
generation section.  The changes that were made to the parser and
code generator were done in this fashion and it took only a short time.
There are no really clever algorithms or subtle interactions with
other parts of the operating system that need separate explaination
here.
.\ Now this will produce a table of contents
.TC  


