This page describes some of the most common commands used when working with VDrift's source code. It assumes you have followed the instructions in the [Cloning/Checking out](Getting_the_development_version#Cloning/Checking_out.md) section of [Getting the development version](Getting_the_development_version.md).

See this documentation for more up to date and complete information: <http://help.github.com/>

Updating
--------

### Git

To update the Git repositories, execute the following command in the root folder of the repository (i.e. **vdrift**, **vdrift/vdrift-win** or **vdrift/vdrift-mac**):

`git pull `[`git://github.com/VDrift/vdrift.git`](git://github.com/VDrift/vdrift.git)

### Subversion

To update the data, run the following command while in **vdrift/data**

`svn update`

Making your own changes
-----------------------

### Git

These instructions also apply for the dependencies - just change the file paths and URLs.

#### Forking

If you want to make your own changes to improve VDrift, the easiest way is to fork the main VDrift repository on [GitHub](https://github.com/VDrift/vdrift/). Just click the button near the top right of the page. You will need a free [GitHub](http://github.com) account. Then clone your new repo onto your computer:

`git clone git@github.com:`*`username`*`/vdrift.git`

And link this to the original VDrift repo:

`cd vdrift`
`git remote add upstream `[`git://github.com/VDrift/vdrift.git`](git://github.com/VDrift/vdrift.git)
`git fetch upstream`

#### Uploading changes

To upload any changes you've made, first add the files to the commit:

`git add `*`filename`*

Then do the commit:

`git commit -m 'Commit message'`

And finally send this to GitHub:

`git push origin master`

#### Checking in changes

You can request a VDrift developer to incorporate your changes by initiating a GitHub pull request. See this documentation for more info: <http://help.github.com/send-pull-requests/>

### Subversion

You will need to ask a VDrift developer in the [forums](http://vdrift.net/Forum/index.php) for Subversion access in order to commit to the repository. You will need a free [Sourceforge](http://sourceforge.net) account. You can then add any new files or folders you've created:

`svn add `*`filename`*

And then commit the changes:

`svn commit -m "Commit message"`

If you don't want to do this, you can create a diff which you can post in the [forums](http://vdrift.net/Forum/index.php):

`svn diff`

#### Thank you for helping make VDrift better for everyone!

<Category:Development>
