# ika 2020

Updates for 2020, building in windows
* Only Debug|Win32 configuration works.
* Put the python31_d.dll i've given you in ika/engine/Debug (later I will fix this, I didn't want to commit big WIP dlls)
* Unload the ikaMap, iked, and rho projects (they dont work yet)
* Ika really needs to be updated more for python3. I see some stuff returning "bytes" that should be returning strings, it makes using it a pain.
* Set ika project properties > debugging > working directory to c:\ika\engine\Debug and place all needed game files there (it's the easiest way)

QUESTIONS:
* How did ika distribute the python runtime libraries? do those need to be copied into Debug also?

# ika

ika is a game engine I worked on between 2001 and 2007 or so.  It uses SDL and OpenGL for graphics and Python for
scripting.

It is hosted here primarily for archival purposes.

I think it was pretty cool in its day.
