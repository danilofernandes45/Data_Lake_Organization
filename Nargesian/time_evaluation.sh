#!/bin/bash
function eval() {
    for i in $(seq 1 50)
    do
        ./local_search --gamma 25 --time $3 --target $4 --Kp 79 --eps 0.019 -i ../Data/DataLakes/Socrata/$1/topic_vectors-$1-$2.txt
    done
}

# eval 100 3 300 0.3615 > data_tttplot-100-3.txt
eval 100 5 300 0.4576 > data_tttplot-100-5.txt
eval 300 6 300 0.1633 > data_tttplot-300-6.txt
eval 500 6 300 0.1089 > data_tttplot-500-6.txt