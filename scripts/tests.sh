#!/bin/bash -e

# echo ""

# # Buscar el directorio 'build'
# if [ -d "../build" ]; then
#     echo "## Found directory 'build' in $(pwd)"
#     echo "## Deleting build directory..."
#     rm -rf ../build/*   
# else
#     echo "## Directory 'build' not found in $(pwd)"
#     echo "## Creating ../build"
#     mkdir ../build
# fi

# echo "Compiling project with coverage flags..."
#cd ../build && cmake -GNinja -DRUN_TESTS=1 .. 
cd ../build && cmake -GNinja -DRUN_COVERAGE=1 ..

echo "Building project..."
ninja

echo ""
echo "  -> Running tests..."
echo ""
ctest --test-dir tests -VV

echo ""
echo "  -> Running coverage..."
echo ""
cd .. && gcovr -r "$PWD"
