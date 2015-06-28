-   [Car parameters for vdrift-2010-06-30](Car_parameters_for_vdrift-2010-06-30.md)
-   [Car parameters for vdrift-2009-06-15 and older](Car_parameters_for_vdrift-2009-06-15_and_older.md)

The units are all in [MKS](http://scienceworld.wolfram.com/physics/MKS.html) (meters, kilograms, seconds). It might also help to read [*The Physics of Racing*](http://www.miata.net/sport/Physics/) by Brian Beckman. For unit conversion you can go to: [*This Site*](http://www.sonar-equipment.com/useful_conversion_factors_table1_p00.htm).

The .car file contains several sections. Each section will now be described, along with example values from the XS.car file. The XS has performance comparable to the Honda S2000.

Coordinate system
-----------------

The .car files use the [right-handed (positive)](http://en.wikipedia.org/wiki/Cartesian_coordinate_system#In_three_dimensions) coordinate system for all parameters:

-   **x axis**: negative is left, positive is right
-   **y axis**: negative is back, positive is forward
-   **z axis**: negative is down, positive is up

Common Parameters
-----------------

    [section]
    texture = diffuse.png, misc1.png, misc2.png
    mesh = model.joe
    position = 0.736, 1.14, -0.47
    rotation = 0, 0, 30
    scale = -1, 1, 1
    color = 0.8, 0.1, 0.1
    draw = transparent
    mass = 40

Every car section supports a set of optional parameters to describe its graphic representation.

Texture is a list of textures that has to contain at least one texture, usually the diffuse color texture. Mesh defines the model mesh to be used with the texture. Texture and mesh paths are relative to car(XS) and carparts(shared components) directory.

Position/rotation(in degrees)/scale will transform the mesh relative to parent. Color defines the color of the mesh(to be blended with the texture according to its alpha channel). Draw allows the options transparent(according to first textures alpha channel) or emissive(won't be affected by lighting, used for brake/reverse light models).

Mass is used to calculate car inertia, weight and center of mass.

Engine
------

    [engine]
    position = 0.86, 0.0, -0.21
    mass = 140.0
    max-power = 1.79e5
    peak-engine-rpm = 7800.0
    rpm-limit = 9000.0
    inertia = 0.25
    idle = 0.02
    start-rpm = 1000
    stall-rpm = 350
    fuel-consumption = 1e-9
    torque-friction = 0.0003
    torque-curve-00 = 1000, 140.0
    torque-curve-01 = 2000, 149.14
    torque-curve-02 = 2200, 145.07
    torque-curve-03 = 2500, 147.78
    torque-curve-04 = 3000, 169.50
    torque-curve-05 = 3300, 172.19
    torque-curve-06 = 4000, 169.50
    torque-curve-07 = 4500, 166.77
    torque-curve-08 = 5600, 172.19
    torque-curve-09 = 5800, 170.83
    torque-curve-10 = 6000, 168.12
    torque-curve-11 = 6100, 177.61
    torque-curve-12 = 6200, 186.42
    torque-curve-13 = 6300, 192.53
    torque-curve-14 = 6500, 195.92
    torque-curve-15 = 6700, 195.92
    torque-curve-16 = 7000, 195.24
    torque-curve-17 = 7600, 190.49
    torque-curve-18 = 8000, 184.39
    torque-curve-19 = 8200, 183.04
    torque-curve-20 = 8300, 146.43
    torque-curve-21 = 9500, 146.43

The position and mass parameters affect the weight distribution of the car. The torque curve is calculated from max-power and peak-engine-rpm using a polynomial expression given in Motor Vehicle Dynamics, Genta (1997), where peak-engine-rpm is the engine speed at which the maximum power output (max-power) is achieved. Alternatively, the torque curve can be explicitly defined, as in the example above. A rev limit can be set with rpm-limit. The rotational inertia of the moving parts is inertia. Idle is the throttle position at idle. Starting the engine initially sets the engine speed to start-rpm. Letting the engine speed drop below stall-rpm makes the engine stall. The rate of fuel consumption is set with fuel-consumption. The actual fuel consumed each second (in units of liters) is the fuel-consumption parameter times RPM times throttle (throttle is from 0.0 to 1.0, where 1.0 is full throttle).

Clutch
------

    [clutch]
    sliding = 0.27
    radius = 0.15
    area = 0.75
    max-pressure = 11079.26

The clutch is described by its sliding friction coefficient, radius, area and maximum applied pressure. The torque capacity(maximum transmitted torque) of the clutch is TC = sliding \* radius \* area \* max-pressure. It should be somewhere between one and two times the maximum enine torque. TC = 1.25 \* max-engine-torque is a good start value.

Transmission
------------

    [transmission]
    gears = 6
    gear-ratio-r = -2.8
    gear-ratio-1 = 3.133
    gear-ratio-2 = 2.045
    gear-ratio-3 = 1.481
    gear-ratio-4 = 1.161
    gear-ratio-5 = 0.943
    gear-ratio-6 = 0.763
    shift-time = 0.2

The number of forward gears is set with the gears parameter. The gear ration for reverse and all of the forward gears is then defined. The shift-time tag tells how long it takes, in total seconds, to change gears (when autoclutch is enabled). Half the time is spent changing the gear and the other half is spent letting the clutch out. This parameter is not required and defaults to 0.2 seconds, which is a reasonable value for a manual transmission. F1 cars take about 50 ms, by comparison.

Differential
------------

For **FWD** cars \[differential.front\] has to be defined. **AWD** cars require \[differential.front\], \[differential.rear\] and \[differential.center\].

    [differential.rear]
    final-drive = 4.100
    anti-slip = 600.0
    anti-slip-torque = 1
    anti-slip-torque-deceleration-factor = 0

The final drive provides an additional gear reduction. The anti-slip parameter defines the maximum anti-slip torque. For speed-sensitive differentials, it also defines the anti-slip torque per radian per second of speed difference between the wheels. If the differential is speed-sensitive, the anti-slip-torque and anti-slip-torque-deceleration-factor parameters must be omitted or set to zero. If the differential is torque-sensitive, then anti-slip-torque defines the amount of anti-slip torque per input torque. The anti-slip-torque-deceleration-factor defines the amount of anti-slip torque per negative input torque. For a 1-way torque-sensitive LSD, set anti-slip-torque-deceleration-factor to zero, for a 2-way torque-sensitive LSD, set anti-slip-torque-deceleration-factor to 1.0, for 1.5-way, set it between 0.0 and 1.0.

Fuel tank
---------

    [fuel-tank]
    position = 0.0, -1.0, -0.26
    capacity = 0.0492
    volume = 0.0492
    fuel-density = 730.0

The fuel tank's position, the current volume of fuel and the density of the fuel affect the car's weight distribution. The capacity tag sets the maximum volume of fuel that the tank can hold. The initial volume is set with the volume tag. The density of the fuel is set with fuel-density.

Camera
------

    [camera.1]
    name = driver                   # name to identify camera, default values are hood, driver, chase rigid, chase loose, orbit and free
    type = mount                    # supported types are mount, chase, orbit, free
    position = -0.023, -0.32, 0.50  # camera position relative to car
    lookat = -0.023, 0.32, 0.3      # optional, defines camera view direction
    stiffness = 0.0                 # optional, bounce effect, 0.0 is a sports car and 1.0 is F1-ish.
    fov = 90                        # optional, overrides default field of view angle

VDrift supports an arbitrary number of cameras per car. The default minimum count is 6, camera.0 - camera.5.

Wing
----

    [wing.rear]
    position = 0.0, -2.14, 0.37
    frontal-area = 0.05
    drag-coefficient = 0.0
    surface-area = 0.5
    lift-coefficient = -0.7
    efficiency = 0.95

Wing identifiers front, center, rear are arbitrary(can be chosen freely). A wing describes the aerodynamics(car body, front/rear wing) of the car. A car has to have at least one wing, to capture body drag. Most cars will use up to three. The frontal area and coefficient of drag, set with frontal-area and drag-coefficient, are used to calculate the drag force.

Downforce can be added with the optional parameters surface-area, lift-coefficient, efficiency. If the lift coefficient is positive, upforce is generated. This is usually undesirable for cars. The efficiency determines how much drag is added as downforce increases. The surface-area is the surface area of the wing. This value is also used in the drag calculation.

Wheel
-----

    [wheel.fl]
    texture = oem_wheel.png, oem_wheel-misc1.png 
    mesh = oem_wheel.joe                         # if genrim not set to false, mesh is used as wheel disk(spokes), rim will be auto-generated
    #genrim = false                              # optional, disables auto-generated rim mesh
    position = -0.736, 1.14, -0.47  #track front/rear 1471/1509
    camber = 0.5
    caster = 6.0
    toe = -0.16
    ackermann = 8.46    # 50% ackermann
    steering = 30

The number of wheels is fixed to four: fl, fr, rl, rr. For a FWD car the wheels fl and fr are powered, for RWD the wheels rl and rr.

The wheel mesh is the wheel disk mesh(wheel mesh without rim). The mesh will be scaled according to tire dimensions, has to fit into a unit cube. The rim mesh is generated automatically.

Wheel alignment is set with the camber, caster, and toe. All angles are in degrees. For a "negative camber" the left wheel camber has to be negative, the right wheel camber positive.

Ackermann and steering are optional. Ackermann is the steering arm angle relative to wheel. Ideal ackermann(100%) is atan(0.5\* track / wheelbase). For the right wheel positive ackermann is positive, for the left negative. Steering is the maximum steering angle of the wheel(for ackermann = 0). A negative steering leads to a reverted steering.

Suspension
----------

    [wheel.fl.hinge]
    wheel = -0.736, 1.14, -0.47
    chassis = 0.0, 0.99, -0.55

Suspension has to be defined per wheel. Hinge suspension is equivalent to a parallel double wishbone setup. The hinge link is attached at chassis to car body and at wheel to wheel hub.

    [wheel.fl.macpherson-strut]
    strut-top = -0.66, 1.34, 0.05
    strut-end = -0.70, 1.34, -0.505
    hinge = -0.36, 1.34, -0.44

Alternatively a macpherson-strut setup can be used. Hinge is the lower link attachment point to car body. The wheel attachment point is the wheel hub position.

Coilover
--------

    [wheel.fl.coilover]
    spring-constant = 49131.9
    bounce = 2600
    rebound = 7900
    damper-factor-1 = 0.06, 1.2
    damper-factor-2 = 0.08, 1.1
    damper-factor-3 = 0.1, 1
    damper-factor-4 = 0.2, 0.9
    damper-factor-5 = 0.5, 0.5
    damper-factor-6 = 1, 0.3
    damper-factor-7 = 5, 0.1
    travel = 0.19
    anti-roll = 800.0

Each wheel has a coilover(spring-damper unit). The spring-constant is the **wheel rate** in N/m. The spring-factor-1 and 2 parameters define a curve for the spring response. These can be omitted if desired, in which case a factor of 1.0 will be used everywhere. Points are defined by specifying an x,y pair where x is the suspension displacement in meters and y is the factor to be applied to the spring coefficient. In this example, the spring factor will be 1.0 when the displacement is between 0 and 0.052 m, and then the spring factor will change linearly to 1.2 at 0.055 m (and beyond). The spring factor gets multiplied by the spring-constant. You can put as many spring-factor points as you want (just increase the spring-factor- number for each additional point). Note that displacement values are relative to the "zero g", "zero force" position. For best results, start VDrift with the -debug option and observe suspension displacements during maneuvering to determine where you want to put your points.

The bounce and rebound parameters are the damping coefficients for compression and expansion of the suspension, respectively, in units of N/m/s. The damper-factor-1 and 2 parameters define a curve for the damper response. These can be omitted if desired, in which case a factor of 1.0 will be used everywhere. Points are defined by specifying an x,y pair where x is an absolute value of suspension velocity in m/s and y is the factor to be applied to the damping coefficient. In this example, the damper factor will be 1.0 when the compression velocity absolute value is between 0 and 0.08 m/s, and then the damper factor will change linearly to 0.7 at 0.1 m/s (and beyond). The damper factor gets applied to the bonce or rebound damper coefficient, depending on the direction of travel. You can put as many damper-factor points as you want (just increase the damper-factor- number for each additional point).

The travel is the maximum wheel travel from wheel extended position. Anti-roll in N/m is currently incorrectly associated with the wheel coilover, acts between front wheels fl and fr and rear wheels rl and rr.

Tire
----

    [wheel.fl.tire]
    texture = tire/touring.png # optional, enables auto-generated tire mesh
    #mesh = tire.joe           # optional, overrides auto-generated tire mesh
    size = 215, 45, 17
    type = &tire/touring

Tire size determines tire dimensions:

-   section width in millimeters, measured from sidewall to sidewall
-   ratio of sidewall height to section width in percent
-   diameter of the wheel in inches

Each wheel has a tire section. Tire size is used to calculate wheel weight and inertia. The tire mesh is optional and has to be centered at origin and fit into a unit box. It will be scaled according to tire dimensions. If omitted a default mesh is generated/used.

Tire type is a tire subsection \[wheel.fl.tire.type\], here a reference. This means car loader will look for a \[tire/touring\] section and alternatively for a file tire/touring relative to car and carparts directory. The first found section definition is used. More info about tire type definition can be found here: [Tire parameters](Tire_parameters.md)

Brake
-----

    [wheel.rl.brake]
    texture = rotor_shiny_slotted_drilled.png # optional, enables auto-generated disk mesh
    #mesh = rotor.joe                         # optional, overrides auto-generated brake disk mesh
    friction = 0.6
    max-pressure = 4.0e6
    bias = 0.45
    radius = 0.14
    area = 0.015
    handbrake = 1.0

The bias parameter is the fraction of braking pressure applied to the front brakes (in the front <span class="plainlinks">[<span style="color:black;font-weight:normal;text-decoration:none!important;background:none!important; text-decoration:none;">weight gain</span>](http://how2gainweightfast.org)</span> brake section) or the rear brakes (in the rear brake section). To make sense, the rear value should equal 1.0 minus the front value. The maximum brake torque is calculated as friction \* area \* bias \* max-pressure \* radius. Handbrake determines the handbrake influence factor. Texture is an optional brake rotor texture. If set a brake disk mesh is generated from brake parameters. This mesh can be overridden by providing a custom brake mesh.

Steering
--------

    [steering]
    texture = steering_wheel.png
    mesh = steering_wheel.joe
    position = -0.37, 0.44, 0.09
    rotation = 87.5, 0.0, 0.0
    max-angle = 320

Steering defines the steering device. The rotation of the steering model is constrained by max-angle. The rotation axis is the local z-axis of the steering mesh.

Particle
--------

    [particle-00]
    position = 0.0, -1.28, -0.36
    mass = 30.0

Mass particles used for weight distribution and rotational inertia. Most cars will use 6-10 particles.

Light
-----

    [light-brake]
    texture = brake.png
    mesh = body.joe
    draw = emissive

    [light-reverse]
    texture = reverse.png
    mesh = body.joe
    draw = emissive

Car lights are treated as car shape models. light-brake is set emissive during braking, light-reverse if reverse gear is selected.

Car shape
---------

    [driver]
    texture = driver2.png, driver-misc1.png
    mesh = driver.joe
    position = -0.37, 0.07, 0.05
    mass = 90.0

    [body]
    texture = body00.png
    mesh = body.joe

    [interior]
    texture = interior.png
    mesh = interior.joe

    [glass]
    texture = glass.png
    mesh = glass.joe
    draw = transparent

The car shape can consist of an arbitrary number of models with arbitrary names excluding the reserved ones: engine, clutch, ...

Shape hierarchies \[body.foo\] are not supported.

<Category:Cars> <Category:Files>
