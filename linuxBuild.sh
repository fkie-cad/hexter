#!/bin/bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

release_build_dir="${ROOT}/build"
debug_build_dir="${ROOT}/build/debug"

MODE_DEBUG=1
MODE_RELEASE=2

DP_FLAG=1
EP_FLAG=2

name=hexter
def_target=app
pos_targets="app|lib"
target=
def_mode=$MODE_RELEASE
mode=$def_mode
help=0
debug_print=EP_FLAG
clean=0

# Clean build directory from meta files
#
# @param $1 build directory
function clean() {
    local dir=$1

    echo "cleaning build dir: $dir"

    if [[ ${dir} != "${release_build_dir}" ]] && [[ ${dir} != "${debug_build_dir}" ]]; then
        echo [e] Invalid clean dir!
        return
    fi

    cd ${dir} || return 1

    rm -f ./*.o 2> /dev/null

    cd - 2>&1 || return 2

    return 0
}

# CMake build a target
#
# @param $1 cmake target
# @param $2 build directory
# @param $3 build mode
function buildTarget() {
    local target=$1
    local dir=$2
    local mode=$3
    local dp=$4
    local ep=0

    if ! mkdir -p ${dir}; then
        return 1
    fi

    if [[ $((dp & $EP_FLAG)) == $EP_FLAG ]]; then
        ep=1
    fi
    dp=$((dp & ~$EP_FLAG))

    if [[ ${mode} == $MODE_DEBUG ]]; then
        local flags="-Wall -pedantic -Wextra -ggdb -O0 -Werror=return-type -Werror=overflow -Werror=format"
    else
        local flags="-Wall -pedantic -Wextra -Ofast -Werror=return-type -Werror=overflow -Werror=format"
    fi

    local dpf=
    if [[ $dp > 0 ]]; then
        dpf=-DDEBUG_PRINT=$dp
    fi

    local epf=
    if [[ $ep > 0 ]]; then
        epf=-DERROR_PRINT
    fi

    if [[ $target == "app" ]]; then
        gcc -o $dir/hexter -Wl,-z,relro,-z,now -D_FILE_OFFSET_BITS=64 $flags $dpf $epf -Ofast src/hexter.c src/Finder.c src/Printer.c src/ProcessHandlerLinux.c src/Writer.c src/utils/*.c
    elif [[ $target == "lib" ]]; then
        gcc -o $dir/hexter.so -shared -Wl,-z,relro,-z,now -fPIC -D_FILE_OFFSET_BITS=64 $flags $dpf $epf -Ofast src/hexter.c src/Finder.c src/Printer.c src/ProcessHandlerLinux.c src/Writer.c src/utils/*.c
    fi
    return 0
}

function printUsage() {
    echo "Usage: $0 [-t ${pos_targets}] [-d|-r] [-h]"
    echo "Default: $0 -t app -r"
    return 0;
}

function printHelp() {
    printUsage
    echo ""
    echo "-t A possible target: ${pos_targets}"
    echo "  * app: build hexter application"
    echo "  * lib: build hexter shared library"
    echo "-c clean up build dir"
    echo "-d Build in debug mode"
    echo "-r Build in release mode"
    echo "-h Print this."
    return 0;
}

while (("$#")); do
    case "$1" in
        -c | -cln | --clean)
            clean=1
            shift 1
            ;;
        -d | --debug)
            mode=$MODE_DEBUG
            shift 1
            ;;
        -r | --release)
            mode=$MODE_RELEASE
            shift 1
            ;;
        -p | -dp | --debug-print)
            debug_print=$2
            shift 2
            ;;
        -t | --target)
            target=$2
            shift 2
            ;;
        -h | --help)
            help=1
            break
            ;;
        -* | --usage)
            usage=1
            break
            ;;
        *) # No more options
            break
            ;;
    esac
done

if [[ ${usage} == 1 ]]; then
    printUsage
    exit $?
fi

if [[ ${help} == 1 ]]; then
    printHelp
    exit $?
fi

if [[ ${mode} == $MODE_DEBUG || ${mode} == $MODE_DEBUG ]]; then
    build_dir=${debug_build_dir}
else
    build_dir=${release_build_dir}
fi

if [[ -z ${target} && ${clean} == 0 ]]; then
    $target=$def_target
fi

echo "clean: "${clean}
echo "target: "${target}
echo "mode: "${mode}
echo "build_dir: "${build_dir}
echo -e

if [[ ${clean} == 1 ]]; then
    clean ${build_dir}
fi

buildTarget ${target} ${build_dir} ${mode} ${debug_print}

exit $?
