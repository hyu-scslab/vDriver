#!/bin/sh




for entry in "gnuplot_data/"perf_*.data
do
#	echo "$entry"
	(sudo perf report -s sym --children -i $entry --stdio > $entry".txt" 2> /dev/null ; sudo chown kihwang:kihwang $entry".txt") &
done

wait










