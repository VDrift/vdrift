#ifndef _LOADCAMERA_H
#define _LOADCAMERA_H

#include <ostream>

class PTree;
class CAMERA;

// Read camera from config file
//
// [cam]
// name = foo			#required
// type = mount			#required types: mount, chase, orbit, free
// position = 0, 0, 0	#optional elative camera position
// lookat = 0, 1, 0		#optional used by mount only atm, determines view direction
// stiffness = 0		#optional used by mount
// fov = 90				#optional [40, 160], overrides global fov
//
CAMERA * LoadCamera(
	const PTree & cfg,
	float camera_bounce,
	std::ostream & error_output);

#endif  // _LOADCAMERA_H

