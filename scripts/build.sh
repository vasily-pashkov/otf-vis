#!/bin/bash

export LD_LIBRARY_PATH=./build-Release/lib:./build-Release/src
#./build-Release/exe/proposer
./build-Release/exe/acceptor
#./build-Release/exe/client
