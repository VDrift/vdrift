Files within a car's folder:

-   CARNAME.car: [Config file format](Config_file_format.md), list of [car parameters](car_parameters.md).
-   about.txt: text format, first line contains the car name, other lines contain additional info.
-   body.joe: [JOE format](JOE_format.md), car body geometry.
-   body00.png: PNG format, the body.joe UV texture. Additional body textures and colors can be placed in the folder using names body01.png, body02.png, etc.
-   brake.png: PNG format, an additive texture using the body.joe UV texture containing brake lights.
-   collision.joe: [JOE format](JOE_format.md), collision box geometry. Note that as of R2396, this file is no longer required.
-   engine.wav: WAVE format, engine sound at 7000 RPM.
-   glass.joe: [JOE format](JOE_format.md), geometry data for any glass elements from the car body (such as windows).
-   glass.png: PNG format, the glass.joe UV texture. Texture transparency is supported.
-   interior.joe: [JOE format](JOE_format.md), geometry data for the car's interior.
-   interior.png: PNG format, the interior.joe UV texture.
-   oem\_wheel.joe: [JOE format](JOE_format.md), geometry data to be used for the wheels.
-   oem\_wheel.png: PNG format, the oem\_wheel.joe UV texture.
-   reverse.png: PNG format, an additive texture using the body.joe UV texture containing reverse lights.

<Category:Cars>
