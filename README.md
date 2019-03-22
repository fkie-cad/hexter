# Hexter #
A hex viewer for the terminal supporting big files and offsets.

POSIX compliant.  
Compilable under Linux and Windows.  

## Version ##
1.0.0 (2019.03.22)

## BUILD ##
### GCC & Linux commandline ### 
```bash
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release/Debug
$ cmake --build . --target hexter
```

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
 * -s:uint64_t Startoffset. Default = 0.
 * -l:uint64_t Length of the part to display. Default = 50.
 * -a ASCII only print.
 * -x HEX only print.

```bash
$ ./headerParser a/file/name -s 20 -l 100 -x
```
Prints 100 bytes from offset 20 in hex only style.

## CREDITS & CONTACT ## 
#### Author ####
- Henning Braun ([henning.braun@fkie.fraunhofer.de](henning.braun@fkie.fraunhofer.de)) 

#### Co-Author ####
common_codeio.h
- Viviane Zwanger ([viviane.zwanger@fkie.fraunhofer.de](viviane.zwanger@fkie.fraunhofer.de))
