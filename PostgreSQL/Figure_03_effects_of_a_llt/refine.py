


import time

import datetime
import dateutil.parser
import random

import glob

import os





base_time = dateutil.parser.parse("2000-01-01 00:00:000000")

temp = ""
temp_date = ""

def line_to_date(line):
	global temp
	global temp_date
	div = line.replace("\t", " ")
	div = div.split(" ")
	t = ' '.join(div[:2])
	if t == temp:
		return temp_date
	else:
		temp = t
		temp_date = dateutil.parser.parse(t)
		return temp_date





# Get recent xids
# These data make base time line and xid range.
max_xid = 0
min_xid = 10000000000000000000

# [datetime(timestamp), xid]
recent_xid_data = []

file_list = glob.glob("gnuplot_data/recent_xid_*.data")
for file_path in file_list:
	with open(file_path, "r") as f:
		data = f.read()
		data = data.split("\n")
		timestamp = float(data[0])
		timestamp = datetime.datetime.fromtimestamp(timestamp)
		xid = int(data[3])
		if xid < min_xid: min_xid = xid
		if xid > max_xid: max_xid = xid
		recent_xid_data.append([timestamp, xid])

recent_xid_data.sort()
begin_time = recent_xid_data[0][0]
end_time = recent_xid_data[-1][0]

print(begin_time, "~", end_time)
print(min_xid, "~", max_xid)



# Get long xid
# [start time, end time, xid]
long_xid_data = []
with open("gnuplot_data/longx_xid.data", "r") as f:
	data = f.read()
	long_xid = int(data.split("\n")[3])
with open("gnuplot_data/longx_begin.data", "r") as f:
	data = f.read()
	timestamp = float(data.split("\n")[0])
	long_begin = datetime.datetime.fromtimestamp(timestamp)
with open("gnuplot_data/longx_end.data", "r") as f:
	data = f.read()
	timestamp = float(data.split("\n")[0])
	long_end = datetime.datetime.fromtimestamp(timestamp)

long_xid_data.append([long_begin, long_end, long_xid])





# Calculate cpu breakdown...........

import re
p = re.compile("(\d*\.?\d+)") # Parse floating point number
n = re.compile("vmstat_(\d+)\.data") # Parse floating point number
util_data = []
util = dict()
util["user"], util["kernel"], util["idle"], util["scan"] = 0, 0, 0, 0
util["index"] = 0
util_data.append([begin_time, util])

file_list = glob.glob("gnuplot_data/vmstat_*.data")
for file_path in file_list:
	util = dict()
	# stating-time
	seconds = int(n.findall(file_path)[0])
	date = begin_time + datetime.timedelta(seconds=seconds)

	# Get vmstat
	with open(file_path, "r") as f:
		data = f.read()
	lines = data.split("\n")[:-1]
	line = lines[-1]
	prev = line
	while True:
		line = line.replace("  ", " ")
		if prev == line:
			break
		prev = line
	line = line.split(" ")
	util["user"] = float(line[12])
	util["kernel"] = float(line[13])
	util["idle"] = float(line[14])

	# Get perf data
	perf_text_path = file_path.replace("vmstat_", "perf_")
	perf_text_path = perf_text_path + ".txt"

	# Open perf text
	with open(perf_text_path, 'r') as f:
		data = f.read()
	lines = data.split("\n")[:-1]

	# Parse each function

	index_list = [" index_insert"]
	index_sum = 0.0
	for func in index_list:
		func_max = 0.0
		for line in lines:
			if func in line:
				result = p.findall(line)
				if len(result) == 0:
					continue
				func_max = max(func_max, float(result[0]))
		index_sum += func_max
	
	scan_list = [" ExecIndexScan"]
	scan_sum = 0.0
	for func in scan_list:
		func_max = 0.0
		for line in lines:
			if func in line:
				result = p.findall(line)
				if len(result) == 0:
					continue
				func_max = max(func_max, float(result[0]))
		scan_sum += func_max

	util["scan"] = scan_sum * ( util["user"] + util["kernel"] ) / 100
	util["index"] = index_sum * ( util["user"] + util["kernel"] ) / 100

	flag = False
	for v in util.values():
		if v > 100:
			flag = True
			break
	if flag == True:
		continue

	util_data.append([date, util])


util_data.sort(key = lambda x: x[0])





# Make file for gnuplot.

# Util data
with open("gnuplot_data/temp.util.data", "w") as f:
	for line in util_data:
		if begin_time <= line[0] <= end_time:
			util = line[1]
#print(shift_time(line[0]), util["kernel"], util["index"], util["scan"], util["view"], util["prune"], util["vacuum"], util["user"], sep = ",", file = f)
			print((line[0] - begin_time).total_seconds(), util["kernel"], util["index"], util["scan"], util["user"], sep = ",", file = f)



# Long X data
long_X = dict()
for line in long_xid_data:
	if begin_time <= line[0] <= end_time:
		xid = int(line[2])
		long_X[xid] = [line[0], line[1]]

with open("gnuplot_data/temp.long_X.data", "w") as f:
	xids = list(long_X.keys())
	xids.sort()
	for xid in xids:
		time_pair = long_X[xid]
		begin, end = time_pair
		print((begin - begin_time).total_seconds(), (end - begin_time).total_seconds(), xid - min_xid, sep = ',', file = f)
		

# Recent X data
with open("gnuplot_data/temp.short_X.data", "w") as f:
	for line in recent_xid_data:
		date = line[0]
		xid = line[1]
		if begin_time <= line[0] <= end_time:
			print((date - begin_time).total_seconds(), xid - min_xid, sep = ',', file = f)
		
with open("gnuplot_data/temp.tps.data", "w") as f:
	last_xid = min_xid
	last_time = begin_time
	for line in recent_xid_data:
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


