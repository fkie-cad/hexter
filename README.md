# Hexter #
A minimal terminal hex viewer supporting reading, writing and searching in files and processes.

Compiles and runs under
- Linux 
- Windows (x86/x64).  
- OsX may work too, but only the -file functionality.
- Android in Termux


## Version ##
1.7.0  
Last changed: 21.12.2021


## REQUIREMENTS ##
- Linux
    - Gcc
    - Building with cmake requires cmake.  
- Windows
    - msbuild


## BUILD ##

### Linux (gcc) & cmake ###
```bash
$ ./linuxBuild.sh [-t hexter] [-m Debug|Release] [-h]
```

### Linux (gcc) ###
```bash
$ mkdir build
$ gcc -o build/hexter -Wl,-z,relro,-z,now -D_FILE_OFFSET_BITS=64 -Ofast src/hexter.c src/Finder.c src/Printer.c src/ProcessHandlerLinux.c src/Writer.c src/utils/*.c
```

Use `clang` instead of `gcc` in Termux on Android.

### Windows (MsBuild) ###
```bash
$ winBuild.bat [/app] [/m Release|Debug] [/b 32|64] [/rt] [/pdb] [/pts <toolset>] [/bt <path>] [/h]
```
The correct path to your build tools may be passed as a parameter or just changed in the script [winBuild.bat](winBuild.bat) itself.  
The Platformtoolset defaults to v142, but may be changed with the `/pts` option.

### DLL : Windows (MsBuild) ### 
```bash
$ winBuild.bat /lib [/b 64] [/m Release] [/rt] [/pdb] [/pts <toolset>] [/bt a\path] [/?]
```

### Windows Context Menu ###
It may be convenient to add Hexter to the context menu to be able to right-click a file and hexter it.
In this scenario, you may use
```bash
$ addHeaderHexterToShellCtxtMenu.bat /p "c:\Hexter.exe" [/l "Open in Hexter"]
```


## USAGE ##
```bash
$ ./hexter [options] -file a/file/name [options]
$ ./hexter [options] -pid xx [options] 
```
Optional Parameters:
 * -file string A path to file.
 * -pid uint32_t A process id.
 * -s uint64_t Start offset in hex or dec. Default = 0.
 * -l uint64_t Length of the part to display in hex or dec. Default = 0x100.
 * -b Force breaking, not continuous mode and print just one block.
 * Printing format:
   * -pa ASCII only print.
   * -pu UNICODE (utf-16) only print.
   * -px HEX only print.
   * -po Print address (only valid in combination with the other options).
   * -p Plain, not styled text output. 
 * File manipulation/examination.
   * -d Delete -l bytes from offset -s. (File mode only.) Pass -l 0 to delete from -s to file end.
   * -ix Insert hex byte sequence (destructive!). Where x is a format option. (File mode only.)
   * -ox Overwrite hex byte sequence (destructive!). Where x is a format option.
   * -fx Find hex byte sequence. Where x is a format option.
   * Format options: 
     * h: plain bytes, 
     * a: ascii/utf-8 text, 
     * u: unicode (windows utf-16) text, 
     * b: byte, 
     * f: fill byte (will be inserted -l times), 
     * w: word, 
     * d: double word, 
     * q: quad word.  
     Expect for the string types, all values have to be passed as hex values, omitting `0x`.  
 * -pid only:
   * -lpx List entire process memory layout.
   * -lpm List all process modules.
   * -lpt List all process threads.
   * -lph List all process heaps.
   * -lphb List all process heaps and its blocks.
   * -lrp List all running processes. Pass any pid or 0 to get it running.
 * -h Print this.

Either use -file or -pid, not both. 
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

Print a list of running processes.
```bash
$ ./hexter -pid 0 -lrp
```

## TESTS ##
(Currently Gtests run linux only) 

### REQUIREMENTS ###
 - The google c++ testing framework gtest [1]  
 - A c++ compiler available to cmake

The test may be built with the target_name=hexter_tests which is the name of the test program as well.


[1] https://github.com/google/googletest


## COPYRIGHT, CREDITS & CONTACT ##
Published under [GNU GENERAL PUBLIC LICENSE](LICENSE).   

#### Author ####
- Henning Braun ([henning.braun@fkie.fraunhofer.de](henning.braun@fkie.fraunhofer.de)) 

#### Co-Author ####
common_codeio.c
- Viviane Zwanger ([viviane.zwanger@fkie.fraunhofer.de](viviane.zwanger@fkie.fraunhofer.de))
