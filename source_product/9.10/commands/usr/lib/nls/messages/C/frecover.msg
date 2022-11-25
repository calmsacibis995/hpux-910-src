$set 1
1 (1001): file trailer does not match file header for
2 (1002): file %s
3 (1003): backup id does not match expected value
4 (1004): backup time does not match expected value
6 (1006): illegal value for remote; bit field must be 
7 (1007): zero or one
8 (1008): unknown type in header or trailer
9 (1009): attempting to continue
10 (1010): unknown id in header or trailer
11 (1011): attempting to continue
12 (1012): unauthorized access to file %s
13 (1013): cannot chdir to directory %s
14 (1014): illegal value for remote; bit field must be 
15 (1015): cannot chdir to directory %s
16 (1016): zero or one
17 (1017): cannot chdir to directory %s
18 (1018): unable to stat file %s
20 (1020): block checksum mismatch
21 (1021): block checksum mismatch
22 (1022): unable to stat file %s
23 (1023): file %s exists but is not a directory
24 (1024): can't create directory %s
25 (1025): can't unlink file %s
26 (1026): can't mknod file %s
27 (1027): can't mknod file %s
28 (1028): Skipping obsolete network special file
29 (1029): cannot change existing file %s to a CDF\n
30 (1030): error (%s) renaming directory to %s\n
32 (1032): %s - can't open
33 (1033): can't create file %s
35 (1035): cannot creat temporary file name,
36 (1036):  skipping file 
37 (1037): cannot create link between %s and %s: %s
38 (1038): system does not support symbolic links
39 (1039): backed up version of %s is newer
40 (1040): recover the link %s first
41 (1041): cannot create link between %s and %s
42 (1042): cannot stat link file %s
43 (1043): error in header on recover
44 (1044): error in file recovery
45 (1045): error in trailer recovery
46 (1046): write error to residual file
47 (1047): I/O error in write of file %s
48 (1048): I/O error in write of file %s
50 (1050): can't position to restart file
51 (1051): incorrect volume mounted
52 (1052): cannot unlink %s
53 (1053): unable to chmod file %s
54 (1054): too many components in file name %s
56 (1056): invalid entries for file %s; ACLs not recovered.
57 (1057): tape drive error during fastsearch mark positioning
58 (1058): unable to write to %s
59 (1059): File %s found, but active/unrecoverable during this retry
60 (1060): %s itself not on media;\n\tcontinuing with %s and following files
61 (1061): attempt to continue recovery with new backup id
62 (1062): attempt to continue recovery with new backup time
63 (1063): Unable to create ACL: unknown user id for user %s
64 (1064): Unable to create ACL: unknown group id for group %s
65 (1065): read error on file header
$set 2
102 (2102): maximum number (%d) resynchronizations exceeded
103 (2103): IOERROR on a disk file. Frecover exiting
104 (2104): zero length read from disk file. Frecover exiting
105 (2105): did not find expected file marker
107 (2107): IOERROR on a disk device. Frecover exiting
108 (2108): zero length read from disk device. Frecover exiting
109 (2109): file marker read
110 (2110): checkpoint record read
111 (2111): DDS setmark read
112 (2112): unexpected DDS marker read.  Status:
113 (2113): unable to resync backup media
114 (2114): read error from input device (%s)
$set 3
203 (3203): unable to open residual file %s
204 (3204): proceeding without residual file
205 (3205): specified options are mutually exclusive
206 (3206): option '%c' expects an argument
207 (3207): additional argument: %s specified
208 (3208): must specify at least one of V, I, x, r, or R
209 (3209): specified both X and F keys; F assumed
210 (3210): illegal option (-i/-e) specified with r option
211 (3211): N option specified.  Ignoring %s
237 (3238): specified options are mutually exclusive
241 (3241): unable to open configuration file %s
242 (3242): illegal configuration file label (%s) ignored
243 (3243): configuration file error at line %d
244 (3244): bad line:\n%s
245 (3245): option '%c' is unrecognized
246 (3246): additional inapplicable options specified on index extraction
247 (3247): additional inapplicable options specified on volume header extraction
248 (3248): missing i or e in graph file %s
249 (3249): unable to open graph file %s
250 (3250): unable to open volume header file %s
251 (3251): number of output files cannot be more than MAXOUTFILES \n
252 (3252): error in expanding output file pattern\n
253 frecover -r [-hosvymFNOX] [-c config] [-f device] [-S skip]\n
254 frecover -R path [-f device]\n
255 frecover -x [-hosvymFNOX] [-c config] [-e path] [-f device]\n\
            [-g graph] [-i path] [-S skip]\n
256 frecover [-i | -V] path [-vy] [-f device] [-c config]\n
257  -{r,x} - full/partial recovery\n
258  -{I,V} - write index/volume-header to path\n
259  -{F,X} - flat/relative recovery\n
260  -{i,e} - specify a path to include/exclude\n
261  -{g,c} - specify a graph/configuration file\n
262  -{N,h} - No files recovered-Only verify/graph information\n
263  -R     - continue an interrupted session\n
264  -f     - specify an input device\n
265  -o     - overwrite existing files\n
266  -v     - verbose mode\n
267  -y     - answer yes to all questions\n
268  -S     - specify a file for skipped blocks\n
269  -s     - sparse file recovery\n
270  -O     - set ownership of files to the user running the recovery\n
271  -m     - Print marker information (checksums, filemarks, setmarks)\n
$set 4
301 (4301): %s not recovered from backup media
302 (4302): frecover interrupted, continue
303 (4303): do you wish to save status for restart
304 (4304): automatic '%s'\n
305 (4305): not enough memory for allocate
306 (4306): can't create restart file, try again
307 (4307): can't write restart file
308 (4308): can't open restart file
309 (4309): can't read restart file
310 frecover(4310): Enter the absolute path name of a file for saving status: 
311 (4311): can't read OBSCURE node
312 (4312): can't save obscure node
$set 5
401 (5401): Volume %d completed.
402 (5402): failed close on device %s
403 (5403): Unable to open or identify device at %s\n
404 frecover(5404): Press return when the next volume is ready on %s:\n
405 (5405): unable to open %s
406 (5406): Do you wish to continue
407 (5407): abort selected
408 (5408): not an fbackup volume; unable to read volume label
409 (5409): unable to read volume header
410 frecover(5410): Press return when the previous volume is ready on %s:\n
411 (5411): Do you wish to try a different volume
412 (5412): Do you wish to try to salvage this volume
413 (5413): attempting to read backup following bad volume header
414 (5414): abort selected
418 (5418): not an fbackup volume; unable to read volume header
419 (5419): checksum on volume header is incorrect.
420 (5420): not an fbackup volume; magic value is incorrect
421 (5421): volume identification does not match the current session
422 (5422): incorrect backup selected on restart
423 (5423): incorrect volume mounted;
424 (5424): expected volume %d, and got %d
425 (5425): unable to open index file
426 (5426): read error while attempting to read index.  Skipping.
427 (5427): unable to open controlling terminal
429 (5429): backup volume was created with LC_COLLATE or LANG environment\nvariable set as %s, and must be recovered with same: Resetting to %s.
430 (5430): unable to open /tmp/sampipe
431 (5431): backup volume was created with LANG and LC_COLLATE\nvariables unset.  Unsetting LANG and LC_COLLATE and continuing.
432 (5432): Media rewind failed
433 (5433): Do you wish to continue using this volume
434 (5434): attempting to resynchronize after volume header
435 (5435): malloc error while attempting to read index
436 (5436): malloc error: not enough virtual memory to use fastsearch.\nContinuing without fastsearch capability.
437 (5437): Unable to find the device file %s\n
