#/bin/bash
if [[ -v $log_version ]] && [[ $log_version =~ ^[0-9]$ ]] && ((log_version>1));then
    return
fi
log_version=1

# if != '', print log to this file
log_to_file=''
# you can override how to write log to file
function log_write_file
{
    echo "$@" >> "$log_to_file"
}
# call after log to stdout, you can override it
function log_after_stdout
{
    return 0
}

# if != '', print log tag
log_flag_tag='[DEFAULT]'

# if != 0, print log line
log_flag_line=1

# if != 0, print log sub
log_flag_sub=1

# * 0, not print filename
# * 1, print log short filename
# * 2, print log long filename
log_flag_file=1

# log print level
# * 0 trace
# * 1 debug
# * 2 info
# * 3 warn
# * 4 error
# * 5 fatal
log_flag_level=0

# * 0 no color
# * 1 color level
# * 2 color metadata
# * 3 color message
# * 4 color metadata+message
log_color=1
# trace color
log_color_trace='97m'
# debug color
log_color_debug='93m'
# info color
log_color_info='92m'
# warn color
log_color_warn='95m'
# error color
log_color_error='91m'
# fatal color
log_color_fatal='31m'

function _log_print
{
    local s="`date '+%F %H:%M:%S'`"
    if [[ "$log_flag_tag" != '' ]];then
        s="$log_flag_tag $s"
    fi
    local caller1=''
    if [[ "$log_flag_line" != 0 ]] || [[ "$log_flag_sub" != 0 ]] || [[ "$log_flag_file" != 0 ]];then
        local line
        local sub
        local file
        read line sub file < <(caller 1)

        case "$log_flag_file" in
            1)
                caller1=`basename "$file"`
            ;;
            2)
                caller1=$file
            ;;
        esac

        if [[ "$log_flag_line" != 0 ]];then
            caller1="$caller1:$line"
        fi
        if [[ "$log_flag_sub" != 0 ]]; then
            if [[ "$caller1" == '' ]];then
                caller1=$sub
            else
                caller1="$caller1 $sub"
            fi
        fi

        if [[ "$caller1" != '' ]];then
            caller1="[$caller1] "
        fi
    fi

    if [[ "$log_to_file" != '' ]];then
        log_write_file "$s $_log_tag $caller1$@"
        return $?
    fi

    case "$log_color" in
        1)
            echo -n "$s"
            echo -en "\e[$_log_color"
            echo -n " $_log_tag "
            echo -en "\e[0m"
            echo "$caller1$@"
        ;;
        2)
            echo -en "\e[$_log_color"
            echo -n "$s $_log_tag $caller1"
            echo -en "\e[0m"
            echo "$@"
        ;;
        3)
            echo -en "\e[$_log_color"
            echo "$s $_log_tag $caller1$@"
            echo -en "\e[0m"
        ;;
        *)
            echo "$s $_log_tag $caller1$@"
        ;;
    esac
    log_after_stdout "$s $_log_tag $caller1$@"
}

# trace(... any)
function log_trace
{
    if (($log_flag_level>0));then
        return 0
    fi

    _log_color="$log_color_trace"
    _log_tag="[trace]"
    _log_print "$@"
}
# debug(... any)
function log_debug
{
    if (($log_flag_level>1));then
        return 0
    fi
    _log_color="$log_color_debug"
    _log_tag="[debug]"
    _log_print "$@"
}
# info(... any)
function log_info
{
    if (($log_flag_level>2));then
        return 0
    fi
    _log_color="$log_color_info"
    _log_tag="[info]"
    _log_print "$@"
}
# warn(... any)
function log_warn
{
    if (($log_flag_level>3));then
        return 0
    fi
    _log_color="$log_color_warn"
    _log_tag="[warn]"
    _log_print "$@"
}
# error(... any)
function log_error
{
    if (($log_flag_level>4));then
        return 0
    fi
    _log_color="$log_color_error"
    _log_tag="[error]"
    _log_print "$@"
}
# fatal(... any) then exit 1
function log_fatal
{
    _log_color="$log_color_fatal"
    _log_tag="[fatal]"
    _log_print "$@"
    exit 1
}