# OH-WES
**OH-WES** is an OS for Wes! Written by me for the sole purpose of learning
about operating systems the hard way *(also because I find this sort of thing
fun)*. OH-WES is a 32-bit protected-mode operating system for the Intel 386 and
is currently in early development.

![ohwes](https://github.com/whampson/ohwes/assets/11916560/b3301a4b-d48e-476b-9956-438748cacbff)

## What does it do?
Not much yet, on the surface anyways. It features a somewhat VT100-compatible
text terminal with working keyboard input. Currently, it runs in ring 3
Protected Mode in a crude userland baked into the kernel image (no program loader
yet). It can talk to the kernel through a system call interface and features the
usual read/write/open/close/ioctl driver framework. Right now, the RTC and console
are the the only supported devices. :-) It runs on real hardware (tested on three
different PCs from 1997-2004, at least) and can theoretically run on any PC with
an Intel 386 processor (tested with various PCem emulations).

Things I still need to do before it's *really* a working OS are: create a floppy
disk driver and program loader, implement basic filesystem support (FAT-12 at first,
maybe ext2 later), enable virtual memory and build a dynamic memory allocator, allow
for terminal switching, and create a program scheduler. Eventually I'd like to try
hooking up a a real serial terminal and see if I can interact with the system over the
serial I/O port, as well as play with the GPU and attempt to implement a 3D graphics
driver. Maybe even port DOOM or GTA3? :-)

##  How do I use it?
### Clone the repo
```
> git clone --recurse-submodules https://github.com/whampson/ohwes
```

### Prepare the Environment
#### Install GCC Cross Compiler
Install a GCC cross-compiler. This will be used to build the operating system
code. Currently, we are using GCC 7.1.0 graciously pre-compiled by *lordmilko*
at [lordmilko/i386-elf-tools](https://github.com/lordmilko/i686-elf-tools):
1. Download [i686-elf-tools-windows.zip](https://github.com/lordmilko/i686-elf-tools/releases/download/7.1.0/i686-elf-tools-windows.zip) or [i686-elf-tools-linux.zip](https://github.com/lordmilko/i686-elf-tools/releases/download/7.1.0/i686-elf-tools-linux.zip).
2. Extract .zip contents to `build/i686-elf-tools/`
    * Your build directory structure should look like this:
        ```
        ohwes/
        └───build/
            └───i686-elf-tools/
                ├───bin/
                ├───include/
                ├───lib/
                ├───libexec/
                └───share/
        ```

#### Install Native Build Tools (Windows)
OH-WES is built on Windows under a MINGW32 environment using a GNU Make and GCC.

1. Install MSYS2 and QEMU.
```
PS> winget install msys2
PS> winget install qemu
```
If you do not have winget, see [Windows Package Manager for Developers](https://learn.microsoft.com/en-us/windows/package-manager/#windows-package-manager-for-developers).

2. Update MSYS2. You will need to run this command again if you are prompted to
restart the shell.
```
MSYS2> pacman -Syuu
```

3. Install native build tools.
```
MSYS2> pacman -Syu msys/make
MSYS2> pacman -Syu mingw32/mingw-w64-i686-toolchain
```

4. Use a MINGW32 shell to build OH-WES (see [Building OH-WES](#building-oh-wes)).

#### Install Native Build Tools (Linux/Debian)
1. Install build-essential
```
> apt-get install build-essential
```

### Build OH-WES
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

### Run OH-WES with QEMU or Bochs
To run with QEMU, install QEMU and make sure `qemu-system-i386` is in your path, then:
```
> make run
```

For Bochs, just install Bochs and make sure `bochs` is in your path, create a bochsrc.bxrc
file and put it in the project root, then:
```
> make run-bochs
```

### Create an OH-WES Floppy Disk Image
You can create a floppy disk image of OH-WES by running the following:
```
> make img
```

This builds my `fatfs` tool then creates a FAT-12 floppy disk image using it.
You can point other emulators to this image, or write it to a real floppy disk
via the `dd` command.

### Create a Bootable OH-WES Floppy Disk 💾
You may also bypass creating an image and write the files directly to disk if your system
has FAT-12 support:
```
> make format-floppy         # DESTROYS ALL DISK CONTENTS
> make floppy                # assumes disk is mounted at /a/
```
The `format-floppy` command only needs to be run if the disk is not FAT-12 formatted or if
the kernel size changes considerably, as the bootloader does not support fragmentation. If you
experience weird glitches while using a real floppy disk , try formatting the disk first. Then
again, it may be bugs in my code... :-) have fun!
