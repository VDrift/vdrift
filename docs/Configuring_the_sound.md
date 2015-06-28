Currently VDrift has 3D sound using OpenAL. In-game sounds include engine RPM sound and tire sounds. The game doesn't have any music yet.

Sound settings
--------------

Configuring the sound is done by editing [VDrift.config](VDrift_config "wikilink") manually, or by changing the settings in the Options -&gt; Sounds menu. As there is no game music yet, the only option is the volume of game sounds:

`[sound]`
`volume = 1`

Troubleshooting sound
---------------------

If the engine sound is very broken or choppy sounding, and you hear some noise or crackling, you need to tell OpenAL to try to use a different sound backend. If you don't have an ~/.openalrc file, create it with these contents, or modify your current one to look like this:

`(define devices                 '(native alsa sdl arts esd null))`
`(define alsa-device             "dsp0")`
`(define speaker-num             2)`
`;(define sampling-rate          22050)`

The important line here is the "define devices" line, OpenAL attempts to use each of those sound output methods in order. It uses the first one that works; in some cases native will work best, in other cases perhaps alsa. If Gnome is running its sound daemon, esd would be the best choice, while KDE usually uses artsd.

Known bugs
----------

If you don't hear lots of crackling and choppiness, but you still hear a 'click, click, click' sound as the engine sample loops, this is a bug in OpenAL for Linux (or a feature we haven't found a way around...). We've heard there is a fix in the works from the OpenAL project.

Also, certain cars have engines that rev very high, and our engine sound system can't pitch shift high enough to play the sound at the correct frequency, after it hits the maximum pitch. This is currently only obvious on the F1 car.

OpenAL Utility Toolkit
----------------------

On older versions of OpenAL, ALUT was part of the OpenAL library. At some point not easily markable with a version number, ALUT became a separate library. This changed the way VDrift must be built, as well as some of the sound code.

To accomodate users of the old version of OpenAL with ALUT included, there is a switch in the SCons build system used on Linux and FreeBSD. To compile VDrift with "old OpenAL" support, simply add the option to the scons compile command:

`scons old_openal=1`

For more information on the build system see [Using SCons](Using_SCons "wikilink").

<Category:Configuration>
