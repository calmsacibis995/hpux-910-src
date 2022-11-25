$set 1
1 Must be a simple file name, not a path name.
2 Must be a value between 0 and 65535.
3 Must be a value between 1 and 65535.
4 Must be a value between 1 and 65535.
5 Exactly one of them has to be specified.
6 At least one of them has to be specified.
7 Must be either 1 or 2.
8 Must be greater than 0.
9 Can be used only if "-l" has been specified.
10 Can't create a mirrored empty logical volume. Specify "-l".
11 Must be either 0 or 1.
12 "%s": can't read disk label; disk type must be specified:\n
13 "%s": can't figure out disk partition.\n
14 "%s": `%c' partition is unavailable.\n
15 "%s": not a character device.\n
16 "%s": not a logical volume.\n
17 writing LVM record
18 clearing the bad block directory
19 too many bad blocks.\n
20 writing the bad block directory
21 writing the DEFECT01 entries
22 must be supplied.
23 must be supplied.
24 must be a block special file.
25 "-p" and "-s" can be used only with "-a y".\n
26 Couldn't read the internal id of volume group "%s" from "%s".\n
27 "PhysicalExtentSize" must be a power of 2 between 1 and 256.
28 Must be a value between 1 and 255.
29 Volume group "%s" does not exist in the "%s" file\n but it exists in the file-system. Cannot proceed.\n
30 Volume group "%s" already exists in the "%s" file.\n
31 Volume group "%s" could not be created:\n
32 Error when writing "%s" and "%s" to "%s".\n
33 "%s" could not be read into memory.\n
34 Physical volume "%s" could not be deleted by the LVM device driver.\n There is now an inconsistency between the information in the file\n system and the "%s" file for the volume group "%s":\n
35 Couldn't query the state of the volume group "%s":\n
36 The volume group "%s" is not active. Only an active\n volume group can be extended.\n
37 Physical volume "%s" could not be removed since it is\n either missing or not attached.\n
38 Physical volume "%s" could not be removed since some of its\n physical extents are still in use.\n
39 Physical volume "%s" could not be deleted from the "%s" file.\n There is now an inconsistency between the LVM device driver\nand the "%s" file.\n
40 Creating "%s".\n
41 Removing "%s"
42 Restoring "%s"
43 Saving "%s" to "%s"
44 Must be a number.
45 Bad value supplied. Legal values are "%s".
46 Program error. Bad usage of options handling routines.
47 Option used more than once.
48 Illegal option.
49 Options with value must not be mixed with other options.
50 A value after option must be supplied.
51 More arguments required.
52 No character after '-'.
53 Physical volume "%s" could not be added to "%s" and\n therefore was not added to the volume group.\n
54 The physical volume "%s" is already recorded in the "%s" file.\n
55 Physical volume "%s" is not a block special file.\n
56 To many physical volumes in PhysicalVolumePath. At least\none physical volume must  stay in the volume group.\n
57 Couldn't install the physical volume "%s".\n
58 Physical volume "%s" was made known to the LVM device driver,\n but it could not be added to "%s".\n The LVM is not in a consistent state.\n
59 Volume group "%s" does not exist in the "%s" file.\n
60 Cannot open the control file "%s":\n
61 Illegal path "%s".\n
62 Couldn't open directory "%s":
63 Couldn't stat physical volume "%s":\n
64 Couldn't open physical volume "%s":\n
65 Couldn't query physical volume "%s":\n
66 Warning: couldn't query physical volume "%s":\n
67 Couldn't delete physical volume:\n
68 Couldn't delete logical volume "%s":\n
69 Couldn't query physical volume:\n
70 Couldn't query the allocation map of the physical volume:\n
71 Couldn't query the allocation map of the logical volume:\n
72 Couldn't set the unique id for volume group "%s":\n
73 Couldn't deactivate volume group "%s":\n
74 Couldn't activate volume group "%s":\n
75 Couldn't query volume group "%s":\n
76 Couldn't query the logical volume:\n
77 Couldn't query logical volume "%s":\n
78 Couldn't reduce the logical volume:\n
79 Warning: Couldn't attach to the volume group physical volume "%s":\n
80 Couldn't extend the logical volume:\n
81 Couldn't update the contents of logical extent:\n
82 Couldn't re-synchronize stale partitions of the logical volume:\n
83 Couldn't access the list of physical volumes for volume group "%s".\n
84 Couldn't access the list of logical volumes for volume group "%s".\n
85 Couldn't query the list of physical volumes.\n
86 Couldn't query the list of logical volumes.\n
87 Couldn't print the information about physical volume "%s".\n
88 Couldn't print the information about logical volume "%s".\n
89 available
90 unavailable
91 Couldn't find the volume group to which\n physical volume "%s" belongs.\n
92 Couldn't find the volume group to which\n logical volume "%s" belongs.\n
93 no
94 yes
95 No volume group name could be read from "%s".\n
96 Cannot display volume group "%s".\n
97 Cannot display logical volume "%s".\n
98 Cannot display physical volume "%s".\n
99 Couldn't get current working directory
100 Cannot figure out the name of volume group to which\nlogical volume "%s" belongs.\n
101 Unable to generate a name for the logical volume.\n
102 A logical volume with name "%s" will be created.\n
103 Volume group "%s" has been successfully created.\n
104 Logical volume "%s" has been successfully created\nwith minor number %d.\n
105 Physical volume "%s" has been successfully created.\n
106 Transferring logical extents of logical volume "%s"...\n
107 Physical volume "%s" has been successfully moved.\n
108 Logical volume "%s" already exists.\n
109 The status of the volume group "%s" could not be read.\n The major number, which is needed when creating the\n logical volume, could therefore not be retrieved.\n
110 The logical volume "%s" could not be created:\n
111 The logical volume "%s" could not be created\n as a special file in the file-system:\n
112 The logical volume "%s" could not be deleted from the\n LVM device driver.\n There is now an inconsistency between the device driver and the file-system:\n
113 read-only
114 read/write
115 available/stale
116 available/syncd
117 unavailable
118 on
119 off
120 on
121 off
122 strict
123 non-strict
124 sequential
125 parallel
126 available
127 unavailable
128 stale
129 current
130 missing
131 stale
132 current
133 free
134 "%s" is not a logical volume.\n
135 Can't get minor number of logical volume "%s".\n
136 Couldn't query from the LVM device driver\n the current setting of the logical volume "%s":\n
137 Volume group "%s" has been successfully changed.\n
138 Logical volume "%s" has been successfully changed.\n
139 Logical volume "%s" could not be changed:\n
140 Logical volume "%s" could not be changed. \nThe driver has wrong number of mirrors information:\n
141 Physical volume "%s" has been successfully changed.\n
142 Physical volume "%s" could not be changed:\n
143 The volume group to which the logical volume "%s" belongs\n could not be found. The logical volume path is probably wrong.\n
144 "MirrorCopies" is not bigger than current setting.\n
145 "LogicalExtentsNumber" is not bigger than current setting.\n
146 "LogicalExtentsNumber" is bigger than the maximum value allowed.\n
147 Volume group "%s" has been successfully extended.\n
148 Logical volume "%s" has been successfully extended.\n
149 Volume group "%s" has been successfully reduced.\n
150 Logical volume "%s" has been successfully reduced.\n
151 Logical volume "%s" is not reduced.\n
152 Volume group "%s" has been successfully removed.\n
153 Logical volume "%s" has been successfully removed.\n
154 Couldn't retrieve the allocation map of\n logical volume "%s".\n
155 Couldn't retrieve the allocation map of\n physical volume "%s".\n
156 Couldn't retrieve the list of the physical volumes\nbelonging to volume group "%s".\n
157 Physical volume "%s" does not belong\n to volume group "%s".\n
158 Not enough free physical extents available.\nLogical volume "%s" could not be extended.\n
159 Failure possibly caused by strict allocation policy\n
160 The LVM device driver could not extend the\nlogical volume "%s".\n
161 It is not possible to add two new mirrors to one physical\n volume when the allocation policy is STRICT.\n
162 "MirrorCopies" is not smaller than current setting;\n therefore no mirrors are removed.\n
163 "LogicalExtentNumber" is not smaller than current setting;\n therefore no logical extents are removed.\n
164 When a logical volume is reduced useful data might get lost;\n do you really want the command to proceed (y/n) : 
165 The logical volume "%s" is not empty;\ndo you really want to delete the logical volume (y/n) : 
166 The physical volume has a file system on it.\nWould you like to proceed (y/n) : 
167 The physical volume already belongs to a volume group\n
168 The LVM device driver failed to reduce\n the logical volume "%s".\n
169 The LVM device driver failed to reduce mirrors on\nthe logical volume "%s".\n
170 Logical volume "%s" can not be removed since it is active.\n
171 Couldn't remove the special file corresponding to\nLogical volume "%s" from the file-system.\n Since the logical volume is not known to the LVM device driver\n any more, it should be removed using the usual Unix commands\n (chmod(1), rm(1)).\n
172 Couldn't retrieve the names of the physical volumes\nbelonging to volume group "%s".\n
173 Couldn't retrieve the names of the logical volumes\nbelonging to volume group "%s".\n
174 Allocation is not allowed on physical volume "%s".\n
175 The source physical volume is also in the set of\ndestination physical volume(s).\n
176 Volume group "%s" does not contain any logical volume.\n
177 Logical volume "%s" does not belong to\nvolume group "%s".\n
178 Cannot open volume group "%s".\n
179 Cannot find a free physical extent for logical extent %d\n of logical volume "%s".\n
180 Warning: physical volume "%s" is missing.\n
181 Couldn't remove volume group "%s".\n
182 Volume group "%s" is still active.\n
183 Volume group "%s" still contains some logical volume.\n
184 Volume group "%s" still contains more than one physical volume.\n
185 Couldn't remove file-system subtree "%s".\n
186 Couldn't remove the entry "%s" from "%s".\n
187 Resynchronised volume group "%s".\n
188 Resynchronised logical volume "%s".\n
189 Couldn't resynchronise volume group "%s".\n
190 Couldn't resynchronise logical volume "%s".\n
191 Either no physical volumes are attached or no valid VGDAs were found\non the physical volumes.
192 Insufficient kernel memory to complete request.
193 I/O error while reading the VGDA.
194 Quorum not present, or some physical volume(s) are missing.\n
195 A component of the path of the physical volume does not exist.
196 The path of the physical volume refers to a device that does not\nexist, or is not configured into the kernel.
197 Inappropriate ioctl for device - the command was attempted on a\nlogical volume device rather than the control device.
198 The supplied minor number refers to a non-existent logical volume.
199 Volume group not activated.
200 The reduction request has not been preceded by a proper deallocation.
201 The supplied minor number refers to an already existent logical volume.
202 The path does not specify a valid physical volume.
203 Unable to read the physical volume.
204 The physical volume has no driver configured.
205 Insufficient space on the volume for the VGRA.
206 Permission denied on open of path of physical volume.
207 The path of the physical volume does not designate a block device.
208 The supplied minor number refers to a non-existent logical volume.
209 The specified logical volume is open.
210 The specified logical volume does not exist.
211 A physical extent described by the extent array is already in use.
212 The physical volume has no driver configured.
213 Write permission denied on the device.
214 The path designates a file that is not a block device.
215 A component of the path was not accessible.
216 Volume group not activated.\n
217 Could not attach to volume group "%s" all of\nits physical volumes.\n
218 Warning: Couldn't deattach the physical volume \n
219 Couldn't remove file "%s":
220 Couldn't remove directory "%s":
221 VG Name
222 VG Status
223 Max LV
224 Cur LV
225 Open LV
226 Max PV
227 Cur PV
228 Act PV
229 PE Size
230 Max PE per PV
231 Total PE
232 Alloc PE
233 Free PE
234 VGDA
235 LV Name
236 VG Name
237 LV Permission
238 LV Status
239 Write verify
240 Mirror copies
241 Schedule
242 Current LE
243 Allocated PE
244 Bad block
245 Allocation
246 Used PV
247 PV Name
248 VG Name
249 PV Status
250 Allocatable
251 VGDA
252 Cur LV
253 PE Size
254 Total PE
255 Free PE
256 Allocated PE
257 Stale PE
258 LE
259 PV1
260 PE1
261 Status 1
262 PV2
263 PE2
264 Status 2
265 PV3
266 PE3
267 Status 3
268 PE
269 Status
270 LV
271 LE
272 PV Name
273 LE on PV
274 PE on PV
275 LV Name
276 LE of LV
277 PE for LV
278 Volume groups
279 Logical volumes
280 Physical volumes
281 Logical extents
282 Physical extents
283 Distribution of logical volume
284 Distribution of physical volume
285 The specified path does not correspond to physical volume attached to \n this volume group
286 The volume group is not activated
287 Bad input parameter\n
288 The number of mirrors can not be extended since the logical \nvolume do not have any logical extents allocated to it.\n
289 Warning: The physical volume "%s" is not \navailable for allocation.\n
290 The newly allocated mirrors is now being synchronized.This operation will \ntake some time. Please wait ....\n
$set 2
1 Can't mix HP-IB and non-HP-IB devices.\n
2 Can't mirror on HP-IB devices.\n
3 Can't relocate bad blocks on HP-IB devices.\n
4 Total PVG
5 Physical volume groups
6 VG Name
7 PVG Name
8 PV Name
9 PVG-strict
10 Failed to add volume group "%s" to file "%s".\n
11 Physical volume group "%s" is already defined in volume group "%s".\n
12 Failed to add physical volume group "%s" to volume group "%s".\n
13 Physical volume group "%s" has been successfully created.\n
14 Failure occurred; physical volume group "%s" not created.\n
15 The destination PVG "%s" has no PVs in it.\n
16 Couldn't create PVG-strict LV because VG "%s" is not defined in "%s".\n
17 Can not change to a more strict allocation policy \nbecause LV "%s" has mirrored copies.\n
18 PVG "%s" is not defined in VG "%s".\n
19 Failure possibly caused by PVG-Strict allocation policy.\n
20 VG "%s" is not defined in file "%s".\n
21 Failed to add PVG "%s".\n
22 Failed to extend PVG "%s".\n
23 Physical volume group "%s" has been successfully extended.\n
24 Failure occurred; physical volume group "%s" not extended.\n
25 Physical volume "%s" could not be deleted from the "%s" file.\n
26 Physical volume "%s" has been successfully deleted from \n physical volume group "%s".\n
27 Couldn't remove volume group "%s" from "%s" file.\n
28 Volume group "%s" has been successfully removed from "%s" file.\n
29 File "%s" is missing.\n
30 Error detected when creating file "%s".\n
31 Error detected when reading from file "%s".\n
32 Error detected when writing to file "%s".\n
33 Bad file "%s": Missing VG keyword.\n
34 Bad file "%s": VG "%s" has no PVG.\n
35 Bad file "%s": Missing PVG keyword.\n
36 Bad file "%s": PVG "%s"has no PV.\n
37 Information in "%s" differs from that in "%s".\n
38 Bad file "%s": Illegal Path "%s".\n
39 Bad file "%s": PVG has no name in VG "%s".\n
40 Couldn't reallocate the logical volume:\n
41 Bad file "%s": PVG "%s" is doubly defined in VG "%s".\n
42 Bad file "%s": PV "%s" in "%s" is doubly defined in VG "%s".\n
43 There is now an inconsistency between file "%s" and file "%s".\n
44 The logical volumes are not in the same volume group.\n
45 The destination logical volume already has extents allocated to it: %s\n
46 The source logical volume doesn't have any extents: %s\n
47 There aren't enough mirrors to make a split copy: %s\n
48 Logical volume "%s" has been successfully split.\n
49 The logical volumes "%s" and "%s"\nare different sizes.\n
50 The logical volume "%s" can't have another mirror copy.\n
51 Logical volume "%s" has been successfully merged\nwith logical volume "%s".\n
52 Caution: Mirrors might be inconsistent on this LV "%s" \n due to improper shutdown.\n
53 All extents are STALE on LV "%s", LX "%d".\n
54 Couldn't set extents to STALE for LV "%s".\n
55 Failed to recover mirror consistency for LV "%s".\n
56 A daemon "%s" has been spawned to synchronize the mirrors for "%s".\n
57 Couldn't change MirrorWriteCache while LV is opened \n or being synchronized.\n
58 "-C" can be used only with "-M n".\n
59 "-C" can only be used when MWC is off or beingturned OFF.\n
60 Consistency Recovery
61 MWC
62 NOMWC
63 NONE
64 available
65 Illegal combination of options
66 IO error on configuration file\n
67 IO error on Physical Volume device\n
68 No memory to malloc\n
69 Invalid LVMREC on Physical Volume %s\n
70 Could not access Physical Volume %s: Retaining previously\nbacked-up information\n
71 Could not find Physical Volume %s\nin configuration file %s. Configuration file not updated.\n
72 Could not open Physical Volume "%s".\n
73 All PVs must be present to backup Volume Group configuration.\n
74 Volume Group configuration for %s has been saved in %s\n
75 Volume Group configuration has been restored to %s\n
76 Configuration info for Physical Volume "%s" not found\n in "%s" :\nCharacter-special file for PV not supplied on command line, OR\nPV did not belong to Volume Group %s when configuration file was created.\n
77 Error opening LVM configuration file : %s\n
78 Configuration file appears corrupted\n
79 Volume Group : %s not found in configuration file %s\n
80 Volume Group Configuration information in "%s"\n
81 (Bootable)
82 (Non-bootable)
83 OldPVPath and PVPath : character special files\n
84 Configuration file "%s" not found or is corrupted\n
85 VGRA appears to be corrupted\n
86 Physical Volume "%s" contains no LVM information\n
87 Physical Volume "%s" is not a Boot Disk\n
88 Physical Volume "%s" is not part of a Volume Group\n
89 Can not change to contiguous allocation policy because \n LV "%s" currently has non-contiguous physical extents.\n
90 none_strict/contiguous
91 strict/contiguous
92 PVG-strict/contiguous
93 Failure possibly caused by contiguous allocation policy.\n
94 Check [PhysicalVolumePath ... | PhysicalVolumeGroupName...] option \n to ensure they cover existing Physical Volumes for all the mirrors.\n
95 Couldn't export volume group "%s".\n
96 Couldn't open MAPFILE "%s".\n
97 The volume group "%s" is active on this system.Can not override "%s" with active volume groups present.\n
98 Couldn't parse MAPFILE "%s".\n
99 Logical Volume "%s" found in "%s" is not defined on physicalvolume.\n
100 Unable to scan Physical Volumes on system\n
101 Unable to Complete Import or Create MapFile for VG%02d.\n
102 Scan of Physical Volumes Complete.\n
103 Unable to issue exec.\n
104 Volume Group Identifier on Imported Disks matches \nexisting Volume Group "%s".\n
105 Warning: Volume Group "%s" is defined in the "%s" file but is missing the Group Special File.\nUnable to determine Volume Group Number.  Continuing.\n
106 Maximum number of Volume Groups already exists on the system.\n
107 Warning: Number of Physical Volumes on command line "%d" \ndoes not match the number of Physical Volumes "%d" in the Volume\nGroup Data on the Imported Physical Volumes.  Continuing.\n
108 Beginning the export process on Volume Group "%s".\n
109 Beginning the import process on Volume Group "%s".\n
110 Warning: Logical Volume number "%d" found on physicalvolume not found in "%s".\n
111 Warning: Logical Volume "%s" already exists but\ndoes not match Volume Group Id for "%s".  Removing.\n
112 Cannot lock "%s" still trying ...
113 Must be a value between 0 and 7039.
114 Too many arguments
115 Logical Volume "%s" must be contiguous.\n
116 Could not open /dev/console\n
117 Physical Volume "%s" on which Logical \nVolume "%s" resides is not a Boot Physical Volume.\n
118 A Root Logical Volume must be assigned before\na Dump or Swap Logical Volume can be assigned.\n
119 A Logical Volume has already been assigned\nto be the Root or Swap Logical Volume.\n
120 No LIF Boot information found on "%s".\n
121 Warning:LABEL file not found or Corrupt on "%s". Creating.\n
122 LIF information corrupt or not present on  "%s".\n
123 LABEL file is full.  No more entries allowed.\n
124 Unable to write LABEL to "%s".\n
125 No valid Boot Data Reserved Areas exist on any of the \nPhysical Volumes in the Volume Group "%s".\nIssue the pvcreate -B command to create a Boot Area on a Physical Volume.\n
126 Logical Volume "%s" found on Physical Volume "%s".\n
127 Root Logical Volume must be the first Logical \nVolume on the Physical Volume.\n
128 Unable to Remove Dump Logical Volume \n
129 Unable to Remove Swap Logical Volume \n
130 Unable to Remove Root Logical Volume \n
131 Logical Volume "%s" already configured to beRoot Logical Volume.\n
132 Logical Volume "%s" is not configured to be\na Root, Swap or Dump Logical Volume.\n
133 A Logical Volume is already configured on \n Physical Volume "%s".  Only one Dump Logical Volume \nper Physical Volume allowed.\n.
134 The Boot Data Area is empty.\n
135 Warning:  Valid Boot Data Area Found. \nIf this Physical Volume "%s" is to become a Boot Device with \nthe use of /etc/mkboot, this newly created LABEL file will be overwritten.\n
136 Too many physical volumes in PhysicalVolumePath.\nMaximum of "%d"\n
137 Maximum number of Dump Logical Volumes has been exceeded.\n   Unable to add another Dump Logical Volume.\n
138 Boot Definitions for Volume Group %s:\n
139 Physical Volumes belonging in Root Volume Group:\n
140 %s: %s\ton: 
141 %s: %s\ton: %s\n
142 Root
143 Swap
144 Dump
145 Label Entry: %d, %s Volume start: %d length %d\n
146 No %s Logical Volume configured\n
147 Logical Volume has no extents.\n
148 Boot Disks: 
149 Not enough physical extents per physical volume.\n Need: %d, Have: %d.\n
150 Increased the number of physical extents per physical volume to %d.\n
$set 3
1 y
2 n
