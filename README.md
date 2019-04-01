# Hexter #
A minimal terminal hex viewer supporting big files and offsets.

Compilable under Linux and Windows.  

## Version ##
1.0.3 
Last changed: 2019.04.01

## REQUIREMENTS ##
- A decent c compiler (gcc, msbuild) is required.  
- Building with cmake obviously requires cmake.  

## BUILD ##

### Building with cmake (Cross OS) ###
Using a build directory for a clean layout:

```bash
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ cmake --build . [--config Release] --target hexter
```
"--config Release" (without the enclosing "[]") is needed just for Windows to build in Release mode. It seems not to bother the Linux build if left there.  
"-DCMAKE_BUILD_TYPE=Release" on the other hand is only meaningful to Linux.  
"Release" will build with -Ofast.  

### MSBUILD & Windows commandline ###
Use cmake for creating .vcxproj without Visual Studio:
```bash
$ cmake . -G "Visual Studio 14 2015 Win64"  
$ msbuild (/p:PlatformToolset=v140) (/p:Platform=x64) (/p:Configuration=Release) hexter.vcxproj
```
The "p" options are more or less mandatory and used without the enclosing "()".  

## USAGE ##
```bash
$ ./hexter a/file/name [options]
```
Optional Parameters:
 * -s:uint64_t Start offset in hex or dec. Default = 0x00.
 * -l:uint64_t Length of the part to display in hex or dec. Default = 0x50.
 * -a ASCII only print.
 * -x HEX only print.
 * -c for a clean, not ANSI formated output. 

For example  
```bash
$ ./headerParser a/file/name -s 20 -l 100 -x
```
prints 100 bytes from offset 20 in hex only style.


## TESTS ##
### REQUIREMENTS ###
 - The google c++ testing framework gtest [1]  
 - A c++ compiler available to cmake

The test may be built with the target_name=hexter_tests which is the name of the test program as well.


[1] https://github.com/google/googletest


## CREDITS & CONTACT ## 
#### Author ####
- Henning Braun ([henning.braun@fkie.fraunhofer.de](henning.braun@fkie.fraunhofer.de)) 

#### Co-Author ####
common_codeio.h
- Viviane Zwanger ([viviane.zwanger@fkie.fraunhofer.de](viviane.zwanger@fkie.fraunhofer.de))
