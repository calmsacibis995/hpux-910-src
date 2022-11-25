

extern	STACK_ELEMENT	*stack;
extern	int		tos;
extern	int		stack_elements;

extern	char	*stream_string;
extern	int	stream_pos;
extern	int	stream_len;

extern	char	*else_string;
extern	int	else_pos;
extern	int	else_len;

extern	int	quest_count;
extern	int	curl_count;
extern	int	semi_cr;

extern	int	line_number;
extern	int	no_update;
extern	int	specifier;

extern	char  *input_files[];
extern	char  cur_output[];
extern	char  dbase_file[];
extern	int   cur_input;
extern	int   num_input;

extern	char  cur_func[];


extern	char  *main_ids[];
extern	int   main_cnt;


extern	int    h_flag;
extern	int    a_flag;
extern	int    q_flag;
extern	int    m_flag;
extern	int    i_flag;


extern	int    e_flag;

#if defined(FORK)
extern	int    f_flag;
#endif

#if defined(EXEC)
extern	int    x_flag;
#endif

#if defined(BFA_KERNEL)
extern	int    k_flag;
#endif

#if defined(TSR)
extern	int    r_flag;
extern	int    vector;
#endif

#if defined(BFAWIN)
extern	int	w_flag;
#endif

extern	char  c_args[];
extern	char  cpp[];
extern	char  cpp_end[];
extern	int    bfa_ret;


/* scanner.l  */
extern	void	InitScanner();
extern	void	BeginScannerState();
extern	void	ReturnToOldState();

extern	FILE	*yyin;
extern	FILE	*outfile;

/* cntlscan.c */
extern	int	input();
extern	void	unput();
extern	void	output();
extern	void	ConvertString();

/* generate.c */
extern	void	ProcessNewFunction();
extern	void	ProcessIf();
extern	void	ProcessLeftCurl();
extern	void	ProcessRightCurl();
extern	void	ProcessSemiColon();
extern	void	ProcessElseIf();
extern	void	ProcessElse();
extern	void	ProcessDo();
extern	void	ProcessFor();
extern	void	ProcessWhile();
extern	void	ProcessSwitch();
extern	void	ProcessCase();
extern	void	ProcessDefault();
extern	void	ProcessLabel();
extern	void	ProcessQuestion();
extern	void	ProcessColon();
extern	void	ProcessId();
extern	void	SetLineNumber();
extern	void	CheckFileAndSetLineNumber();
extern	void	CheckReturn();
