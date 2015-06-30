Listedit is part of the [Track editor](Track_editor.md) tools that simplifies working on [list.txt](Track_files_and_formats.md) files. It is a command line style program.

list.txt format
---------------

The list.txt file contains object definitions in sections, separated by a blank line. A description of each field in the list.txt file can be found at the following location:

<https://github.com/VDrift/trackeditor/blob/master/listedit/format.txt>

The numbers next to each of the lines is important to the way the listedit program works.

Commands
--------

The basic commands are

-   **load**
-   **save**
-   **ls**
-   **set**
-   **quit**
-   **addparam**

Load, save, and quit are obvious. You can supply arguments to load and save to specify the file if you want. The ls and set commands work using the object list file format.

### ls

The first argument to ls is the object property you want to search. For example, specify 1 if you want to search objects based on the texture filename. The second argument to ls is what you want to search for. You can use \* and ? wildcards. For example, to find all of the objects that use a texture called sky\*, you'd do:

`ls 1 sky*`

To find all of the objects that have the skybox property set to true (1), do:

`ls 4 1`

### set

The **set** command is similar to **ls**. It allows you to both search for objects and set their properties in one step. The first two arguments to set are exactly the same as ls. The second two arguments the property you want to set on the objects that match your search. For example, say want to set the skybox property to true for all objects that have a texture starting with "sky"... you'd do:

`set 1 sky* 4 1`

### addparam

This command takes 1 argument. This command will add one parameter to each object and is primarily used when upgrading a list.txt file from an old version (say, with 14 parameters per object) to a new version (say, with 15 parameters per object). The argument is the default value that all objects will start with for the new parameter.

<Category:Tracks>
