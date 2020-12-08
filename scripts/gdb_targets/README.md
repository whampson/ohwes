# GDB Target Descriptions
GDB used in conjunction with QEMU is quirky sometimes. Newer versions seem to
not like debugging 16-bit x86 code. Some helpful folks over on StackOverflow
figured out a way to get GDB to play nicely with 16-bit Real Mode code. Thanks
to Matan Shahar and Kirill Spitsyn for these handy target description files!
    https://stackoverflow.com/a/61981253
    https://gist.github.com/MatanShahar/1441433e19637cf1bb46b1aa38a90815

To use, launch GDB, connect to the QEMU target, then run the following:
    (gdb) set tdesc filename scripts/gdb_targets/i386-16bit.xml
    (gdb) set architecture i8086

You may not need to run the latter line, but if you are still seeing 32-bit
instructions when you *know* you're running 16-bit code, this should do the
trick. Happy Real Mode debugging! :-)
    
    - Wes
