#
#	$Header: version.awk,v 1.1.109.4 92/02/28 16:02:24 ash Exp $
#
BEGIN {
	maxfields = 4;
	max = 0; strmax = ""; test =""; local="";
	for (i = 1; i <= maxfields; i++) {
		power[i] = exp(log(10)*(maxfields-i));
	}
}
/\$Header/ && !/flex/ {
	if (NF >= 3) {
		version = "";
		if ( substr($2,1,6) == "*rcsid" ) {
			version = $6;
                        locked = $10;
			newlock = $11;
		} 
		if ( $1 == "*" ) {
			version = $4;
			locked = $8;
			newlock = $9;
		}
		if ( version == "" ) {
			continue;
		}
                if ( locked == "Locked" || newlock == "Locker:") {
			test = ".development";
		}
		sum = 0;
		num = split(version, string, ".")
		if (num > maxfields) {
			local = ".local";
			num = maxfields;
		}
		for (i = 1; i <= num; i++) {
			sum += string[i]*power[i];
		}
		if ( sum > max ) {
			max = sum;
			strmax = version;
		}
	}
}
END {
	print "#include \"include.h\""
	print "const char *version = \"" strmax local test "\";"
}

