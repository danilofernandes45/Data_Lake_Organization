#!/bin/bash
function eval() {
	for i in $(seq 1 15)
	do
		./local_search --target 1 --gamma 25 --time $2 --Kp 79 --eps 0.019 -i ../Data/DataLakes/Socrata/$1/topic_vectors-$1-$3.txt
	done
}

for i in $(seq 1 2)
do
	# eval $1 $2 $i > ../Data/Performance/Socrata/Nargesian/$1/perform-$1-$i-2.csv
	eval $1 $2 $i > ../Data/Performance/Socrata/ILS/$1/perform-$1-$i-2.csv
done
