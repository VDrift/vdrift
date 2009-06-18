#ifndef _TRACK_OBJECT_H
#define _TRACK_OBJECT_H

#include <string>

#include "collision_detection.h"
#include "model.h"
#include "carsurfacetype.h"

class TEXTURE_GL;
class DRAWABLE;

class TRACK_OBJECT
{
	private:
		COLLISION_OBJECT collision;
		
		MODEL * model;
		TEXTURE_GL * texture;
		
		float bump_wavelength;
		float bump_amplitude;
		bool driveable;
		bool collideable;
		float friction_notread;
		float friction_tread;
		float rolling_resistance;
		float rolling_drag;
		int surface_type;
		SURFACE::CARSURFACETYPE surface;
		
	public:
		TRACK_OBJECT(MODEL * nmodel, TEXTURE_GL * ntexture,
			     float bw, float ba, bool dr, bool cl, float fn,
			     float ft, float rr, float rd, int st) :
			model(nmodel), texture(ntexture),
			bump_wavelength(bw), bump_amplitude(ba), driveable(dr),
			collideable(cl), friction_notread(fn), friction_tread(ft),
			rolling_resistance(rr), rolling_drag(rd), surface_type(st)
		{
			assert(model);
			assert(texture);
			//set the surface based on the surface_type int
			if (surface_type == 0)
				surface = SURFACE::NONE;
			else if (surface_type == 1)
				surface = SURFACE::ASPHALT;
			else if (surface_type == 2)
				surface = SURFACE::GRASS;
			else if (surface_type == 3)
				surface = SURFACE::GRAVEL;
			else if (surface_type == 4)
				surface = SURFACE::CONCRETE;
			else if (surface_type == 5)
				surface = SURFACE::SAND;
			else if (surface_type == 6)
				surface = SURFACE::COBBLES;
		}
		
		void InitCollisionObject()
		{
			if (collideable || driveable)
			{
				COLLISION_OBJECT_SETTINGS settings;
				settings.SetStaticObject();
				settings.SetObjectID(this);
				collision.InitTrimesh(model->GetVertexArray(), settings);
			}
		}
		
		///returns NULL if the object is not collide-able
		COLLISION_OBJECT * GetCollisionObject()
		{
			if (collideable || driveable)
				return &collision;
			else
				return NULL;
		}

	float GetBumpWavelength() const
	{
		return bump_wavelength;
	}

	float GetBumpAmplitude() const
	{
		return bump_amplitude;
	}

	float GetFrictionTread() const
	{
		return friction_tread;
	}

	float GetRollingResistanceCoefficient() const
	{
		return rolling_resistance;
	}

	float GetRollingDrag() const
	{
		return rolling_drag;
	}

	float GetFrictionNoTread() const
	{
		return friction_notread;
	}

	SURFACE::CARSURFACETYPE GetSurfaceType() const
	{
		return surface;
	}
	int GetSurfaceInt() const
	{
		return surface_type;
	}
};

#endif
