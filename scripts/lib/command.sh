#!/bin/bash
if [[ -v command_version ]] && [[ $command_version =~ ^[0-9]$ ]] && ((command_version>=1));then
    return
fi
command_version=2

if [[ ! -v __command_id ]];then
    __command_name=''
    __command_short=''
    __command_long=''
    __command_func=''
    __command_children=()
    __command_flag=0
    __command_flags=()
    
    __command_id=0

fi
__command_panic(){
    local i=0
    echo "Panic: $result_errno"
    while true; do
        if ! read line sub file < <(caller $i);then
            break
        fi
        i=$((i+1))
        echo "  - $file $sub:$line"
    done
    exit 1
}
__command_join(){
    result=''
    local s
    for s in "${@}";do
        s=${s// /\\ }
        if [[ "$result" == '' ]];then
            result=$s
        else
            result="$result $s"
        fi
    done
}
# get flag describe
# out s
# in type
# in describe
# in max
# in min
# in value
# in pattern
# in regexp
# in default
__command_flags_describe(){
    local n
    local result
    s=$describe
    if [[ "$type" == *s ]];then
        n=${#default[@]}
        if ((n>0));then
            __command_join "${default[@]}"
            s="$s (default [$result])"
        fi
    else
        case "$type" in
            string)
                if [[ "$default" != '' ]];then
                    s="$s (default $default)"
                fi
            ;;
            int|uint)
                if [[ "$default" != 0 ]];then
                    s="$s (default $default)"
                fi
            ;;
            bool)
                if [[ "$default" != false ]];then
                    s="$s (default $default)"
                fi
            ;;
        esac
    fi
    case "$type" in
        int|uint|ints|uints)
            if [[ "$max" != '' ]] || [[ "$min" != '' ]];then
                if [[ "$min" == '' ]];then
                    s="$s (range x to"
                else
                    s="$s (range $min to"
                fi
                if [[ "$max" == '' ]];then
                    s="$s x)"
                else
                    s="$s $max)"
                fi
            fi
        ;;
    esac
    n=${#value}
    if ((n>0));then
        __command_join "${value[@]}"
        s="$s (option [$result])"
    fi
    n=${#pattern}
    if ((n>0));then
        __command_join "${pattern[@]}"
        s="$s (== $result})"
    fi
    n=${#regexp}
    if ((n>0));then
        __command_join "${regexp[@]}"
        s="$s (=~ $result})"
    fi
}

# (long: string, short:string, arg0: string, args1: string) (shift_val: string, shift_n: string) 
__command_flags_parse(){
    if [[ "$3" == "--$1" ]];then
        shift_val=$4
        shift_n=2
        return
    fi
    local s="--$1="
    local n=${#s}
    if [[ "${3:0:n}" == "$s" ]];then
        shift_val=${3:n}
        shift_n=1
        return
    fi
    
    if [[ $2 == '' ]];then
        return 1
    elif [[ "$3" == "-$2" ]];then
        shift_val=$4
        shift_n=2
        return
    fi
    s="-$2="
    n=${#s}
    if [[ "${3:0:n}" == $s ]];then
        shift_val=${3:n}
        shift_n=1
        return
    fi
    s="-$2"
    n=${#s}
    if [[ "${3:0:n}" == $s ]];then
        shift_val=${3:n}
        shift_n=1
        return
    fi
    return 1
}

__command_flags_verify_value(){
    case "$_type" in 
        bool|bools)
            if [[ "$_result" != true ]] && [[ "$_result" != false ]];then
                _errno=1
                result_errno="invalid flag ${_type}: $_name $1 $_result"
                return
            fi
        ;;
        int|ints)
            if [[ ! "$_result" =~ ^-?[0-9]+$ ]];then
                _errno=1
                result_errno="invalid flag ${_type}: $_name $1 $_result"
                return
            fi
        ;;
        uint|uints)
            if [[ ! "$_result" =~ ^[0-9]+$ ]];then
                _errno=1
                result_errno="invalid flag [${_type}]: $_name $1 $_result"
                return
            fi
        ;;
    esac

    if [[ $_max != '' ]] && ((_result>_max));then
        _errno=1
        result_errno="invalid flag [${_type}<=$_max]: $_name $1 $_result"
        return
    fi
    if [[ $_min != '' ]] && ((_result<_min));then
        _errno=1
        result_errno="invalid flag [${_type}>=$_min]: $_name $1 $_result"
        return
    fi
    local matched=1
    local s
    for s in "${_value[@]}";do
        matched=0
        if [[ "$_result" == "$s" ]];then
            return
        fi
    done
    for s in "${_pattern[@]}";do
        matched=0
        if [[ "$_result" == $s ]];then
            return
        fi
    done
    for s in "${_regexp[@]}";do
        matched=0
        if [[ "$_result" =~ $s ]];then
            return
        fi
    done
    if [[ $matched == 0 ]];then
        _errno=1
        local rules
        s="${_value[@]}"
        if [[ "$s" != '' ]];then
            rules=" (== $s)"
        fi
        s="${_pattern[@]}"
        if [[ "$s" != '' ]];then
            rules="$rules (== $s)"
        fi
        s="${_regexp[@]}"
        if [[ "$s" != '' ]];then
            rules="$rules (== $s)"
        fi
        result_errno="invalid flag [${_type}$rules]: $_name $1 $_result"
    fi
}
# _name
# _input 
# _n
#
# _long
# _short
# _type
# _max
# _min
# _value
# _pattern
# _regexp
#
# out _errno
# out _result
# out _shift
__command_flags_parse_value(){
    _errno=0
    _shift=-1
    if [[ "$_input" == '' ]];then
        if [[ "$1" == "--$_long" ]];then
            if [[ "$_type" == bool ]] || [[ "$_type" == bools ]];then
                _result=true
                _shift=1
                return
            fi
            if ((_n<2));then
                _errno=1
                result_errno="flag ${_type} requires a value: $_name $1"
                return
            fi
            _result=$2
            _shift=2
            __command_flags_verify_value "--$_long"
            return
        fi
        local s="--${_long}="
        local n=${#s}
        if [[ "${1:0:n}" == "$s" ]];then
            _result=${1:n}
            _shift=1
            __command_flags_verify_value "--$_long"
            return
        fi
    fi

    
    if [[ "$_short" == '' ]];then
        return 
    elif [[ "$1" == "-$_short" ]];then
        if [[ "$_type" == bool ]] || [[ "$_type" == bools ]];then
            _result=true
            _shift=1
            return
        fi
        if ((_n<2));then
            _errno=1
            result_errno="flag ${_type} requires a value: $_name $1"
            return
        fi
        _result=$2
        _shift=2
        __command_flags_verify_value "-$_short"
        return
    fi
    s="-$_short="
    n=${#s}
    if [[ "${1:0:n}" == "$s" ]];then
        _result=${1:n}
        _shift=1
        __command_flags_verify_value "-$_short"
        return
    fi
    s="-$_short"
    n=${#s}
    if [[ "${1:0:n}" == "$s" ]];then
        _result=${1:n}
        if [[ "$_type" == bool ]] || [[ "$_type" == bools ]];then
            _shift=0
            _input="-$_result"
            _result=true
            return
        fi
        _shift=1
        __command_flags_verify_value "-$_short"
        return
    fi
}

# (...): panic
# define a flag for current command
# -v, --var string(^[a-zA-Z_][a-zA-Z0-9_]*$)  Varname of this flag 
# -l, --long string   Long name of this flag
# -s, --short char   Short name of this flag 
# -t, --type string     Flag type (default bool) (value [string, strings, int, ints, uint, uints, bool, bools])
# -d, --describe string     How to use descriptive information
#   , --max number  Max value, only valid for type int
#   , --min number  Min value, only valid for type int
# -V, --value string   Lists of valid values are compared using == "$value[i]"
# -P, --pattern string Lists of valid values are compared using == $pattern[i]
# -R, --regexp string Lists of valid values are compared using =~ $pattern[i]
# -D, --default string  Default value when not specified
command_flags(){
    if [[ "$__command_name" == '' ]];then
        result_errno="please call command_begin to begin a new command"
        __command_panic
    fi

    local var
    local long
    local short
    local type=bool
    local describe
    local max
    local min
    local value=()
    local pattern=()
    local regexp=()
    local default=()
    # parse
    local n=${#@}

    local shift_val
    local shift_n
    while ((n>0)); do
        if __command_flags_parse var v "$1" "$2";then
            var=$shift_val
            shift $shift_n
        elif __command_flags_parse long l "$1" "$2";then
            long=$shift_val
            shift $shift_n
        elif __command_flags_parse short s "$1" "$2";then
            short=$shift_val
            shift $shift_n
        elif __command_flags_parse type t "$1" "$2";then
            type=$shift_val
            shift $shift_n
        elif __command_flags_parse describe d "$1" "$2";then
            describe=$shift_val
            shift $shift_n
        elif __command_flags_parse max '' "$1" "$2";then
            max=$shift_val
            shift $shift_n
        elif __command_flags_parse min '' "$1" "$2";then
            min=$shift_val
            shift $shift_n
        elif __command_flags_parse value V "$1" "$2";then
            value+=("$shift_val")
            shift $shift_n
        elif __command_flags_parse pattern P "$1" "$2";then
            pattern+=("$shift_val")
            shift $shift_n
        elif __command_flags_parse regexp R "$1" "$2";then
            regexp+=("$shift_val")
            shift $shift_n
        elif __command_flags_parse default D "$1" "$2";then
            default+=("$shift_val")
            shift $shift_n
        else
            result_errno="[command_flags] unknow flags: $1"
            __command_panic
        fi
        n=${#@}
    done
    if [[ "$var" == '' ]];then
        if [[ "$long" != help ]];then
            result_errno='--var flag must be specified with =~ ^[a-zA-Z_][a-zA-Z0-9_]*$'
            __command_panic
        fi
    elif [[ ! "$var" =~ ^[a-zA-Z_][a-zA-Z0-9_]*$ ]];then
        result_errno='--var must matched with =~ ^[a-zA-Z_][a-zA-Z0-9_]*$'
        __command_panic
    fi
    if [[ "$long" == '' ]];then
        long=$var
    fi
    if [[ "$short" != '' ]];then
      if [[ "$short" != ? ]];then
            result_errno='short flag must be a char'
            __command_panic
        fi
    fi

    local flag=$__command_flag
    local id=$__command_id
    local prefix="__command_${id}_flag_${flag}"

    local s_val
    case "$type" in
        int|ints)
            if [[ "$max" != '' ]] && [[ ! "$max" =~ ^-?[0-9]+$ ]];then
                result_errno="--max must specify a valid int value"
                __command_panic
            fi
            if [[ "$min" != '' ]] && [[ ! "$min" =~ ^-?[0-9]+$ ]];then
                result_errno="--min must specify a valid int value"
                __command_panic
            fi
            for s_val in "${value[@]}";do
                if [[ ! "$s_val" =~ ^-?[0-9]+$ ]];then
                    result_errno="--value must specify a valid int value"
                    __command_panic
                fi
            done
            for s_val in "${default[@]}";do
                if [[ ! "$s_val" =~ ^-?[0-9]+$ ]];then
                    result_errno="--default must specify a valid int value"
                    __command_panic
                fi
            done
        ;;
        uint|uints)
            if [[ "$max" != '' ]] && [[ ! "$max" =~ ^[0-9]+$ ]];then
                result_errno="--max must specify a valid uint value"
                __command_panic
            fi
            if [[ "$min" != '' ]] && [[ ! "$min" =~ ^[0-9]+$ ]];then
                result_errno="--min must specify a valid uint value"
                __command_panic
            fi
            for s_val in "${value[@]}";do
                if [[ ! "$s_val" =~ ^[0-9]+$ ]];then
                    result_errno="--value must specify a valid uint value"
                    __command_panic
                fi
            done
            for s_val in "${default[@]}";do
                if [[ ! "$s_val" =~ ^[0-9]+$ ]];then
                    result_errno="--default must specify a valid uint value"
                    __command_panic
                fi
            done
        ;;
        bool|bools)
            for s_val in "${value[@]}";do
                if [[ "$s_val" != true ]] && [[ "$s_val" != false ]];then
                    result_errno="--value must specify a valid bool value"
                    __command_panic
                fi
            done
            for s_val in "${default[@]}";do
                if [[ "$s_val" != true ]] && [[ "$s_val" != false ]];then
                    result_errno="--default must specify a valid bool value"
                    __command_panic
                fi
            done
        ;;
    esac
    local s_default
    case "$type" in
        string|int|uint|bool)
            s_default="${prefix}_default=\"\${default[0]}\""
        ;;
        strings|ints|uints|bools)
            s_default="${prefix}_default=(\"\${default[@]}\")"
        ;;
        *)
            result_errno="[command_flags] unknow type: $type"
            __command_panic
        ;;
    esac
    n=${#default[@]}
    if [[ $n == 0 ]];then
        case "$type" in
            string)
                default=('')
            ;;
            int|uint)
                default=(0)
            ;;
            bool)
                default=(false)
            ;;
        esac
    elif [[ $n != 1 ]];then
        case "$type" in
            string|int|uint|bool)
                default=("${default[n-1]}")
            ;;
        esac
    fi
    local vars=""
    local longs=""
    local shorts=""
    local i
    for i in "${__command_flags[@]}";do
        vars="$vars  \"\$__command_${__command_id}_flag_${i}_var\""
        longs="$longs  \"\$__command_${__command_id}_flag_${i}_long\""
        shorts="$shorts  \"\$__command_${__command_id}_flag_${i}_short\""
    done
    local s="__command_flag_commit(){
    local vars=($vars)
    local longs=($longs)
    local shorts=($shorts)
    local s
    for s in \"\${vars[@]}\";do
        if [[ \"\$s\" == \"\$var\" ]];then
            result_errno=\"var flag already exists: \$s\"
            return 1
        fi
    done
    for s in \"\${longs[@]}\";do
        if [[ \"\$s\" == \"\$long\" ]];then
            result_errno=\"long flag already exists: \$s\"
            return 1
        fi
    done
    if [[ \"\$short\" != '' ]];then
        for s in \"\${shorts[@]}\";do
            if [[ \"\$s\" == \"\$short\" ]];then
                result_errno=\"short flag already exists: \$s\"
                return 1
            fi
        done
    fi
    ${prefix}_var=\$var
    ${prefix}_long=\$long
    ${prefix}_short=\$short
    ${prefix}_type=\$type
    ${prefix}_describe=\$describe
    ${prefix}_max=\$max
    ${prefix}_min=\$min
    ${prefix}_value=(\"\${value[@]}\")
    ${prefix}_pattern=(\"\${pattern[@]}\")
    ${prefix}_regexp=(\"\${regexp[@]}\")
    $s_default
}
"

    # echo "$s"
    if eval "$s";then
        if __command_flag_commit ;then
            __command_flags+=("$flag")
            __command_flag=$((__command_flag+1))
        else
            __command_panic
        fi
    else
        result_errno="eval has error: $s"
        __command_panic
    fi
}
# (...) (id: number, panic)
# begin a new command
# -n, --name string   Name of command
# -l, --long string   Long describe of command
# -s, --short string   Short describe of command
# -f, --func string   Function name of command
command_begin(){
    if [[ $__command_name != '' ]];then
        result_errno="there is an uncommitted command: $__command_name"
        __command_panic
    fi
    local name
    local long
    local short
    local func
    # parse
    local n=${#@}

    local shift_val
    local shift_n
    while ((n>0)); do
        if __command_flags_parse name n "$1" "$2";then
            name=$shift_val
            shift $shift_n
        elif __command_flags_parse long l "$1" "$2";then
            long=$shift_val
            shift $shift_n
        elif __command_flags_parse short s "$1" "$2";then
            short=$shift_val
            shift $shift_n
        elif __command_flags_parse func f "$1" "$2";then
            func=$shift_val
            shift $shift_n        
        else
            result_errno="[command_begin] unknow flags: $1"
            __command_panic
        fi
        n=${#@}
    done

    if [[ "$name" == '' ]];then
        result_errno="command name invalid: $name"
        __command_panic
    fi

    __command_name=$name
    __command_short=$short
    if [[ "$long" == '' ]];then
        __command_long=$short
    else
        __command_long=$long
    fi
    __command_func=$func
    __command_children=()
    __command_flags=()

    command_flags --type bool --describe "Help for $name"\
        --long help --short h
    result=$__command_id
}
__command_generate_help(){
    s="$s
${prefix}_help(){
    local min=0
    local i
    local n
    local s
    local format
    if [[ \$__command_parent == '' ]];then
        local name=\$${prefix}_name
    else
        local name=\"\$__command_parent \$${prefix}_name\"
    fi
    printf '%s\n\nUsage:\n  %s [flags]\n' \"\$${prefix}_long\" \"\$name\"
"
    local n=${#__command_children[@]}
    # children command
    if ((n>0));then
        local i=0
        local names
        local shorts
        local child
        for child in "${__command_children[@]}";do
            if [[ $i == 0 ]];then
                names="(\"\$__command_${child}_name\""
                shorts="(\"\$__command_${child}_short\""
            else
                names="$names \"\$__command_${child}_name\""
                shorts="$shorts \"\$__command_${child}_short\""
            fi
            i=$((i+1))
        done
        s="$s    
    # children command
    printf '  %s [command]\n\nAvailable Commands:\n' \"\$name\"
    local names=$names)
    local shorts=$shorts)
    for s in \"\${names[@]}\";do
        n=\${#s}
        if ((min<n));then
            min=\$n
        fi
    done
    format=\"  %-\${min}s  %s  %s\n\"
    n=0
    for s in \"\${names[@]}\";do
        printf \"\$format\" \"\$s\" \"\${shorts[n]}\"
        n=\$((n+1))
    done
"
    fi

    # flags
    local flags_long
    local flags_short
    local flags_type
    local flag
    local i=0
    for flag in "${__command_flags[@]}";do
        if [[ $i == 0 ]];then
            flags_long="(\"\$${prefix}_flag_${flag}_long\""
            flags_short="(\"\$${prefix}_flag_${flag}_short\""
            flags_type="(\"\$${prefix}_flag_${flag}_type\""
        else
            flags_long="$flags_long \"\$${prefix}_flag_${flag}_long\""
            flags_short="$flags_short \"\$${prefix}_flag_${flag}_short\""
            flags_type="$flags_type \"\$${prefix}_flag_${flag}_type\""
        fi
        i=$((i+1))
    done
    if [[ "$flags_long" == '' ]];then
        flags_long="("
        flags_short="("
        flags_type="("
    fi
    s="$s
    # flags
    printf '\nAvailable Commands:\n'
    local flags_long=$flags_long)
    local flags_short=$flags_short)
    local flags_type=$flags_type)
    min=9
    i=0
    for s in \"\${flags_long[@]}\";do
        s=\"\$s \${flags_type[i]}\"
        n=\${#s}
        if ((min<n));then
            min=\$n
        fi
        i=\$((i+1))
    done
    format=\"  %3s --%-\${min}s   %s\n\"
    local short
    local type
    local describe
    local max
    local min
    local value
    local pattern
    local regexp
    local default
"
    local sf
    for flag in "${__command_flags[@]}";do
        sf="$sf    short=\$${prefix}_flag_${flag}_short
    if [[ \"\$short\" != '' ]];then
        short=\"-\$short,\"
    fi
    type=\$${prefix}_flag_${flag}_type
    describe=\$${prefix}_flag_${flag}_describe
    max=\$${prefix}_flag_${flag}_max
    min=\$${prefix}_flag_${flag}_min
    value=(\"\${${prefix}_flag_${flag}_value[@]}\")
    pattern=(\"\${${prefix}_flag_${flag}_pattern[@]}\")
    regexp=(\"\${${prefix}_flag_${flag}_regexp[@]}\")
    if [[ \"\$type\" == *s ]];then
        default=(\"\${${prefix}_flag_${flag}_default[@]}\")
    else
        default=\$${prefix}_flag_${flag}_default
    fi
    __command_flags_describe
    printf \"\$format\" \"\$short\" \"\$${prefix}_flag_${flag}_long \$${prefix}_flag_${flag}_type\" \"\$s\"
"
    done
    s="$s$sf"
    # echo "$s"
    # children help
    n=${#__command_children[@]}
    if ((n>0));then
        s="$s    
    # children help
    printf '\nUse \"%s [command] --help\" for more information about a command.\n' \"\$name\"
}"
    else
        s="$s}"
    fi
}
# in prefix: string
# in flag: number
# out s0: string
__command_define_var(){
    local errno=0
    local type
    local var
    s0=''
    if eval "type=\$${prefix}_flag_${flag}_type
var=\$${prefix}_flag_${flag}_var
";then
        if [[ "$var" == '' ]];then
            return
        fi
    else
        errno=$?
        return $errno
    fi
    if [[ "$type" == *s ]];then
        s0="    local $var=(\"\${${prefix}_flag_${flag}_default[@]}\")
    local _${flag}_default=1
"

    else
        s0="    local $var=\$${prefix}_flag_${flag}_default
"
    fi
}
# in prefix: string
# in flag: number
# out s0: string
# out s1: string
__command_parse_var(){
    local errno=0
    if eval "type=\$${prefix}_flag_${flag}_type
var=\$${prefix}_flag_${flag}_var
";then
    s0="
           _long=\$${prefix}_flag_${flag}_long
           _short=\$${prefix}_flag_${flag}_short
           _type=\$${prefix}_flag_${flag}_type
           _max=\$${prefix}_flag_${flag}_max
           _min=\$${prefix}_flag_${flag}_min
           _value=(\"\${${prefix}_flag_${flag}_value[@]}\")
           _pattern=(\"\${${prefix}_flag_${flag}_pattern[@]}\")
           _regexp=(\"\${${prefix}_flag_${flag}_regexp[@]}\")
           __command_flags_parse_value \"\$_input_flag\" \"\$2\""
        if [[ "$var" == '' ]];then
            var="_help"
        fi
    else
        errno=$?
        return $errno
    fi

    if [[ "$type" == *s ]];then
        s1="if [[ \$_${flag}_default == 1 ]];then
                    _${flag}_default=0
                    $var=(\"\$_result\")
                else
                    $var+=(\"\$_result\")
                fi"
    else
        s1="$var=\"\$_result\""
    fi
}
__command_generate_execute(){
    local s0
    local s1
    local flag
    local i
    
    s="$s
${prefix}_execute(){
    local _help=false
    local _value
    local _s
    local _args=()
    local _i
    local _n=\${#@}
    if [[ \$__command_parent == '' ]];then
        local _name=\$${prefix}_name
    else
        local _name=\"\$__command_parent \$${prefix}_name\"
    fi
"
    local n=${#__command_children[@]}
    if ((n>0));then
        local children
        local children_f
        for s0 in "${__command_children[@]}";do
            children="$children \"\$__command_${s0}_name\""
            children_f="$children_f \"__command_${s0}_execute\""
        done
        s="$s
    # children
    if ((_n>0));then
        local _children=($children)
        local _children_f=($children_f)
        _i=0
        for _s in \"\${_children[@]}\";do
            if [[ \"\$1\" == \"\$_s\" ]];then
                __command_parent=\$_name
                _s=\${_children_f[_i]}
                shift
                \"\$_s\" \"\$@\"
                return \$?
            fi
            _i=\$((_i+1))
        done
    fi
"
    fi

    s="$s
    # flags
    local _input
    local _input_flag
    local _long
    local _short
    local _type
    local _max
    local _min
    local _value
    local _pattern
    local _regexp
    local _errno
    local _result
    local _shift=0
"
    for flag in "${__command_flags[@]}";do
        if __command_define_var "$flag";then
            s="$s$s0"
        else
            return $?
        fi
    done

    s="$s    while true;do
        if [[ \$_help == true ]];then
            __command_${id}_help
            return $?
        fi
        if [[ \$_shift != 0 ]];then
            shift \$_shift
            _input=''
        fi
        _n=\${#@}
        if [[ \$_n == 0 ]];then
            break
        fi
        if [[ \"\$1\" == -* ]];then
            if [[ \"\$_input\" == '' ]];then
                _input_flag=\$1
            else
                _input_flag=\$_input
            fi
"

    for flag in "${__command_flags[@]}";do
        if __command_parse_var "$flag";then
            s="$s           $s0
           if [[ \$_errno != 0 ]];then
                __command_panic
           elif ((_shift>=0));then
                $s1
                continue
           fi
"
        else
            return $?
        fi
        i=$((i+1)) 
    done
    s="$s           if [[ \"\$_input\" == '' ]];then
               result_errno=\"unknown flag: \$_name \$1\"
           else
               result_errno=\"unknown flag: \$_name \$_input\"
           fi
           __command_panic
"
    s="$s        else
            _args+=(\"\$1\")
            _shift=1
        fi
    done
"

    s="$s
    # callbackup
    if [[ \"\$${prefix}_func\" != '' ]];then
        \"\$${prefix}_func\" \"\${_args[@]}\"
    fi
}"
    # echo "$s"
}
# () (string, errno)
# get current command eval string
command_string(){
    if [[ "$__command_name" == '' ]];then
        result_errno="please call command_begin to begin a new command"
        return 1
    fi
    local id=$__command_id
    local prefix="__command_${id}"

    local s="${prefix}_id=$id
${prefix}_name=\$__command_name
${prefix}_short=\$__command_short
${prefix}_long=\$__command_long
${prefix}_func=\$__command_func
"
    __command_generate_help
    __command_generate_execute
    result=$s
}
# values: []string
# names: []string
__sort_values(){
    # echo "${#values[@]}: ${values[@]}"
    # echo "${#names[@]}: ${names[@]}"
    local len=${#values[@]}
    local i
    local j
    local left
    local right
    local index
	for ((i = 0; i < len - 1; i++)); do
        for ((j=0;j<len-i-1;j++));do
            left=${names[j]}
            right=${names[j+1]}
            if [[ "$left" > "$right" ]];then
                names[j]=$right
                names[j+1]=$left

                left=${values[j]}
                right=${values[j+1]}
                values[j]=$right
                values[j+1]=$left
            fi
        done
    done
}
# (): panic
# generate command code and load it with eval
command_commit(){
    if [[ "$__command_name" == '' ]];then
        result_errno="please call command_begin to begin a new command"
        __command_panic
    fi
    local n=${#__command_flags[@]}
    if (($n>1));then
        local values=("${__command_flags[@]}")
        # sort flags
         local names="names=("
         local s
         for s in "${__command_flags[@]}";do
            names="$names \"\${__command_${__command_id}_flag_${s}_long}\${__command_${__command_id}_flag_${s}_short}\""
         done
         s="$names)
__sort_values"
        #  echo "$s"
         if eval "$s";then
            __command_flags=("${values[@]}")
         else
            result_errno="eval sort_flags has error: $s"
            __command_panic
         fi
    fi
    n=${#__command_children[@]}
    if (($n>1));then
        local values=("${__command_children[@]}")
        # sort flags
         local names="names=("
         local s
         for s in "${__command_children[@]}";do
            names="$names \"\$__command_${s}_name\""
         done
         s="$names)
__sort_values"
        #  echo "$s"
         if eval "$s";then
            __command_children=("${values[@]}")
         else
            result_errno="eval sort_children has error: $s"
            __command_panic
         fi
    fi
    if command_string ;then
        # echo "$result"
        if eval "$result";then
            __command_name=''
            __command_flag=0
            __command_id=$((__command_id+1))
        else
            result_errno="eval string has error: $result"
            __command_panic
        fi
    else
        __command_panic
    fi
}
# (id: number, ...args: []string): panic
command_execute(){
    result_errno=''
    local id="$1"
    shift
    if [[ ! "$id" =~ ^[0-9]+$ ]];then
        result_errno="command id invalid: $id"
        __command_panic
    fi

    local s="if [[ \"\$__command_${id}_name\" == '' ]];then
    result_errno=\"command id invalid: $id\"
    __command_panic
fi
local __command_parent=''
__command_${id}_execute \"\$@\"
"
    eval "$s"
}
# (...children_id: []string): panic
command_children(){
    result_errno=''

    local n=${#@}
    if [[ $n == 0 ]];then
        __command_children=()
        return
    fi

    local s
    local i=0
    local j
    local sj
    local names
    for s in "$@";do
        if [[ ! "$s" =~ ^[0-9]+$ ]];then
            result_errno="command id[$i] invalid: $id"
            __command_panic
        fi
        j=0
        for sj in "$@";do
            if [[ $i != $j ]] && [[ "$s" == "$sj" ]];then
                result_errno="children id[$i,$j] repeat: $s"
                __command_panic
            fi
            j=$((j+1))
        done
        names="$names \"\$__command_${s}_name\""
        i=$((i+1))
    done
    if eval "names=($names)";then
        i=0
        for s in "${names[@]}";do
            if [[ "$s" == '' ]];then
                result_errno="children name[$i] invalid: $s"
                __command_panic
            fi
            j=0
            for sj in "${names[@]}";do
                if [[ $i != $j ]] && [[ "$s" == "$sj" ]];then
                    result_errno="children name[$i,$j] repeat: $s"
                    __command_panic
                fi
                j=$((j+1))
            done
            i=$((i+1))
        done
    else
        return $?    
    fi
    
    __command_children=("$@")
}