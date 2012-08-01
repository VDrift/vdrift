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

#ifndef _RENDERPASSINFO_H
#define _RENDERPASSINFO_H

#include "joeserialize.h"
#include "macros.h"

#include <string>
#include <vector>
#include <set>

struct RealtimeExportPassInfo
{
	std::string name;

	bool clearColor;
	bool clearDepth;
	bool clearStencil;
	std::vector <float> clearColorValue;
	float clearDepthValue;
	int clearStencilValue;
	std::vector <std::string> drawGroups;

	std::map <std::string, std::string> userDefinedFields;

	std::string vertexShader;
	std::set <std::string> vertexShaderDefines;
	std::string fragmentShader;
	std::set <std::string> fragmentShaderDefines;

	std::vector <std::string> shaderAttributeBindings;

	struct UniformData
	{
		std::vector <float> data;

		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s,data);
			return true;
		}
	};
	std::map <std::string, UniformData> uniforms;

	// render states
	struct RenderState
	{
		std::string type; //ENUM, FLOAT, INT
		std::string enumdata;
		std::vector <int> intdata;
		std::vector <float> floatdata;

		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s,type);
			_SERIALIZE_(s,enumdata);
			_SERIALIZE_(s,intdata);
			_SERIALIZE_(s,floatdata);
			return true;
		}
	};
	std::vector <std::string> stateEnable;
	std::vector <std::string> stateDisable;
	std::vector <std::pair <std::string, int> > stateEnablei;
	std::vector <std::pair <std::string, int> > stateDisablei;
	std::map <std::string, RenderState> stateEnum;

	// render target information
	struct RenderTargetInfo
	{
		std::string name;
		std::string variable;
		std::string format;
		std::string target;
		bool autoMipmap;
		float width, height;
		bool widthHeightAreMultiples;

		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s,name);
			_SERIALIZE_(s,variable);
			_SERIALIZE_(s,format);
			_SERIALIZE_(s,target);
			_SERIALIZE_(s,autoMipmap);
			_SERIALIZE_(s,width);
			_SERIALIZE_(s,height);
			_SERIALIZE_(s,widthHeightAreMultiples);
			return true;
		}
	};
	std::map <std::string, RenderTargetInfo> renderTargets;

	// samplers
	struct Sampler
	{
		std::string textureName;
		std::map <std::string, RenderState> state;

		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s,textureName);
			_SERIALIZE_(s,state);
			return true;
		}
	};
	std::map <std::string, Sampler> samplers;

	bool Serialize(joeserialize::Serializer & s)
	{
		_SERIALIZE_(s,name);
		_SERIALIZE_(s,clearColor);
		_SERIALIZE_(s,clearDepth);
		_SERIALIZE_(s,clearStencil);
		_SERIALIZE_(s,clearColorValue);
		_SERIALIZE_(s,clearDepthValue);
		_SERIALIZE_(s,clearStencilValue);
		_SERIALIZE_(s,drawGroups);
		_SERIALIZE_(s,userDefinedFields);
		_SERIALIZE_(s,vertexShader);
		_SERIALIZE_(s,vertexShaderDefines);
		_SERIALIZE_(s,fragmentShader);
		_SERIALIZE_(s,fragmentShaderDefines);
		_SERIALIZE_(s,shaderAttributeBindings);
		_SERIALIZE_(s,uniforms);
		_SERIALIZE_(s,stateEnable);
		_SERIALIZE_(s,stateDisable);
		_SERIALIZE_(s,stateEnablei);
		_SERIALIZE_(s,stateDisablei);
		_SERIALIZE_(s,stateEnum);
		_SERIALIZE_(s,renderTargets);
		_SERIALIZE_(s,samplers);
		return true;
	}
};

#endif
