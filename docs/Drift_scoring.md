To find out more about what drifting is and how to do it read [Drifting Techniques](Drifting_techniques.md).

Overview
--------

The player's drift score is displayed on the left side of the screen, underneath the lap timer box.

Start of scoring
----------------

The game starts drift scoring when a car satisfies the following conditions,

-   At least **2** of the wheels are on track, and
-   Car speed is above **10 m/s**, and
-   The drift angle (angle between car's orientation and its velocity direction) is more than **0.2 radian** (about 11.5 degrees), and less than **PI/2** (i.e. spin out).

End of scoring
--------------

The drift scoring ends when either one the following occurs,

-   Less than **2** wheels are on track, or
-   Car speed is less than **10 m/s**, or
-   The drift angle is less than **0.1 radian** (about 5.7 degrees)

Scoring rules
-------------

At the beginning of a drift, the game starts accumulating the score for this drift. At the end of a drift, the accumulated score is added to the total score. The accumulated score consists of base score and bonus score;

-   **Base score** - This is simply the length the car has travelled in this drift (in meters). A longer drift will earn higher base score.
-   **Bonus score** - This contains 3 components:
    -   **drift length bonus** - same value as the base score, effectively giving double bonus to longer drifts.
    -   **maximum drift speed bonus** - the value is

`the maximum drift speed (in m/s) / 2`

e.g. a maximum drift speed of 20 m/s will earn a bonus of 10.

-   -   **maximum drift angle bonus** - the value is

`the maximum drift angle (in radian) * 40 / PI`

e.g. a maximum drift angle of PI/4 (45 degrees) will earn a bonus of 10.

If the car goes off track or spins out during a drift, the accumulated drift score is not added to the total score. An accumulated drift score of less than 5 is not added to the total either.

<Category:Playing>
