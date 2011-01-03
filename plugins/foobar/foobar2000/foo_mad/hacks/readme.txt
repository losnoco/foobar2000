Place this set of files inside your patched libmad-0.15.1b directory.

(Set your path to include your MinGW binaries directory.)

(Optionally recompile objfix2.c and/or alloca.nas.)

(Optionally change the C_OPT line in build.bat to suit your setup.)

Execute "build profile" to produce a minimad.exe suitable for profiling
it. If you don't want it to hog your system, comment out the priority
adjusting code for the duration of the profiling process. You'll want to
run it on at least one MP2 file and at least one MP3 file.

Execute "build" to produce a set of object files.

Execute "convert" to produce a release libmad.lib, which will be copied to
msvc++\Release. Whatever projects you have depending on the libmad
project file should link to the new library. If MSVC++ attempts to rebuild,
rerun "convert" and try again. *shrug*
