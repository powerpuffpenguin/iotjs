#!/bin/bash
set -e

cd `dirname "$BASH_SOURCE"`

./build.sh c --arch arm --os linux \
    --build-type Release -cmd \
    "$@"