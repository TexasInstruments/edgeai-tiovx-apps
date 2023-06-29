#!/bin/bash
rm -rf build/ bin/ lib/
mkdir build
cd build
cmake ..
make -j2
