import time
import datetime
import dateutil.parser
import random
import math
import glob
import os
import re
import pprint

re_recent_xid = re.compile("recent_xid_([a-zA-Z]+)_(\d+)_(\d+)\.data")

file_list = glob.glob("gnuplot_data/recent_xid_*_*_*.data")

max_xid = 0
min_xid = 10000000000000000000
recent_xid_data = {}
for file_path in file_list:
	configure = re_recent_xid.findall(file_path)[0]
	engine = configure[0]
	record_size = configure[1]
	
	if engine not in recent_xid_data:
		recent_xid_data[engine] = {}
	if record_size not in recent_xid_data[engine]:
		recent_xid_data[engine][record_size] = []

	with open(file_path, "r") as f:
		data = f.read()
	data = data.split("\n")
	timestamp = float(data[0])
	timestamp = datetime.datetime.fromtimestamp(timestamp)
	xid = int(data[3])
	if xid < min_xid: min_xid = xid
	if xid > max_xid: max_xid = xid
	recent_xid_data[engine][record_size].append([timestamp, xid])

for engine in recent_xid_data:
	for record_size in recent_xid_data[engine]:
		recent_xid_data[engine][record_size].sort()
		begin_time = recent_xid_data[engine][record_size][0][0]
		end_time = recent_xid_data[engine][record_size][-1][0]

		with open("gnuplot_data/temp.tps_" + engine + "_" + record_size + ".data", "w") as f:
			last_xid = min_xid
			last_time = begin_time
			for line in recent_xid_data[engine][record_size]:
				date, xid = line
				if date == last_time:
					continue
				if date < last_time:
					continue
				print(((date - begin_time).total_seconds() + (last_time - begin_time).total_seconds()) / 2,
						(xid - last_xid)/((date - begin_time).total_seconds() - (last_time - begin_time).total_seconds()),
						sep = ',', file = f)
				last_xid = xid 
				last_time = date
	
	# Get long xid
	# [start time, end time, xid]
	long_data = []
	with open("gnuplot_data/longx_01_begin_" + engine + ".data", "r") as f:
		data = f.read()
		timestamp = float(data.split("\n")[0])
		long_begin = datetime.datetime.fromtimestamp(timestamp)
	with open("gnuplot_data/longx_01_end_" + engine + ".data", "r") as f:
		data = f.read()
		timestamp = float(data.split("\n")[0])
		long_end = datetime.datetime.fromtimestamp(timestamp)

	long_data.append([long_begin, long_end])

	# Long X data
	long_txn = []
	for line in long_data:
		if begin_time <= line[0] <= end_time:
			long_txn = [line[0], line[1]]

	with open("gnuplot_data/temp.long_" + engine + ".data", "w") as f:
		begin, end = long_txn
		print((begin - begin_time).total_seconds(), (end - begin_time).total_seconds(), sep = ',', file = f)

