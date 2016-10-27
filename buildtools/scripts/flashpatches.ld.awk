{
	if ($2 == "FLASHPATCH")
		print ".text." $4 " " $1 " : { KEEP(" $3 " (.*." $4 ")) }";
}
