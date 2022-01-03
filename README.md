# sdgbc

_NOTE: this project is not actively maintained, though I do sometimes apply
bugfixes due to dependency updates._  

A software emulator for the Nintendo Game Boy Color written in C++ using SFML
and wxWidgets. Available for Windows and Linux (tested using the MSVC and GCC
compilers).  

![img](https://github.com/seandewar/sdgbc/blob/master/docs/img/gui_pkmn_crystal.png?raw=true "sdgbc running Pokémon Crystal Version")
![img](https://github.com/seandewar/sdgbc/blob/master/docs/img/gui_toy_story_racer.png?raw=true "sdgbc running Toy Story Racer")
![img](https://github.com/seandewar/sdgbc/blob/master/docs/img/gui_loz_dx_dmg_mode.png?raw=true "sdgbc running The Legend of Zelda: Link's Awakening in DMG mode")

This emulator can run most Game Boy and Game Boy Color games from their ROM dump
files to an acceptable level of accuracy, including those requiring MBC1, MBC2, MBC3
(with minimal RTC) and MBC5 (without Rumble) support.

sdgbc was developed as a final year BSc. computer science project at the University
of Leicester, UK.  
A copy of the submitted dissertation and viva presentation slides can be
found in the `docs` directory.


## Short Overview Video

Click on the image below to be taken a 1 minute overview video describing sdgbc: 

[![video](https://img.youtube.com/vi/l-BObaR3Klc/0.jpg)](https://www.youtube.com/watch?v=l-BObaR3Klc "sdgbc - A Short Overview")


## Building

sdgbc is configured to be built using CMake (version 3.5 or above). The code relies
on some C++14 features, and thus, requires a C++14 standard-compliant compiler.  

The following instructions assume some familiarity with CMake. Running `cmake-gui`
or `ccmake` is recommended to easily follow these instructions:


### Building for Windows

*The following has been tested using CMake 3.5, MSVC 14, wxWidgets 3.0.1 and SFML
2.4.1:*

1. Launch CMake.
2. Set the source code directory to the sdgbc repository root directory.
3. Set the binary output directory to any directory you wish.
4. Create a `PATH` cache variable called `SFML_ROOT`. Set it to root directory of
your SFML installation.
5. Create a `PATH` cache variable called `wxWidgets_ROOT_DIR`. Set it to root
directory of your wxWidgets installation.
6. Create a `PATH` cache variable called `wxWidgets_LIB_DIR`. Set it to the
`lib\vc140_dll` directory of your wxWidgets installation.
7. Configure the project using the `Visual Studio 14 2015` generator.
8. Generate the project file. A `sdgbc.sln` Visual Studio solution file will be
created inside of the specified binary output directory.

After opening the solution file in Visual Studio, press Ctrl+F5 to build and run
the software (without debugging).


### Building for Linux

*The following has been tested using CMake 3.5, GCC 5.4, wxWidgets 3.0.1 and SFML
2.3.1 on X11:*  
**(You may experience some X11 compatibility issues running the software with any
version of SFML greater than 2.3.1!)**

1. Launch CMake.
2. Set the source code directory to the sdgbc repository root directory.
3. Set the binary output directory to any directory you wish.
4. Create a `PATH` cache variable called `SFML_ROOT`. Set it to root directory of
your SFML installation.
5. Configure the project using the `Unix Makefile` generator.
6. Generate the makefile. It will be created inside of the specified binary output
directory.

Using a shell, change your current working directory to the binary output directory
and run `make`.  
This can be done using sh or bash by running the following command:

```bash
cd BINARY_DIR_PATH && make
```


## Running

The emulator can be ran in a GUI mode by simply opening the built `sdgbc` executable
file. The emulated joypad can be interacted with using the keyboard as follows:

| Joypad Key | Keyboard Key |
| ---------- | ------------ |
| D-pad Keys | Arrow Keys   |
| B          | Z            |
| A          | X            |
| Start      | Return       |
| Select     | Shift        |

![img](https://github.com/seandewar/sdgbc/blob/master/docs/img/cpu_cmd.png?raw=true "sdgbc running with the --cpu-cmd command-line argument")

Additionally, a command-line interface for debugging the emulated CPU is
available for use. It can be accessed by passing the `--cpu-cmd` argument to
the program. A list of commands can be found by executing the `help` command.

Optionally, a path to a GB or GBC ROM file can be given via the command-line
arguments for both modes:
* `sdgbc ROM_PATH` *(for the main GUI mode)*
* `sdgbc --cpu-cmd ROM_PATH` *(for the command-line CPU debugger mode)*


## License

The software is distributed using the MIT license as follows:

```
The MIT License (MIT)

Copyright (c) 2017-2018 Sean Dewar (seandewar @ Github)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
