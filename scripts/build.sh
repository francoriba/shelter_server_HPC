#!/bin/bash -e

echo ""

# Buscar el directorio 'build'
if [ -d "../build" ]; then
    echo "## Found directory 'build' in $(pwd)"
    echo "## Deleting build directory..."
    rm -rf ../build/*   
else
    echo "## Directory 'build' not found in $(pwd)"
    echo "## Creating ../build"
    mkdir ../build
fi

echo "Compiling project with coverage flags..."
#cd ../build && cmake -GNinja ..
cd ../build && cmake -GNinja -DCMAKE_CXX_FLAGS="-O3 -fopenmp" -DCMAKE_BUILD_TYPE=Release ..

echo "Building project..."
ninja
