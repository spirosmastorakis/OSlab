#!/bin/bash

for i in {1..100}
do
	echo $i:
	./test_crypto /dev/cryptodev0
done
