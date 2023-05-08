#!/bin/bash
if [[ -v core_version ]] && [[ $core_version =~ ^[0-9]$ ]] && ((core_version>=1));then
    return
fi
core_version=1


# (...msg)
# 'exit 1' after message and call stack
core_panic(){
    local i=0
    echo "Panic: $@"
    while true; do
        if ! read line sub file < <(caller $i);then
            break
        fi
        i=$((i+1))
        echo "  - $file $sub:$line"
    done
    exit 1
}

__core_check_function(){
    if [[ ! "$1" =~ ^[0-9_a-zA-Z\.,]+$ ]];then
        core_panic "function name invalid: $1"
    fi
}
__core_check_var(){
    if [[ ! "$1" =~ ^[0-9_a-zA-Z]+$ ]];then
        core_panic "var name invalid: $1"
    fi
}
# -f name args...
# ?-a # assert not error, if any error exit bash
# ?-t # trace on error
# ?-c # output caller on trace
# ?-v varname # result var name
# ?-e varname # result_errno var name
__core_getopt() {
    name='' # function name
    args=() # args for function
    is_assert=0
    is_trace=0
    is_caller=0
    var_result='' # result varname
    var_errno='' # result_errno varname
    local n=${#@}
    local s
    local c
    while ((n>0)); do
        if [[ "$name" != '' ]];then
            args+=("$@")
            break
        fi
        if [[ "$1" != -* ]] || [[ "$1" == - ]];then
            core_panic "core_call not define flags: $1"
        fi
        s=${1:1}
        while [[ $s != '' ]]; do
            c=${s:0:1}
            s=${s:1}
            case $c in
                a)
                    is_assert=1
                ;;
                t)
                    is_trace=1
                ;;
                c)
                    is_caller=1
                ;;
                f)
                    if [[ "$s" == '' ]];then
                        __core_check_function "$2"
                        name=$2
                        shift
                    else
                        name=$s
                        s=''
                    fi
                ;;
                v)
                    if [[ "$s" == '' ]];then
                        __core_check_var "$2"
                        var_result=$2
                        shift
                    else
                        var_result=$s
                        s=''
                    fi
                ;;
                e)
                    if [[ "$s" == '' ]];then
                        __core_check_var "$2"
                        var_errno=$2
                        shift
                    else
                        var_errno=$s
                        s=''
                    fi
                ;;
                *)
                    core_panic "unknow flags: $c"
                ;;
            esac
        done
        shift
        n=${#@}
    done

    if [[ "$name" == '' ]];then
        core_panic "unknow name, use core_call -f xxx args..."
    fi
}

# proxy access to a command
# -f name args...
# ?-a # assert not error, if any error exit bash
# ?-t # trace on error
# ?-c # output caller on trace
# ?-v varname # result var name
# ?-e varname # result_errno var name
core_call(){
    local name
    local args=()
    local is_assert=0
    local is_trace=0
    local is_caller=0
    local is_fatal=0
    local var_result
    local var_errno
    if ! __core_getopt "$@";then
        echo '__core_getopt unknow error '
        exit 1
    fi
    if [[ "$var_result" != '' ]];then
        local result
    else
        result=''
    fi
    if [[ "$var_errno" != '' ]];then
        local result_errno
    else
        result_errno=''
    fi
    local errno=0
    if "$name" "${args[@]}";then
        if [[ "$var_result" != '' ]];then
            eval "$var_result=\$result"
        fi
    else
        errno=$?
        if [[ "$var_errno" != '' ]];then
            eval "$var_errno=\$result_errno"
        fi
        if [[ $is_trace != 0 ]];then
            if [[ "$result_errno" == '' ]];then
                echo "Error: $errno"
            else
                echo "Error: $errno, $result_errno"
            fi
            if [[ $is_caller != 0 ]];then
                local line
                local sub
                local file
                local i=0
                while true; do
                    if ! read line sub file < <(caller $i);then
                        break
                    fi
                    i=$((i+1))
                    echo "  - $file $sub:$line"
                done
            fi
        fi
    fi
    if [[ $errno != 0 ]] && [[ $is_assert == 1 ]];then
        exit $errno
    fi
    return $errno
}

# (f: string, ...args)
# core_call -tcf "$@" 
core_call_default(){
    core_call -tcf "$@"
}
# (f: string, ...args)
# core_call -atcf "$@" 
core_call_assert(){
    core_call -atcf "$@"
}