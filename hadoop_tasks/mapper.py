#!/usr/bin/python2

import sys

def map(line):
	if line.strip().startswith("<td id=\"LC"):
		s = line[line.find('>') + 1 : line.rfind('<')]
		x,y = s.split(',')
		x = float(x)
		y = float(y)
		yield (x * x + y * y <= 1), 1

for line in sys.stdin:
	for key, value in map(line):
		print str(key) + "\t" + str(value)
		
