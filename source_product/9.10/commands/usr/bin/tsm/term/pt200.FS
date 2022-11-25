# FacetTerm  pt200  description file 06/21/88
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: pt200.FS,v 70.1 92/03/09 16:02:04 ssa Exp $
lines=24
enter_ca_mode_2=\E[1Q\E[1;24y\E[12;>7;>10;>13h\E[4;>1;>3;>8;>9;>12;>14;>15;>16;>18;>19l\E[1N$<10>
### NOT_IN_COMMON ### num_lock  func=\E[>10h  num_lock=\E[>10l  default is func
### NOT_IN_COMMON parm_mode_val-00000000-APPKEYON=>10
parm_mode_val-PASS....-NOTIMP..=>10
use=ptclearcom.fc
use=ptcommon.fc
use=pt2.fm
