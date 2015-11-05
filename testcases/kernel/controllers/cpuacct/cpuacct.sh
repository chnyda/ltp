#!/bin/bash

################################################################################
##                                                                            ##
## Copyright (c) 2015 SUSE                                                    ##
##                                                                            ##
## This program is free software;  you can redistribute it and#or modify      ##
## it under the terms of the GNU General Public License as published by       ##
## the Free Software Foundation; either version 2 of the License, or          ##
## (at your option) any later version.                                        ##
##                                                                            ##
## This program is distributed in the hope that it will be useful, but        ##
## WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY ##
## or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   ##
## for more details.                                                          ##
##                                                                            ##
## You should have received a copy of the GNU General Public License          ##
## along with this program;  if not, write to the Free Software               ##
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301   ##
## USA                                                                        ##
##                                                                            ##
## Author: Cedric Hnyda <chnyda@suse.com>                                     ##
##                                                                            ##
################################################################################

# Usage
# ./cpuacct.sh nbsubgroup nbprocess
#
# 1) nbsubgroup : number of subgroup to create
# 2) nbprocess : number of process to attach to each subgroup
#
# Description
#
# 1) Find if cpuacct is mounted, if not mounted, cpuacct will be mounted
# 2) Create a subgroup ltp_test in cpuacct
# 3) Create nbsubgroup subgroup in ltp_test and attach them nbprocess process
# 4) Check that every subgroup's tasks file is not empty (test1)
# 5) Kill all created process
# 6) Check that ltp_test/subgroup*/cpuacct.usage != 0 (test2)
# 7) Check that sum ltp_test/subgroup*/cpuacct.usage = ltp_test/cpuacct.usage
# (test3)
# 8) cleanup


mounted=1
mount_point=""
max=$1
nbprocess=$2

cd $LTPROOT/testcases/bin

cnt=1
for arg; do
	if [ $cnt -gt 1 ]; then
		NAME+="_"
		NAME+=$arg
	fi
	cnt=$(( $cnt + 1 ))
done

PREFIX="cpuacct_"

export TCID=$PREFIX$1$NAME
export TESTROOT=`pwd`
export TST_TOTAL=3;
export TST_COUNT=1;
TMPFILE="$TESTROOT/tmp_tasks"
status=0

verify_result()
{
	result=$1
	expected=$2
	message=$3
	if [ "$result" -ne "$expected" ]; then
		tst_resm TINFO "expected $expected, got $result"
		cleanup;
		tst_brkm TCONF NULL $message
		exit 32
	fi
}

mount_cpuacct()
{
	mount -t cgroup -o cpuacct none $mount_point
	verify_result $? 0 "Error occured while mounting cgroup"
}

umount_cpuacct()
{
	tst_resm TINFO "Umounting cpuacct"
	umount $mount_point
	verify_result $? 0 "Error occured while umounting cgroup"
}

do_mkdir()
{
	path=$1
	mkdir -p $path
	verify_result $? 0 "Error occured with mkdir"
}

do_rmdir()
{
	path=$1
	rmdir $path
	verify_result $? 0 "Error occured with rmdir"
}

setup()
{
	mount_point=`grep -w cpuacct /proc/mounts | cut -f 2 | cut -d " " -f2`
	tst_resm TINFO "cpuacct: $mount_point"
	if [ "$mount_point" = "" ]; then
		mounted=0
		mount_point=/dev/cgroup
	fi

	testpath=$mount_point/ltp_test
	if [ -e $testpath ]; then
		rmdir $testpath/subgroup_*;
		rmdir $testpath;
	fi

	if [ "$mounted" -eq "0" ]; then
		do_mkdir $mount_point
		mount_cpuacct
	fi
	do_mkdir $testpath
	for i in `seq 1 $max`
	do
		do_mkdir $testpath/subgroup_$i
	done

	for i in `seq 1 $max`
	do
		for j in `seq 1 $nbprocess`
		do
			cpuacct_task.sh $testpath/subgroup_$i/tasks &
		done
	done

	sleep 1
}

cleanup()
{
	tst_resm TINFO "removing created directories"
	rmdir $testpath/subgroup_*;
	rmdir $testpath;
	killall -9 cpuacct_task.sh 1> /dev/null 2>&1;
	if [ "$mounted" -ne 1 ] ; then
		umount $mount_point
		do_rmdir $mount_point
	fi
}

##########################  main   #######################

setup;

error=0
tst_resm TINFO "killing created process"
for j in `seq 1 $max`
do
	cat $testpath/subgroup_$j/tasks > $TMPFILE
	nlines=`cat $TMPFILE | wc -l`
	if [ "$nlines" -eq "0" ]; then
		error=1
		status=1
	fi
	for i in `seq 1 $nlines`
	do
		cur_pid=`sed -n "$i""p" $TMPFILE`
		if [ -e /proc/$cur_pid/ ];then
			kill "$cur_pid"
		fi
	done
done

sleep 1

## check that every subgroup has at least one process in tasks file
if [ "$error" -eq "1" ]; then
	tst_resm TFAIL "Some subgroup didn't have any pid in their tasks file"
else
	tst_resm TPASS "subgroups' tasks files are not empty"
fi

acc=0
error=0
for i in `seq 1 $max`
do
    tmp=`cat $testpath/subgroup_$i/cpuacct.usage`
    if [ "$tmp" -eq "0" ]; then
    	error=1
    	status=1
    fi
    ((acc = acc + tmp))
done

## check that cpuacct.usage != 0 for every subgroup
((TST_COUNT = TST_COUNT + 1))
if [ "$error" -eq "1" ]; then
	tst_resm TFAIL "cpuacct.usage should not be equal to 0"
else
	tst_resm TPASS "cpuacct.usage is not equal to 0 for every subgroup"
fi

## check that ltp_subgroup/cpuacct.usage == sum ltp_subgroup/subgroup*/cpuacct.usage
((TST_COUNT = TST_COUNT + 1))
ref=`cat $testpath/cpuacct.usage`
if [ "$ref" != "$acc" ]; then
	tst_resm TFAIL "ltp_test/cpuacct.usage not equal to ltp_test/subgroup*/cpuacct.usage"
	status=1
else
	tst_resm TPASS "ltp_test/cpuacct.usage equal to ltp_test/subgroup*/cpuacct.usage"
fi

cleanup;
exit $status
