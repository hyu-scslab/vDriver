#!/bin/bash

# Change to this-file-exist-path.
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"/
cd $DIR

# Default
BASE_DIR="$DIR""../../../PostgreSQL/"
CONFIGURE=YES
GDB=NO

# Parse parameters.
for i in "$@"
do
case $i in
    -b=*|--base_dir=*)
    BASE_DIR="${i#*=}"
    shift
    ;;

    -c=*|--compile_option=*)
    COMPILE_OPTION="${i#*=}"
    shift
    ;;

    --no_configure)
    CONFIGURE=NO
    shift
    ;;

    --gdb)
    GDB=YES
    shift
    ;;

    *)
          # unknown option
    ;;
esac
done


SOURCE_DIR=$BASE_DIR"postgres/"
TARGET_DIR=$BASE_DIR"pgsql/"

cd $SOURCE_DIR

make clean -j96 --silent

# gdb
if [ "$GDB" == "YES" ]
then
    COMPILE_OPTION+=" -ggdb -O0 -g3 -fno-omit-frame-pointer"
fi

echo $COMPILE_OPTION

# configure
if [ "$CONFIGURE" == "YES" ]
then
    ./configure --silent --prefix=$TARGET_DIR CFLAGS="$COMPILE_OPTION"
fi

# make
make -j96 --silent

# make install
make install -j96 --silent

cd $DIR


