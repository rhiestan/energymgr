#!/usr/bin/bash
#

# Get the absolute directory of the script path
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $SCRIPT_DIR

mkdir build
cd build


cmake -Wno-dev -G Ninja -DCMAKE_BUILD_TYPE="Release" -DQt6_DIR=$HOME/qt/install/lib/cmake/Qt6 ../src/

cmake --build . --target all
