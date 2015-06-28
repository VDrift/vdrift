This article gives a rough idea of what it takes to create a car for VDrift.

Get the Art Tools
-----------------

Download the [Blender export scripts](https://github.com/VDrift/blender-scripts) from [GitHub](Getting_the_development_version "wikilink").

Create Models
-------------

You will need to create 3D models using Blender for the car's body, the glass pieces, and the wheels. Each different model must be a separate Blender object and each must have a separate texture. Models must be entirely made of triangles. Units of the models are in meters.

### Coordinate Systems

See [Coordinate systems](Coordinate_systems "wikilink")

### Shading and Smoothing

Faces (and perhaps individual vertices) can be set to smooth or solid shading, and that will get exported in the joe file. The best way to do smoothing is to select all of the faces, set solid, and then select groups that should be blended together and do set smooth individually. That makes it so that smooth parts that intersect in a hard edge have correct normals. Don't use any double-sided faces.

### Body Model

In the model pack you will find a Blender file "test.blend". This is the default car model. The glass is one object and the rest of the car is one object. This allows you to export the glass as a "glass.joe" file and the rest of the car as a "body.joe" file. The car should be in the neighborhood of 3500 faces (car and windows combined), although less is of course possible and more is probably also acceptable. The body can be placed anywhere, although by convention the body is usually placed so that the center of the model is near the origin.

### Interior Model

The interior model should fit inside the body model to provide the inside of the car. This is a separate model so it can appear flat instead of shiny like the painted exterior. The interior model should share the center point or object handle of the body model so they fit together perfectly without being translated.

### Wheel Model

The file "wheel.blend" is the default wheel model. Try to keep your wheel model under 1000 faces. The wheel model must be centered at the origin.

Texture the Models
------------------

A single UV map can be used per object for texturing. The .png files are the textures for each associated .joe model. The textures must be 512x512 24- or 32-bit PNG images. Their names should be the same as the model they coordinate with except for the .png extension at the end. For example, the texture for the model "body.joe" must be named "body.png".

### Brake lights

Add a texture "brake.png" that is the same as the body graphic, but the brake lights are now on and the rest of the file is black. This should be a 24-bit png file (no alpha channel).

Export the Models
-----------------

Using the Python scripts ("export-joe-0.3.py" and "export-all-joe-0.3.py") you can export objects modeled in Blender to JOE format. The mesh needs to be all triangles before export. The currently selected object is exported. Object level transformations are not exported, so make sure any rotation or moving or scaling is done in edit mode, not object mode. You can actually position the car wherever you want, but all of your positioning must match up with the values in the car's .car file. The default exporter setting of 1 frame is what you should use. The export-all script exports all the objects to files based on the objects' names.

About file
----------

Write a short text file about your car. This goes in the about.txt file. This information is displayed in the car selection menu. Please include information such as authorship and license. See the other cars for examples.

Car Definition File
-------------------

Finally you must write car definition file, which contains all of the [car parameters](car_parameters "wikilink"). You can start by copying tools/cars/blank.car and entering values to fit your vehicle. Try to find accurate information regarding the specifications of the car and duplicate it as closely as possible. The units are all in [MKS](http://scienceworld.wolfram.com/physics/MKS.html) (meters, kilograms, seconds). It might also help to read [*The Physics of Racing*](http://www.miata.net/sport/Physics/) by Brian Beckman.

Needless to say, it requires a lot of knowledge to create a car definition file from scratch so it is suggested that you try using values from other cars. Many of the [community-made cars for Racer](http://www.racer-xtreme.com/) have very similar values that can be used for VDrift.

Locations
---------

All the files needed for a car go into the directory **data/cars/car\_name/**, where *car\_name* is the short name of the car.

Need Help?
----------

Try the related wiki articles on [car files and formats](car_files_and_formats "wikilink"), [car parameters](car_parameters "wikilink"), or the [car modeling tutorial](car_modeling_tutorial "wikilink"). If you get stuck, feel free to ask questions in our [VDrift.net Help forum](http://vdrift.net/Forum/viewforum.php?f=1) or on [VDrift IRC](http://vdrift.net/staticpages/index.php?page=irc-chat)

Contribute
----------

Once you create a new car please contribute it back to the game so that others can enjoy it. The best way to do this is to start a thread in our [Development forum](http://vdrift.net/Forum/viewforum.php?f=5) or to create an account on [cars.vdrift.net](http://cars.vdrift.net) and upload it there.

<Category:Cars>
