Shared sounds
-------------

VDrift has a number of sounds that are used for all cars. They can be found in the carparts directory.

    brake.wav        # Brake engage sound (in driver view)
    handbrake.wav    # Handbrake engage sound (in driver view)
    gear.wav         # Gear shift sound (in driver view)
    bump_front.wav   # Front suspension bump sound
    bump_rear.wav    # Rear suspension bump sound
    crash.wav        # Collision sound
    grass.wav        # Tires rolling over grass
    gravel.wav       # Tires rolling over gravel
    tire_squeal.wav  # Tire squeal(skidding over asphalt) sound
    wind.ogg         # Wind noise

Individual sounds
-----------------

Car engine sounds are stored per car. Default minimum configuration is a single sound.

    engine.wav       # Engine sound at 7000rpm 

#### Advanced engine sound setup

VDrift supports multiple engine sounds with interpolation (blending). To use this feature a <CARNAME>.aud text file has to be created containing sound information. It can contain an arbitrary number of sections, one per sound file.

    [sound1]
    filename=engine_accel_1.wav  # relative path to the sound file
    MinimumRPM=1                 # minimum engine rpm to use this sound for
    MaximumRPM=1500              # maximum engine rpm to use this sound for
    NaturalRPM=1000              # rpm this sound has been recorded at
    power=on                     # (optional) use this sound for throttle open(on) or closed(off); default is both

The rpm ranges of two neighboring sounds can(should) overlap to create a smooth transition.

Example TL2/TL2.aud:

    [poweroff1]
    filename=sounds/1kbehind.wav
    MinimumRPM=1
    MaximumRPM=1500
    NaturalRPM=1000
    power=off

    [poweron1]
    filename=sounds/2kbehind.wav
    MinimumRPM=1
    MaximumRPM=2500
    NaturalRPM=2000
    power=on

    [poweroff2]
    filename=sounds/5kbehind.wav
    MinimumRPM=1000
    NaturalRPM=4650
    MaximumRPM=4500
    power=off

    [poweron2]
    filename=sounds/4kbehind.wav
    MinimumRPM=2000
    NaturalRPM=3560
    MaximumRPM=4000
    power=on

    [poweroff3]
    filename=sounds/6kbehind.wav
    MinimumRPM=4000
    NaturalRPM=5130
    MaximumRPM=10000
    power=off

    [poweron3]
    filename=sounds/7kbehind.wav
    MinimumRPM=3500
    NaturalRPM=6130
    MaximumRPM=10000
    power=on

<Category:Cars> <Category:Files>
