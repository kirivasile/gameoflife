#!/usr/bin/python2

import sys

num_true = 0
num = 0

for line in sys.stdin:
	key, value = line.split("\t")
	if key == 'True':
		num_true += 1
	num += 1

print 4 * float(num_true) / float(num)	
