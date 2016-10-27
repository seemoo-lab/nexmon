{
	if ($2 == "PATCH")
		print ".text." $4 " " $1 " : { KEEP(" $3 " (.*." $4 ")) }";
	else if ($2 == "DUMMY")
		print ".text.dummy." $4 " " $1 " : { " $3 " (.*." $4 ") }";
	else if ($2 == "REGION")
		print ".text." $4 " : { KEEP(" $3 " (.*." $4 ")) } >" $1;
	else if ($2 == "TARGETREGION")
		print ".text." $1 " : { " $3 " (.text .text.* .data .data.* .bss .bss.* .rodata .rodata.*) } >"  $1;
}
