#!/bin/bash

target=hexter
mode=Release

if [[ ! -z "$1" && $1 == "-h" ]]
then
	echo "Usage: $0 [${target} [Debug/Release]]"
  echo "Default: $0 [${target} ${mode}]"
  exit
fi

if [ ! -z "$1" ]
then
    target=$1
fi

if [ ! -z "$2" ]
then
    mode=$2
fi

build_dir=build

echo "target: "${target}
echo "mode: "${mode}
echo "build_dir: "${build_dir}

mkdir -p ${build_dir}
cmake . -B${build_dir} -DCMAKE_BUILD_TYPE=${mode}
cmake --build ${build_dir} --target ${target}
