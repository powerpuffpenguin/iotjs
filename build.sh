#!/bin/bash
set -e

cd `dirname "$BASH_SOURCE"`
rootDir=`pwd`

source scripts/lib/core.sh
source scripts/lib/log.sh
source scripts/lib/time.sh
source scripts/lib/command.sh

source "scripts/js.sh"
js=$result
source "scripts/c.sh"
c=$result

command_begin --name "`basename $BASH_SOURCE`" \
    --short 'build tools scripts' 
root=$result

command_children "$js" "$c"
command_commit
command_execute $root "$@"