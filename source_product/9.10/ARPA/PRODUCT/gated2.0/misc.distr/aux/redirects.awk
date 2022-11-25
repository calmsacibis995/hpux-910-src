BEGIN {
	printf "\n%25s\n\n%-15s %12s\n\n", "Redirect Summary", "Gateway", "Redirects";
}
$1 == "ADD" && $5 == "Redirect" {
	sum[$4]++;
}
END {
	for (net in sum) {
		printf "%-15s %12s\n", net, sum[net];
		sum_gws += 1 ;
		total += sum[net];
	}
	printf "\n%-6s %8d %12s\n", "Total", sum_gws, total;
}
