#!/bin/bash

i=0
declare -a arr=("10" "20" "50" "100" "200" "500")
for j in "${arr[@]}";
do
	for i in {1..10} 
	do	
		/usr/bin/time -p ./waf --run "src/clock/examples/computation --nodes="$j""  >/home/aguirre/Desktop/computation/Notclock"$i"-"$j" 2>&1
		/usr/bin/time -p ./waf --run "src/clock/examples/computation-clock --nodes="$j""  >/home/aguirre/Desktop/computation/Clock"$i"-"$j" 2>&1
		/usr/bin/time -p ./waf --run "src/clock/examples/computation-clock-update --nodes="$j""  >/home/aguirre/Desktop/computation/ReScheduleClock"$i"-"$j" 2>&1		
		i=$(( i + 1 ))
	done
done

exit 0


