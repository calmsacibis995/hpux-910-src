/* $Header: Mkmf.h,v 66.4 90/08/10 16:47:18 nicklin Exp $ */

/*
 * Mkmf definitions
 *
 * Author: Peter J. Nicklin
 */

/*
 * Buffer sizes
 */
#define ANSWERBUFSIZE		256	/* interactive answer buffer size */
#define INCLUDETABLESIZE	1021	/* include file table size */
#define MACRODEFSIZE		16384	/* macro definition body size */
#define MACRONAMSIZE		64	/* macro definition name size */
#define MDEFTABLESIZE		127	/* macro definition table size */
#define RULETABSIZE		256	/* rule table size */
#define SFXTABSIZE		256	/* suffix table size */
#define SUFFIXSIZE		16	/* suffix size */
/*
 * Predefined macro names
 */
#define MCFLAGS		"CFLAGS"
#define MCOMPILESYSTYPE	"COMPILESYSTYPE"
#define MCXXFLAGS	"CXXFLAGS"
#define MDESTDIR	"DEST"
#define MEXTERNALS	"EXTHDRS"
#define MFFLAGS		"FFLAGS"
#define MHEADERS	"HDRS"
#define MLIBLIST	"LIBS"
#define MLIBRARY	"LIBRARY"
#define MMAKEFILE	"MAKEFILE"
#define MOBJECTS	"OBJS"
#define MPFLAGS		"PFLAGS"
#define MPROGRAM	"PROGRAM"
#define MSOURCES	"SRCS"
#define MSUFFIX		"SUFFIX"
#define MSYSHDRS	"SYSHDRS"
/*
 * Predefined $(macro) instances
 */
#define DCFLAGS		"$(CFLAGS)"
#define CXXFLAGS	"$(CXXFLAGS)"
#define DDESTDIR	"$(DEST)"
#define DEXTERNALS	"$(EXTHDRS)"
#define DFFLAGS		"$(FFLAGS)"
#define DHEADERS	"$(HDRS)"
#define DLIBLIST	"$(LIBS)"
#define DLIBRARY	"$(LIBRARY)"
#define DMAKEFILE	"$(MAKEFILE)"
#define DOBJECTS	"$(OBJS)"
#define DPFLAGS		"$(PFLAGS)"
#define DPROGRAM	"$(PROGRAM)"
#define DSOURCE		"$(SRCS)"
#define DSUFFIX		"$(SUFFIX)"
#define DSYSHDRS	"$(SYSHDRS)"
/*
 * Predefined ${macro} instances
 */
#define dCFLAGS		"${CFLAGS}"
#define dCXXFLAGS	"${CXXFLAGS}"
#define dDESTDIR	"${DEST}"
#define dEXTERNALS	"${EXTHDRS}"
#define dFFLAGS		"${FFLAGS}"
#define dHEADERS	"${HDRS}"
#define dLIBLIST	"${LIBS}"
#define dLIBRARY	"${LIBRARY}"
#define dMAKEFILE	"${MAKEFILE}"
#define dOBJECTS	"${OBJS}"
#define dPFLAGS		"${PFLAGS}"
#define dPROGRAM	"${PROGRAM}"
#define dSOURCE		"${SRCS}"
#define dSUFFIX		"${SUFFIX}"
#define dSYSHDRS	"${SYSHDRS}"
/*
 * Predefined macro values
 */
#define VERROR		       -1
#define VUNKNOWN		0
#define VREADONLY		1
#define VREADWRITE		2
#define VDYNAMIC		3
#define VDESTDIR		4
#define VPROGRAM		5
#define VLIBRARY		6
/*
 * Predefined template suffixes
 */
#define SPROGRAM		".p"
#define SLIBRARY		".l"
#define SLIBRARY2		".L"
/*
 * Include statement styles
 */
#define INCLUDE_NONE		0	/* no include file */
#define INCLUDE_C		1	/* #include "file" */
#define INCLUDE_CXX		2	/* #include "file" for C++ */
#define INCLUDE_FORTRAN		3	/* include "file" or #include "file" */
#define INCLUDE_PASCAL		4	/* #include "file" */
/*
 * Marker to indicate start of included file dependencies
 */
#define DEPENDMARK		"###"
/*
 * Mkmf directories
 */
#define INSTALLDIR	"/usr"
#define MKMFLIB		"lib/mf"
