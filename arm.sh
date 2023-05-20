#!/bin/bash
set -e

cd `dirname "$BASH_SOURCE"`

./build.sh --arch arm --os linux \
    --build-type Release -cdm \
    "$@"