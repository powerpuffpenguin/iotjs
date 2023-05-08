#!/bin/bash
set -e

cd `dirname "$BASH_SOURCE"`

./build.sh --arch csky --os linux --toolchain /home/king/c/csky-linux "$@"