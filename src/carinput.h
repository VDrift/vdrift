/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _CARINPUT_H
#define _CARINPUT_H

namespace CARINPUT
{
enum CARINPUT
{
	//actual car inputs that the car uses
	THROTTLE,
	NOS,
 	BRAKE,
  	HANDBRAKE,
   	CLUTCH,
  	STEER_LEFT,
   	STEER_RIGHT,
 	SHIFT_UP,
  	SHIFT_DOWN,
   	START_ENGINE,
    ABS_TOGGLE,
    TCS_TOGGLE,
    NEUTRAL,
    FIRST_GEAR,
	SECOND_GEAR,
 	THIRD_GEAR,
  	FOURTH_GEAR,
   	FIFTH_GEAR,
    SIXTH_GEAR,
    REVERSE,
	ROLLOVER_RECOVER,

    //inputs that are used elsewhere in the game only
    GAME_ONLY_INPUTS_START_HERE,
	VIEW_REAR,
 	VIEW_PREV,
 	VIEW_NEXT,
    VIEW_HOOD,
    VIEW_INCAR,
    VIEW_CHASERIGID,
    VIEW_CHASE,
	VIEW_ORBIT,
 	VIEW_FREE,
	FOCUS_PREV,
	FOCUS_NEXT,
  	PAN_LEFT,
   	PAN_RIGHT,
    PAN_UP,
    PAN_DOWN,
    ZOOM_IN,
    ZOOM_OUT,
	REPLAY_FF,
	REPLAY_RW,
	SCREENSHOT,
 	PAUSE,
  	RELOAD_SHADERS,
    RELOAD_GUI,
    GUI_LEFT,
    GUI_RIGHT,
    GUI_UP,
    GUI_DOWN,
    GUI_SELECT,
    GUI_CANCEL,

	INVALID
};
}

#endif
