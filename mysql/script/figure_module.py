import os, sys, re, os.path
import platform
import subprocess, datetime, time, signal
from multiprocessing import Process
from calc_chain import convert_version_chain
from calc_chain import convert_version_chain_17
from system_util import perf_and_vmstat, perf_to_text, make_eps

# zipfian exp value.
zipf_exp=[1.2]

# mysql server path
mysql_path="../mysql-server-8.0/"

# sysbench script path
sysbench_path="../sysbench/script/"
sysbench_socket="../../mysql-server-8.0/run/mysql.sock"

# plot path
plot_path = "./plot/"

# logfile path
logfile_path=mysql_path

# sysbench standard = uniform
sysbench_script_3 = [sysbench_path+"script_sysbench_3-std.sh", sysbench_path+"script_sysbench_3.sh"]
sysbench_script_11 = [sysbench_path+"script_sysbench_11-std.sh", sysbench_path+"script_sysbench_11.sh"]
sysbench_script_13 = [sysbench_path+"script_sysbench_13-std.sh", sysbench_path+"script_sysbench_13.sh"]
sysbench_script_15 = [sysbench_path+"script_sysbench_15-std.sh", sysbench_path+"script_sysbench_15.sh"]

# univ standard = w/ vDriver
univ_i = [mysql_path+"storage/innobase/include/univ-std.i", mysql_path+"storage/innobase/include/univ.i"]

# vDriver0seg.h std.
vDriver_header = [mysql_path+"storage/innobase/include/vDriver0seg-std.h", mysql_path+"storage/innobase/include/vDriver0seg.h"]

# my.cnf standard
my_cnf = [mysql_path+"my.cnf-std", mysql_path+"my.cnf"]

def init_server():
	print("Begin to initialize server")
	pre_cmake_compile()
	pre_compile()
	ret = os.system("cd " + mysql_path + "; bash init_server.sh > /dev/null 2>&1")
	if ret != 0:
		print("Error in init_server()")
		exit(0)
	print("Success to initialize server")

def create_db():
	ret = os.system("cd " + mysql_path + "; bash create_db.sh")
	if ret != 0:
		print("Error in create_db()")
		exit(0)

def run_server(socket=None):
	if socket == None:
		ret = os.system("cd " + mysql_path + "; bash run_server.sh &")
	elif socket == 1:
		ret = os.system("cd " + mysql_path + "; taskset -c 0-23 bash run_server.sh &")
	elif socket == 2:
		ret = os.system("cd " + mysql_path + "; taskset -c 0-47 bash run_server.sh &")
	elif socket == 4:
		ret = os.system("cd " + mysql_path + "; taskset -c 0-95 bash run_server.sh &")
	else:
		print(str(socket) + " sockets is(are) not defined.")
		exit(0)
	
	if ret != 0:
		print("Error in run_server()")
		exit(0)
	# Sleep time for starting server
	time.sleep(30)
	# create database for sysbench [ CREATE DATABASE IF NOT EXISTS sbtest ]	
	create_db()

	print("Success to run mysql server")

def shutdown_server():
	ret = os.system("cd " + mysql_path + "; bash shutdown.sh")
	if ret != 0:
		print("Error in shutdown_server()")
		exit(0)
	print("Success to shutdown mysql server")

def loop_procedure():
	ret = os.system("cd " + mysql_path + "; bash lt_loop_prod_15.sh")
	if ret != 0:
		print("Error in loop_procedure()")
		exit(0)
	
	ret = os.system("cd " + mysql_path + "; bash lt_loop_prod_11.sh")
	if ret != 0:
		print("Error in loop_procedure()")
		exit(0)
	
	ret = os.system("cd " + mysql_path + "; bash lt_loop_prod_3.sh")
	if ret != 0:
		print("Error in loop_procedure()")
		exit(0)
	print("Success to build loop procedure")

def sysbench_cleanup_prepare(figure=None):
	print("Begin to sysbench cleanup and prepare")
	if figure == 11:
		ret = os.system(sysbench_script_11[1] + " cleanup > /dev/null 2>&1")
		if ret != 0:
			print("Error in sysbench_cleanup()")
			exit(0)
		ret = os.system(sysbench_script_11[1] + " prepare > /dev/null 2>&1")
		if ret != 0:
			print("Error in sysbench_prepare()")
			exit(0)
	elif figure == 15:
		ret = os.system(sysbench_script_15[1] + " cleanup > /dev/null 2>&1")
		if ret != 0:
			print("Error in sysbench_cleanup()")
			exit(0)
		ret = os.system(sysbench_script_15[1] + " prepare > /dev/null 2>&1")
		if ret != 0:
			print("Error in sysbench_prepare()")
			exit(0)
	elif figure == 3:
		ret = os.system(sysbench_script_3[1] + " cleanup > /dev/null 2>&1")
		if ret != 0:
			print("Error in sysbench_cleanup()")
			exit(0)

		ret = os.system(sysbench_script_3[1] + " prepare > /dev/null 2>&1")
		if ret != 0:
			print("Error in sysbench_prepare()")
			exit(0)
	elif figure == 13:
		ret = os.system(sysbench_script_13[1] + " cleanup > /dev/null 2>&1")
		if ret != 0:
			print("Error in sysbench_cleanup()")
			exit(0)

		ret = os.system(sysbench_script_13[1] + " prepare > /dev/null 2>&1")
		if ret != 0:
			print("Error in sysbench_prepare()")
			exit(0)

	print("Success to sysbench cleanup and prepare")

def sysbench_run(dirname, filename, figure=None, socket=None):
	print("Begin to sysbench run")
	if socket == None:
		if figure == 11:
			ret = os.system("taskset -c 48-95 " + sysbench_script_11[1] + ' ' + dirname + '/' + filename)
		elif figure == 3:
			ret = os.system(sysbench_script_3[1] + ' ' + dirname + '/' + filename)
		elif figure == 13:
			ret = os.system(sysbench_script_13[1] + " run")
		else:
			exit(0)
	elif socket == 1:
		ret = os.system("taskset -c 0-23 "  + sysbench_script_15[1] + ' ' + dirname + '/' + filename)
		if ret != 0:
			print("Error in sysbench_run()")
			exit(0)
	elif socket == 2:
		ret = os.system("taskset -c 0-47 "  + sysbench_script_15[1] + ' ' + dirname + '/' + filename)
		if ret != 0:
			print("Error in sysbench_run()")
			exit(0)
	elif socket == 4:
		ret = os.system("taskset -c 0-95 "  + sysbench_script_15[1] + ' ' + dirname + '/' + filename)
		if ret != 0:
			print("Error in sysbench_run()")
			exit(0)
	else:
		print(str(socket) + " sockets is(are) not defined")
		exit(0)
	print("Success to sysbench run")

def run_long_transaction_socket(socket=None):
	for i in range(1, 5):
		ret = os.system("cd " + mysql_path + "; (sleep 10; taskset -c " + str(24 * int(socket) - i) + " \
			 bash run_lt_loop_15.sh) & ")
		if ret != 0:
			print("Error in run_long_transaction()")
			exit(0)

def run_long_transaction_3():
	for i in range(0, 10):
		ret = os.system("cd " + mysql_path + "; (sleep 20; bash run_lt_loop_3.sh) & ")
		if ret != 0:
			print("Error in run_long_transaction_3()")
			exit(0)

def run_long_transaction_13():
	for i in range(0, 4):
		ret = os.system("cd " + mysql_path + "; (sleep 10; bash run_lt_loop_13.sh) & ")
		if ret != 0:
			print("Error in run_long_transaction_13()")
			exit(0)

def run_long_transaction_og():
	it = 1
	os.system("cd " + mysql_path + "; (sleep 50; bash run_lt_loop_for_chain.sh 1) &");
	for i in range(2, 49):
		os.system("cd " + mysql_path + "; (sleep 50; bash run_lt_loop_11.sh " + str(i) + ") &");
	ret = 0
	if ret !=0:
		print("Error in run_long_transaction()")
		exit(0)

def run_long_transaction_for_chain_vd():
	ret = os.system("cd " + mysql_path + "; (bash run_lt_loop_for_chain_vd.sh 1) &");
	if ret != 0:
		print("Error in run_long_transaction_for_chain_vd()")
		exit(0)

def run_long_transaction_vd():
	for i in range(2, 49):
		os.system("cd " + mysql_path + "; (sleep 50; bash run_lt_loop_11.sh " + str(i) + ") &");
	ret = 0
	if ret !=0:
		print("Error in run_long_transaction()")
		exit(0)

def replace(filename, pattern, replacement):
	f = open(filename)
	s = f.read()
	f.close()
	s = re.sub(pattern, replacement, s)
	f = open(filename, 'w')
	f.write(s)
	f.close()

def pre_cmake_compile():
	# cmake local
	ret = os.system("cd " + mysql_path + "; bash cmake_local.sh > /dev/null;")
	
def pre_compile(cond=None, figure=None, size=None):
	# univ.i
	os.system("cp " + univ_i[0] + ' ' + univ_i[1])
	
	if figure != 11 and figure != 17:
		pattern = r"\#ifndef HYU_CHAIN\s"
		replacement = "#ifdef HYU_CHAIN\n"
		replace(univ_i[1], pattern, replacement)

	if figure != 13:
		pattern = r"\#ifndef HYU_PERF\s"
		replacement = "#ifdef HYU_PERF\n"
		replace(univ_i[1], pattern, replacement)

	if cond == "vanilla":
		pattern = r"\#ifndef HYU\s"
		replacement = "#ifdef HYU\n"
		replace(univ_i[1], pattern, replacement)
	
	# vDriver0seg.h
	os.system("cp " + vDriver_header[0] + ' ' + vDriver_header[1])
	if figure == 17:
		pattern = r"\#define VSEG_SIZE.*\)"
		replacement = "#define VSEG_SIZE (" + size + ")"
		replace(vDriver_header[1], pattern, replacement)

	ret = os.system("cd " + mysql_path +"; bash make_local.sh >/dev/null")
	if ret != 0:
		print("Error in compiling=" + cond)
		exit(0)

	# my.cnf
	os.system("cp " + my_cnf[0] + ' ' + my_cnf[1])
	
	if figure == 3:
		pattern = r"\#innodb_sync_spin_loops"
		replacement = "innodb_sync_spin_loops"
		replace(my_cnf[1], pattern, replacement)

	print("Success to compile mysql code")

def post_compile():
		os.system("cp " + my_cnf[0] + ' ' + my_cnf[1])
		os.system("cp " + univ_i[0] + ' ' + univ_i[1])
		os.system("cp " + vDriver_header[0] + ' ' + vDriver_header[1])
		os.system("cp " + sysbench_script_3[0] + ' ' + sysbench_script_3[1])
		os.system("cp " + sysbench_script_11[0] + ' ' + sysbench_script_11[1])
		os.system("cp " + sysbench_script_13[0] + ' ' + sysbench_script_13[1])
		os.system("cp " + sysbench_script_15[0] + ' ' + sysbench_script_15[1])
		
		return

def sysbench_compile_3(cond=None, zipf=None):
	os.system("cp " + sysbench_script_3[0] + ' ' + sysbench_script_3[1])
	pattern = r"SOCKET=\s*" + r'.*'
	replacement = "SOCKET=\"" + sysbench_socket + "\""
	replace(sysbench_script_3[1], pattern, replacement)
	print("Success to set sysbench parameters")

def sysbench_compile_11(cond=None, zipf=None):
	os.system("cp " + sysbench_script_11[0] + ' ' + sysbench_script_11[1])
	pattern = r"SOCKET=\s*" + r'.*'
	replacement = "SOCKET=\"" + sysbench_socket + "\""
	replace(sysbench_script_11[1], pattern, replacement)
	if cond == "zipfian":
		pattern = r"RAND_EXP=\s*" + r'.*'
		replacement = "RAND_EXP=\""+str(zipf)+"\""
		replace(sysbench_script_11[1], pattern, replacement)
	print("Success to set sysbench parameters")

def sysbench_compile_13(zipf=None):
	os.system("cp " + sysbench_script_13[0] + ' ' + sysbench_script_13[1])
	pattern = r"SOCKET=\s*" + r'.*'
	replacement = "SOCKET=\"" + sysbench_socket + "\""
	replace(sysbench_script_13[1], pattern, replacement)
	pattern = r"RAND_EXP=\s*" + r'.*'
	replacement = "RAND_EXP=\""+str(zipf)+"\""
	replace(sysbench_script_13[1], pattern, replacement)
	
	print("Success to set sysbench parameters")

def sysbench_compile_15(socket=None):
	os.system("cp " + sysbench_script_15[0] + ' ' + sysbench_script_15[1])
	pattern = r"SOCKET=\s*" + r'.*'
	replacement = "SOCKET=\"" + sysbench_socket + "\""
	replace(sysbench_script_15[1], pattern, replacement)
	pattern = r"THREADS=\s*" + r'.*'
	replacement = "THREADS=\"--threads="+str(int(socket) * 24) +"\""
	replace(sysbench_script_15[1], pattern, replacement)

	print("Success to set sysbench parameters")

def copy_logfile(dirname, filename):
	os.system("cd " + logfile_path + "; cp logfile.err " + dirname + '/' + filename)

def convert_raw_data_11(files, output):
	result = []
	for i in range(0,5):
			result.append(str(i) + ",0\n")

	for file_no in range(len(files)):
		f = open(files[file_no], 'r')
		lines = f.readlines()
		for line in lines:
			s_line = line.split(' ')
			if s_line[0] != '[':
				continue
			if (int(s_line[1][0:-1]) >= 276):
				break
			if file_no == len(files) - 1:
				result_line = str(300 + 5 + int(s_line[1][0:-1])) + "," + s_line[6] + "\n"
			else:
				result_line = str(5 + int(s_line[1][0:-1])) + "," + s_line[6] + "\n"
			result.append(result_line)
		# You should carefully put think time.
		# For now, its 10 sec [ default ]
		if file_no == len(files) - 1:
			break
		for i in range(0, 25):
			result_line = str(280 + int(i)) + ",0\n"
			result.append(result_line)

	for i in range(580, 600):
		result.append(str(i) + ",0\n")
	f.close()
	f = open(output, 'w')
	f.writelines(result)
	f.close()

def convert_raw_data_15(files, outputs):
	for file_no in range(len(files)):
		result=[]
		f = open(files[file_no], 'r')
		lines = f.readlines()
		for line in lines:
			s_line = line.split(' ')
			if s_line[0] != '[':
				continue
			result_line = str(int(s_line[1][0:-1])) + "," + s_line[6] + "\n"
			result.append(result_line)
		f.close()
		f = open(outputs[file_no], 'w')
		f.writelines(result)
		f.close()

def trace_undolog(dirname, filename):
	data_filepath = mysql_path + "data" + '/'

	start_time = datetime.datetime.now()

	f = open(dirname + '/' + filename, 'w')
	result = []
	data_size = int(subprocess.check_output(['du','-sb', os.getcwd() + '/' + data_filepath]).split()[0].decode('utf-8'))
	data_size1 = (int(os.path.getsize(os.getcwd() + '/' + data_filepath + "ib_logfile0")) + int(os.path.getsize(os.getcwd() + '/' + data_filepath + "ib_logfile1")))
	data_size -= data_size1
	result.append("0," + str(data_size) + "\n")

	while True:
		time.sleep(0.5)
		time_delta = (datetime.datetime.now() - start_time).total_seconds()
		if time_delta > 601:
			break
		try:
			data_size = int(subprocess.check_output(['du','-sb', os.getcwd() + '/' + data_filepath]).split()[0].decode('utf-8'))
			data_size -= data_size1
			result.append(str(time_delta) + "," + str(data_size) + "\n")
		except:
			continue

	f.writelines(result)
	f.close()

def run_tracer(dirname, filename):
	proc = Process(target=trace_undolog, args=(dirname, filename,))
	proc.start()
	return proc

def test_run_11(zipf):
	# Directory name.
	dirname=os.getcwd()+"/report/11"
	os.system("mkdir -p " + dirname)
	plot_path_11 = plot_path + "11/"
	print("Begin to run test 11")
	
	# pre_cmake_compile()
	print("Mysql w/o vDriver Begin")
	pre_compile("vanilla", 11)
	sysbench_compile_11()
	run_server()

	loop_procedure()
	sysbench_cleanup_prepare(11)
	
	proc = run_tracer(plot_path_11, "temp.mysql_vanilla_size.data")

	# run script will start after 5 sec.
	run_long_transaction_og()
	time.sleep(5)
	sysbench_run(dirname, "og_unif.data", 11)
	
	sysbench_compile_11("zipfian", zipf)
	
	time.sleep(20)
	run_long_transaction_og()
	time.sleep(5)
	sysbench_run(dirname, "og_zipf.data", 11)

	proc.join()
	shutdown_server()
	copy_logfile(dirname, "logfile_og.err")
	
	convert_raw_data_11([dirname + '/' + "og_unif.data", dirname + '/' + "og_zipf.data"], plot_path_11 + "temp.mysql_vanilla_tps.data")
	# You should collect your data from logfile.err.
	print("Mysql w/o vDriver Done")
	print("Mysql w vDriver Begin")
	
	pre_compile(None, 11)
	sysbench_compile_11()
	run_server()

	loop_procedure()
	sysbench_cleanup_prepare(11)
	
	proc = run_tracer(plot_path_11, "temp.mysql_vdriver_size.data")
	run_long_transaction_vd()
	run_long_transaction_for_chain_vd()
	time.sleep(5)
	sysbench_run(dirname, "vd_unif.data", 11)
	
	sysbench_compile_11("zipfian", zipf)
	time.sleep(20)
	run_long_transaction_vd()
	time.sleep(5)
	sysbench_run(dirname, "vd_zipf.data", 11)
	
	proc.join()
	shutdown_server()
	copy_logfile(dirname, "logfile_vd.err")
	
	convert_raw_data_11([dirname + '/' + "vd_unif.data", dirname + '/' + "vd_zipf.data"], plot_path_11 + "temp.mysql_vdriver_tps.data")
	
	# You should collect your data from logfile.err.
	print("Mysql w vDriver test Done")
	convert_version_chain(dirname, plot_path_11)
	make_eps(plot_path_11)
	print("Success to test 11")
	return

def figure_11():
	for i in zipf_exp:
		test_run_11(i)
	return

def test_run_15():
	# Directory name.
	dirname=os.getcwd()+"/report/" + datetime.datetime.now().strftime('%Y.%m.%d__%H-%M') + "_FIG_15"
	plot_path_15 = plot_path + "15/"
	os.system("mkdir -p " + dirname)
	print("Begin to run test 15")
	vanilla_out = "temp.mysql_vanilla_tps_"
	vdriver_out = "temp.mysql_vdriver_tps_"
	# pre_cmake_compile()	
	print("Mysql w/o vDriver Begin")
	pre_compile("vanilla", 15)

	# 1 socket test
	run_server(1)
	loop_procedure()

	sysbench_compile_15(1)
	sysbench_cleanup_prepare(15)
	
	run_long_transaction_socket(1)
	sysbench_run(dirname, "og_1socket.data", 15, 1)
	shutdown_server()
	
	time.sleep(10)
	
	# 2 socket test
	run_server(2)
	loop_procedure()

	sysbench_compile_15(2)
	sysbench_cleanup_prepare(15)
	
	run_long_transaction_socket(2)
	sysbench_run(dirname, "og_2socket.data", 15, 2)
	shutdown_server()
	
	time.sleep(10)

	# 4 socket test
	run_server(4)
	loop_procedure()

	sysbench_compile_15(4)
	sysbench_cleanup_prepare(15)
	
	run_long_transaction_socket(4)
	sysbench_run(dirname, "og_4socket.data", 15, 4)
	shutdown_server()

	convert_raw_data_15([dirname + '/' + "og_1socket.data", dirname + '/' + "og_2socket.data", dirname + '/' + "og_4socket.data"],\
		[plot_path_15 + vanilla_out + "12.data", plot_path_15 + vanilla_out + "24.data", plot_path_15 + vanilla_out + "48.data"])
	print("Mysql w/o vDriver test Done")
	print("Mysql w vDriver Begin")
	
	pre_compile(None, 15)

	# 1 socket test
	run_server(1)
	loop_procedure()


	sysbench_compile_15(1)
	sysbench_cleanup_prepare(15)
	
	run_long_transaction_socket(1)
	sysbench_run(dirname, "vd_1socket.data", 15, 1)
	shutdown_server()
	
	time.sleep(10)
	
	# 2 socket test
	run_server(2)
	loop_procedure()


	sysbench_compile_15(2)
	sysbench_cleanup_prepare(15)
	
	run_long_transaction_socket(2)
	sysbench_run(dirname, "vd_2socket.data", 15, 2)
	shutdown_server()
	
	time.sleep(10)

	# 4 socket test
	run_server(4)
	loop_procedure()

	sysbench_compile_15(4)
	sysbench_cleanup_prepare(15)
	
	run_long_transaction_socket(4)
	sysbench_run(dirname, "vd_4socket.data", 15, 4)
	shutdown_server()

	convert_raw_data_15([dirname + '/' + "vd_1socket.data", dirname + '/' + "vd_2socket.data", dirname + '/' + "vd_4socket.data"],\
		[plot_path_15 + vdriver_out + "12.data", plot_path_15 + vdriver_out + "24.data", plot_path_15 + vdriver_out + "48.data"])

	make_eps(plot_path_15)
	print("Success to test 15")
	return

def figure_15():
	test_run_15()
	return


def convert_raw_data_3(dirname):
	result_dirname = "./plot/03"
	
	# file : report.data
	report_file = dirname+"/report.data"
		
	f = open(report_file, 'r')
	lines = f.readlines()

	result_tps = []
	result_tid = []
	
	result_tps.append("0 0\n")
	result_tid.append("0 0\n")

	trx_id = 0
	lt_id = 0

	for line in lines:
		sp_line = line.split(' ')
		if sp_line[0] != '[':
			continue
		result_tps_line = sp_line[1][0:-1] + " " + sp_line[6] + "\n"
		trx_id = trx_id + float(sp_line[6])
		result_tid_line = sp_line[1][0:-1] + " " + str(trx_id) + "\n"

		result_tps.append(result_tps_line)
		result_tid.append(result_tid_line)

		if int(sp_line[1][0:-1]) >= 20 and lt_id == 0:
			lt_id = trx_id
	
	f.close()
	f = open(result_dirname+"/throughput.data", 'w')
	f.writelines(result_tps)
	f.close()
	
	f = open(result_dirname+"/tid.data", 'w')
	f.writelines(result_tid)
	f.close()

	f = open(result_dirname+"/lt_id.data", 'w')
	f.write("20 80 " + str(lt_id))
	f.close()

	
	# file : vmstat and perf
	result_filenames = os.listdir(dirname)
	result_vmstat = []
	result_vmstat.append(["0", "0", "0", "0"])
	for vfile in result_filenames:
		filename = os.path.join(dirname, vfile)
		if "vmstat" in filename:
			sp_line = filename.split("_")
			time = sp_line[-1][0:-5]

			f = open(filename, 'r')
			lines = f.readlines()
			tmp1 = lines[3].split(' ')
			tmp2 = []
			for t in tmp1:
				if t != '':
					tmp2.append(t)
			f.close()
			result_line = [time, tmp2[-7], tmp2[-6]]

			filename = filename.replace('vmstat', 'perf')
			filename += ".txt"

			f = open(filename, 'r')
			lines = f.readlines()
			for line in lines:
				if "[.] buf_page_get_gen" in line:
					sp_buf = line.split(' ')
					for sp in sp_buf:
						if sp == '':
							continue
						else:
							value = float(sp[:-1])
							value = (float(tmp2[-7]) + float(tmp2[-6])) * value / 100
							result_line = [time, tmp2[-7], tmp2[-6], str(value)]
							break
					break
			f.close()
			result_vmstat.append(result_line)
	
	sorted_res = sorted(result_vmstat, key=lambda x: int(x[0]))
	sorted_res = [" ".join(x)+'\n' for x in sorted_res]
	f = open(result_dirname+"/vmstat.data", 'w')
	f.writelines(sorted_res)
	f.close()


def test_run_3():
	# Directory name.
	dirname=os.getcwd()+"/report/03"
	plot_path_03 = "./plot/03"
	os.system("mkdir -p " + dirname)
	print("Begin to run test 3")
	# pre_cmake_compile()
	pre_compile("vanilla", 3)
	run_server()
	loop_procedure()
	
	time.sleep(10)
	
	sysbench_compile_3()
	sysbench_cleanup_prepare(3)
	
	run_long_transaction_3()
	perf_and_vmstat(mysql_path, dirname)
	
	sysbench_run(dirname, "report.data", 3)
	
	shutdown_server()
	
	perf_to_text(mysql_path, dirname, "jaeseon")
	
	convert_raw_data_3(dirname)
	make_eps(plot_path_03)
	print("Success to test 3")

def figure_3():
	test_run_3()
	return

def convert_raw_data_13(filename, zipf, LLT):
	no_llt_filename = plot_path + "13/no_llt_breakdown.data"
	llt_filename = plot_path + "13/llt_breakdown.data"

	perf_string = "HYU_PERF"

	f = open(filename, 'r')
	lines = f.readlines()
	result = []
	values = []
	values.append(str(zipf))
	value = 0

	for line in lines:
		if perf_string not in line:
			continue
		value = value + float(line.split(' ')[-1])
		if value >= 100:
			value = 100
		values.append(str(value))
	f.close()
	for v in range(len(values) - 1):
		result.append(values[v] + ", ")
	result.append(values[-1] + "\n")
	
	if LLT is True:
		f = open(llt_filename, 'a')
		f.writelines(result)
	else:
		f = open(no_llt_filename, 'a')
		f.writelines(result)
	f.close()
	
def test_run_13():
	# Directory name.
	dirname=os.getcwd()+"/report/13"
	plot_path_13 = "./plot/13"
	os.system("mkdir -p " + dirname)
	print("Begin to run test 13")
	zipf_var = [0.0, 0.4, 0.8, 1.0, 1.1, 1.4]
	# pre_cmake_compile()
	pre_compile("", 13)
	
	for zipf in zipf_var:
		sysbench_compile_13(zipf)
		run_server()
		sysbench_cleanup_prepare(13)
		sysbench_run(dirname, "report.data", 13)
		shutdown_server()
		os.system("cd " + mysql_path + "; cp logfile.err " + dirname + "/no_llt_" + str(zipf))
		convert_raw_data_13(dirname + "/no_llt_" + str(zipf), zipf, False)
	
	for zipf in zipf_var:
		sysbench_compile_13(zipf)
		run_server()
		sysbench_cleanup_prepare(13)
		run_long_transaction_13()
		sysbench_run(dirname, "report.data", 13)
		shutdown_server()
		os.system("cd " + mysql_path + "; cp logfile.err " + dirname + "/llt_" + str(zipf))
		convert_raw_data_13(dirname + "/llt_" + str(zipf), zipf, True)
	make_eps(plot_path_13)
	print("Success to test 13")

def figure_13():
	test_run_13()
	return

def test_run_17(zipf):
	# Directory name.
	dirname=os.getcwd()+"/report/17"
	os.system("mkdir -p " + dirname)
	plot_path_17 = plot_path + "17/"
	print("Begin to run test 17")
	
	print("Mysql w vDriver Begin")

	S_16M="1024*1024*16"
	S_1M="1024*1024*1"
	S_64K="1024*64"
	
	# Segment size : 16 M 	
	pre_compile(None, 17, S_16M)
	sysbench_compile_11()
	run_server()

	loop_procedure()
	sysbench_cleanup_prepare(11)
	
	run_long_transaction_vd()
	run_long_transaction_for_chain_vd()
	time.sleep(5)
	sysbench_run(dirname, "vd_unif.data", 11)
	
	sysbench_compile_11("zipfian", zipf)
	time.sleep(20)
	run_long_transaction_vd()
	time.sleep(5)
	sysbench_run(dirname, "vd_zipf.data", 11)
	time.sleep(20)
	
	shutdown_server()
	copy_logfile(dirname, "logfile_vd.err")
	
	# You should collect your data from logfile.err.
	convert_version_chain_17(dirname, plot_path_17 + "temp.mysql_vdriver_chain_16M.data")

	# Segment size : 1 M
	pre_compile(None, 17, S_1M)
	sysbench_compile_11()
	run_server()

	loop_procedure()
	sysbench_cleanup_prepare(11)
	
	run_long_transaction_vd()
	run_long_transaction_for_chain_vd()
	time.sleep(5)
	sysbench_run(dirname, "vd_unif.data", 11)
	
	sysbench_compile_11("zipfian", zipf)
	time.sleep(20)
	run_long_transaction_vd()
	time.sleep(5)
	sysbench_run(dirname, "vd_zipf.data", 11)
	time.sleep(20)
	
	shutdown_server()
	copy_logfile(dirname, "logfile_vd.err")
	
	# You should collect your data from logfile.err.
	convert_version_chain_17(dirname, plot_path_17 + "temp.mysql_vdriver_chain_1M.data")
	
	# Segment size : 64K
	pre_compile(None, 17, S_64K)
	sysbench_compile_11()
	run_server()

	loop_procedure()
	sysbench_cleanup_prepare(11)
	
	run_long_transaction_vd()
	run_long_transaction_for_chain_vd()
	time.sleep(5)
	sysbench_run(dirname, "vd_unif.data", 11)
	
	sysbench_compile_11("zipfian", zipf)
	time.sleep(20)
	run_long_transaction_vd()
	time.sleep(5)
	sysbench_run(dirname, "vd_zipf.data", 11)
	time.sleep(20)
	
	shutdown_server()
	copy_logfile(dirname, "logfile_vd.err")
	
	# You should collect your data from logfile.err.
	convert_version_chain_17(dirname, plot_path_17 + "temp.mysql_vdriver_chain_64K.data")
	make_eps(plot_path_17)
	print("Success to test 17")
	return

def figure_17():
	for i in zipf_exp:
		test_run_17(i)
	return
