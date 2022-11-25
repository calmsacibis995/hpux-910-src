# FacetTerm  mcg3320   (80 column only)  description file  05/13/91
## For the micro-colour graphics 3320 from micro vitec
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: m3320w.FS,v 70.1 92/03/09 16:11:52 ssa Exp $
##--- MULTIPAGE ---##
pages=10
# parm_display_page_next=\E[%p1%dU
# parm_display_page_prev=\E[%p1%dV
## 
use=vt220mode7.fc
use=vt220narr.fc
use=vt220mode.fc
use=vt220com.fc
use=m3320attr.fc
use=vt220func.fc
use=vt220keys.fc
use=facetcolor.fm
pass-ansi_mode=\E%%!1
# global color
perwindow-5-ORWLSQ=\E[50m
perwindow_also-.=\E[51m
perwindow_also-.=\E[52m
perwindow_also-.=\E[53m
perwindow_also-.=\E[54m
perwindow_also-.=\E[55m
perwindow_also-.=\E[56m
perwindow_also-.=\E[57m
perwindow_default-global_black=\E[50m
##				auto_color  on=\E[=3h  off=\E[=3l
perwindow-5-ORWLSQ=\E[=3l
perwindow_also-.=\E[=3h
perwindow_default-auto_color_off=\E[=3l
display_page_number=\E[=%p1%{1}%+%ds
enter_ca_mode=\EP10;10;6;25;26;82;1;1;1;2;!W57696e646f77203130\E\\
enter_ca_mode=\EP2;2;1;1;26;82;1;1;2;2;!W57696e646f772032\E\\
enter_ca_mode=\EP3;3;2;2;26;82;1;1;2;2;!W57696e646f772033\E\\
enter_ca_mode=\EP4;4;3;3;26;82;1;1;2;2;!W57696e646f772034\E\\
enter_ca_mode=\EP5;5;4;4;26;82;1;1;2;2;!W57696e646f772035\E\\
enter_ca_mode=\EP6;6;1;51;26;82;1;1;2;2;!W57696e646f772036\E\\
enter_ca_mode=\EP7;7;2;50;26;82;1;1;2;2;!W57696e646f772037\E\\
enter_ca_mode=\EP8;8;3;49;26;82;1;1;2;2;!W57696e646f772038\E\\
enter_ca_mode=\EP9;9;4;48;26;82;1;1;2;2;!W57696e646f772039\E\\
enter_ca_mode=\EP1;1;5;25;26;82;1;1;2;2;!W57696e646f772031\E\\
enter_ca_mode=\E[=1s
exit_ca_mode=\E[=2y\E[=3y\E[=4y
exit_ca_mode=\E[=5y\E[=6y\E[=7y\E[=8y\E[=9y\E[=10y
