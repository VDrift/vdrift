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
#include "mathvector.h"
#include "quaternion.h"
#include <vector>

class TextureInterface;
class Shader;

class RenderInputPostprocess : public RenderInput
{
public:
	RenderInputPostprocess();

	~RenderInputPostprocess();

	void SetSourceTextures(const std::vector <TextureInterface*> & textures);

	void SetShader(Shader * newshader);

	virtual void Render(GraphicsState & glstate, std::ostream & error_output);

	void SetWriteColor(bool write);

	void SetWriteAlpha(bool write);

	void SetWriteDepth(bool write);

	void SetDepthMode(int mode);

	void SetClear(bool newclearcolor, bool newcleardepth);

	void SetBlendMode(BlendMode::BLENDMODE mode);

	void SetContrast(float value);

	// these are used only to upload uniforms to the shaders
	void SetCameraInfo(
		const Vec3 & newpos,
		const Quat & newrot,
		float newfov, float newlodfar,
		float neww, float newh);

	void SetSunDirection(const Vec3 & newsun);

private:
	std::vector <TextureInterface*> source_textures;
	Shader * shader;
	bool writealpha;
	bool writecolor;
	bool writedepth;
	Vec3 lightposition;
	Quat cam_rotation;
	Vec3 cam_position;
	float w, h;
	float camfov;
	float lod_far;
	int depth_mode;
	bool clearcolor;
	bool cleardepth;
	BlendMode::BLENDMODE blendmode;
	float contrast;

	void SetBlendMode(GraphicsState & glstate);
};

#endif // _RENDER_INPUT_POSTPROCESS_H
