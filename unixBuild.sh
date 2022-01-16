#!/bin/bash
# NOTE: Should only be executed from project root directory.

mkdir -p build # Make a build directory if one doesn't already exist.
cd build
cmake -S ../ -B . 
make && make Shaders && ./VeEngine
cd ..