#!/usr/bin/env bash
set -ex

KLOG=3 mkn clean build run -dtOa "-std=c++20 -fPIC"
KLOG=3 mkn clean build -dtOa "-std=c++20 -fPIC" -p test_module
python3 -c "import test_module as tm; tm.lol()"
