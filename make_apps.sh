#!/bin/bash
rm -rf build/ bin/ lib/
mkdir build
cd build
export SOC=j721e
cmake ..
make -j2
