from datetime import datetime
import math

def convert_version_chain(dirname, outdir):
	vanilla_filename = dirname + '/' + "logfile_og.err"
	vdriver_filename = dirname + '/' + "logfile_vd.err"
	
	vanilla_out_file = outdir + '/' + "temp.mysql_vanilla_chain.data"
	vdriver_out_file = outdir + '/' + "temp.mysql_vdriver_chain.data"

	version_string = "HYU_CHAIN"
	# VANILLA VERSION CHAIN
	f_og = open(vanilla_filename, 'r')
	lines_og = f_og.readlines()
	f_og.close()

	result_og = []
	for line in lines_og:
		if version_string not in line:
			continue
		time = line.split(' ')[0].split('T')[-1].split('.')[0]
		start_time = datetime.strptime(time, "%H:%M:%S")
		break

	diff = -1
	for line in lines_og:
		if version_string not in line:
			continue
		time = line.split(' ')[0].split('T')[-1].split('.')[0]
		curr_time = datetime.strptime(time, "%H:%M:%S")
		delta_time = curr_time - start_time
		result_line = str(delta_time.seconds + 50) + ','
		if int(delta_time.seconds) == diff:
			continue
		else:
			diff = int(delta_time.seconds)
		
		if int(line.split(' ')[-1]) == 0:
			result_line = result_line + "0\n"
		else:
			result_line = result_line + str(math.log10(int(line.split(' ')[-1]))) + '\n'
		result_og.append(result_line)
		
		if diff == 200:
			result_og.append("250,0\n")

	result_og.append("550,0\n")
	f_og = open(vanilla_out_file, 'w')
	f_og.writelines(result_og)
	f_og.close()
	# VDRIVER VERSION CHAIN
	f_vd = open(vdriver_filename, 'r')
	lines_vd = f_vd.readlines()
	f_vd.close()

	result_vd = []
	for line in lines_vd:
		if version_string not in line:
			continue
		time = line.split(' ')[0].split('T')[-1].split('.')[0]
		start_time = datetime.strptime(time, "%H:%M:%S")
		break

	diff = -1
	for line in lines_vd:
		if version_string not in line:
			continue

		time = line.split(' ')[0].split('T')[-1].split('.')[0]
		curr_time = datetime.strptime(time, "%H:%M:%S")
		delta_time = curr_time - start_time
		result_line = str(delta_time.seconds) + ','
		if int(delta_time.seconds) == diff:
			continue
		else:
			diff = int(delta_time.seconds)

		if int(line.split(' ')[-1]) == 0:
			result_line = result_line + "0\n"
		else:
			result_line = result_line + str(math.log10(int(line.split(' ')[-1]))) + '\n'
		result_vd.append(result_line)
		
	f_vd = open(vdriver_out_file, 'w')
	f_vd.writelines(result_vd)
	f_vd.close()



def convert_version_chain_17(dirname, out_file):
	vdriver_filename = dirname + '/' + "logfile_vd.err"

	version_string = "HYU_CHAIN"

	# VDRIVER VERSION CHAIN
	f_vd = open(vdriver_filename, 'r')
	lines_vd = f_vd.readlines()
	f_vd.close()

	result_vd = []
	for line in lines_vd:
		if version_string not in line:
			continue
		time = line.split(' ')[0].split('T')[-1].split('.')[0]
		start_time = datetime.strptime(time, "%H:%M:%S")
		break

	diff = -1
	for line in lines_vd:
		if version_string not in line:
			continue

		time = line.split(' ')[0].split('T')[-1].split('.')[0]
		curr_time = datetime.strptime(time, "%H:%M:%S")
		delta_time = curr_time - start_time
		result_line = str(delta_time.seconds) + ','
		if int(delta_time.seconds) == diff:
			continue
		else:
			diff = int(delta_time.seconds)

		if int(line.split(' ')[-1]) == 0:
			result_line = result_line + "0\n"
		else:
			result_line = result_line + str(math.log10(int(line.split(' ')[-1]))) + '\n'
		result_vd.append(result_line)
		
	f_vd = open(out_file, 'w')
	f_vd.writelines(result_vd)
	f_vd.close()
