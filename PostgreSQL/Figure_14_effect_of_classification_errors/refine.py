import time
import datetime
import dateutil.parser
import random
import math
import glob
import os
import re

re_llt_cuttime = re.compile("llt_get_cuttime_([a-zA-Z]+)_([a-zA-Z]+)\.data")

file_list = glob.glob("gnuplot_data/llt_get_cuttime_*_*.data")
for file_path in file_list:
	configure = re_llt_cuttime.findall(file_path)[0]
	distribution = configure[0]
	classification = configure[1]
	
	with open(file_path, "r") as f:
		data = f.read()
	data = data.split("\n")
	
	# Make file for gnuplot.
	with open("gnuplot_data/temp.llt_get_cuttime_" + distribution + "_" + classification + ".data", "w") as f_cuttime:
		for i in range(3, 62):
			print(data[i][:-1], file = f_cuttime) # remove '+' for each line end
