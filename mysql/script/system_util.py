import os

def perf_and_vmstat(basedir, dirname):
	os.system("cd " + basedir + "; bash perf_and_vmstat.sh " + dirname + ">/dev/null")

def perf_to_text(basedir, dirname, username):
	os.system("cd " + basedir + "; bash perftotext.sh " + dirname + " " + username + ">/dev/null")

def make_eps(dirname):
	os.system("cd " + dirname + "; bash plot.sh")
