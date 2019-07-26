# Hexter #
A minimal terminal hex viewer supporting big files and offsets.

Compilable under Linux and Windows.  

## Version ##
1.4.4  
Last changed: 2019.07.03

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

### CMake on Windows (recommended way)###
Since keeping control off the bitness of the built program seems to be complicated on Windows, using the appropriate "x86/x64 Native Tools Command Prompt for VS XXXX" is advised, if you plan to build for a different bitness than you actual OS.
Alternatively running
```bash
"C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsXX.bat"
```
in a normal terminal, where the XX (in vcvarsXX.bat) is 32 or 64, should do the same trick.
Either way will set the necessary environment variables correctly.

Then you may simply run:
```
$ mkdir build
$ cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug/Release -G "CodeBlocks - NMake Makefiles"
cmake --build . --config Debug/Release --target hexter
```

### MSBUILD & Windows commandline ###
Run the appropriate "x86/x64 Native Tools Command Prompt for VS XXXX".

Use cmake for creating .vcxproj without Visual Studio:
```bash
$ mkdir build
$ cd build
$ cmake .. -G "Visual Studio 14 2015 Win64"  
$ msbuild (/p:PlatformToolset=v140) (/p:Platform=x64) (/p:Configuration=Release) hexter.vcxproj
```
For a 32-bit (x86) build, delete the "Win64" part.
On some systems this did nonetheless only support the creation of 64-bit binaries.
The "p" options are more or less mandatory and used without the enclosing "()".  

## USAGE ##
```bash
$ ./hexter a/file/name [options]
$ ./hexter [options] a/file/name
```
Optional Parameters:
 * -s:uint64_t Start offset in hex or dec. Default = 0x00.
 * -l:uint64_t Length of the part to display in hex or dec. Default = 0x50.
 * -a ASCII only print.
 * -x HEX only print.
 * -ix Insert hex byte sequence (destructive!). Where x is an format option.
 * -ox Overwrite hex byte sequence (destructive!). Where x is an format option.
 * -fx Find hex byte sequence. Where x is an format option.
 * Format options: 
   * h: plain bytes, 
   * a: ascii text, 
   * b: byte, 
   * w: word, 
   * d: double word, 
   * q: quad word.  
   Expect for the ascii string, all values have to be passed as hex values.
 * -d Delete -l bytes from offset -s.
 * -t Type of source ['file', 'pid']. Defaults to 'file'. If 'pid', a process id is passed as 'filename'.
 * -pid only.
   * -lpx List whole process memory layout.
   * -lpm List all process modules.
   * -lpt List all process threads.
   * -lph List all process heaps.
   * -lphb List all process heaps and its blocks.
 * -b Force breaking, not continuous mode and print just one block.
 * -p For a plain, not styled text output. 
 * -h Print this.

The program runs in continuous mode by default, expect for the -i, -o and -d option, or if the -b option is set.  
Step through the file by pressing ENTER.  
Quit with "q".  
If searching something in continuous mode, type "n" to find next occurrence.  
The length value will be padded to fit a block size in continuous mode.

Examples:  
Print 100 bytes from offset 20 in hex only style.
```bash
$ ./hexter a/file/name -s 20 -l 100 -x
```

Insert bytes at offset 0x20 with 0xdead0bea
```bash
$ ./hexter a/file/name -s 0x20 -ih dead0bea
```

Overwrite dword at offset 0x20 with 0xdead0bea
```bash
$ ./hexter a/file/name -s 0x20 -od dead0bea
```

Find ascii string "PE"
```bash
$ ./hexter a/file/name -fa PE
```

Delete 16 bytes from offset 16
```bash
$ ./hexter a/file/name -d -s 16 -l 16
```

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
