/* @(#) $Revision: 27.2 $ */     
/*

 *      Copyright (c) 1984, 1985 AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */
/* capnames.c: Made automatically from caps and maketerm.ex - don't edit me! */
char *boolnames[] = {
"bw", "am", "xsb", "xhp", "xenl", "eo", "gn", "hc", "km", "hs", 
"in", "da", "db", "mir", "msgr", "os", "eslok", "xt", "hz", "ul", 
"xon", "nxon", "mc5i", "chts", "nrrmc", 
0
};

char *boolcodes[] = {
"bw", "am", "xb", "xs", "xn", "eo", "gn", "hc", "km", "hs",
"in", "da", "db", "mi", "ms", "os", "es", "xt", "hz", "ul",
"xo", "nx", "5i", "HC", "NR",
0
};

char *boolfnames[] = {
"auto_left_margin", "auto_right_margin", "beehive_glitch", "ceol_standout_glitch",
"eat_newline_glitch", "erase_overstrike", "generic_type", "hard_copy",
"has_meta_key", "has_status_line", "insert_null_glitch", "memory_above",
"memory_below", "move_insert_mode", "move_standout_mode", "over_strike",
"status_line_esc_ok", "teleray_glitch", "tilde_glitch", "transparent_underline",
"xon_xoff", "needs_xon_xoff", "prtr_silent", "hard_cursor",
"non_rev_rmcup",
0
};

char *numnames[] = {
"cols", "it", "lines", "lm", "xmc", "pb", "vt", "wsl", "nlab", "lh", 
"lw", 
0
};

char *numcodes[] = {
"co", "it", "li", "lm", "sg", "pb", "vt", "ws", "Nl", "lh",
"lw",
0
};

char *numfnames[] = {
"columns", "init_tabs", "lines", "lines_of_memory",
"magic_cookie_glitch", "padding_baud_rate", "virtual_terminal", "width_status_line",
"num_labels", "label_height", "label_width",
0
};

char *strnames[] = {
"cbt", "bel", "cr", "csr", "tbc", "clear", "el", "ed", "hpa", "cmdch", 
"cup", "cud1", "home", "civis", "cub1", "mrcup", "cnorm", "cuf1", "ll", "cuu1", 
"cvvis", "dch1", "dl1", "dsl", "hd", "smacs", "blink", "bold", "smcup", "smdc", 
"dim", "smir", "invis", "prot", "rev", "smso", "smul", "ech", "rmacs", "sgr0", 
"rmcup", "rmdc", "rmir", "rmso", "rmul", "flash", "ff", "fsl", "is1", "is2", 
"is3", "if", "ich1", "il1", "ip", "kbs", "ktbc", "kclr", "kctab", "kdch1", 
"kdl1", "kcud1", "krmir", "kel", "ked", "kf0", "kf1", "kf10", "kf2", "kf3", 
"kf4", "kf5", "kf6", "kf7", "kf8", "kf9", "khome", "kich1", "kil1", "kcub1", 
"kll", "knp", "kpp", "kcuf1", "kind", "kri", "khts", "kcuu1", "rmkx", "smkx", 
"lf0", "lf1", "lf10", "lf2", "lf3", "lf4", "lf5", "lf6", "lf7", "lf8", 
"lf9", "rmm", "smm", "nel", "pad", "dch", "dl", "cud", "ich", "indn", 
"il", "cub", "cuf", "rin", "cuu", "pfkey", "pfloc", "pfx", "mc0", "mc4", 
"mc5", "rep", "rs1", "rs2", "rs3", "rf", "rc", "vpa", "sc", "ind", 
"ri", "sgr", "hts", "wind", "ht", "tsl", "uc", "hu", "iprog", "ka1", 
"ka3", "kb2", "kc1", "kc3", "mc5p", "meml", "memu", "rmp", "acsc", "pln", "kcbt", "smxon",
"rmxon", "smam", "rmam", "xonc", "xoffc", "enacs", "smln", "rmln", "kbeg", "kcan", 
"kclo", "kcmd", "kcpy", "kcrt", "kend", "kent", "kext", "kfnd", "khlp", "kmrk", 
"kmsg", "kmov", "knxt", "kopn", "kopt", "kprv", "kprt", "krdo", "kref", "krfr", 
"krpl", "krst", "kres", "ksav", "kspd", "kund", "kBEG", "kCAN", "kCMD", "kCPY", 
"kCRT", "kDC", "kDL", "kslt", "kEND", "kEOL", "kEXT", "kFND", "kHLP", "kHOM", 
"kIC", "kLFT", "kMSG", "kMOV", "kNXT", "kOPT", "kPRV", "kPRT", "kRDO", "kRPL", 
"kRIT", "kRES", "kSAV", "kSPD", "kUND", "rfi", "kf11", "kf12", "kf13", "kf14", 
"kf15", "kf16", "kf17", "kf18", "kf19", "kf20", "kf21", "kf22", "kf23", "kf24", 
"kf25", "kf26", "kf27", "kf28", "kf29", "kf30", "kf31", "kf32", "kf33", "kf34", 
"kf35", "kf36", "kf37", "kf38", "kf39", "kf40", "kf41", "kf42", "kf43", "kf44", 
"kf45", "kf46", "kf47", "kf48", "kf49", "kf50", "kf51", "kf52", "kf53", "kf54", 
"kf55", "kf56", "kf57", "kf58", "kf59", "kf60", "kf61", "kf62", "kf63", "el1", 
"mgc", "smgl", "smgr", 
0
};

char *strcodes[] = {
"bt", "bl", "cr", "cs", "ct", "cl", "ce", "cd", "ch", "CC",
"cm", "do", "ho", "vi", "le", "CM", "ve", "nd", "ll", "up",
"vs", "dc", "dl", "ds", "hd", "as", "mb", "md", "ti", "dm",
"mh", "im", "mk", "mp", "mr", "so", "us", "ec", "ae", "me",
"te", "ed", "ei", "se", "ue", "vb", "ff", "fs", "i1", "is",
"i3", "if", "ic", "al", "ip", "kb", "ka", "kC", "kt", "kD",
"kL", "kd", "kM", "kE", "kS", "k0", "k1", "k;", "k2", "k3",
"k4", "k5", "k6", "k7", "k8", "k9", "kh", "kI", "kA", "kl",
"kH", "kN", "kP", "kr", "kF", "kR", "kT", "ku", "ke", "ks",
"l0", "l1", "la", "l2", "l3", "l4", "l5", "l6", "l7", "l8",
"l9", "mo", "mm", "nw", "pc", "DC", "DL", "DO", "IC", "SF",
"AL", "LE", "RI", "SR", "UP", "pk", "pl", "px", "ps", "pf",
"po", "rp", "r1", "r2", "r3", "rf", "rc", "cv", "sc", "sf",
"sr", "sa", "st", "wi", "ta", "ts", "uc", "hu", "iP", "K1",
"K3", "K2", "K4", "K5", "pO", "ml", "mu", "rP", "ac", "pn",
"kB", "SX", "RX", "SA", "RA", "XN", "XF", "eA", "LO", "LF",
"@1", "@2", "@3", "@4", "@5", "@6", "@7", "@8", "@9", "@0",
"%1", "%2", "%3", "%4", "%5", "%6", "%7", "%8", "%9", "%0",
"&1", "&2", "&3", "&4", "&5", "&6", "&7", "&8", "&9", "&0",
"*1", "*2", "*3", "*4", "*5", "*6", "*7", "*8", "*9", "*0",
"#1", "#2", "#3", "#4", "%a", "%b", "%c", "%d", "%e", "%f",
"%g", "%h", "%i", "%j", "!1", "!2", "!3", "RF", "F1", "F2",
"F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC",
"FD", "FE", "FF", "FG", "FH", "FI", "FJ", "FK", "FL", "FM",
"FN", "FO", "FP", "FQ", "FR", "FS", "FT", "FU", "FV", "FW",
"FX", "FY", "FZ", "Fa", "Fb", "Fc", "Fd", "Fe", "Ff", "Fg",
"Fh", "Fi", "Fj", "Fk", "Fl", "Fm", "Fn", "Fo", "Fp", "Fq",
"Fr", "cb", "MC", "ML", "MR",
0
};

char *strfnames[] = {
"back_tab", "bell", "carriage_return", "change_scroll_region",
"clear_all_tabs", "clear_screen", "clr_eol", "clr_eos",
"column_address", "command_character", "cursor_address", "cursor_down",
"cursor_home", "cursor_invisible", "cursor_left", "cursor_mem_address",
"cursor_normal", "cursor_right", "cursor_to_ll", "cursor_up",
"cursor_visible", "delete_character", "delete_line", "dis_status_line",
"down_half_line", "enter_alt_charset_mode", "enter_blink_mode", "enter_bold_mode",
"enter_ca_mode", "enter_delete_mode", "enter_dim_mode", "enter_insert_mode",
"enter_secure_mode", "enter_protected_mode", "enter_reverse_mode", "enter_standout_mode",
"enter_underline_mode", "erase_chars", "exit_alt_charset_mode", "exit_attribute_mode",
"exit_ca_mode", "exit_delete_mode", "exit_insert_mode", "exit_standout_mode",
"exit_underline_mode", "flash_screen", "form_feed", "from_status_line",
"init_1string", "init_2string", "init_3string", "init_file",
"insert_character", "insert_line", "insert_padding", "key_backspace",
"key_catab", "key_clear", "key_ctab", "key_dc",
"key_dl", "key_down", "key_eic", "key_eol",
"key_eos", "key_f0", "key_f1", "key_f10",
"key_f2", "key_f3", "key_f4", "key_f5",
"key_f6", "key_f7", "key_f8", "key_f9",
"key_home", "key_ic", "key_il", "key_left",
"key_ll", "key_npage", "key_ppage", "key_right",
"key_sf", "key_sr", "key_stab", "key_up",
"keypad_local", "keypad_xmit", "lab_f0", "lab_f1",
"lab_f10", "lab_f2", "lab_f3", "lab_f4",
"lab_f5", "lab_f6", "lab_f7", "lab_f8",
"lab_f9", "meta_off", "meta_on", "newline",
"pad_char", "parm_dch", "parm_delete_line", "parm_down_cursor",
"parm_ich", "parm_index", "parm_insert_line", "parm_left_cursor",
"parm_right_cursor", "parm_rindex", "parm_up_cursor", "pkey_key",
"pkey_local", "pkey_xmit", "print_screen", "prtr_off",
"prtr_on", "repeat_char", "reset_1string", "reset_2string",
"reset_3string", "reset_file", "restore_cursor", "row_address",
"save_cursor", "scroll_forward", "scroll_reverse", "set_attributes",
"set_tab", "set_window", "tab", "to_status_line",
"underline_char", "up_half_line", "init_prog", "key_a1",
"key_a3", "key_b2", "key_c1", "key_c3",
"prtr_non", "memory_lock", "memory_unlock", "char_padding", "acs_chars", "plab_norm",
"key_btab", "enter_xon_mode", "exit_xon_mode", "enter_am_mode",
"exit_am_mode", "xon_character", "xoff_character", "ena_acs",
"label_on", "label_off", "key_beg", "key_cancel",
"key_close", "key_command", "key_copy", "key_create",
"key_end", "key_enter", "key_exit", "key_find",
"key_help", "key_mark", "key_message", "key_move",
"key_next", "key_open", "key_options", "key_previous",
"key_print", "key_redo", "key_reference", "key_refresh",
"key_replace", "key_restart", "key_resume", "key_save",
"key_suspend", "key_undo", "key_sbeg", "key_scancel",
"key_scommand", "key_scopy", "key_screate", "key_sdc",
"key_sdl", "key_select", "key_send", "key_seol",
"key_sexit", "key_sfind", "key_shelp", "key_shome",
"key_sic", "key_sleft", "key_smessage", "key_smove",
"key_snext", "key_soptions", "key_sprevious", "key_sprint",
"key_sredo", "key_sreplace", "key_sright", "key_srsume",
"key_ssave", "key_ssuspend", "key_sundo", "req_for_input",
"key_f11", "key_f12", "key_f13", "key_f14",
"key_f15", "key_f16", "key_f17", "key_f18",
"key_f19", "key_f20", "key_f21", "key_f22",
"key_f23", "key_f24", "key_f25", "key_f26",
"key_f27", "key_f28", "key_f29", "key_f30",
"key_f31", "key_f32", "key_f33", "key_f34",
"key_f35", "key_f36", "key_f37", "key_f38",
"key_f39", "key_f40", "key_f41", "key_f42",
"key_f43", "key_f44", "key_f45", "key_f46",
"key_f47", "key_f48", "key_f49", "key_f50",
"key_f51", "key_f52", "key_f53", "key_f54",
"key_f55", "key_f56", "key_f57", "key_f58",
"key_f59", "key_f60", "key_f61", "key_f62",
"key_f63", "clr_bol", "clear_margins", "set_left_margin",
"set_right_margin",
0
};

