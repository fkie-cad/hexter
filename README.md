# Hexter #
A minimal terminal hex viewer supporting big files and offsets.

POSIX compliant.  
Compilable under Linux and Windows.  

## Version ##
1.0.1  
Last changed: 2019.03.26

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
"-DCMAKE_BUILD_TYPE=Release" is only meaningful to Linux.   
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

For example  
```bash
$ ./headerParser a/file/name -s 20 -l 100 -x
```
prints 100 bytes from offset 20 in hex only style.


## TESTS ##
Not implemented yet.


## CREDITS & CONTACT ## 
#### Author ####
- Henning Braun ([henning.braun@fkie.fraunhofer.de](henning.braun@fkie.fraunhofer.de)) 

#### Co-Author ####
common_codeio.h
- Viviane Zwanger ([viviane.zwanger@fkie.fraunhofer.de](viviane.zwanger@fkie.fraunhofer.de))
