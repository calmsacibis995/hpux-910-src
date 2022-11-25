# FacetTerm  pt250  description file 06/21/88
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: pt250.FS,v 70.1 92/03/09 15:50:31 ssa Exp $
lines=24
enter_ca_mode=\E[>25l
enter_ca_mode_2=\E[1Q\E[1;24y\E[12;>7;>10;>13h\E[4;>1;>3;>8;>9;>12;>14;>15;>16;>18;>19l\E[1N$<10>
### NOT_IN_COMMON ### num_lock  func=\E[>10h  num_lock=\E[>10l  default is func
### NOT_IN_COMMON parm_mode_val-00000000-APPKEYON=>10
parm_mode_val-PASS....-NOTIMP..=>10
use=ptclearcom.fc
use=ptcommon.fc
## >21 E2 mode  set=\E[>21h  reset=\E[>21l (default)
mode-00000000-00000100=\E[>21l
mode-00000100-00000000=\E[>21h
parm_mode_val-00000100-00000000=>21
use=pt2.fm
