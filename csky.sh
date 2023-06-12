#!/bin/bash
set -e

cd `dirname "$BASH_SOURCE"`

./build.sh c --arch csky --os linux \
    --toolchain /home/king/c/csky-linux \
    --build-type Release -cmd \
    "$@"