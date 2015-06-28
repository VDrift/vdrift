VDrift must be able to access its game data to run. This data is stored in the data directory.

Location
--------

VDrift looks in several places to find its game data at startup. This is the order in which the different locations are checked.

1.  The directory specified in the environment variable **VDRIFT\_DATA\_DIRECTORY**
2.  The subdirectory "data" of the current working directory (the location from which VDrift was run)
3.  A directory named at [compile](Compiling.md)-time via the environment variable **DATA\_DIR** (usually set by the [SCons](Using_SCons.md) build setup)

Validation
----------

To ensure that it has the correct location, VDrift checks for the file **data/settings/[options.config](Options_config.md)**. If this file can't be found in any of the above locations, VDrift exits immediately.

<Category:Files>
