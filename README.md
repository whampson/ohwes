# OH-WES
**OH-WES** is a toy operating system for the i386 architecture written by
Wes Hampson. OH-WES is currently in early development.

## Cloning
```
> git clone --recurse-submodules https://github.com/whampson/ohwes
```

## Preparing the Environment
### Windows
OH-WES is built on Windows under a MINGW32 environment using a GNU Make and GCC.

1. Install MSYS2 and QEMU.
```
PS> winget install msys2
PS> winget install qemu
```
If you do not have winget, see [Windows Package Manager for Developers](https://learn.microsoft.com/en-us/windows/package-manager/#windows-package-manager-for-developers).

2. Update MSYS2. You may need to run this command twice if you are prompted to
restart the shell.
```
MSYS2> pacman -Syuu
```

3. Install 32-bit GCC cross-compiler. This will be used to build the operating system code. Currently, we are using GCC 7.1.0 graciously pre-compiled by *lordmilko* at [lordmilko/i386-elf-tools](https://github.com/lordmilko/i686-elf-tools).
    1. Download [i686-elf-tools-windows.zip](https://github.com/lordmilko/i686-elf-tools/releases/download/7.1.0/i686-elf-tools-windows.zip)
    2. Extract .zip to `tools/bin/i686-elf-tools-windows`

4. Install 32-bit native toolchain. This will install GNU Make and GCC, among
other tools, used for parsing the Makefile and building external tools.
```
MINGW32> pacman -Syu mingw32/mingw-w64-i686-toolchain
```

5. Use a MINGW32 shell to build OH-WES (see [Building OH-WES](#building-oh-wes))

## Building OH-WES
Open a Bash shell (MINGW32 on Windows) and navigate to the project root, then
source the environment script. This script will ensure your PATH is set
correctly and that the necessary tools are accessible (this only needs to be
done once per session).
```
> cd ohwes/
> . scripts/env.sh
```

Now, simply run `make` to begin building OH-WES!
```
> make
```
