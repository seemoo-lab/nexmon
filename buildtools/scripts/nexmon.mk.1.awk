BEGIN {
	src_file = src_file;
}
{
	if ($2 == "TARGETREGION" || $2 == "REGION") {
		sec = ($2 == "TARGETREGION") ? (".text." $1) : (".text." $4)
		key = src_file SUBSEP sec
		if (key in _cache) {
			result = _cache[key]
		} else {
			# Cache getsectionaddr.sh results — previously spawned
			# 30× objdump processes per build. Use ENVIRON when
			# available, fallback to shell expansion.
			if (ENVIRON["NEXMON_ROOT"] != "") {
				cmd = ENVIRON["NEXMON_ROOT"] "/buildtools/scripts/getsectionaddr.sh " sec " " src_file
			} else {
				cmd = "$NEXMON_ROOT/buildtools/scripts/getsectionaddr.sh " sec " " src_file
			}
			result = ""
			cmd | getline result
			close(cmd)
			_cache[key] = result
		}
		print $0 " " result
	} else {
		print;
	}; 
}
