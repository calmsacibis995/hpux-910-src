# FacetTerm  pt250-merge  description file 06/21/88
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: pt250-merge.FS,v 70.1 92/03/09 16:02:10 ssa Exp $
lines=25
graphics_8_bit
graphics_escape_control
enter_ca_mode=\E[1N$<10/>\E[1Q\E[1;25y\E[12;>7;>10;>13;>25h\E[4;>1;>3;>8;>9;>12;>14;>15;>16;>18;>19l\E[4N$<10>
### NOT_IN_COMMON ### num_lock  func=\E[>10h  num_lock=\E[>10l  default is func
### NOT_IN_COMMON parm_mode_val-40000000-NOTIMP..=>10
enter_appl_keypad_mode=\E[>10l
exit_appl_keypad_mode=\E[>10h
parm_mode_val-00000000-APPKEYON=>10
#
#     !!! IMPORTANT NOTE !!!
#     Set Terminal to: 8-bit-no-parity
#
use=ptclearcom.fc
use=ptcommon.fc
use=pt2.fm
