#!/bin/bash

#  Copyright (C) 2023 Texas Instruments Incorporated - http://www.ti.com/
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
#    Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the
#    distribution.
#
#    Neither the name of Texas Instruments Incorporated nor the names of
#    its contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Global variables
topdir=/opt/edgeai-tiovx-apps
BRIGHTWHITE='\033[0;37;1m'
RED='\033[0;31m'
GREEN='\033[0;32m'
NOCOLOR='\033[0m'

cpuload=0

debug() {
	if [ ! -z $TEST_ENGINE_DEBUG ]; then
		printf "[DEBUG] $1\n"
	fi
}

usage() {
	cat <<- EOF
		test_engine.sh - Run different tests on the edgeai apps
		Usage:
		  test_engine.sh TSUIT TARGS TIMEOUT PARSE_SCRIPT FILTER
		    TSUIT        : Unique string for each varient of test
		                   Must start with one of PY, CPP No underscores
		    TARGS        : Arguments to be passed to the test scripts
		                   Escape the quotes to pass as single argument
		    TIMEOUT      : No of seconds to run the test before killing it
		                   Useful for forever running camera tests
		    PARSE_SCRIPT : Script to handle the test logs and decide the
		                   validation criteria for the test run
		    FILTER       : Optionally filter out models like below:
		                   "grep CL" to run only classification models
		                   "grep TVM" to run only those models with TVM runtime
		                   "grep TFL-CL-007" to run only specific model
	EOF
}

# Run a single command with a timeout, save the stdout/stderr
# Returns the status of the command
run_single_test() {
	test_name=$1
	test_dir=$2
	timeout=$3
	parse_script=$4
	test_command=$5

	# Setup log files for stout and stderr for each test run
	stdout="$topdir/logs/"$test_name"_stdout.log"
	stderr="$topdir/logs/"$test_name"_stderr.log"
	echo -e "$command\n\n" > $stdout

	echo
	printf "[TEST] START $test_name\n"

	cd $test_dir
	echo -e "$test_command\n\n" > $stdout
	debug "Running $BRIGHTWHITE$test_command$NOCOLOR"

    # Run the app with the timeout and force kill the test 4 after seconds
	timeout -s INT -k $(($timeout + 4)) $timeout $test_command >> $stdout 2> $stderr
	test_status=$?

	if [ "$test_status" -ne "0" ] && [ "$test_status" -ne "124" ]; then
		# Test failed while running
		printf "[TEST]$RED FAIL_RUN$NOCOLOR $test_name (retval $test_status)\n"
		printf "        Test Log saved @ $RED$stdout$NOCOLOR\n"
		return $test_status
	fi

	if [ "$parse_script" != "null" ]; then
		cd $topdir/tests/
		parse_command="$parse_script $stdout"
		debug "Running $BRIGHTWHITE$parse_command$NOCOLOR"

		$parse_command
		parse_status=$?

		if [ "$parse_status" -ne "0" ]; then
			printf "[TEST]$RED FAIL_PARSE$NOCOLOR $test_name (retval $parse_status)\n"
			printf "        Test Log saved @ $RED$stdout$NOCOLOR\n"
			printf "        Parse command $parse_command\n"
			return $parse_status
		fi
	fi

	printf "[TEST]$GREEN PASS$NOCOLOR $test_name\n"
	return 0
}

################################################################################
# Main script starts from here

if [ "$#" -lt "3" ] || [ "$#" -gt "6" ]; then
	echo "ERROR: Invalid arguments"
	usage
	exit 1
fi

test_suite=$1
config_file=$2
timeout=$3
parse_script=${4:-"null"}
test_filter=${5:-"null"}
modelname=${6:-"null"}

# Create a directory to store logs
mkdir -p $topdir/logs
cd $topdir/

if [ "$modelname" == "null" ]; then
	# Find out a list of models for which to run the test
	searchcmd="find ../model_zoo -maxdepth 1 -mindepth 1 -type d"
	if [ "$test_filter" != "null" ]; then
		searchcmd="$searchcmd | $test_filter"
	fi
	searchcmd="$searchcmd | sort"
else
	searchcmd="find ../model_zoo/$modelname -maxdepth 0 -mindepth 0 -type d"
	if [ -f "$searchcmd" ]; then
		echo "ERROR: $searchcmd does not exist."
		exit 1
	fi
fi

# Iterate over the list of filtered models
for model_path in $(eval $searchcmd); do
	model=`echo $model_path | cut -d'/' -f3`
	model_type=`echo $model | cut -d'-' -f2`
	case $model_type in
		"CL")
			model_type="classification"
			app_tag="classify"
			;;
		"OD")
			model_type="detection"
			app_tag="objdet"
			;;
		"SS")
			model_type="segmentation"
			app_tag="semseg"
			;;                                           
		*)
			continue
			;;
	esac

	test_app="./bin/Release/app_edgeai_tiovx"
    test_dir="$topdir"

	# Update the config file with correct model path if modelname isnt specified
	if [ "$modelname" == "null" ]; then
		sed -i "s@model_path:.*@model_path: $topdir/$model_path@" $config_file
	fi

	# Generate a unique test name for each test_suite / model
	# This way, logs will not be overwritten
	tname="$test_suite"_"$model"
    command="$test_app $config_file"

	# This will run the test and save the logs
	run_single_test $tname $test_dir $timeout $parse_script "$command"

done
