#!/bin/bash
set -e

cd `dirname "$BASH_SOURCE"`


source scripts/lib/core.sh
source scripts/lib/log.sh
source scripts/lib/time.sh
source scripts/lib/command.sh

on_main(){
    core_call_assert time_unix
    local start=$result

    local target="${os}_$arch"
    local exec_cmake=(cmake 
        '../../'
        -DCMAKE_SYSTEM_NAME=Linux
    )
    case "$target" in
        linux_csky)
            exec_cmake+=(
                "-DCMAKE_C_COMPILER=$toolchain/bin/csky-linux-gcc"
                "-DCMAKE_CXX_COMPILER=$toolchain/bin/csky-linux-g++"                
            )
        ;;
        linux_default)
        ;;
        *)
            log_fatal "unknow target: '$target'"
        ;;
    esac
    local dir="dst/$target"
    if [[ $delete == true ]];then
        log_info "delete cache '$dir'"
        if [[ -d "$dir" ]];then
            core_call_assert rm "$dir" -rf
        fi
    fi

    if [[ $cmake == true ]] || [[ $make == true ]];then
        if [[ ! -d "$dir" ]];then
            core_call_assert mkdir "$dir" -p
        fi
        core_call_assert cd "$dir"
        
        if [[ $cmake == true ]];then
            log_info "cmake for $target"
            log_info "${exec_cmake[@]}"
            core_call_assert "${exec_cmake[@]}"
        fi
        if [[ $make == true ]];then
            log_info "make for $target"
            core_call_assert make
        fi
    fi

    core_call_assert time_since "$start"
    local used=$result
    log_info "success, used ${used}s"
}
core_call_assert    command_begin --name "`basename $BASH_SOURCE`" \
    --short 'build tools scripts' \
    --func on_main
root=$result

core_call_assert    command_flags -t string -d 'Build arch' \
    -v arch \
    -V csky -V default \
    -D default
core_call_assert    command_flags -t string -d 'Build os' \
    -v os \
    -V linux \
    -D linux
core_call_assert    command_flags -t string -d 'GCC toolchain path' \
    -v toolchain \
    -D "/usr"

core_call_assert    command_flags -t bool -d 'Delete build' \
    -v delete -s d
core_call_assert    command_flags -t bool -d 'Execute cmake' \
    -v cmake -s c
core_call_assert    command_flags -t bool -d 'Execute make' \
    -v make -s m

core_call_assert    command_commit
core_call_assert    command_execute $root "$@"