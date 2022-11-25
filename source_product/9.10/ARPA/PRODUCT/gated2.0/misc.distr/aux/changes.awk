BEGIN {
	printf "\n%55s\n\n",\
		"Activity Summary";
	printf "%-15s %12s %12s %12s %12s %12s %12s\n",\
		"", "", "", "", "",\
		"Gateway", "Metric";
	printf "%-15s %12s %12s %12s %12s %12s %12s\n\n",\
		"Destination", "Additions", "Holddowns", "Deletions", "Redirects",\
		"Changes", "Changes"
}
$1 == "DELETE" {
	sum[$2]++;
	delete[$2]++;
	gateway[$2] = "";
	metric[$2] = "";
}
$1 == "ADD" {
	sum[$2]++;
	add[$2]++;
	gateway[$2] = $4;
	metric[$2] = $9;
	if ($5 == "Redirect") {
		redirects[$2]++;
	}
}
$1 == "CHANGE" && !/HoldDown/ {
	sum[$2]++;
	if (gateway[$2] != $4) {
		gateway[$2] = $4;
		gateway_change[$2]++;
	} 
	if (metric[$2] != $9) {
		metric[$2] = $9;
		metric_change[$2]++;
	}
}
$1 == "CHANGE" && /HoldDown/ {
	sum[$2]++;
	holddown[$2]++;
}
END {
	for (net in sum) {
		printf "%-15s %12s %12s %12s %12s %12s %12s\n",\
			net,\
			add[net],\
			holddown[net],\
			delete[net],\
			redirects[net],\
			gateway_change[net],\
			metric_change[net];
		sum_nets += 1 ;
		sum_adds += add[net] ;
		sum_holddowns += holddown[net] ;
		sum_deletes += delete[net] ;
		sum_redirects += redirects[net] ;
		sum_gates += gateway_change[net] ;
		sum_metrics += metric_changes[net] ;
	}
	printf "\n" ;
	printf "%-6s %8d %12s %12s %12s %12s %12s %12s\n",\
		"Totals",\
		sum_nets,\
		sum_adds,\
		sum_holddowns,\
		sum_deletes,\
		sum_redirects,\
		sum_gates,\
		sum_metrics ;
}
