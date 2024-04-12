# BC64-Blitz-crosscompiler-for-the-Commodore-64

BC64 is a cross-development tool for the Commodore 64. It accepts a BASIC sourcefile (crunched, or tokenized) as saved by
the C64, and compiles it to a ready-to-run program. It's based on the famous Blitz! runtime/pcode interpreter.

usage : bc64 inputfile /Ooutputfile /Llistfile /G

If no output- or listfile is given, the defaultnames <inputbas>.cp  and <inputbase>.lst will be used.
/G : this moves the output to $4000, so the $2000..$3fff area is free for graphics data in VIC bank 0

Sources are included, the .ZIP file contains the sources and the windows executable bc64.exe

There's no makefile, as my development environment won't be the same as yours, but building the .exe is simple :
just compile all .c files and link them.

Ofcourse VICE and other emulators will run the compiled program as well.

The crunched inputfile can be either created on the C64/emulator, or you can use someting like CBMprgStudio to create it.
I'll add a cross-development crunch utility later.
