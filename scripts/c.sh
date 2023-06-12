#!/bin/bash
wolfssl_version=wolfssl-5.6.0-stable
libevent_version=libevent-2.1.12-stable
libtomcrypt_version=libtomcrypt-1.18.2
duktape_version=duktape-2.7.0
define_command(){
    command_begin --name c \
    --short 'build *.c' \
    --func c_execute
    local cmd=$result

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
    command_flags -t string -d 'set CMAKE_BUILD_TYPE' \
        -v build_type -l build-type \
        -V None -V Debug -V Release -V RelWithDebInfo -V MinSizeRel \
        -D Release

    command_commit
    result=$cmd
}
define_command
c_wolfssl(){
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
    if [[ -d wolfssl ]] && [[ ! -f ../include/wolfssl/wolfio.h ]];then
        cp wolfssl ../include/wolfssl -r
    fi
    if [[ -f src/.libs/libwolfssl.a ]] && [[ ! -f ../lib/libwolfssl.a ]];then
        cp src/.libs/libwolfssl.a ../libs/libwolfssl.a
    fi
}
c_libevent(){
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
            local args=(cmake ../../../third_party/$libevent_version
                -DCMAKE_BUILD_TYPE=Release
                -DEVENT__DISABLE_SAMPLES=ON
                -DEVENT__DISABLE_TESTS=ON
                "-DEVENT__DISABLE_OPENSSL=$DISABLE_OPENSSL"
                "-DOPENSSL_INCLUDE_DIR=$rootDir/$dir/wolfssl"
                "-DOPENSSL_LIBRARIES=$rootDir/$dir/wolfssl/src/.libs/libwolfssl.a"
                -DEVENT__LIBRARY_TYPE=STATIC
            )
            args+=("${cmake_args[@]}")
            log_info "${args[@]}"
            "${args[@]}"
        fi
        if [[ $make == true ]];then
            if [[ $third_party == true ]] || [[ ! -f lib/libevent_openssl.a ]];then
                log_info "make libevent for $target"
                make
            fi
        fi
    fi
    if [[ -f include/event2/event-config.h ]] && [[ ! -f ../include/event2/event-config.h ]];then
        mkdir -p ../include/event2
        cp ../../../third_party/$libevent_version/include/*.h ../include/
        cp ../../..//third_party/$libevent_version/include/event2/*.h ../include/event2/
        cp include/event2/event-config.h ../include/event2
    fi
    if [[ -f lib/libevent_openssl.a ]] && [[ ! -f ../libs/libevent_openssl.a ]];then
        cp lib/*.a ../libs/
    fi
}
c_libtomcrypt(){
    cd "$rootDir/$dir"
    if [[ ! -d libtomcrypt ]];then
        log_info "cp $libtomcrypt_version"
        cp "$rootDir/third_party/$libtomcrypt_version" "$rootDir/$dir/libtomcrypt" -r
    fi
    if [[ $make == true ]];then
        cd libtomcrypt
        log_info "make libtomcrypt for $target"
        make CFLAGS="-DLTC_NO_TEST" 
    fi
    if [[ -f src/headers/tomcrypt.h ]] && [[ ! -f ../include/tomcrypt.h ]];then
        cp src/headers/*.h ../include/
    fi
    if [[ -f libtomcrypt.a ]] && [[ ! -f ../libs/libtomcrypt.a ]];then
        cp libtomcrypt.a ../libs/libtomcrypt.a
    fi
}
c_iotjs(){
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
    cd ..

    if [[ ! -f /include/duktape.h ]];then
        cp "$rootDir/third_party/${duktape_version}/duk_config.h" include/
        cp "$rootDir/third_party/${duktape_version}/duktape.h" include/
    fi
    if [[ ! -f iotjs/include/iotjs/core/defines.h ]];then
        mkdir -p iotjs/include/iotjs/core
        cp ../../src/iotjs/core/*.h iotjs/include/iotjs/core/
    fi

    if [[ $test == true ]];then
        log_info "test for $target"
        cd "$rootDir"
        "$dir/bin/iotjs_test"
    fi
}
c_execute(){
    core_call_assert time_unix
    local start=$result

    local target="${os}_$arch"
    local iotjs_arch=$arch
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
        if [[ $third_party == true ]];then
            if [[ -d "$dir/include" ]];then
                rm "$dir/include" -rf
            fi
            if [[ -d "$dir/libs" ]];then
                rm "$dir/libs" -rf
            fi
            if [[ -d "$dir/libevent" ]];then
                log_info "delete cache '$dir/libevent'"
                rm "$dir/libevent" -rf
            fi
            if [[ -d "$dir/libtomcrypt" ]];then
                log_info "delete cache '$dir/libtomcrypt'"
                rm "$dir/libtomcrypt" -rf
            fi
            if [[ -d "$dir/wolfssl" ]];then
                log_info "delete cache '$dir/wolfssl'"
                rm "$dir/wolfssl" -rf
            fi
        fi
    fi
    if [[ $cmake == true ]] || [[ $make == true ]];then
    if [[ ! -d "$dir/include" ]];then
            mkdir -p "$dir/include"
        fi
        if [[ ! -d "$dir/libs" ]];then
            mkdir -p "$dir/libs"
        fi
        c_wolfssl
        c_libevent
        c_libtomcrypt
        c_iotjs
    fi
    core_call_assert time_since "$start"
    local used=$result
    log_info "success, used ${used}s"
}