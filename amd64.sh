#!/bin/bash
set -e

cd `dirname "$BASH_SOURCE"`

./build.sh c --arch amd64 --os linux \
    --build-type Release -cmd \
    "$@"