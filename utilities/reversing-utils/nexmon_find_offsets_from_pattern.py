import idc
from idaapi import *

chip = "CHIP_VER_BCM43455c0"
fw = "FW_VER_7_45_189"

def find_offset(pattern, name):
	ea = idc.FindBinary(0, SEARCH_DOWN, pattern);
	i = 0
	while ea != idc.BADADDR:
		print(name + " (#" + str(i) +  ") found at " + hex(ea))
		print("AT(" + chip + ", " + fw + ", " + hex(ea) + ")")
		print("")
		ea = idc.FindBinary(ea + 2, SEARCH_DOWN, pattern);
		i+=1


def main():
	filename = askfile_c(0, "*.pat", "Choose a pattern file")
	f = open(filename)
	line = f.readline()
	while line:
		if ":0000" in line:
			pattern = line[:line.find(" ")]
			pattern = ' '.join([pattern[i:i+2] for i in range(0, len(pattern), 2)])
			pattern = pattern.replace("..", "?")
			name = line[line.find(":0000") + 6:][:-1]
			find_offset(pattern, name)
		line = f.readline()
	f.close()

if __name__ == "__main__":
    main()