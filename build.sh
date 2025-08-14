#!/bin/bash
# Quick build script for SeaJay

BUILD_TYPE=${1:-Release}
mkdir -p build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
ninja
echo "Build complete! Binary location: /workspace/bin/seajay"
