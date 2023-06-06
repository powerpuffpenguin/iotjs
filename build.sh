#!/bin/bash
set -e

cd `dirname "$BASH_SOURCE"`
rootDir=`pwd`

wolfssl_version=wolfssl-5.6.0-stable

source scripts/lib/core.sh
source scripts/lib/log.sh
source scripts/lib/time.sh
source scripts/lib/command.sh
build_xxd(){
    cd "$rootDir/src/iotjs"
    local dirs=("core")
    local dir
    local file
    local output
    local once
    for dir in "${dirs[@]}";do
        output="$dir/xxd.h"
        once=${dir//\//_}
        echo "#ifndef IOTJS_${once}_XXD_H" > "$output"
        echo "#define IOTJS_${once}_XXD_H" >> "$output"
        local ifs=$IFS
        IFS=$'\n'
        files=(`find "$dir/" -iname *.js`)
        IFS=$ifs
        # local min
        for file in "${files[@]}";do
            # min="${file%js}min.js"
            # webpack "./$file" -o "./$min"
            xxd -i "$file" >> "$output"
        done
        echo "#endif // IOTJS_${once}_XXD_H" >> "$output"
    done
}
build_min_xdd(){
    cd "$rootDir/src"
    
    local ifs=$IFS
    IFS=$'\n'
    files=(`find "$1/" -iname "*.min.js"`)
    IFS=$ifs
    local file
    for file in "${files[@]}";do
        log_info "xdd -i $file"
        local output=${file%.min.js}.h
        local once=${output//\//_}
        once=${once//./_}
        once=${once//-/_}
        echo "#ifndef IOTJS_${once}_XXD_H" > "$output"
        echo "#define IOTJS_${once}_XXD_H" >> "$output"
       
        xxd -i "$file" >> "$output"
        echo "#endif // IOTJS_${once}_XXD_H" >> "$output"
    done
}
build_min_js(){
    local ifs=$IFS
    IFS=$'\n'
    local files=(`find "$1/" -iname *.js`)
    IFS=$ifs
    local dst
    local src
    local cut
    for src in "${files[@]}";do
        if [[ $src == *.min.js ]] || [[ $src == *.cut.js ]];then
            continue
        fi
        dst=${src%.js}.min.js
        log_info "google-closure-compiler --js '$src' --js_output_file '$dst'"
        local name=`basename $src`
        if [[ $name == tsc.* ]] ;then
            cut=${src%.js}.cut.js
            touch "$cut"
            local line
            local ok=0
            while read -r line || [[ -n $line ]]; do
                if [[ "$ok" == 0 ]] && [[ "$line" == Object.defineProperty* ]];then
                    ok=1
                    echo "(function(){
\"use strict\";
return (function(exports,_iotjs,deps){
var __values=_iotjs.__values;
var __extends=_iotjs.__extends;
var __awaiter=_iotjs.__awaiter;
var __generator=_iotjs.__generator;
var __read=_iotjs.__read;
var __spreadArray=_iotjs.__spreadArray;
" > "$cut"
                fi
                echo "$line" >> "$cut"
            done < "$src"
            if [[ $ok == 1 ]];then
                echo "return exports;});})();" >> "$cut"
            fi
            google-closure-compiler --language_in ECMASCRIPT5 --language_out ECMASCRIPT5 --js "$cut" --js_output_file "$dst"
        else
            google-closure-compiler --language_in ECMASCRIPT5 --language_out ECMASCRIPT5 --js "$src" --js_output_file "$dst"
        fi
    done
}
build_js(){
    if [[ $js == false ]];then
        return
    fi
    cd "$rootDir/src"
    log_info "build js"
    tsc

    local nodes=(
        'iotjs/core'
        'iotjs/modules'
    )
    local node
    for node in "${nodes[@]}";do
        build_min_js "$node"
    done
    for node in "${nodes[@]}";do
        build_min_xdd "$node"
    done
}
build_wolfssl(){
    if [[ $cmake == true ]] || [[ $make == true ]];then
        cd "$rootDir/$dir"
        if [[ ! -d wolfssl ]];then
            log_info "cp $wolfssl_version"
            cp "$rootDir/third_party/$wolfssl_version" ./wolfssl -r
        fi
        cd wolfssl
        if [[ ! -f configure ]];then
            log_info "autogen.sh $wolfssl_version"
            ./autogen.sh
        fi
        if [[ ! -f Makefile ]];then
            log_info "configure $wolfssl_version"
            ./configure --host=$wolfssl_host    \
                --enable-benchmark=no \
                --enable-selftest=no \
                --enable-crypttests=no \
                --enable-memtest=no \
                --enable-examples=no \
                --enable-debug=no \
                --enable-opensslall \
                --enable-opensslextra \
                --enable-secure-renegotiation \
                --enable-sessioncerts \
                --enable-sni \
                --enable-static=yes \
                --enable-shared=no 
        fi
        if [[ $make == true ]];then
            if [[ $third_party == true ]] || [[ ! -f src/.libs/libwolfssl.a ]];then
                log_info "make wolfssl for $target"
                make
            fi
        fi
        return 0
        if [[ ! -d wolfssl ]];then
            mkdir wolfssl
        fi
        cd wolfssl
        if [[ $cmake == true ]];then
            if [[ $third_party == true ]] || [[ ! -f Makefile ]];then
                log_info "cmake wolfssl for $target"
                local args=(cmake "$rootDir/third_party/$wolfssl_version"
                    -DWOLFSSL_EXAMPLES=no
                    -DWOLFSSL_OPENSSLEXTRA=ON
                    -DWOLFSSL_OPENSSLALL=ON
                )
                args+=("${cmake_args[@]}")
                log_info "${args[@]}"
                "${args[@]}"
            fi
        fi
        if [[ $make == true ]];then
            if [[ $third_party == true ]] || [[ ! -f libwolfssl.a ]];then
                log_info "make wolfssl for $target"
                make
            fi
        fi

        if [[ ! -d include ]];then
            mkdir include
        fi
        if [[ ! -d "include/wolfssl" ]];then
            cp "$rootDir/third_party/$wolfssl_version/wolfssl" include/ -r
        fi
    fi
}
build_libevent(){
    if [[ $cmake == true ]] || [[ $make == true ]];then
        cd "$rootDir/$dir"
        if [[ ! -d libevent ]];then
            mkdir libevent
        fi
        cd libevent

        if [[ $cmake == true ]];then
            if [[ $third_party == true ]] || [[ ! -f Makefile ]];then
                log_info "cmake libevent for $target"
                echo "$rootDir/$dir/wolfssl/src/.libs/libwolfssl.a"
                local DISABLE_OPENSSL=ON
                DISABLE_OPENSSL=OFF
                local args=(cmake ../../../third_party/libevent-2.1.12-stable
                    -DCMAKE_BUILD_TYPE=Release
                    -DEVENT__DISABLE_SAMPLES=ON
                    -DEVENT__DISABLE_TESTS=ON
                    "-DEVENT__DISABLE_OPENSSL=$DISABLE_OPENSSL"
                    "-DOPENSSL_INCLUDE_DIR=$rootDir/$dir/wolfssl"
                    "-DOPENSSL_LIBRARIES=$rootDir/$dir/wolfssl/src/.libs/libwolfssl.a"
                    -DEVENT__LIBRARY_TYPE=STATIC
                )
                # local args=(cmake ../../../third_party/libevent-2.1.12-stable
                #     -DCMAKE_BUILD_TYPE=Release
                #     -DEVENT__DISABLE_SAMPLES=ON
                #     -DEVENT__DISABLE_TESTS=ON
                #     "-DEVENT__DISABLE_OPENSSL=$DISABLE_OPENSSL"
                #     "-DOPENSSL_INCLUDE_DIR=$rootDir/$dir/wolfssl/include"
                #     "-DOPENSSL_LIBRARIES=$rootDir/$dir/wolfssl/libwolfssl.a"
                #     -DEVENT__LIBRARY_TYPE=STATIC
                # )
                args+=("${cmake_args[@]}")
                log_info "${args[@]}"
                "${args[@]}"
            fi
        fi
        if [[ $make == true ]];then
            if [[ $third_party == true ]] || [[ ! -f lib/libevent_core.a ]];then
                log_info "make libevent for $target"
                make
            fi
        fi
    fi
}
build_libtomcrypt(){
    if [[ $cmake == true ]] || [[ $make == true ]];then
        cd "$rootDir/$dir"
        if [[ ! -d libtomcrypt ]];then
            cp "$rootDir/third_party/libtomcrypt-1.18.2" "$rootDir/$dir/libtomcrypt" -r
        fi
        if [[ $make == true ]];then
            cd libtomcrypt
            log_info "make libtomcrypt for $target"
            make CFLAGS="-DLTC_NO_TEST" 
        fi
    fi
}
build_iotjs(){
    if [[ $cmake == true ]] || [[ $make == true ]];then
        # if [[ $cmake == true ]];then
        #     build_xxd
        # fi


        cd "$rootDir/$dir"
        if [[ ! -d iotjs ]];then
            mkdir iotjs
        fi
        cd iotjs
        
        if [[ $cmake == true ]];then
            log_info "cmake iotjs for $target"
            local args=(cmake ../../../
                -DCMAKE_BUILD_TYPE=$build_type
                -DVM_IOTJS_OS=$os
                -DVM_IOTJS_ARCH=$iotjs_arch
                "-DOUTPUT_ROOT_DIR=dst/$target"
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
    local iotjs_arch=$arch
    if [[ $iotjs_arch == "default" ]];then
        iotjs_arch=amd64
    fi
    local cmake_args=(
        -DCMAKE_SYSTEM_NAME=Linux
    )
    local wolfssl_host
    case "$target" in
        linux_csky)
            export CC="$toolchain/bin/csky-linux-gcc"
            cmake_args+=(
                "-DLINK_STATIC_GLIC=ON"
                "-DCMAKE_C_COMPILER=$toolchain/bin/csky-linux-gcc"
                "-DCMAKE_CXX_COMPILER=$toolchain/bin/csky-linux-g++"
            )
            wolfssl_host=i386-linux
        ;;
        linux_arm)
            export CC="$toolchain/bin/arm-linux-gnueabihf-gcc"
            wolfssl_host=arm-linux
            cmake_args+=(
                "-DCMAKE_C_COMPILER=$toolchain/bin/arm-linux-gnueabihf-gcc"
                "-DCMAKE_CXX_COMPILER=$toolchain/bin/arm-linux-gnueabihf-g++"
            )
        ;;
        linux_amd64)
            wolfssl_host=x86_64-linux
            export CC="$toolchain/bin/gcc"
            cmake_args+=(
                "-DCMAKE_C_COMPILER=$toolchain/bin/gcc"
                "-DCMAKE_CXX_COMPILER=$toolchain/bin/g++"
            )
        ;;
        *)
            log_fatal "unknow target: '$target'"
        ;;
    esac
    local dir="dst/$target"
    if [[ $delete == true ]];then
        if [[ -d "$dir/iotjs" ]];then
            log_info "delete cache '$dir/iotjs'"
            rm "$dir/iotjs" -rf
        fi
        if [[ -d "$dir/libevent" ]] && [[ $third_party == true ]];then
            log_info "delete cache '$dir/libevent'"
            rm "$dir/libevent" -rf
        fi
    fi
    if [[ $cmake == true ]] || [[ $make == true ]];then
        if [[ ! -d "$dir" ]];then
            mkdir "$dir" -p
        fi
    fi
    build_js
    build_wolfssl
    build_libevent
    build_libtomcrypt
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
    -V amd64 -V arm -V csky \
    -D amd64
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
command_flags -t bool -d 'Build third party' \
    -v third_party -l third-party
command_flags -t bool -d 'Build js code' \
    -v js -s j

command_flags -t string -d 'set CMAKE_BUILD_TYPE' \
    -v build_type -l build-type \
    -V None -V Debug -V Release -V RelWithDebInfo -V MinSizeRel \
    -D Release

command_commit
command_execute $root "$@"