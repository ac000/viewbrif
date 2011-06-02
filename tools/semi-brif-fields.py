#!/usr/bin/python
#

import sys

def do_main_record(line):
	print "Main Record Line"
	print
	print "\tF3  = " + line[2:3]
	print "\tF5  = " + line[4:5]
	print "\tF15 = " + line[72:84]
	print "\tF18 = " + line[136:]

def do_purchasing_card(line):
	print "Purchasing Card Line"
	print
	print "\tF6  = " + line[5:17]
	print "\tF8  = " + line[30:42]
	print "\tF9  = " + line[42:50]
	print "\tF10 = " + line[50:62]
	print "\tF11 = " + line[62:74]
	print "\tF15 = " + line[97:101]
	print "\tF17 = " + line[102:122]
	print "\tF19 = " + line[134:149]
	print "\tF22 = " + line[184:196]

def do_purchasing_card_item(line):
	print "Purchasing Card Item Line"
	print
	print "\tF4  = " + line[3:4]
	print "\tF6  = " + line[5:8]
	print "\tF7  = " + line[8:20]
	print "\tF8  = " + line[20:32]
	print "\tF10 = " + line[47:87]
	print "\tF11 = " + line[87:99]
	print "\tF12 = " + line[99:103]
	print "\tF13 = " + line[103:115]
	print "\tF14 = " + line[115:127]
	print "\tF15 = " + line[127:139]
	print "\tF16 = " + line[139:151]


#
# Main program start
#

if len(sys.argv) < 2:
	print "Usage: " + sys.argv[0] + " <semi brif file>"
	sys.exit(-1)

bfp = open(sys.argv[1], 'r')

for line in bfp:
	if line[2:3] == "R" or line[2:3] == "S":
		do_main_record(line)
	elif line[3:4] == " ":
		print
		do_purchasing_card(line)
	elif line[3:4] == "0" or line[3:4] == "1":
		print
		do_purchasing_card_item(line)

bfp.close()
