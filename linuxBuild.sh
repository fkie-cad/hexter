#!/bin/bash

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

release_build_dir="${ROOT}/build"
debug_build_dir="${ROOT}/build/debug"

MODE_DEBUG=1
MODE_RELEASE=2

DP_FLAG_DEBUG=1
DP_FLAG_ERROR=2

BUILD_FLAG_STATIC=1

name="hexter"
def_target="app"
pos_targets="app|sh"
target=
def_mode=$MODE_RELEASE
build_mode=$def_mode
build_flags=0
help=0
debug_print=$DP_FLAG_ERROR
clean=0

# Clean build directory from meta files
#
# @param $1 build directory
function clean() {
    local dir=$1
    local type=$2

    if [[ ${dir} != "${release_build_dir}" ]] && [[ ${dir} != "${debug_build_dir}" ]]; then
        echo [e] Invalid clean dir!
        return
    fi

    if [[ ${type} == 1 ]]; then
        echo "cleaning build dir: $dir"
        rm -rf ${dir}/*.o 2> /dev/null
    elif [[ ${type} == 2 ]]; then
        echo "deleting dir: $dir"
        rm -rf ${dir}/* 2> /dev/null
    fi


    return 0
}

# CMake build a target
#
# @param $1 cmake target
# @param $2 build directory
# @param $3 build build_mode
function buildTarget() {
    local target=$1
    local dir=$2
    local build_mode=$3
    local dp=$4
    local build_flags=$5
    local ep=0

    if ! mkdir -p ${dir}; then
        return 1
    fi

    if [[ $((dp & $DP_FLAG_ERROR)) == $DP_FLAG_ERROR ]]; then
        ep=1
    fi
    dp=$((dp & ~$DP_FLAG_ERROR))
    
    local bin_name=hexter

    if [[ ${build_mode} == $MODE_DEBUG ]]; then
        local flags="-Wl,-z,relro,-z,now -D_FILE_OFFSET_BITS=64 -Wall -pedantic -Wextra -ggdb -O0 -Werror=return-type -Werror=overflow -Werror=format"
    else
        local flags="-Wl,-z,relro,-z,now -D_FILE_OFFSET_BITS=64 -Wall -pedantic -Wextra -Ofast -Werror=return-type -Werror=overflow -Werror=format"
    fi

    if [[ $((build_flags & $BUILD_FLAG_STATIC)) == $BUILD_FLAG_STATIC ]]; then
        flags="${flags} -static"
    fi

    local dpf=
    if [[ $dp > 0 ]]; then
        dpf=-DDEBUG_PRINT=$dp
    fi

    local epf=
    if [[ $ep > 0 ]]; then
        epf=-DERROR_PRINT
    fi

    local bin_name=$name
    local app_src="src/hexter.c src/Finder.c src/Printer.c src/ProcessHandlerLinux.c src/Writer.c src/utils/*.c"
    local sh_src="src/hexter.c src/Finder.c src/Printer.c src/ProcessHandlerLinux.c src/Writer.c src/utils/*.c"

    case $target in
        "app")
            gcc -o $dir/${bin_name} $flags $dpf $epf -Ofast $app_src
            ;;

            
        "sh" | "shared")
            gcc -o $dir/lib${bin_name}.so -fPIC $flags $dpf $epf -Ofast $sh_src
            ;;
            
        #"st" | "static")
            #if ! mkdir -p ${dir}/st; then
                #return $?
            #fi

            #gcc $flags -o ${dir}/st/hexter.o $ROOT/src/hexter.c
            #gcc $flags -o ${dir}/st/Finder.o $ROOT/src/Finder.c
            #gcc $flags -o ${dir}/st/Printer.o $ROOT/src/Printer.c
            #gcc $flags -o ${dir}/st/ProcessHandlerLinux.o $ROOT/src/ProcessHandlerLinux.c
            #gcc $flags -o ${dir}/st/Writer.o $ROOT/src/Writer.c
            #gcc $flags -o ${dir}/st/common_file_io.o $ROOT/src/utils/common_file_io.c
            #gcc $flags -o ${dir}/st/utils/Converter.o $ROOT/src/utils/Converter.c
            #gcc $flags -o ${dir}/st/utils/Helper.o $ROOT/src/utils/Helper.c
            #gcc $flags -o ${dir}/st/utils/Strings.o $ROOT/src/utils/Strings.c
            #gcc $flags -o ${dir}/st/utils/TerminalUtil.o $ROOT/src/utils/TerminalUtil.c

            #ar rcs $dir/lib${bin_name}.a ${dir}/st/*.o
            #;;
            
        *)
            echo "Unknown target: ${target}"
            ;;
    esac
    
    return $?
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
    echo "  * sh: build hexter shared library"
    #echo "  * st: build hexter static library"
    echo "-d Build in debug build_mode"
    echo "-r Build in release build_mode"
    echo "-s Build statically linked binary"
    echo "-c clean up build dir"
    echo "-x delete all files in build dir"
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
            build_mode=$MODE_DEBUG
            shift 1
            ;;
        -r | --release)
            build_mode=$MODE_RELEASE
            shift 1
            ;;
        -p | -dp | --debug-print)
            debug_print=$2
            shift 2
            ;;
        -s | --static)
            build_flags=$((build_flags | $BUILD_FLAG_STATIC))
            shift 1
            ;;
        -t | --target)
            target=$2
            shift 2
            ;;
        -h | --help)
            help=1
            break
            ;;
        -x | --delete)
            clean=2
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

if [[ ${build_mode} == $MODE_DEBUG || ${build_mode} == $MODE_DEBUG ]]; then
    build_dir=${debug_build_dir}
else
    build_dir=${release_build_dir}
fi

if [[ -z ${target} && ${clean} == 0 ]]; then
    target=$def_target
fi

echo "clean: "${clean}
echo "target: "${target}
echo "build_mode: "${build_mode}
echo "build_dir: "${build_dir}
echo "build_flags: "${build_flags}
echo -e

if [[ ${clean} > 0 ]]; then
    clean ${build_dir} ${clean} 
fi

if [[ -n ${target} ]]; then
    buildTarget ${target} ${build_dir} ${build_mode} ${debug_print} ${build_flags}
fi

exit $?
