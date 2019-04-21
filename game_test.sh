#!/bin/bash

n=5
s=6

termite -e "./build/0x/0x"&
sleep 0.1

for i in $(seq 1 $n); do
	termite -e "python python_client/client.py coj p hej $s 10 true"&
done
termite -e "python python_client/snake.py coj p hej $s"&

#for i in $(seq 1 $n); do
#	termite -e "python python_client/client.py coj p test $s 10 true"&
#done
#termite -e "python python_client/snake.py coj p test $s"&

#for i in $(seq 1 $n); do
#	termite -e "python python_client/client.py coj p test $n"&
#done
#for i in $(seq 1 $n); do
#	termite -e "python python_client/client.py coj p snabb $n"&
#done
