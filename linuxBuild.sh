#!/bin/bash

name=hexter
def_target=app
pos_targets="app|lib|pck|cln"
target=${def_target}
def_mode=Release
mode=${def_mode}
help=0

# Clean build directory from meta files
#
# @param $1 build directory
function clean() {
    local dir=$1

    echo "cleaning build dir: $dir"

    if [[ ${dir} == "${ROOT}" ]]; then
        return
    fi

    cd ${dir} || return 1

    rm -r ./CMakeFiles 2> /dev/null
    rm -r ./CTestTestfile.cmake 2> /dev/null
    rm -r ./CMakeCache.txt 2> /dev/null
    rm -r ./cmake_install.cmake 2> /dev/null
    rm -rf ./tests 2> /dev/null
    rm -f ./*.cbp 2> /dev/null
    rm -r ./Makefile 2> /dev/null
    rm -rf ./debug 2> /dev/null

    cd - || return 2

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

    if ! mkdir -p ${dir}; then
        return 1
    fi

    # if no space at -B..., older cmake (ubuntu 18) will not build
    if ! cmake -S ${ROOT} -B${dir} -DCMAKE_BUILD_TYPE=${mode} -DDEBUG_PRINT=${dp}; then
        return 2
    fi

    if ! cmake --build ${dir} --target ${target}; then
        return 3
    fi

    # if [[ ${mode} == "Release" || ${mode} == "release" ]]; then
    #     sha256sum ${dir}/${target} | awk '{print $1}' > ${dir}/${target}.sha256
    # fi

    return 0
}

# Build a clean runnable package without metafiles.
#
# @param $1 cmake target
# @param $2 build directory
# @param $3 build mode
function buildPackage()
{
    local target=$1
    local dir=$2
    local mode=$3

    if ! buildTarget ${target} ${dir} ${mode} 0; then
        return 1
    fi

    if ! clean ${dir}; then
        return 4
    fi

    return 0
}

function printUsage() {
    echo "Usage: $0 [-t ${pos_targets}] [-m Debug|Release] [-h]"
    echo "Default: $0 [-t app] [-m ${def_mode}]"
    return 0;
}

function printHelp() {
    printUsage
    echo ""
    echo "-t A possible target: ${pos_targets}"
    echo "  * app: build hexter application"
    echo "  * lib: build hexter shared library"
    echo "  * pck: build hexter application and clean up build dir"
    echo "  * cln: clean up build dir"
    echo "-m A compile mode: Release|Debug"
    echo "-h Print this."
    return 0;
}

while (("$#")); do
    case "$1" in
        -m | --mode)
            mode=$2
            shift 2
            ;;
        -p | --debug-print)
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

if [[ ${help} == 1 ]]; then
    printHelp
    exit $?
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"

release_build_dir="${ROOT}/build"
debug_build_dir="${ROOT}/build/debug"
if [[ ${mode} == "Debug" || ${mode} == "debug" ]]; then
    build_dir=${debug_build_dir}
else
    build_dir=${release_build_dir}
fi

echo "target: "${target}
echo "mode: "${mode}
echo "build_dir: "${build_dir}

if [[ ${target} == "clean" || ${target} == "cln" ]]; then
    clean ${build_dir}
    exit $?
elif [[ ${target} == "pck" ]]; then
    buildPackage ${name} ${release_build_dir} Release
    exit $?
else
    if [[ ${target} == "app" ]]; then
        target=${name}
    elif [[ ${target} == "lib" ]]; then
        target=${name}_so
    else
        exit $?
    fi

    buildTarget ${target} ${build_dir} ${mode} ${debug_print}
    exit $?
fi

exit $?
