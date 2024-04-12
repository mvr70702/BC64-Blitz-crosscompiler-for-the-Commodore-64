# BC64-Blitz-crosscompiler-for-the-Commodore-64

BC64 is a cross-development tool for the Commodore 64. It accepts a BASIC sourcefile (crunched, or tokenized) as saved by
the C64, and compiles it to a ready-to-run program. It's based on the famous Blitz! runtime/pcode interpreter.

usage : bc64 inputfile /Ooutputfile /Llistfile /G

If no output- or listfile is given, the defaultnames <inputbas>.cp  and <inputbase>.lst will be used.
/G : this moves the output to $4000, so the $2000..$3fff area is free for graphics data in VIC bank 0

I included a feature from the Blitz version for the C128 : forced integers. In BC64 you can use integer
variables in a FOR loop, i.e FOR i% = 1 TO 100 STEP...
You cannot test this before compiling though, as BASIC won't accept it. To overcome this problem, BC64 can
force floating point variables to be treated as integers, using a special REM statement:
REM **FI var,var,..
So in the FOR example above, you would add "REM **FI I" to your BASIC program.(BEFORE the variable is used !)

Sources are included, the .ZIP file contains the sources and the windows executable bc64.exe

There's no makefile, as my development environment won't be the same as yours, but building the .exe is simple :
just compile all .c files and link them.

Ofcourse VICE and other emulators will run the compiled program as well.

The crunched inputfile can be either created on the C64/emulator, or you can use someting like CBMprgStudio to create it.
I'll add a cross-development crunch utility later.
