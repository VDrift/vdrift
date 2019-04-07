Doxygen documentation is available for VDrift. Currently only the Subversion trunk is available. It is generated daily from the latest version of the [Development version](Getting_the_development_version.md).

Other versions
--------------

Doxygen output for the last release will be added soon. When new releases are made, documentation will be made available with the release.

Generate Locally
----------------

1. [Install doxygen](http://doxygen.nl/manual/install.html)
1. Set the `VDRIFT_ROOT` environment variable to the directory of your project. Do not use a trailing slash.
    ```sh
    # on Linux
    export VDRIFT_ROOT=/home/yourname/vdrift
    ```
1. Run `doxygen -s` to generate docs.

Read Online
-----------

You can find [Doxygen for VDrift trunk](http://vdrift.net/doxygen/trunk/) at the following URL: <http://vdrift.net/doxygen/trunk/>

Download
--------

Downloadable PDF and Zip archives of the HTML version for offline browsing will be available soon.

Help improve documentation
--------------------------

If you are writing new code for VDrift, it would help to write comments using a format Doxygen can take advantage of. Here are a few handy links describing how to do that:

-   [Doxygen Manual: Documenting Code](http://www.stack.nl/~dimitri/doxygen/docblocks.html)
-   [Doxygen Manual: Command Reference](http://www.stack.nl/~dimitri/doxygen/commands.html)
-   [Entire Doxygen Manual](http://www.stack.nl/~dimitri/doxygen/manual.html)

<Category:Development>
