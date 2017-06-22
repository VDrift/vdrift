VDrift aims to be very user input friendly, and thus there are many different ways to adjust the controls in the game to fit each user's needs. There are options which affect all the controls, options which affect only a certain type of controls, and options that affect each individual control assignment.

Features
--------

-   Any input method can be used to navigate the menu.
-   Any input can be assigned to any control.
-   Gas, brake, and steering controls only allow a single setting. All other controls can have any number of settings on any types of input devices.

Control options
---------------

Control options are defined in the text configuration file [options.config](Options_config.md), set in the Options -&gt; Controls menu and its submenus, and stored in the text configuration file [VDrift.config](VDrift_config.md).

### AutoClutch

-   type: boolean
-   setting: control.autoclutch
-   values: on, off

Enable/disable automatic clutching (prevents car from stalling). Simulates the driver pressing the clutch in using foot when RPM gets close to the stall point.

### AutoShift

-   type: boolean
-   setting: control.autotrans
-   values: on, off

Enable/disable automatic transmission shifting.

### Button Control Delay

-   type: float
-   setting: control.button\_ramp
-   values: off (0.0), long (5.0), medium (7.5), short (10.0)

Slow down application of button inputs on analog controls.

### Speed Affect on Steering

-   type: float
-   setting: control.speed\_sens\_steering
-   values: range 0.0 to 1.0

The higher the value on this setting, the more steering is limited as car speed increases.

### Joystick Type

-   type: string
-   setting: joystick.type
-   values: "joystick", "wheel"

Change the type of joystick device.

### Force Feedback Device

-   type: string
-   setting: joystick.ff\_device
-   values: something like "/dev/input/eventX"

Device file for force feedback events. For more detail, see [Setting up force feedback](Setting_up_force_feedback.md).

### Force Feedback Gain

-   type: float
-   setting: joystick.ff\_gain
-   values: range 0.5 to 5.0

Multiplier to adjust strength of force feedback.

### Invert Force

-   type: boolean
-   setting: joystick.ff\_invert
-   values: on, off

Reverse the force feedback, if necessary for your wheel.

### 200 Degree Wheel

-   type: boolean
-   setting: joystick.two\_hundred
-   values: on, off

Limit steering range to 200 degrees, gives a realistic feel to limited range wheels.

Assigning controls
------------------

Control assignments can be configured through the submenus of the Options -&gt; Controls -&gt; Assign Controls menu, and stored in the text configuration file [controls](controls.md). Control assignments can be edited after they are set.

There are two basic types of control, analog and digital. Analog controls include joystick axes and mouse motion, while digital controls are keys, mouse buttons and joystick buttons. Any type of input can be assigned to any type of control.

So, for example, a digital control - like a key - can be assigned to an analog control. If the "Button Control Delay" option is something other than 0.0, then the key will behave just like a true analog control.

### Car Controls

##### Gas

-   control name: gas

The gas control causes the car to speed up.

##### Brake

-   control name: brake

The brake control causes the car to slow down.

##### Steer Left

-   control name: steer\_left

The steer left control causes the car to turn left.

##### Steer Right

-   control name: steer\_right

The steer right control causes the car to turn right.

##### Start Engine

-   control name: start\_engine

The start engine control will restart the engine if it stalls.

##### Handbrake

-   control name: handbrake

The handbrake brakes only on the back wheels.

##### ABS Toggle

-   control name: abs\_toggle

The ABS Toggle turns anti-lock braking on or off while playing.

##### TCS Toggle

-   control name: tcs\_toggle

The TCS Toggle turns traction control on or off while playing.

#### Transmission

##### Shift Up

-   control name: disengage\_shift\_up

The shift up control changes the car's gear to the next one.

##### Shift Down

-   control name: disengage\_shift\_down

The shift down control changes the car's gear to the previous one.

##### Engage Clutch

-   control name: engage

The engage clutch control lets out the clutch. This must be done after every shift.

##### Analog Clutch

-   control name: clutch

The analog clutch control can allow you to use an external clutch pedal.

#### Gears

##### Neutral

-   control name: neutral

The neutral control shifts the car into neutral.

##### 1st

-   control name: first\_gear

The first gear control shifts the car into first gear.

##### 2nd

-   control name: second\_gear

The second gear control shifts the car into second gear.

##### 3rd

-   control name: third\_gear

The third gear control shifts the car into third gear.

##### 4th

-   control name: fourth\_gear

The fourth gear control shifts the car into fourth gear.

##### 5th

-   control name: fifth\_gear

The fifth gear control shifts the car into fifth gear.

##### 6th

-   control name: sixth\_gear

The sixth gear control shifts the car into sixth gear.

##### Reverse

-   control name: reverse

The reverse control puts the car into reverse gear.

### Game Controls

##### Pause

-   control name: pause

The pause control freezes the game (except in multiplayer).

#### Camera Views

##### Previous Camera

-   control name: view\_prev\_camera

This moves to the previous camera in the set (hood, in-car, chase rigid, chase loose).

##### Next Camera

-   control name: view\_next\_camera

This moves to the next camera in the set (hood, in-car, chase rigid, chase loose).

##### Hood

-   control name: view\_hood

The hood camera control moves the camera to the car's hood.

##### In-Car

-   control name: view\_incar

The in-car camera control moves the camera to driver's view.

##### Chase (Rigid)

-   control name: view\_chaserigid

The rigid chase camera control moves the camera to a fixed distance behind the car.

##### Chase (Loose)

-   control name: view\_chase

The loose chase camera control moves the camera to follow the car like a helicopter.

##### Orbit

-   control name: view\_orbit

The orbit camera control swings around the car [as the camera moves](Configuring_the_controls#Camera_Movement.md).

##### Free

-   control name: view\_free

The free camera control can be moved anywhere using the arrow keys.

##### Focus Next

-   control name: focus\_next\_car

Changes the camera to focus on the next car.

##### Focus Previous

-   control name: focus\_prev\_car

Changes the camera to focus on the previous car.

#### Camera Movement

##### Pan Left

-   control name: pan\_left

Turn the camera view to the left.

##### Pan Right

-   control name: pan\_right

Turn the camera view to the right.

##### Pan Up

-   control name: pan\_up

Turn the camera view upwards.

##### Pan Down

-   control name: pan\_down

Turn the camera view downwards.

##### Zoom In

-   control name: zoom\_in

Zoom camera in.

##### Zoom Out

-   control name: zoom\_out

Zoom camera out.

#### Replays

##### Skip Forward

-   control name: replay\_ff

The skip forward control skips ahead ten seconds during a replay.

##### Skip Backward

-   control name: replay\_rw

The skip backward control goes back ten seconds during a replay.

#### Tools

##### Screen Shot

-   control name: screen\_shot

The screen shot control saves a snapshot of the game while playing.

##### Joystick Info

-   control name: joystick\_info

The joystick info control shows debugging info for available joysticks.

Editing Controls
----------------

After controls have been assigned they can be edited to change some properties of the control. To edit a control just click on it in one of the control assignment menus.

### Analog control properties

Analog controls include joystick axes and mouse motion. These have options for deadzone, gain and exponent.

#### Deadzone

Deadzone allows motion under a certain threshold percentage to be ignored. This is useful if you use a joystick which "wobbles" around the center. You may see the brake lights come on when you let go of the gas, or the car is hard to keep going straight, deadzone will help to fix these things.

#### Gain

Gain multiplies the input value by a percentage. This will make the input value increase linearly.

#### Exponent

Exponent raises the input value on an exponential curve. This helps give more fine-tuning of controls such as steering around the center but still allows for making sharp turns if needed.

### Digital control properties

#### Up/Down

This controls whether the action will be triggered when the button/key is pressed (down), or released (up).

#### Held/Once

This should be set to "held" when they a digital input is mapped to an analog control, like when using keys for gas, brake or steering. This is set automatically when the control is assigned.

Deleting controls
-----------------

There is a Delete button on each Control Editing screen, so to delete a control just click on it, then click Delete.

<Category:Configuration>
