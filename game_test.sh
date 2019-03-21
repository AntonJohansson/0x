#!/bin/bash

n=2

termite -e "./build/0x/0x"&
sleep 0.1
for i in $(seq 1 $n); do
	termite -e "python python_client/client.py coj p hej $n"&
done
for i in $(seq 1 $n); do
	termite -e "python python_client/client.py coj p test $n"&
done
for i in $(seq 1 $n); do
	termite -e "python python_client/client.py coj p snabb $n"&
done
