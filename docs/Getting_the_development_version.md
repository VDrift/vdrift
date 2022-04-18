This page describes how to get the source code of VDrift. It assumes your computer meets the [requirements](Requirements.md).

VDrift's code is kept in a [Git](http://git-scm.com/) repository, hosted by [GitHub](https://github.com/VDrift/vdrift). Some operating system's dependencies are also hosted on GitHub, and the data is hosted on [Sourceforge](https://vdrift.svn.sourceforge.net/svnroot/vdrift/vdrift-data) in a [Subversion](http://subversion.apache.org/) repository.

Downloading a snapshot
----------------------

Both GitHub and Sourceforge allow you to download a snapshot of the current state of the repository.

### Source code

The source code can be downloaded by going to the [downloads section](https://github.com/VDrift/vdrift/downloads) of GitHub and click either "Download as zip" or "Download as tar.gz". Once it has downloaded, uncompress the resulting file into a folder called **vdrift**.

### Dependencies

Dependencies for Windows and OS X can be downloaded in the same way as the source code, from <https://github.com/VDrift/vdrift-win/downloads> or <https://github.com/VDrift/vdrift-mac/downloads> respectively. They should be uncompressed into **vdrift/vdrift-win** or **vdrift/vdrift-mac**.

### Data

The data can be downloaded from the Sourceforge website or from the Subversion repository. Instructions for [using Subversion are below](#cloningchecking-out).

To download using the Sourceforge snapshot feature:

1. Visit [Sourceforge to generate a snapshot](https://sourceforge.net/p/vdrift/code/HEAD/tarball?path=/vdrift-data). The snapshot is very large, 1.5GB at time of writing, and could take some time.
1. Uncompress the file to the **data** folder in your vdrift location.

Cloning/Checking out
--------------------

To easily stay up to date with the latest changes, and to avoid re-downloading large amounts of data, you can use [Git](http://git-scm.com/) and [Subversion](http://subversion.apache.org/) to clone/check out the repositories to your computer. For the following sections it is assumed you have installed [Git](http://git-scm.com/) and [Subversion](http://subversion.apache.org/), and have some knowledge of the command line. If you are using a Git or Subversion client, follow its instructions for downloading repositories.

### Source code

The following command will clone the source code to a folder called **vdrift** in the current directory:

    git clone git://github.com/VDrift/vdrift.git

Then navigate to the **vdrift** folder:

    cd vdrift

### Dependencies

Cloning the dependencies for Windows and OS X is similar to cloning the source code. Make sure you are still in the **vdrift** folder, then execute the command for your operating system:

-   Windows:

        git clone git://github.com/VDrift/vdrift-win.git

-   OS X:

        git clone git://github.com/VDrift/vdrift-mac.git

### Data

The data can be checked out into the **vdrift/data** folder with the following command. Make sure you are still in the **vdrift** folder.

    svn co https://svn.code.sf.net/p/vdrift/code/vdrift-data data

This will take a while to complete as the data is quite big.

<Category:Development>
