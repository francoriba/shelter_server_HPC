#!/bin/bash

# Define the path to clang-format and the root directory of your project
CLANG_FORMAT=clang-format
PROJECT_ROOT=$(dirname $(dirname "$(realpath $0)"))

# Define the directories containing your source and header files
DIRECTORIES=(
    "$PROJECT_ROOT/include/*"
    "$PROJECT_ROOT/src/server/*"
    "$PROJECT_ROOT/src/clients/*"
    "$PROJECT_ROOT/lib/alertInfection/src/*"
    "$PROJECT_ROOT/lib/alertInfection/include/*"
    "$PROJECT_ROOT/lib/emergNotif/src/*"
    "$PROJECT_ROOT/lib/emergNotif/include/*"
    "$PROJECT_ROOT/lib/suppliesData/src/*"
    "$PROJECT_ROOT/lib/suppliesData/include/*"
    "$PROJECT_ROOT/tests/unit/*"
    "$PROJECT_ROOT/tests/e2e/*"
)

# Loop through directories and format files using clang-format
for DIR in "${DIRECTORIES[@]}"
do
    # Use the clang-format command with the -style=file option to use the .clang-format file
    $CLANG_FORMAT -style=file -i $DIR
done
