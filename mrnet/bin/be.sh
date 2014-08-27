#!/bin/sh
PARENT_PORT=$1
PARENT_RANK=$2
BE_RANK=$3
APP_PARAM=$4


REM_SHELL=${XPLAT_RSH:-"ssh"}
echo $REM_SHELL

echo "init BE process... [PARENT_PORT: $PARENT_PORT] [PARENT_RANK: $PARENT_RANK] [BE_RANK: $BE_RANK] [PARAM: $APP_PARAM]  "

export LD_LIBRARY_PATH=/usr/local/lib:/home/usw/Install/sight/sight/sight/mrnet/bin

unset SIGHT_FILE_OUT
export MRNET_MERGE_EXEC="./smrnet_be 127.0.0.1 $PARENT_PORT $PARENT_RANK 127.0.0.1 $BE_RANK";
#export MRNET_MERGE_EXEC="./smrnet_be 0.0.0.0 $PARENT_PORT $PARENT_RANK 0.0.0.0 $BE_RANK";
#export MRNET_MERGE_EXEC="smrnet_be 0.0.0.0 $PARENT_PORT $PARENT_RANK 0.0.0.0 $BE_RANK";

#execute sight programme

