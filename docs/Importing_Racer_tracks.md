Directions for importing Racer tracks
-------------------------------------

-   Unzip and put racer files in temporary folder *RACER\_TP* (temporary track path). Make sure the path has no spaces (dof2joe doesn't like spaces).
-   Create new folder for track in track editor folder *TRACKEDITOR\_TP*.
-   Convert all texture names to lowercase in *RACER\_TP* via

        find . -type f -name \*.tga|sort -r|awk '{f=tolower($1);
        if(f!=$1 && p[f]!=1){print "mv "$1" "f}p[$1]=1}' | /bin/sh

-   Make folder ***TRACKEDITOR\_TP*/objects/**
-   Run the following command:

        dof2joe/dof2joe -p TRACKEDITOR_TP/objects/ RACER_TP/*.dof

If there are thousands of .dof files, you may have to do this in steps to avoid a "too many arguments" error from your shell. This will take a while. Textures are automatically converted using nconvert. No filenames should have spaces.

-   Open track editor **data/tracks/editor.config** and set active track to *TRACKEDITOR\_TP*.
-   Run the track editor. Trace the roadways and mark the starting position (press H for help).
-   cd to the ***TRACKEDITOR\_TP*/objects** folder (this is important, the packfile stores relative paths) and run

        joepack/joepack -c objects.jpk *.joe

-   Copy *TRACKEDITOR\_TP* into the main VDrift tracks folder *VDRIFT\_TP*. Erase ***VDRIFT\_TP*/objects/\*.joe** (since they are in the pack file).
-   Add ***VDRIFT\_TP*/about.txt** and ensure that the first line is the name of the track.
-   Run VDrift and check out what the track looks like in-game. Note that you will only be able to drive on the roadways you defined in the track editor since no other surfaces have been flagged as collideable. Also take a screenshot for the track selection screen.
-   Add the track selection screenshot to ***VDRIFT\_TP*/trackshot.png** (hopefully these png files will be moved into the folders of the individual tracks soon).
-   Open up all of the texture files in ***TRACKEDITOR\_TP*/objects** and review which textures belong to objects that should be collide-able (roads and walls), have full brightness (trees), be mipmapped (fences and fine transparent objects should not be mipmapped), or be skyboxes.
-   Any textures that have transparent areas are usually colored \#FF00FF in the Racer textures. You'll need to make these truly transparent in the PNG files. An easy way to do this is to use ImageMagick. Use the commands

        mogrify -transparent rgb`\(255,0,255\)` *.png
        mogrify -fill rgb`\(128,128,128\)` -opaque rgba`\(255,0,255,0\)` *.png

This can be scripted to speed things up of course.

-   Set the correct object properties using the **trackeditor/listedit** [Listedit tool](Listedit_tool.md).
-   Done!

<Category:Tracks>
