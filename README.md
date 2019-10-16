# Hexter #
A minimal terminal hex viewer supporting reading, writing and searching in files and processes.

Compilable under Linux and Windows.  

## Version ##
1.5.4  
Last changed: 2019.10.16

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

Running
```bash
./linuxBuild.bat [-h]
```
will do all this in one rush.

### CMake on Windows and bitness using NMake ###
Since keeping control of the bitness of the built program seems to be complicated on Windows, using the appropriate "x86/x64 Native Tools Command Prompt for VS XXXX" is advised, if you plan to build for a different bitness than your actual OS.  
Alternatively running
```bash
"C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsXX.bat"
```
in a normal terminal, where the XX (in vcvarsXX.bat) is 32 or 64, should do the same trick.
Either way will set the necessary environment variables correctly.

Then you may simply run:
```bash
$ mkdir build
$ cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - NMake Makefiles"
cmake --build . --config Release --target hexter
```
Running
```bash
winBuild.bat [/h]
```
will do all this in one rush.
  

### CMake & msbuild mix using vcxproj files ###
Run the appropriate "x86/x64 Native Tools Command Prompt for VS XXXX".

Use cmake for creating .vcxproj without Visual Studio:
```bash
$ mkdir build
$ cd build
$ cmake .. -G "Visual Studio 16 2019 Win64"  
$ msbuild /p:PlatformToolset=v160 /p:Platform=x64 /p:Configuration=Release hexter.vcxproj
```
The version of VisualStudio and the PlatformToolset should be adjusted to your installation.  
For a 32-bit (x86) build, delete the "Win64" part in the third line and change to x32 in the fourth line.  
Change "Release" to "Debug" in the fourth line for a debug build.  
On some systems this did nonetheless build 64-bit binaries, even though the 32 bit flags were set.

## USAGE ##
```bash
$ ./hexter [options] -file a/file/name [options]
$ ./hexter [options] -pid xx [options] 
```
Optional Parameters:
 * -s:uint64_t Start offset in hex or dec. Default = 0x00.
 * -l:uint64_t Length of the part to display in hex or dec. Default = 0x50.
 * -a ASCII only print.
 * -x HEX only print.
 * -ix Insert hex byte sequence (destructive!). Where x is an format option. (File mode only.)
 * -ox Overwrite hex byte sequence (destructive!). Where x is an format option.
 * -fx Find hex byte sequence. Where x is an format option.
 * Format options: 
   * h: plain bytes, 
   * a: ascii text, 
   * b: byte, 
   * f: fill byte (with the length of -l), 
   * w: word, 
   * d: double word, 
   * q: quad word.  
   Expect for the ascii string, all values have to be passed as hex values.  
 * -d Delete -l bytes from offset -s. (File mode only.)
 * -t Type of source ['file', 'pid']. Defaults to 'file'. If 'pid', a process id is passed as 'a/file/name'.
 * -pid only.
   * -lpx List whole process memory layout.
   * -lpm List all process modules.
   * -lpt List all process threads.
   * -lph List all process heaps.
   * -lphb List all process heaps and its blocks.
   * -lrp List all running processes. Pass any pid or zero to get it running.
 * -b Force breaking, not continuous mode and print just one block.
 * -p For a plain, not styled text output. 
 * -h Print this.

The program runs in continuous mode by default, expect for the -i, -o and -d option, or if the -b option is set.  
Step through the file by pressing ENTER.  
Quit with "q".  
If searching something in continuous mode, type "n" to find next occurrence.  
The length value will be padded to fit a block size in continuous mode.

### EXAMPLES ###
Print 100 bytes from offset 20 in hex only style.
```bash
$ ./hexter -file a/file/name -s 20 -l 100 -x
```

Insert bytes at offset 0x20 with 0xdead0bea
```bash
$ ./hexter -file a/file/name -s 0x20 -ih dead0bea
```

Overwrite dword at offset 0x20 with 0xdead0bea
```bash
$ ./hexter -file a/file/name -s 0x20 -od dead0bea
```

Find ascii string "PE"
```bash
$ ./hexter -file a/file/name -fa PE
```

Delete 16 bytes from offset 16
```bash
$ ./hexter -file a/file/name -d -s 16 -l 16
```

Print my process (0) and a list of its modules
```bash
$ ./hexter -pid 0 -lpm
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
