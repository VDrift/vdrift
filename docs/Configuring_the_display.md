Configuring the display is done by editing [VDrift.config](VDrift_config "wikilink") manually, or by changing the settings in the Options -&gt; Display and Options -&gt; Display -&gt; Advanced menus.

Display Options
---------------

### Resolution

-   type: integer pair
-   settings: display.width, display.height
-   values: depends on the file **vdrift/data/lists/videomodes** and the resolutions your video card makes available

Change the resolution of the game display.

See [Adding video modes](Adding_video_modes "wikilink") for how to make new video modes available to choose from.

### Fullscreen

-   type: boolean
-   setting: display.fullscreen
-   values: on, off

Make the game take up the entire screen.

### Speed Units

-   type: boolean
-   setting: display.mph
-   values: on = "MPH", off = "km/h"

Change the units that speed is displayed in.

### Framerate Counter

-   type: boolean
-   setting: display.show\_fps
-   values: on, off

Enable/disable the framerate counter.

### Heads Up Display

-   type: boolean
-   setting: display.show\_hud
-   values: on, off

Enable/disable the heads up display.

### Menu Skin

-   type: string
-   setting: display.skin
-   values: default "simple", the name of any directory in **vdrift/data/skins/**

Change the graphics and layout of the VDrift menus.

### Input Graph

-   type: boolean
-   setting: display.input\_graph
-   values: on, off

Visualize the steering and acceleration/braking on screen.

Advanced Display Options
------------------------

### Color Depth

-   type: integer
-   setting: display.depth
-   values: 16, 32

Adjust the amount of colors available.

### Texture Size

-   type: string
-   setting: display.texture\_size
-   values: "small", "medium", "large"

Change the size of the textures displayed.

### View Distance

-   type: floating-point
-   setting: display.view\_distance
-   values: any positive decimal number of meters

Change the maximum view distance.

### Anisotropic Filtering

-   type: integer
-   setting: display.anisotropic
-   values: depends on your video card

Set anisotropic filtering level for textures.

### Antialiasing

-   type: integer
-   setting: display.antialiasing
-   values: depends on your video card

Set the full scene antialiasing level.

### Car Shadows

-   type: boolean
-   setting: display.car\_shadows
-   values: on, off

Draw simple static shadows beneath the cars.

### Field of View

-   type: floating-point
-   setting: display.FOV
-   values: any positive decimal number

Field of view angle in the vertical direction.

### Lighting Quality

-   type: integer
-   setting: display.lighting
-   values: 0, 1

Set how good the lighting looks during gameplay. 0 is Low, which is totally static lighting. 1 is Medium, static cube-mapped lighting.

### Reflection Quality

-   type: integer
-   setting: display.reflections
-   values: 0, 1, 2

Set how good the reflections look during gameplay. 0 is Low, static sphere-mapped reflections. 1 is Medium, static cube-mapped reflections, and 2 is High, dynamic cube-mapped reflections.

<Category:Configuration>
