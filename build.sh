#!/bin/bash
set -e

cd `dirname "$BASH_SOURCE"`
rootDir=`pwd`

source scripts/lib/core.sh
source scripts/lib/log.sh
source scripts/lib/time.sh
source scripts/lib/command.sh

build_libevent(){
    if [[ $cmake == true ]] || [[ $make == true ]];then
        cd "$rootDir/$dir"
        if [[ ! -d libevent ]];then
            mkdir libevent
        fi
        cd libevent
        
        if [[ $cmake == true ]];then
            log_info "cmake libevent for $target"
            local args=(cmake ../../../third_party/libevent-2.1.12-stable
                -DCMAKE_BUILD_TYPE=Release
                -DEVENT__DISABLE_TESTS=ON
                -DEVENT__DISABLE_OPENSSL=ON
                -DEVENT__LIBRARY_TYPE=STATIC                
            )
            args+=("${cmake_args[@]}")
            log_info "${args[@]}"
            "${args[@]}"
        fi
        if [[ $make == true ]];then
            log_info "make libevent for $target"
            make
        fi
    fi
}
build_iotjs(){
    if [[ $cmake == true ]] || [[ $make == true ]];then
        cd "$rootDir/$dir"
        if [[ ! -d iotjs ]];then
            mkdir iotjs
        fi
        cd iotjs
        
        if [[ $cmake == true ]];then
            log_info "cmake iotjs for $target"
            local args=(cmake ../../../
                -DCMAKE_BUILD_TYPE=$build_type
            )
            args+=("${cmake_args[@]}")
            log_info "${args[@]}"
            "${args[@]}"
        fi
        if [[ $make == true ]];then
            log_info "make iotjs for $target"
            make
        fi
    fi
    if [[ $test == true ]];then
        log_info "test for $target"
        cd "$rootDir"
        "$dir/bin/iotjs_test"
    fi
}
on_main(){
    core_call_assert time_unix
    local start=$result

    local target="${os}_$arch"
    local cmake_args=(
        -DCMAKE_SYSTEM_NAME=Linux
        "-DOUTPUT_ROOT_DIR=dst/$target"
    )
    case "$target" in
        linux_csky)
            cmake_args+=(
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
            rm "$dir" -rf
        fi
    fi
    if [[ $cmake == true ]] || [[ $make == true ]];then
        if [[ ! -d "$dir" ]];then
            mkdir "$dir" -p
        fi
    fi
    build_libevent
    build_iotjs

    core_call_assert time_since "$start"
    local used=$result
    log_info "success, used ${used}s"
}
command_begin --name "`basename $BASH_SOURCE`" \
    --short 'build tools scripts' \
    --func on_main
root=$result

command_flags -t string -d 'Build arch' \
    -v arch \
    -V csky -V default \
    -D default
command_flags -t string -d 'Build os' \
    -v os \
    -V linux \
    -D linux
command_flags -t string -d 'GCC toolchain path' \
    -v toolchain \
    -D "/usr"

command_flags -t bool -d 'Delete build' \
    -v delete -s d
command_flags -t bool -d 'Execute cmake' \
    -v cmake -s c
command_flags -t bool -d 'Execute make' \
    -v make -s m
command_flags -t bool -d 'Run test' \
    -v test -s t

command_flags -t string -d 'set CMAKE_BUILD_TYPE' \
    -v build_type -l build-type \
    -V None -V Debug -V Release -V RelWithDebInfo -V MinSizeRel \
    -D Release

command_commit
command_execute $root "$@"