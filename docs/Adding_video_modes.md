You can add a new resolution to the list that gets displayed in the in-game display options by editing the **data/lists/videomodes** file.

When VDrift is run, it reads this file for the base list of video modes. Then it gets a list of available graphics modes that your monitor supports using SDL. During this process it checks the provided modes to see if they are valid (invalid modes are removed from the list).

The list of video modes which is then made available through the Display Options menu is all working modes from the videomodes file plus any other modes SDL reported as working.

<Category:Files>
