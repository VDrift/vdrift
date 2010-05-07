#ifndef SKY_H
#define SKY_H

#include <time.h>
#include "fbtexture.h"
#include "camera.h"
#include "mathvector.h"
#include "matrix4.h"

class GRAPHICS_SDLGL;

class SKY
{
public:
	SKY(GRAPHICS_SDLGL & gs, std::ostream & info, std::ostream & error);
	bool Load(const std::string & path);
	void Update();
	void Draw(MATRIX4<float> & iviewproj, FBTEXTURE * output); // iviewproj is the inverse view projection matrix without translation!

	typedef MATHVECTOR<float, 3> VEC3;
	VEC3 GetSunColor() const {return suncolor;}
	VEC3 GetSunDirection() const {return sundir;}
	float GetZenith() const {return ze;}
	float GetAzimuth() const {return az;}
	
	void SetTime(const struct tm & t) {datetime = t;};
	void SetTurbidity(float t) {turbidity = t;};
	void SetExposure(float e) {exposure = e;};
	
protected:
	GRAPHICS_SDLGL & graphics;
	std::ostream & info_output;
	std::ostream & error_output;
	
	FBTEXTURE sky_texture;
	//DRAWABLE * quad;
	//SCENENODE * node;
	
	// scattering
	VEC3 suncolor;
	VEC3 wavelength;
    float turbidity;
    float exposure;

	// solar position
	VEC3 sundir;
    float ze;			// solar zenith angle
    float az;			// solar azimuth angle (clockwise from north)
    float azdelta;		// local orientation offset relative to north(z-axis)
    float timezone;		// timezone in hours from UTC
    float longitude;	// laguna seca raceway
    float latitude;		// in degrees
    struct tm datetime;	// time
    
    void UpdateSunDir();
    void UpdateSunColor();
    void UpdateSkyTexture();
};

#endif // SKY_H
