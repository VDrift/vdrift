Cars in VDrift are composed of multiple components. Their graphical representation consists of geometry(mesh), textures and a draw flag which denotes whether the object is to be handled as opaque, transparent or emissive. The setup is done in [car parameters](car_parameters.md) files <CARNAME>.car.

Geometry(meshes) can share textures. The number of meshes and textures should be kept low for performance reasons.

Example of a graphical component setup:

    [some_car_part]
    texture = diffuse.png, misc1.png, misc2.png  # (required) the first texture(diffuse.png) is required, additional textures(misc1.png, misc2.png)are optional
    mesh = model.joe                             # (required) geometry (mesh)
    position = 0.736, 1.14, -0.47                # (optional) relative mesh position
    rotation = 0, 0, 30                          # (optional) relative mesh orientation 
    scale = -1, 1, 1                             # (optional) mesh scale factor
    color = 0.8, 0.1, 0.1                        # (optional) base diffuse color; blended with diffuse texture using alpha channel as mask
    draw = transparent                           # (optional) draw flag: opaque, transparent, emissive; opaque is default

Geometry
--------

Car geometry is stored in VDrift native [JOE format](JOE_format.md). Import/export scripts for [Blender](http://www.blender.org) are available here: <https://github.com/VDrift/blender-scripts>

#### Shared Geometry

When loading a mesh VDrift first checks cars/<CARNAME> directory and then falls back to the carparts (and trackparts) directory. The assets in this directory are shared, can be used by all cars.

#### Generated Geometry

Car tire geometry \[wheel.<id>.tire\] (with id = {fl, fr, rl, rr}) will be auto-generated when no mesh is provided. The dimensions are derived from the provided tire parameters.

    [wheel.fl.tire]
    texture = tire.png        # (optional) enables auto-generated tire mesh
    mesh = tire.joe           # (optional) overrides auto-generated tire mesh

Car brake geometry \[wheel.<id>.brake\] will be auto-generated from brake dimensions when no mesh is provided.

    [wheel.fl.brake]
    texture = brake_disk.png  # (optional) enables auto-generated disk mesh
    mesh = brake_disk.joe     # (optional) overrides auto-generated brake disk mesh

Car wheel \[wheel.<id>\] mesh contains only the wheel disc at unit size (0.5 m radius). The rim is auto-generated from tire dimensions. The disk mesh is scaled to fit it. To be able to use a custom wheel mesh (which already has a rim) the property genrim has to be set to false.

    [wheel.fl]
    texture = wheel.png       # (required) wheel (disk + rim) diffuse texture
    mesh = wheel.joe          # (required) wheel disk mesh; if genrim=false wheel mesh with rim
    genrim = false            # (optional) enable auto-generated wheel rim and scaling; default true

Textures
--------

VDrift supports multi-texturing for more realistic geometry surface graphics. Texture are RGBA images in PNG format. As of April 2013 DDS(DXT1-3) is also supported.

-   1. Texture (required): Diffuse color (diffuse albedo or diffuse reflectance) in the RGB channels and Color Blending mask in the A channel.
-   2. Texture (optional): Specular reflection (fresnel reflection coeff at 0 deg) in the RGB channels and Glossiness (surface roughness) in the A channel.
-   3. Texture (optional): Normal map (tangent space normals) in the RGB channels.

#### Shared Textures

When loading a texture VDrift first looks for it in cars/<CARNAME> directory and then falls back to the carparts (and trackparts) directory.

#### Car Skins

The diffuse texture of the car \[body\] component can be swapped by the player against textures placed in <CARNAME>/skins directory.

<Category:Cars>
