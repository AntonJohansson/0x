#!/bin/bash

n=5

termite -e "./build/0x/0x"&
sleep 0.1
for i in $(seq 1 $n); do
	echo $i
	termite -e "python python_client/client.py coj p hej $n"&
done
