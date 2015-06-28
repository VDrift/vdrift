VDrift has a set of options which can be added to menus, referenced by the game, and used as a template for a player's settings. These options are defined in **data/settings/options.config** in the [Configfile format](Config_file_format "wikilink") that VDrift uses for many of its other text files. Once defined, these options can be referred to by the menus to do several things, including: pick default values for options, to get names, descriptions, and lists of values for options, check the type of options, and much more.

The options tell what values are available to the user for a given setting. This may seem confusing. Here's an example definition of an option in the file:

`[ option-17 ]`
`cat = display`
`name = view_distance`
`title = View Distance`
`desc = Change the maximum view distance.`
`type = float`
`default = 500.0`
`values = list`
`num_vals = 5`
`opt00 = Very Low (0.25 km)`
`val00 = 250.0`
`opt01 = Low (0.5 km)`
`val01 = 500.0`
`opt02 = Medium (1 km)`
`val02 = 1000.0`
`opt03 = High (2.5 km)`
`val03 = 2500.0`
`opt04 = Very High (10 km)`
`val04 = 10000.0`

The first line has the option ID number. This must be in the format `[ option-## ]` and number must be less than num\_options. No two widgets should have the same ID. The first field, `cat`, is just for categorization. This option is in the display category. Next comes the option's name, which combined with the category, makes up the name by which the option is referred to in the menu (here, "display.view\_distance"). The `title` field tells the human-readable name for this option. `desc` tells a little more about the option, this usually goes in the tip for the option in the menu. `type` is very important because it tells the game how to interpret the values given to it for each option. This particular option is a floating point option, which means it has a decimal, so it is `type` float. `default` is, as you probably guessed, the default value for the option.

The `values` needs a little extra explanation. In this case it is "list" which means that the possible values for the option will be listed along with this option definition. Sometimes `values` is given an alternate value which tells the game to get the list of values from some other special place. Since this option needs a list of values, we define the list: `num_vals` tells how many possible options there are; and the following `opt-##` and `val-##` fields describe the displayed name for the value, and the actual value, respectively. The `opt-##` fields are assumed to be strings, but the `val-##` fields must be of the same type as the type of the object, as must the `default` setting described earlier. Putting the wrong type of values here can cause very unexpected results.

<Category:Files>
