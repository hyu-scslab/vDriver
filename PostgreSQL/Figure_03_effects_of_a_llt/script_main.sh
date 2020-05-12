#!/bin/bash



DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"/

cd $DIR


echo "This script use the perf, which has to be run with superuser permission."
echo "Please check whether the perf is installed on this machine before execute this script."

sudo bash ${DIR}_script_main.sh

