s/[	][	]*/ /g
s@ */\*.*@@
s/ *=.*/;/
s/.*://
s/export/extern/

t clear1
: clear1
s/^.*externdefine/#define/
t

s/(/(/
t proc

s/\[[^[z]*\]/[]/g
b

: proc
/void/d
s/(.*)/();/
