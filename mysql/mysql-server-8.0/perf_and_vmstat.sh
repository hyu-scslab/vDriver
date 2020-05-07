#!/bin/bash

#perf
for ((i=3;i<99;i+=3)); do
	(sleep $(expr $i - 1) ; sudo timeout 2 perf record -g -a --output="$1"/plot_03_perf_$i.data"") &
done
#vmstat
for ((i=3;i<99;i+=3)); do
	(sleep $i ; vmstat -t 1 2 > "$1"/plot_03_vmstat_$i.data"") &
done

