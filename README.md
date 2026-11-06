# OH-WES
**OH-WES** is *(yet another)* Unix-like operating system for the i386 family of
CPUs, written for the sole purpose of learning about OS development the hard
way. OH-WES aims to include a standard set of features for a desktop PC from
around, say, 1996 in terms of hardware support. This means support for serial
terminals, floppy drives, hard disk drives, VGA graphics, and basic networking.
For the time-being, I intend to keep it a text-only interface, but I'm planning
to make it rather robust.

![ohwes](https://github.com/whampson/ohwes/assets/11916560/b3301a4b-d48e-476b-9956-438748cacbff)

I'm choosing to follow the Unix-style of systems for design guidelines because
Unix is very well documented and exposes me to all parts of the system. I am not
aiming for total POSIX compliance or any kind of ABI compatibility. Maybe
one day I'll try my own design, but that's a project for another day :-)

## What does it do?
OH-WES boots into 32-bit Protected Mode and drops straight into a user-mode
(ring 3) shell baked into the kernel image. Highlights so far:

- A "pretty compatible" VT100-style text terminal and keyboard, with a real
  line discipline and termios-style ioctls.
- A read/write/open/close/ioctl driver framework, with the usual system calls
  wired up for talking to the kernel.
- Serial I/O on multiple ports, with full hardware *and* software flow control.
- A very crude virtual file system, many "files" are still hardcoded.
- A hot-pluggable PS/2 keyboard driver.

It even runs on real hardware! I've booted OH-WES on five PCs dating from
around 1997-2004, plus a pile of PC emulators.

## Roadmap
Things I still need to do before it really feels like a working OS:

- A floppy disk driver and program loader.
- Basic filesystem support (FAT-12 first, maybe ext2 later).
- Grow the page allocator into a general-purpose memory allocator.
- A proper program scheduler.

Stretch goals: getting DOOM and GTA3 to run. :-)

##  How do I use it?
### Clone the Repo
```
git clone https://github.com/whampson/ohwes
```

### Prepare the Environment
#### Install Native Build Tools (Windows)
OH-WES is built on Windows under a MINGW64 (MSYS2) environment using GNU Make
and GCC.

First, install MSYS2 from PowerShell:
```powershell
winget install msys2
```
If you do not have winget, see [Windows Package Manager for Developers](https://learn.microsoft.com/en-us/windows/package-manager/#windows-package-manager-for-developers).

Then, from an MSYS2 shell, update, then install GNU Make, the MINGW64 toolchain,
and some other tools:
```bash
pacman -Syuu
pacman -S msys/make msys/unzip msys/dosfstools mingw64/mingw-w64-x86_64-toolchain
```
Finally, open a **MINGW64** shell and [build OH-WES](#build-and-run-oh-wes).

#### Install Native Build Tools (Linux/Debian)
First, install native toolchain, debugger, and some other tools:
```bash
sudo apt-get install build-essential gdb dosfstools unzip
```

Then, [build OH-WES](#build-and-run-oh-wes) in a Bash shell.

#### Install GCC Cross Compiler
Install a GCC cross-compiler. This is used to build the operating system code.
I like to use the pre-built toolchain graciously provided by *lordmilko* at
[lordmilko/i686-elf-tools](https://github.com/lordmilko/i686-elf-tools).

From the repo root, download the archive for your platform and extract it
under a version-named directory (e.g. GCC 15.2.0):

```bash
VERSION=15.2.0
mkdir -p build/i686-elf-tools/$VERSION

# Windows (MSYS2)
curl -L -o /tmp/i686-elf-tools.zip \
  https://github.com/lordmilko/i686-elf-tools/releases/download/$VERSION/i686-elf-tools-windows.zip

# ...or Linux/WSL:
# curl -L -o /tmp/i686-elf-tools.zip \
#   https://github.com/lordmilko/i686-elf-tools/releases/download/$VERSION/i686-elf-tools-linux.zip

unzip -q /tmp/i686-elf-tools.zip -d build/i686-elf-tools/$VERSION
ln -sfn $VERSION build/i686-elf-tools/latest
```

That leaves you with:
```
ohwes/
└───build/
    └───i686-elf-tools/
        ├───latest -> 15.2.0/
        └───15.2.0/
            ├───bin/
            ├───include/
            ├───lib/
            ├───libexec/
            └───share/
```

The `latest` symlink is optional — `scripts/env.sh` checks it first, then
falls back to the versioned directories, newest first.

The cross-compiler prefix defaults to `i686-elf-` and can be overridden when
building, e.g. `make TOOLCHAIN=my-cross` (it's set in `scripts/def.sh`). The
toolchain location searched by `scripts/env.sh` can be overridden with the
`ELF_TOOLS_ROOT` environment variable.

#### Install Emulators
OH-WES runs under an emulator. **QEMU** is the default; **Bochs** is also
supported.

**QEMU (recommended):**
- **Windows:** `winget install qemu`.
- **Linux:** `sudo apt-get install qemu-system-x86`, which provides
  `qemu-system-i386` for `make run`.

**Bochs (optional):** install it only if you want `make run-bochs`:
- **Windows:** `winget install Bochs.Bochs` (or the installer from
  [bochs.sourceforge.io](https://bochs.sourceforge.io/)) — `scripts/env.sh`
  auto-detects `C:\Program Files\Bochs-*`.
- **Linux:** `sudo apt-get install bochs`, then adjust the BIOS/VGA ROM paths
  in `build/bochs/linux/bochsrc.bxrc` to match your install.

Bochs config files live in `build/bochs/<platform>/bochsrc.bxrc`.
`scripts/env.sh` detects your platform and exports `BOCHSRC`, which
`make run-bochs` passes along.

### Build and Run OH-WES
In a `bash` session, navigate to the project root, then source the environment
script (it must be sourced, not executed). This sets up your `PATH` and checks
that the necessary tools are present. It only needs to be done once per
session; re-sourcing is safe because PATH additions are idempotent and exported
overrides such as `QEMU` are preserved:
```bash
cd ohwes/
. scripts/env.sh
```

Use `make` to build and run OH-WES:
```bash
make              # build kernel binaries
make img          # build floppy disk image
make run          # boot the floppy image on QEMU
make run-bochs    # boot the floppy image on Bochs
make debug        # boot on QEMU and attach a debugger at kernel entry
make debug-boot   # boot on QEMU and attach a debugger at early boot
make floppy       # deploy kernel binaries and boot loader to /dev/fd0
```
Parallel compilation with `make -j` is supported. Make sure `qemu-system-i386`
(and `bochs`, if you use it) are on your PATH.

The emulator, debugger, and host-tool compiler executables are resolved in
`scripts/def.sh` (sourced by both `scripts/env.sh` and `scripts/run.sh`) and are
each overridable by an environment variable. For example, to point at a
non-standard Bochs install:
```bash
export BOCHS=/path/to/bochs
make run-bochs
```

#### The Run Script
Every `make run`/`make run-bochs`/`make debug` target is a thin wrapper around
`scripts/run.sh`, which launches the emulator and, in debug modes, attaches gdb.
Run `scripts/run.sh --help` for the full list. The debug modes are `kernel`,
`boot`, and `setup` (each boots QEMU paused with a gdb stub and sets
mode-specific breakpoints), plus one with no `make` equivalent — `serial`, which
attaches gdb to a *real* machine over a COM port instead of booting an emulator:
```bash
scripts/run.sh --debug serial --port com1   # or `--port /dev/ttyS1` on Linux
```

All run and debug targets also accept a `RUNFLAGS` variable that is forwarded
to `scripts/run.sh`; everything after a `--` separator is passed verbatim to
QEMU:
```bash
make run RUNFLAGS="-- -monitor stdio"     # open the QEMU monitor on stdio
make debug RUNFLAGS="--log my-qemu.log"   # redirect QEMU's debug-mode output
```
Other handy direct invocations:
```bash
scripts/run.sh --img bin/ohwes.img --debug boot --no-break
scripts/run.sh --dry-run --debug kernel -- -d int
```

### The Floppy Disk Image
You can create a floppy disk image of OH-WES by running:
```bash
make img
```
This builds my `fatfs` tool then creates a FAT-12 floppy disk image using it.
You can point other emulators to this disk image, or write it to a real floppy
disk using the `dd` command. However, this is not needed because you can just
write the files directly to the disk (see below).

### Create a Bootable OH-WES Floppy Disk 💾
You may also bypass creating an image and write the files directly to disk if your system
has FAT-12 support:
```bash
make format-floppy         # DESTROYS ALL DISK CONTENTS
make floppy                # writes to a mounted floppy; mountpoint is
                           # chosen per platform (e.g. /mnt/floppy on Linux,
                           # /a on MSYS2), overridable via
                           # `make floppy DISK_MOUNT=/mnt/fd`
```
The `format-floppy` command only needs to be run if the disk is not FAT-12 formatted or if
the kernel size changes considerably, as the bootloader does not support fragmentation. If you
experience weird glitches while using a real floppy disk, try formatting the disk first. Then
again, it may be bugs in my code... :-) have fun!
