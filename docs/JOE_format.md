This format is based on the md2 format but with a few enhanced features. It can be written to using a python export script for the Blender 3D modeling package.

Technical specification
-----------------------

"JOE" is a binary file format in which multi-byte values are expressed with the little-endian byte order. This section details version 3 of the file format.

### Data Type Map

The following table explicitly defines the various data types used in a JOE file.

| Identifier | Detailed Description                                                                                         |
|------------|--------------------------------------------------------------------------------------------------------------|
| int        | 32-bit signed integer                                                                                        |
| short      | 16-bit signed integer                                                                                        |
| short3     | Three consecutive shorts forming an array                                                                    |
| float      | 32-bit floating-point value. Unsure of the exact format... probably whatever is implemented by x86 machines. |
| float3     | Three consecutive floats forming an array.                                                                   |

### File Header

This block of information initiates every file.

| data type | block offset | name        | description                                                                                                                               |
|-----------|--------------|-------------|-------------------------------------------------------------------------------------------------------------------------------------------|
| int       | 0            | magic       | **844121161**(0x32504449): number used to identify the file as a JOE file. Currently unchecked by vdrift.                                 |
| int       | 4            | version     | report the file version that this file conforms to. This specification details version 3 of the format.                                   |
| int       | 8            | num\_faces  | every frame is expected to contain the same number of faces (polygons) this value specifies how many. This is currently limited to 32000. |
| int       | 12           | num\_frames | Presumably, this details the number of frames used in an animation. Currently constrained to "1"                                          |

### Frame Format

details a single configuration of a model. "File Header.num\_frames" frames follow the file header.

| data type | block offset | name            | description                                           |
|-----------|--------------|-----------------|-------------------------------------------------------|
| int       | 0            | num\_verts      | the number of vertices used in this frame             |
| int       | 4            | num\_textcoords | the number of texture coordinates used in this frame. |
| int       | 8            | num\_normals    | the number of vertex normals used in this frame.      |

after each frame header, you will find

-   File Header.num\_faces Face blocks
-   num\_verts Vertex blocks
-   num\_normals Vertex blocks
-   num\_textcoords Texture Coordinate blocks

### Face Format

Each frame as "File Header.num\_faces" face records immediately following the Frame header (decribed above). A face is basically several lists of indices in to vertex, normal, and text coord arrays described in following sections.

For this version, a face is always a triangle ( i.e. a three-vertex polygon ).

| data type | block offset | name         | description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|-----------|--------------|--------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| short3    | 0            | vertexIndex  | Indeces in to the vertex array. These two constructs together define the location of each corner of the face in 3d space.                                                                                                                                                                                                                                                                                                                                                                                                |
| short3    | 6            | normalIndex  | Indeces in to the normals array. These two constructs help define how light sources interact with the face at each corner. Normals also help define which side of a face is "front".                                                                                                                                                                                                                                                                                                                                     |
| short3    | 12           | textureIndex | Indeces in to the texture coordinate array. These two constructs together help define which portions of a texture image map to each corner of the face. Note that in the future there may be a multiple of 3 entries in this array to accomidate applying many textures to a single face. Which images are used as textures by a particular model are defined by convention and explained in [Car files and formats](Car_files_and_formats.md) and [Track files and formats](Track_files_and_formats.md) |

### Vertex Format

A vertex block simply contains a three-element float array. Vertex blocks may have a misleading name because they are used for both vertices and normals.

| data type | block offset | name   | description                                                                                                     |
|-----------|--------------|--------|-----------------------------------------------------------------------------------------------------------------|
| float3    | 0            | vertex | A three-element float array defining the X, Y, and Z components of a vertex at offsets 0, 4, and 8 respectively |

### Texture Coordinate Format

A texture coordinate is a 2-dimensional coordinate used to map prtions of a texture to a model vertex.

| data type | block offset | name | description                                                                    |
|-----------|--------------|------|--------------------------------------------------------------------------------|
| float     | 0            | u    | a value between 0 and 1 indicating a position along the width of the texture   |
| float     | 4            | v    | a value between 0 and 1 indicating a position along the height of the texture. |

<Category:Files>
