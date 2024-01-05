#!/bin/sh
for i in $(seq 1 100)
do
	echo write num $i start.
	./wr.sh
	echo write num $i end.
	echo -e "\n"

	echo read num $i start.
	./rd.sh
	echo read num $i end.
	echo -e "\n"

done

