/* This is the SWIG interface file to generate vdrift python wrapper */
%module vdrift

%include "std_string.i"
%include "std_list.i"
%include "std_vector.i"

%immutable game;
%immutable GAME::settings;
%immutable GAME::world;
%immutable GAME::objects;

%{
#include "../include/game.h"
extern GAME game;
%}

using namespace std;
namespace VDrift {};

%include "../include/definitions.h"
%include "../include/bipointer.h"
%include "../include/scenegraph.h"
%include "../include/settings.h"
%include "../include/3dmath.h"
%include "../include/graphics.h"
%include "../include/camera.h"
%include "../include/objects.h"
%include "../include/physics.h"
%include "../include/vamosworld.h"
%include "../include/track.h"
%include "../include/model.h"
%include "../include/textures.h"

%template(DRAWABLE_BIPOINTER) BIPOINTER<DRAWABLE>;

class GAME {
public:
	SETTINGS settings;
	GRAPHICS graphics;
	CAMERA   cam;
	OBJECTS  objects;
	TRACK    track;
	VAMOSWORLD world;
	PHYSICS  physics;
};

extern GAME game;

