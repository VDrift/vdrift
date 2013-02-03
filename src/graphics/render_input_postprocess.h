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

#ifndef _RENDER_INPUT_POSTPROCESS_H
#define _RENDER_INPUT_POSTPROCESS_H

#include "render_input.h"
#include "frustum.h"
#include "mathvector.h"
#include "quaternion.h"
#include <vector>

class TEXTURE_INTERFACE;
class SHADER_GLSL;

class RENDER_INPUT_POSTPROCESS : public RENDER_INPUT
{
public:
	RENDER_INPUT_POSTPROCESS();

	~RENDER_INPUT_POSTPROCESS();

	void SetSourceTextures(const std::vector <TEXTURE_INTERFACE*> & textures);

	void SetShader(SHADER_GLSL * newshader);

	virtual void Render(GLSTATEMANAGER & glstate, std::ostream & error_output);

	void SetWriteColor(bool write);

	void SetWriteAlpha(bool write);

	void SetWriteDepth(bool write);

	void SetDepthMode(int mode);

	void SetClear(bool newclearcolor, bool newcleardepth);

	void SetBlendMode(BLENDMODE::BLENDMODE mode);

	void SetContrast(float value);

	// these are used only to upload uniforms to the shaders
	void SetCameraInfo(
		const MATHVECTOR <float, 3> & newpos,
		const QUATERNION <float> & newrot,
		float newfov, float newlodfar,
		float neww, float newh);

	void SetSunDirection(const MATHVECTOR <float, 3> & newsun);

private:
	std::vector <TEXTURE_INTERFACE*> source_textures;
	SHADER_GLSL * shader;
	bool writealpha;
	bool writecolor;
	bool writedepth;
	MATHVECTOR <float, 3> lightposition;
	QUATERNION <float> cam_rotation;
	MATHVECTOR <float, 3> cam_position;
	float w, h;
	float camfov;
	float lod_far;
	FRUSTUM frustum;
	int depth_mode;
	bool clearcolor;
	bool cleardepth;
	BLENDMODE::BLENDMODE blendmode;
	float contrast;

	void SetBlendMode(GLSTATEMANAGER & glstate);
};

#endif // _RENDER_INPUT_POSTPROCESS_H
