#!/bin/bash

target=hexter
targets="${target}|${target}_shared"
mode=Release


printUsage()
{
	echo "Usage: $0 [${targets} [Debug|Release]]"
	echo "Default: $0 [${target} ${mode}]"
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

	if ! mkdir -p ${dir}; then
		return 1
	fi

	# if no space at -B..., older cmake will not build
	if ! cmake -S ${ROOT} -B${dir} -DCMAKE_BUILD_TYPE=${mode}; then
		return 2
	fi

	if ! cmake --build ${dir} --target ${target}; then
		return 3
	fi

	return 0
}

if [[ -n "$1" && $1 == "-h" ]]
then
	printUsage
	exit 0
fi

if [ -n "$1" ]
then
    target=$1
fi

if [ -n "$2" ]
then
    mode=$2
fi

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
build_dir="${ROOT}/build"
if [[ ${mode} == "Debug" || ${mode} == "debug" ]]
then
	build_dir=build/debug
fi

echo "target: "${target}
echo "mode: "${mode}
echo "build_dir: "${build_dir}

buildTarget ${target} ${build_dir} ${mode}

exit $?
