# cloth-shader
(work in progress!)

The files in the folder `mitsubadir` show the additions made to the the mitsuba source to add cloth shading functionality.

In order to compile move the added files into their respective directories in the mitsuba source. For bsdfs, do not foroget to add them to the build process by modifying `mitsubadir/src/bsdfs/SConscript` and adding the desired shaders.