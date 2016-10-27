BEGIN {
	src_file = src_file;
}
{
	if ($2 == "TARGETREGION") {
		"$NEXMON_ROOT/buildtools/scripts/getsectionaddr.sh .text." $1 " " src_file | getline result; print $0 " " result
	} else {
		print;
	}; 
}
