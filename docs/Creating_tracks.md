Video Tutorial
--------------

NaN has produced this nifty video tutorial (Windows, but mostly applicable to Linux too): <http://www.youtube.com/watch?v=oju-vKVVaho>

What you need
-------------

-   VDrift
-   VDrift level editor
-   Blender 2.33 or higher. Tested on 2.45 with Python 2.5.1
-   Blender JOE export script. Get that here: <https://github.com/VDrift/blender-scripts>

Get the **export-all-joe-0.3.py** script. The difference in the files is that one exports all the object in the scene and the other only exports the one that is selected.

Getting the level editor
------------------------

In the Linux console, copy *everything* below:

    git clone https://github.com/VDrift/trackeditor vdrift-trackeditor

Directions for creating tracks
------------------------------

-   Model the scene. See [3D modeling](3D_modeling.md) for resources to help with this step.
-   If you use a 3D editor other than blender, import the track into blender.
-   Use the **export-all-joe-0.3.py** blender export script to export all objects. This script can be found in the VDrift art repository under the tools folder. The export script creates a number of **.joe** files and a **list.txt** file. The **list.txt** file may be named **somename-list.txt**, in which case you should rename it to **list.txt**. At least one **.joe** file should get created for the curve track. Also verify that **list.txt** is mentioning all the **.joe** files. An empty **list.txt** will not load anything in the editor.
-   Create new folder for track in track editor folder *TRACKEDITOR\_TP* (if your track is called parkinglot, the path could be **/home/joe/trackeditor/data/tracks/parkinglot**).
-   Make folder ***TRACKEDITOR\_TP*/objects/**
-   Copy all of the **.joe** files and the **list.txt** file to ***TRACKEDITOR\_TP*/objects/**
-   Open track editor **data/tracks/editor.config** and set active track to *TRACKEDITOR\_TP*.
-   Create a ***TRACKEDITOR\_TP*/track.txt** file with at least a line "cull faces = on". **track.txt** is modified by track editor to add all starting positions and lap sequence points. Read the track editor inhelp for more information.
-   Run the track editor. Trace the roadways and mark the starting position (press H for help). A track may not always appear on the screen. Move the mouse around and you could see it in the black space. The first time, check the console output of track editor for any warnings.

|                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Why trace roadways?** ![](Track-smoothing.png "fig:Track-smoothing.png") This is a visual depiction of the track smoothing that occurs when tracing a roadway. Imagine this image is showing the track surface from a side view. The black lines represent the track mesh, and the red lines represent the bezier patches. Once the track has been traced in the track editor, VDrift will use the red lines to do collision instead of the black lines. On the top, this represents a dip in the road. You can see how collision using the red line will behave properly. On the bottom, this represents a bump road. You can see that the red line doesn't change the magnitude of the bumps, it just makes them realistically smooth instead of unrealistically pointy. |

-   `cd` to the **trackeditor/joepack** folder. Compile the joepack tool by running

        scons

-   `cd` to the ***TRACKEDITOR\_TP*/objects** folder (this is important, the packfile stores relative paths) and run

        /path/to/trackeditor/joepack/joepack -c objects.jpk *.joe

-   If you want, this command will show you the files in the joepack to allow you to verify the previous step worked correctly:

        /path/to/VDrift-trackeditor/joepack/joepack -l objects.jpk

-   Copy *TRACKEDITOR\_TP* into the main VDrift tracks folder *VDRIFT\_TP* (for example **/home/joe/vdrift/data/tracks/parkinglot**). Erase ***VDRIFT\_TP*/objects/\*.joe** since they are all in the pack file now.
-   Add ***VDRIFT\_TP*/about.txt** and ensure that the first line is the name of the track. You should put information about the track author, where the track came from, etc in the second line and on.
-   Run VDrift and check out what the track looks like in-game. Note that you will only be able to drive on the roadways you defined in the track editor since no other surfaces have been flagged as collideable. Also take a screenshot for the track selection screen.
-   Create a track selection image (a 512x512 png file works best) and save it to ***VDRIFT\_TP*/trackshot.png**
-   Open up all of the texture files in ***TRACKEDITOR\_TP*/objects** and review which textures belong to objects that should be collide-able (roads and walls), have full brightness (trees), be mipmapped (fences and fine transparent objects sometimes look better when not mipmapped), or be skyboxes.
-   Set the correct object properties using the **trackeditor/listedit** tool (more documentation to come).
-   Done!

Other Notes
-----------

-   A track should be of a minimum size for loading within VDrift. If the editor is not allowing to adjust the camera poistions correctly, probably the track is very small. Scale everything in the blender twice or more and try again.
-   Starting points are set within the track editor. After the track is loaded, position the track like you were in the car on the track i.e. first person view. Press L to save the position as a starting position. Continue to add positions depending on your track. Also add a lap sequence i.e. lap starting/ending point track.
-   Track editor does not paint or mark the starting points or lap sequence numbers on the track. These are only saved in track.txt. Also, the editor will always continue adding more starting positions if track.txt had some already. Therefore, consider deleting everything in **track.txt** if you wish to reedit the positions.
-   A .joe file gets created when the track has a texture.
-   The export-joe script should be loaded within blender along with the track, and executed.

<Category:Tracks>
