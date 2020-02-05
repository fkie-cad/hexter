# Hexter #
A minimal terminal hex viewer supporting reading, writing and searching in files and processes.

Compilable under Linux and Windows.  

## Version ##
1.5.6  
Last changed: 05.02.2020

## REQUIREMENTS ##
- A decent c compiler (gcc, msbuild) is required.  
- Building with cmake obviously requires cmake.  

## BUILD ##

### Linux ###
```bash
$ ./linuxBuild.sh [-h]
```

### MsBuild & Windows & cmake ###
```bash
$ winBuild.bat [/h]
```
The correct path to your build tools may be passed as a parameter or just changed in the script [winBuild.bat](winBuild.bat) itself.

### DLL : MsBuild & Windows & cmake ### 
```bash
$ winBuild.bat hexter_shared [64 Release]
```

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
