from figure_module import *
import sys, os

def main():
	figure_13()
	
if __name__ == '__main__':
	os.system("mkdir -p report")
	os.system("cd ./plot/13; rm -f *.data;")
	os.system("mkdir -p ./plot/13")
	main()
