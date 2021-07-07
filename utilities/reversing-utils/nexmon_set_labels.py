import idc
from idaapi import *

def rename_function(address, name):
	MakeName(int(address, 16), name)


def main():
	filename = askfile_c(0, "*.c", "Choose a file")
	f = open(filename)
	chip = "CHIP_VER_BCM43455c0"
	fw = "FW_VER_7_45_189"
	line = f.readline()
	while line:
		if chip in line and (fw in line or "FW_VER_ALL" in line) and line[0:3] == "AT(":
			line = line.upper()
			addr_start = line.find("0X")
			addr_end = line.find(")", addr_start)
			address = line[addr_start:addr_end]
			while line[0:3] == "AT(":
				line = f.readline()
			function = f.readline()
			name = function[:function.find("(")]
			print(address, name)
			rename_function(address, name)
		elif chip in line and (fw in line or "FW_VER_ALL" in line) and "__attribute__" in line:
			line = line.upper()
			addr_start = line.find("0X")
			addr_end = line.find(",", addr_start)
			address = line[addr_start:addr_end]
			while "__attribute__".upper() in line:
				line = f.readline()
			name_start = line.find("(") + 1
			name_end = line.find(",", name_start)
			name = line[name_start:name_end]
			print(address, name)
		line = f.readline()
	f.close()

if __name__ == "__main__":
    main()