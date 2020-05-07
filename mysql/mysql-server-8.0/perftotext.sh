#!/bin/sh

for entry in $1"/"plot_03_perf_*.data
do
	echo "$entry"
	(sudo perf report -s sym --children -i $entry --stdio > $entry".txt" ; sudo chown $2:$2 $entry".txt") &
done

wait
