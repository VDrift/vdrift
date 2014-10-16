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

#ifndef _GRAPHICS_CONFIG_H
#define _GRAPHICS_CONFIG_H

#include "graphics_config_condition.h"

#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <set>

struct GraphicsConfigShader
{
	std::string name; ///< the name of the shader package
	std::string fragment; ///< fragment shader name
	std::string vertex; ///< vertex shader name
	std::string defines; ///< space delimited list of #defines to be set

	bool Load(std::istream & f, std::ostream & error_output, int & linecount);
};

struct GraphicsConfigOutput
{
	class Size
	{
	private:
		unsigned int value;
		unsigned int fb_div;
		unsigned int fb_mult;

	public:
		Size() : value(0), fb_div(0), fb_mult(0) {}

		unsigned int GetSize(unsigned int framebuffer_size) const
		{
			if (value != 0)
				return value;
			else
			{
				if (fb_div != 0)
					return framebuffer_size / fb_div;
				else if (fb_mult != 0)
					return framebuffer_size / fb_mult;
				else
					return framebuffer_size;
			}
		}
		void Parse(const std::string & str);
	};

	std::string name; ///< the name of the output
	Size width; ///< sizes can be absolute numbers, "framebuffer", "framebuffer/X" (where X is an int), or "framebuffer*X"
	Size height;
	std::string type; ///< can be "2D", "rectangle", "cube", or "framebuffer" -- note that if it's framebuffer, the other fields are ignored
	std::string filter; ///< can be "linear" or "nearest"
	std::string format; ///< can be "RGB", "RGBA", or "depth"
	bool mipmap;
	int multisample; ///< zero indicates no multisampling, any negative number means use the same as the framebuffer (can also specify "framebuffer")
	GraphicsConfigCondition conditions;

	bool Load(std::istream & f, std::ostream & error_output, int & linecount);
};

struct GraphicsConfigInputs
{
	/// this maps texture units to output names (to be used as inputs)
	std::map <unsigned int, std::string> tu;

	/// str should be a space-delimited lists of the form tunumber:outputname where tunumber: is optional
	void Parse(const std::string & str);
};

struct GraphicsConfigPass
{
	std::string camera; ///< the name of the camera
	std::vector <std::string> draw; ///< can be "postprocess" or a comma delimited list of drawable layers
	GraphicsConfigInputs inputs; ///< assigns outputs of other passes to inputs
	std::string light; ///< the name of the scene-wide directional light
	std::string output; ///< must correspond to a GRAPHICS_CONFIG_OUTPUT
	std::string shader; ///< the name of the shader to use
	bool clear_color; ///< whether or not to clear the color buffer before rendering, note that this happens at the beginning of the pass, not for each drawable layer in draw
	bool clear_depth; ///< whether or not to clear the depth buffer before rendering, note that this happens at the beginning of the pass, not for each drawable layer in draw
	bool write_color; ///< whether or not to write to the RGB parts of the color buffer during rendering
	bool write_alpha; ///< whether or not to write to the alpha part of the color buffer during rendering
	bool write_depth; ///< whether or not to write to the depth buffer during rendering
	bool cull; ///< whether or not to do frustum and distance culling
	std::string blendmode; ///< which blending mode to use: "disabled" "add" "alphablend" "alphablend_premultiplied"
	std::string depthtest; ///< values: lequal, equal, gequal, disabled
	GraphicsConfigCondition conditions;

	bool Load(std::istream & f, std::ostream & error_output, int & linecount);
};

struct GraphicsConfig
{
	std::vector <GraphicsConfigShader> shaders;
	std::vector <GraphicsConfigOutput> outputs;
	std::vector <GraphicsConfigPass> passes;

	bool Load(const std::string & filename, std::ostream & error_output);

	bool Load(std::istream & f, std::ostream & error_output);
};

#endif
