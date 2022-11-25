/*
 * @(#)ext.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 16:22:27 $
 * $Locker:  $
 */

/*
 * Original version based on:
 * Revision 63.8  88/05/27  16:43:43  16:43:43  markm
 */

/*
 * Copyright Third Eye Software, 1983.
 * Copyright Hewlett Packard Company 1985.
 *
 * This module is part of the CDB/XDB symbolic debugger.  It is available
 * to Hewlett-Packard Company under an explicit source and binary license
 * agreement.  DO NOT COPY IT WITHOUT PERMISSION FROM AN APPROPRIATE HP
 * SOURCE ADMINISTRATOR.
 */

/*
 * Symbolic debugger external declarations file.
 */


/*
 * EXTERNAL VARIABLES
 * ------------------
 */

#ifndef NOTDEF
#undef printf
#undef Printf
#define Printf xdbprintf
#define printf xdbprintf
#endif

#ifdef S200
extern  ADRT    adrFpa;
#endif
extern 	FLAGT 	clearCmdw;
extern  char   *sccsid;
extern  int	sdebug;
extern	uint 	vaddr;
extern	ADRT 	vadrCall;
extern	ADRT 	vadrMain;
extern	ADRT 	vadrStart;
extern 	ADRT 	vap;
extern	ASE 	vas;
extern  int     vbadadr;
extern 	BSE 	vbs;
extern	int 	vcBadMax;
extern 	int 	vcbCmdBuf;
extern	int 	vcbLeHdr;
extern	int 	vcbLHdr;
extern	int 	vcbLnHdr;
extern	long 	vcbLstFirst;
extern	int 	vcbMalloc;
extern	long 	vcbNpFirst;
extern	int 	vcbPeek;
extern	int 	vcbProcHdr;
extern	int 	vcbSbCache;
extern	int 	vcbSbFirst;
extern	long 	vcbSltFirst;
extern	long 	vcbSymFirst;
extern	int 	vcbTok;
extern  uint    vcbUnwindFirst;
extern  uint    vcbStubUnwindFirst;
extern	int 	vcbVarHdr;
extern 	int 	vcLines;
#ifdef CDB
extern 	int 	vcLinesMax;
#endif
extern 	int 	vcmdDef;
extern	int 	vcNest;
extern	int 	vcNestProc;
extern	int 	vcStepPlayback;
extern  ADRT    vdp;
extern 	int    *venv;
extern 	int    *venv1;
extern 	char  **venvpParent;
extern  char   *xdbid;
extern 	FLAGT 	vfassembly;
extern 	FLAGT 	vfBadAccess;
extern	FLAGT 	vfcaseMod;
extern  FLAGT   vfChildTty;
#ifdef S200
extern  FLAGT   vffloat_soft;
extern  FLAGT   vffpa;
#endif
extern	FLAGT	vfFPECaught;
extern	FLAGT	vfinkernel;
extern	FLAGT	vfin_screen_update;
extern	FLAGT	vfJustChecking;
extern	FLAGT	vfLineMode;
#ifdef S200
extern	FLAGT	vfmc68881;
#endif
extern	FLAGT	vfmore;
extern	int 	vfnCore;
extern	int 	vfnDecl;
extern  FLAGT   vfNoDebugInfo;
extern  int	vfnstdin;
extern	long   *vfnStats;
extern	int 	vfnSym;
extern 	ADRT 	vfp;
extern  FLAGT   vfprntipdifd;
extern	FLAGT 	vfPascal;
extern 	FLAGT 	vfProcBp;
extern	FILE	*vfpRecord[];
extern	FILE	*vfpPlayback;
extern 	FILE   *vfpSrc;
extern	FLAGT 	vfRecord[];
extern 	FLAGT 	vfredraw;
extern 	FLAGT 	vfRegValid;
extern	FLAGT 	vfRunAssert;
extern 	FLAGT 	vfSegValid;
#ifdef INSTR
extern  ulong   vfStatProc[];
#endif
extern 	FLAGT 	vfsplitscrn;
extern  FLAGT   vfTty;
extern 	FLAGT 	vftmpfdchange;
#ifdef S200
extern 	FLAGT 	vfUMM;
#endif
extern 	FLAGT 	vfUniqueProblem;
extern  FLAGT   vfUnwind;
#ifdef S200
extern 	FLAGT 	vfW310;
#endif
extern  int     viadoption;
extern	int 	viadMac;
extern	int 	viadMax;
extern	int 	vibp;
extern 	int 	vibpMac;
extern 	int 	vibpMax;
extern 	int 	vicolumns;
#ifdef INSTR
extern	ulong	viCmdCount;
#endif
extern 	int 	videbugmode;
extern	int 	vifd;
extern	int 	vifdMac;
extern	int 	vifdNil;
extern	int 	vifdTemp;
#ifdef S200
extern 	int   	viFpa_flag;
extern 	int   	viFpa_reg;
#endif
extern 	int 	vilines;
extern 	int 	viln;
extern 	int 	vilnNext;
extern 	int 	vilnPrev;
extern	long 	vilstMax;
extern	long 	viLSTPdMax;
extern	int 	vilvMax;
extern 	int 	vimacMac;
extern 	int 	vimap;
extern 	int 	vimdMac;
extern	long 	vinpMax;
extern	int 	viopMac;
extern 	int 	vioverlap;
extern	int 	vipd;
extern	long 	vipdfdmd;
extern	int 	vipdMac;
extern	int 	vipd_END_;
extern	int 	vipd_MAIN_;
extern  ADRT    vipreDp;
extern	long 	vipxdbheader;
extern 	int 	viscreenfd;
#ifdef HPSYMTAB
extern	long	visdMac;
#endif
extern	long 	vislt;
extern	long 	visltLim;
extern	long 	visltLo;
extern	long 	visltMax;
extern  long    visomloc;
extern	int 	vissLim;
extern	int 	vissLo;
extern	int 	vissMax;
extern	long 	visym;
extern	long 	visymGlobal;
extern	long 	visymLim;
extern	long 	visymLo;
extern	long 	visymMax;
extern	int 	vityMac;
extern  uint    viUnwindMax;
extern  uint    viStubUnwindMax;
extern	int 	vivarMac;
extern 	int 	viwindow;
extern 	int 	viwindowmode;
#define JIMTK 1
#ifdef JIMTK 
extern	LCE 	vlc;
extern	LCE 	vlcSet;
#endif
extern 	int 	vmacs;
extern	int 	vmagicSym;
extern	MAPR 	vmapCore;
extern	MAPR 	vmapText;
extern	pMODER 	vmode;
extern	long 	vmtimeSym;
extern 	ADRT 	vpc;
extern 	int 	vpid;
extern 	int 	vpidMyself;
#ifdef HPE
extern  PLABEL 	vplFixer;
#endif
extern  ADRT	vpriv;
extern  ADRT    vpsw;
extern 	long 	vPtEscape;
extern 	long 	vPtStat;
extern	long 	vResult;
extern	pADR 	vrgAd;
extern 	pBPR 	vrgBp;
extern	pFDR 	vrgFd;
#ifdef HPSYMTAB
extern  pLSTPDR vrgLSTPd;
#endif
#ifdef SPECTRUM
extern  LSTR  *vrglst;
extern  char  *vrgnp;
extern	pMDR 	vrgMd;
#endif
extern	int	vrgOffset[];
extern	pPDR 	vrgPd;
#ifdef HPSYMTAB
extern  pSDR    vrgSd;
#endif
#ifdef SPECTRUM
extern  pSUR     vrgUnwind;
extern  pSTUBSUR vrgStubUnwind;
#endif
extern	char   *vsbACTIVE;
extern	char   *vsbActive;
extern	char 	vsbArgsChild [];
extern	char   *vsbAssState;
extern	char   *vsbBpState;
extern 	char   *vsbCmd;
extern 	char 	vsbCmdBuf[];
extern 	char   *vsbCmdsIgnored;
extern 	char   *vsbclear_line;
extern 	char   *vsbcoredumped;
extern	char   *vsbCorefile;
extern 	char   *vsbdelete_line;
extern 	char   *vsbdelete_char;
extern 	char   *vsbErrAccess;
extern 	char   *vsbErrAdr;
extern 	char   *vsbErrNoChild;
extern 	char   *vsbErrSfail;
extern 	char   *vsbErrSfailSD;
extern 	char   *vsbErrSoutSD;
extern 	char 	vsbFileTemp[];
extern	char 	vsbFmt[];
extern  char   *vsbfmt1;
extern  char   *vsbfmt2;
extern  char   *vsbfmt3;
extern  char   *vsbfmt4;
extern  char   *vsbfmt5;
extern  char   *vsbHitRETURN;
extern  char   *vsbhome_down;
extern 	char   *vsbignore;
extern	char   *vsbLeHdr;
extern	char   *vsbLHdr;
extern	char   *vsbLnHdr;
extern 	char   *vsbMore;
extern	char 	vsbMsgBuf[];
extern 	char   *vsbnocore;
extern 	char   *vsbnoignore;
extern 	char   *vsbnosourcefile;
extern 	char   *vsbnostop;
extern 	char   *vsbPanic;
extern	char 	vsbPeek [];
extern 	char   *vsbProcBpCmd;
extern	char   *vsbProcHdr;
extern  char   *vsbPrompt;
#ifdef CDB
extern  char   vsbPushBack[];
#endif
extern	char   *vsbSbrkFirst;
extern	char   *vsbStatsFile;
extern	char   *vsbSUSPENDED;
extern	char   *vsbSuspended;
extern	char   *vsbSymfile;
extern	char   *vsbstandout_off;
extern	char   *vsbstandout_on;
extern	char 	vsbTok  [];
extern 	char   *vsbunknown;
extern 	char   *vsbunnamed;
extern 	char   *vsbup_line;
extern 	char   *vsbYesNo;
extern	uint 	vsig;
extern	pSLTR 	vsltCur;
extern 	ADRT 	vsp;
extern 	int 	vspcCur;
extern	pSYMR 	vsymCur;
extern	ADRT 	vTextAdrMax;
extern	ADRT 	vTextAdrMin;
extern  long	vTextFileStart;
extern	long 	vtimeStart;
#ifdef JIMTK 
extern	TKE 	vtk;
extern	TKE 	vtkPeek;
#endif
extern	pTYR 	vtyChar;
#ifdef XDB
extern  pTYR    vtyCnCobol;
#endif
extern  pTYR    vtyCnBool;
extern	pTYR 	vtyCnChar;
extern	pTYR 	vtyCnDouble;
extern	pTYR 	vtyCnFloat;
extern	pTYR 	vtyCnInt;
extern	pTYR 	vtyCnLong;
extern	pTYR 	vtyCnShort;
extern	pTYR 	vtyDouble;
extern	pTYR 	vtyFloat;
extern	pTYR 	vtyInt;
extern	pTYR 	vtyLong;
extern	pTYR 	vtyShort;
extern	pTYR 	vtyUChar;
extern	pTYR 	vtyULong;
extern	pTYR 	vtyUShort;
extern	pTYR 	vtyZeros;
extern	long 	vUarea;


/*
 * EXTERNAL FUNCTIONS
 * ------------------
 */


extern 	void 	AddMacro ();
extern 	ADRT 	AdrFAdr ();
extern 	ADRT 	AdrFAdrTy ();
extern	ADRT 	AdrFEndOfProc ();
extern	ADRT 	AdrFField ();
extern	ADRT 	AdrFGlobal ();
extern	ADRT 	AdrFIfdLn ();
extern	ADRT 	AdrFIslt ();
extern	ADRT 	AdrFIsym ();
extern	ADRT 	AdrFLabel ();
extern	ADRT 	AdrFLocal ();
extern	ADRT 	AdrFReg ();
extern	ADRT 	AdrFStackFix ();
extern	ADRT 	AdrFVar ();
extern	long 	AgeFFn ();
extern	long 	CbFDntt ();
extern	int 	CbFLocal ();
extern	int 	CbFTy ();
extern	long 	CblFFn ();
extern	char 	ChFEscape ();
extern 	FLAGT 	DelMacro ();
extern 	FLAGT 	Do_LongName ();
extern	long 	DoProc ();
#ifdef S200
extern  double  DoubleFExt ();
#endif
extern	char   *EatCmdList();
extern 	int 	FAdrGood ();
extern 	FLAGT 	FClearBp ();
extern	FLAGT 	FDoCommand ();
extern	void	Fixer();	
extern	FLAGT 	FHdrCmp ();
extern	FLAGT 	FNameCmp ();
extern	FLAGT 	FNextLocal ();
extern	FLAGT 	FNextSlt ();
extern	FLAGT 	FNextSym ();
extern	FLAGT 	FNextVar ();
extern	FLAGT 	FPrevSym ();
extern	FLAGT 	FProcCmp ();
extern	FLAGT 	FSbCmp ();
extern	FLAGT 	FValFTok ();
extern	FLAGT 	FYesNo ();
extern 	int 	GetByte ();
extern  char   *GetCmd();
extern	long 	GetExpr ();
extern 	REGT 	GetReg ();
#ifdef S200
extern 	FLAGT 	GetRegs68881 ();
extern 	FLAGT 	GetRegs98248A ();
extern 	FLAGT 	GetRegs98635 ();
#endif
extern 	REGT 	GetUser ();
extern 	int 	GetWord ();
extern 	int 	Hit_Return ();
extern 	int 	IbpFAdr ();
extern 	long 	IbpFBrkHit ();
extern 	int 	IbpFBrkIn ();
extern 	int 	IbpFBrkOut ();
extern 	long 	IbpFIbpDup ();
extern	int 	IbpFNewChild ();
extern 	int 	IbpFPc ();
extern	int 	IbpFRun ();
extern 	int 	IbpFSet ();
extern	int 	IbpFSingle ();
extern	int 	IfdFAdr ();
extern	int 	IfdFName ();
extern	int 	IFSbSb ();
extern	int 	IlnFIsym ();
extern	int 	IncFTyMode ();
#ifdef S200
extern	ulong 	InstSize ();
#endif
extern	int 	IpdFAdr ();
extern	int 	IpdFEntryIpd ();
extern	int 	IpdFName ();
extern	int 	IpdFNameStr ();
extern	long 	IsltFIsym ();
extern	void 	IssFPxStruct ();
extern	long 	IssFStart ();
extern	long 	IsymFNextTqSym ();
extern 	void 	ListMacros ();
extern 	long 	LongRound ();
extern	void	Oops();
extern 	FLAGT 	MacLookup ();
extern 	char   *Malloc ();
extern 	void    More_Complete ();
extern 	void    More_Message ();
extern	char   *NameFCurSym ();
extern	void 	NextFrame ();
extern 	char 	outchar();
extern 	void 	PushBack ();
extern 	ADRT 	PushWord ();
extern 	char   *Realloc ();
extern 	void    RetToXdb ();
extern	char   *SbFAdr ();
extern	char   *SbFEob ();
extern	char   *SbFInp ();
extern	SBT 	SbSafe ();
extern 	char   *strchr ();
extern 	int 	strcspn ();
extern 	char   *strrchr ();
#ifdef JIMTK 
extern	TKE 	TkFExpr ();
extern	TKE 	TkFStr ();
extern	TKE 	TkNext();
extern	TKE 	TkPeek();
#endif
extern	int 	TqFTy ();
extern	void 	TyFCurSym();
extern	long 	UareaFLst ();
extern	char   *VTInCore ();


/* 
 * SYSTEM LIBRARY EXTERNALS
 * ------------------------
 */

extern	long	atol ();
extern	double	atof ();
extern  void    free ();
extern  char   *gets ();
extern	char   *malloc ();
extern  char   *realloc ();
extern	char   *sbrk ();
