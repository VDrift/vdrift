The menus are defined by text files in the **data/skins/*skin\_name*/menus/** directories, where *skin\_name* is the name of one of the skins [Skin system](Skin_system.md). The menus are defined in text files in the [Configfile format](Config_file_format.md). Each of the menu "pages" contains a number of menu "widgets", which have a variety of functions.

The menu system was designed to be used with any type of controller. Every action may be triggered by a simple button push, except things like text entry. Users should be able to choose if they wish to point and click with the mouse, use the keyboard's arrow keys, or joystick axes and buttons. Keep this in mind when designing or editing menu pages.

Other guidelines for menu writing include:

-   Try to keep the layouts similar to other menus in the skin.
-   Keep the menus simple and break them up into subgroups where possible.
-   Put the most used functions closer to the main menu, and the lesser used things in deeper submenus.
-   Do not put too many widgets on any one page.
-   Always check to make sure navigating a menus with the keyboard works in the order expected.

Adding Menus
------------

To add a new menu, simply create a new text file under **data/skins/*skin\_name*/menus/** with the name of the file the same as the name of the menu in camelcase.

### Writing a Menu

Now you need to define the new menu. Let's look at the Options menu. The game needs to know a little about the menu itself, so the first few lines of every menu look something like this:

`name = Options`
`widgets = 5`
`background = gui/box.png`
`dialog = false`

The `name` field should be the same as the filename. The `widgets` field tells the game how many widgets to load from this menu file (it starts at 0, and stops at `widgets - 1`). The `background` field tells the game which graphic to use as the menu background. The `dialog` tells the game if this is a dialog box or not, but right now this option has no effect (dialog boxes are drawn as full size menus).

### Adding Widgets

Next, you will need to add some widget definitions to your menu.

Here's the format for a label widget, this example taken from the Options menu:

`[ widget-00 ]`
`type = label`
`name = OptionsLabel`
`text = Options`
`center = 0.5, 0.1`
`width = auto`
`height = auto`
`fontsize = 9`
`enabled = 0`
`selected = 0`
`default = 0`
`cancel = 0`

The first line is the widget ID. This is the identification number of this widget on this menu page. The number also dictates the order in which the menu will be traversed if you use the arrow keys to move through it. The `type` field tells VDrift what kind of widget it is. `name` is a non-numeric identifier. `text` is the text that will appear on the label. `center` is the relative position of the widget on the screen. `width` and `height` are the relative width and height of the label (here we tell VDrift to figure it out automatically, this is generally a good idea). `fontsize` is the size to draw the text, the value 9 is pretty big, which is good because this label is the menu title.

The next values, `enabled`, `selected`, `default`, and `cancel`, are boolean values. These are available in every widget and tell how the user can interact with it. `enabled` allows the user to use the widget. A label has no function so in this case it is turned off. `selected` indicates if this item should be the one selected when the menu is displayed for the first time. Since a user can't use a label, it shouldn't be selected, so this is off too. `default` is usually used for OK buttons, or buttons that need to trigger saving all the values on the page when they're pressed. `cancel` is for Cancel buttons and indicates if this is the button to be pressed when Escape is pressed on this menu, and discards any changes made on the menu.

### Making the Menus Accessible from Other Menus

Navigation through the menu system is done using buttons. Let's say for example you are creating a menu called GameOptions which will allow users to change certain aspects of the gameplay (perhaps such as difficulty level). This menu would be accessed from the existing Options menu. So you'll need to add a button for GameOptions to the Options menu. Here's an example button widget that could do this.

`[ widget-01 ]`
`type = button`
`name = GameOptionsButton`
`text = Game`
`tip = Change game settings.`
`action = GameOptions`
`center = 0.5, 0.3`
`color = 0.0, 0.0, 0.8`
`width = auto`
`height = auto`
`fontsize = 7`
`enabled = 1`
`selected = 1`
`default = 0`
`cancel = 0`

The first thing you notice is all the properties it shares with the label widget. In this case let's say the button is now the first one on the Options menu. The `type` is now "button" appropriately. Some new options available to a button are: `tip`, which shows a description of the widget at the bottom of the screen; `action`, which dictates what happens when the button is pressed (in this case the name of the menu to go to, in other cases, a special function name that tells the game to do something); and finally `color`, which allows you to change the color of the text in %R, %G, %B format. This particular button has 80% blue text.

Take a look at some of the existing menu entries for more examples.

Warning
-------

The menu system is still a little light on the error checking. Therefore care must be taken when writing menus or editing them that you don't miss something, or the menu may not work at all, may be unreachable or inescapable, or may even cause the game to crash.

<Category:Files>
