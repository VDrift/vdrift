**VDrift.config** is the main user settings file for VDrift. It is in the [config file format](Config_file_format.md). It is not the only file which VDrift uses for storing user settings (see also [configuring the controls](Configuring_the_controls.md)); however, it is the one which contains most of the settings important to users. VDrift does not use any kind of registry or configuration database. The options available to this configuration file are the ones defined in [options.config](Options_config.md).

Location and Defaults
---------------------

**VDrift.config** is located inside the [user settings directory](User_settings_directory.md). The default **VDrift.config** is located in **data/settings/VDrift.config**. If VDrift fails to start, the next time it runs, it will move the user's current **VDrift.config** to **VDrift.config.backup** and make a new **VDrift.config** from the default one.

Example
-------

This is an example **VDrift.config** file.

    [control]
    autoclutch = true
    autotrans = true
    button_ramp = 5
    mousegrab = true
    speed_sens_steering = 1
    
    [display]
    FOV = 45
    anisotropic = 0
    antialiasing = 0
    bloom = true
    camerabounce = 1
    contrast = 1
    depth = 16
    fullscreen = true
    height = 1050
    input_graph = true
    language = English
    lighting = 0
    mph = true
    normalmaps = true
    racingline = true
    reflections = 1
    shaders = true
    shadow_distance = 1
    shadow_quality = 1
    shadows = true
    show_fps = true
    show_hud = true
    skin = simple
    texture_size = medium
    trackmap = true
    view_distance = 1000
    width = 1680
    zdepth = 16

    [game]
    ai_difficulty = 1
    antilock = true
    camera_mode = chase
    number_of_laps = 1
    opponent = XS
    opponent_color = 1,1,1
    opponent_color_blue = 1.000000
    opponent_color_green = 0.000000
    opponent_color_red = 0.000000
    opponent_paint = 00
    player = XS
    player_color = 1,1,1
    player_color_blue = 1.000000
    player_color_green = 0.000000
    player_color_red = 0.000000
    player_paint = 00
    record = false
    reverse = false
    selected_replay = 0
    track = paulricard88
    traction_control = true
    [joystick]
    ff_device = /dev/input/event0
    ff_gain = 2
    ff_invert = false
    hgateshifter = false
    two_hundred = false
    type = joystick

Options
-------

### Control section

For information on the Control settings, see [Configuring the controls](Configuring_the_controls.md).

### Display section

See [Configuring the display](Configuring_the_display.md) for detailed descriptions of each option.

### Game section

See [Starting the game](Starting_the_game.md) for detailed descriptions of each option.

### Joystick section

For information on the Joystick settings, see [Configuring the controls](Configuring_the_controls.md).

### Sound section

See [Configuring the sound](Configuring_the_sound.md) for more information.

<Category:Files> <Category:Configuration>
