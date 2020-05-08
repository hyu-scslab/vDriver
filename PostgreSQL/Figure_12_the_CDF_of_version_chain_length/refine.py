


import time

import datetime
import dateutil.parser
import random
import math

import glob

import os









import re
re_xid_engine = re.compile("recent_xid_([a-zA-Z]+)_\d+\.data")
re_stat_engine = re.compile("stat_([a-zA-Z]+)_\d+\.data")
re_size_engine = re.compile("size_([a-zA-Z]+)_\d+\.data")
re_llt_begin_engine = re.compile("longx_\d+_begin_([a-zA-Z]+)\.data")
re_llt_begin_id = re.compile("longx_(\d+)_begin_[a-zA-Z]+\.data")
re_llt_end_engine = re.compile("longx_\d+_end_([a-zA-Z]+)\.data")
re_llt_end_id = re.compile("longx_(\d+)_end_[a-zA-Z]+\.data")

re_int = re.compile(": *(\d+) *\+")
re_float = re.compile(": *(\d*\.?\d+) *\+")

infos = dict()

file_list = glob.glob("gnuplot_data/recent_xid_*_*.data")
for file_path in file_list:
	engine = str(re_xid_engine.findall(file_path)[0])
	if not engine in infos:
		infos[engine] = dict()
	with open(file_path, "r") as f:
		data = f.read()
	data = data.split("\n")
	timestamp = float(data[0])
	timestamp = datetime.datetime.fromtimestamp(timestamp)
	xid = int(data[3])
	if not "xid" in infos[engine]:
		infos[engine]["xid"] = []
	infos[engine]["xid"].append([timestamp, xid])


re_integer = re.compile("(\d+)\D*")
file_list = glob.glob("gnuplot_data/size_*_*.data")
for file_path in file_list:
	engine = str(re_size_engine.findall(file_path)[0])
	if not engine in infos:
		infos[engine] = dict()
	with open(file_path, "r") as f:
		data = f.read()
	data = data.split("\n")
	timestamp = float(data[0])
	timestamp = datetime.datetime.fromtimestamp(timestamp)
	total_size = int(re_integer.findall(data[1])[0])
	redo_size = int(re_integer.findall(data[2])[0])
	if not "size" in infos[engine]:
		infos[engine]["size"] = []
	infos[engine]["size"].append([timestamp, total_size - redo_size])


file_list = glob.glob("gnuplot_data/longx_*_begin_*.data")
for file_path in file_list:
	engine = str(re_llt_begin_engine.findall(file_path)[0])
	if not engine in infos:
		infos[engine] = dict()
	with open(file_path, "r") as f:
		data = f.read()
	data = data.split("\n")
	timestamp = float(data[0])
	begin_timestamp = datetime.datetime.fromtimestamp(timestamp)
	with open(file_path.replace("_begin_", "_end_"), "r") as f:
		data = f.read()
	data = data.split("\n")
	timestamp = float(data[0])
	end_timestamp = datetime.datetime.fromtimestamp(timestamp)
	if not "llt" in infos[engine]:
		infos[engine]["llt"] = []
	infos[engine]["llt"].append([begin_timestamp, end_timestamp])


file_list = glob.glob("gnuplot_data/stat_*_*.data")
for file_path in file_list:
	stat = dict()
	stat_list = [
		"_recent xid_",
		"_vanilla version chain counter_",
	]
	engine = str(re_stat_engine.findall(file_path)[0])
	if not engine in infos:
		infos[engine] = dict()
	with open(file_path, "r") as f:
		data = f.read()
	lines = data.split("\n")
	for line in lines:
		for key in stat_list:
			if key in line:
				stat[key] = int(re_int.findall(line)[0])
	for line in lines:
		if "_time_" in line:
			timestamp = float(re_float.findall(line)[0])
	timestamp = datetime.datetime.fromtimestamp(timestamp)
	for key in stat_list:
		if not key in infos[engine]:
			infos[engine][key] = []
		infos[engine][key].append([timestamp, stat[key]])


file_list = glob.glob("gnuplot_data/stat_VDRIVER.data")
for file_path in file_list:
	stat_list = [
		"_recent xid_",
		"_vdriver version chain counter_",
	]
	engine = "VDRIVER"
	if not engine in infos:
		infos[engine] = dict()
	with open(file_path, "r") as f:
		data = f.read()
	lines = data.split("\n")
	timestamp = None
	for line in lines:
		if "_time_" in line:
			if timestamp != None:
				for key in stat_list:
					if not key in infos[engine]:
						infos[engine][key] = []
					infos[engine][key].append([timestamp, stat[key]])
			timestamp = float(re_float.findall(line)[0])
			timestamp = datetime.datetime.fromtimestamp(timestamp)
			stat = dict()
		for key in stat_list:
			if key in line:
				stat[key] = int(re_int.findall(line)[0])




vdriver_chain_list = []
file_list = glob.glob("gnuplot_data/sample_VDRIVER.data")
for file_path in file_list:
	with open(file_path, "r") as f:
		data = f.read()
	lines = data.split("\n")
	for line in lines:
		if "_vdriver version chain counter_" in line:
			vdriver_chain_list.append(int(re_int.findall(line)[0]))

vanilla_chain_list = []
file_list = glob.glob("gnuplot_data/sample_VANILLA.data")
for file_path in file_list:
	with open(file_path, "r") as f:
		data = f.read()
	lines = data.split("\n")
	for line in lines:
		if "_vanilla version chain counter_" in line:
			vanilla_chain_list.append(int(re_int.findall(line)[0]))

# Make file for gnuplot.

with \
	open("gnuplot_data/temp.postgresql_vanilla_tps.data", "w") as f_vanilla_tps,\
	open("gnuplot_data/temp.postgresql_vanilla_size.data", "w") as f_vanilla_size,\
	open("gnuplot_data/temp.postgresql_vanilla_chain.data", "w") as f_vanilla_chain,\
	open("gnuplot_data/temp.postgresql_vanilla_llt.data", "w") as f_vanilla_llt,\
	open("gnuplot_data/temp.postgresql_vdriver_tps.data", "w") as f_vdriver_tps,\
	open("gnuplot_data/temp.postgresql_vdriver_size.data", "w") as f_vdriver_size,\
	open("gnuplot_data/temp.postgresql_vdriver_chain.data", "w") as f_vdriver_chain,\
	open("gnuplot_data/temp.postgresql_vdriver_llt.data", "w") as f_vdriver_llt,\
	open("gnuplot_data/temp.vanilla_chain_spectrum.data", "w") as f_vanilla_chain_spectrum,\
	open("gnuplot_data/temp.vdriver_chain_spectrum.data", "w") as f_vdriver_chain_spectrum:

	vanilla_chain_list = sorted(vanilla_chain_list)
	vanilla_chain_min = min(vanilla_chain_list)
	vanilla_chain_max = max(vanilla_chain_list)
	vanilla_chain_hist = [ 0 for _ in range(vanilla_chain_min, vanilla_chain_max + 1) ]
	print(vanilla_chain_min, vanilla_chain_max)
	for chain in vanilla_chain_list:
		vanilla_chain_hist[chain - vanilla_chain_min] = vanilla_chain_hist[chain - vanilla_chain_min] + 1
	vanilla_chain_spec = []
	acc = 0
	for chain in vanilla_chain_hist:
		acc = acc + chain
		vanilla_chain_spec.append(acc)
	for k, number in enumerate(vanilla_chain_spec):
		print(math.log10(k + vanilla_chain_min + 1), number / acc * 100, sep = ",", file = f_vanilla_chain_spectrum)

	vdriver_chain_list = sorted(vdriver_chain_list)
	vdriver_chain_min = min(vdriver_chain_list)
	vdriver_chain_max = max(vdriver_chain_list)
	vdriver_chain_hist = [ 0 for _ in range(vdriver_chain_min, vdriver_chain_max + 1) ]
	print(vdriver_chain_min, vdriver_chain_max)
	for chain in vdriver_chain_list:
		vdriver_chain_hist[chain - vdriver_chain_min] = vdriver_chain_hist[chain - vdriver_chain_min] + 1
	vdriver_chain_spec = []
	acc = 0
	for chain in vdriver_chain_hist:
		acc = acc + chain
		vdriver_chain_spec.append(acc)
	for k, number in enumerate(vdriver_chain_spec):
		print(math.log10(k + vdriver_chain_min + 1), number / acc * 100, sep = ",", file = f_vdriver_chain_spectrum)

	for engine, info in infos.items():
		if engine == "VANILLA":
			file_size = f_vanilla_size
			file_tps = f_vanilla_tps
			file_chain = f_vanilla_chain
			file_llt = f_vanilla_llt
		if engine == "VDRIVER":
			file_size = f_vdriver_size
			file_tps = f_vdriver_tps
			file_chain = f_vdriver_chain
			file_llt = f_vdriver_llt

		size_list = info["size"]
		size_list = sorted(size_list, key = lambda x: x[0])

		begin_time = size_list[0][0]
			
		for timestamp, size in size_list:
			print((timestamp - begin_time).total_seconds(), size, sep = ",", file = file_size)


		if engine == "VANILLA":
			chain_list = info["_vanilla version chain counter_"]
		if engine == "VDRIVER":
			chain_list = info["_vdriver version chain counter_"]
		chain_list = sorted(chain_list, key = lambda x: x[0])

		begin_time = chain_list[0][0]
			
		for timestamp, chain in chain_list:
			chain = math.log10(chain if chain != 0 else 1)
			print((timestamp - begin_time).total_seconds(), chain, sep = ",", file = file_chain)


		xid_list = info["xid"]
		xid_list = sorted(xid_list, key = lambda x: x[0])

		begin_time = xid_list[0][0]
		last_time = begin_time

		begin_xid = xid_list[0][1]
		last_xid = begin_xid

		for timestamp, xid in xid_list:
			if timestamp == last_time:
				continue
			tps = (xid - last_xid) / (timestamp - last_time).total_seconds()
			print(((timestamp - begin_time).total_seconds() + (last_time - begin_time).total_seconds()) / 2, tps, sep = ",", file = file_tps)
			last_time = timestamp
			last_xid = xid


		llt_list = info["llt"]
		llt_list = sorted(llt_list, key = lambda x: x[0])

		for begin, end in llt_list:
			print((begin - begin_time).total_seconds(), (end - begin_time).total_seconds(), sep = ",", file = file_llt)
