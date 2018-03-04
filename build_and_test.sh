#!/bin/bash
set -e
MODE=DEBUG
BUILD_DIR=Debug
cmake -E make_directory ${BUILD_DIR}
cmake -E chdir ${BUILD_DIR} cmake .. -DCMAKE_BUILD_TYPE=${MODE}
cmake --build ${BUILD_DIR} -- -j2
(cd glperf2; make)
