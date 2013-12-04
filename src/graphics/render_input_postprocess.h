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

struct GraphicsCamera;
class TextureInterface;
class Shader;

class RenderInputPostprocess : public RenderInput
{
public:
	RenderInputPostprocess();

	~RenderInputPostprocess();

	void SetShader(Shader * newshader);

	void ClearOutput(GraphicsState & glstate, bool clearcolor, bool cleardepth);

	void SetColorMask(GraphicsState & glstate, bool write_color, bool write_alpha);

	void SetDepthMode(GraphicsState & glstate, int mode, bool write_depth);

	void SetBlendMode(GraphicsState & glstate, BlendMode::BLENDMODE mode);

	void SetTextures(
		GraphicsState & glstate,
		const std::vector <TextureInterface*> & textures,
		std::ostream & error_output);

	// these are used only to upload uniforms to the shaders
	void SetCamera(const GraphicsCamera & cam);

	void SetSunDirection(const Vec3 & newsun);

	void SetContrast(float value);

	virtual void Render(GraphicsState & glstate, std::ostream & error_output);

private:
	Shader * shader;
	Quat cam_rotation;
	Vec3 cam_position;
	Vec3 frustum_corners[4];
	Vec3 frustum_corners_ws[4];
	Vec3 lightposition;
	float contrast;
	float maxu;
	float maxv;
};

#endif // _RENDER_INPUT_POSTPROCESS_H
