#!/bin/bash

target=hexter
mode=Release

if [[ -n "$1" && $1 == "-h" ]]
then
	echo "Usage: $0 [${target} [Debug/Release]]"
	echo "Default: $0 [${target} ${mode}]"
	exit
fi

if [ -n "$1" ]
then
    target=$1
fi

if [ -n "$2" ]
then
    mode=$2
fi

build_dir=build
if [[ ${mode} == "Debug" ]]
then
	build_dir=build/debug
fi

echo "target: "${target}
echo "mode: "${mode}
echo "build_dir: "${build_dir}

mkdir -p ${build_dir}
cmake . -B${build_dir} -DCMAKE_BUILD_TYPE=${mode}
cmake --build ${build_dir} --target ${target}
