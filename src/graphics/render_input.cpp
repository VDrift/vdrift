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

#include "render_input.h"
#include "matrix4.h"
#include "glew.h"

void RENDER_INPUT::PushShadowMatrices()
{
	glActiveTexture(GL_TEXTURE3);
	glMatrixMode(GL_TEXTURE);
	MATRIX4<float> m;
	glGetFloatv (GL_TEXTURE_MATRIX, m.GetArray());
	m = m.Inverse();

	//std::cout << m << std::endl;

	for (int i = 0; i < 3; i++)
	{
		glActiveTexture(GL_TEXTURE4+i);
		glPushMatrix();
		glMultMatrixf(m.GetArray());
	}

	glActiveTexture(GL_TEXTURE0);
	glMatrixMode(GL_MODELVIEW);
}

void RENDER_INPUT::PopShadowMatrices()
{
	glMatrixMode(GL_TEXTURE);

	for (int i = 0; i < 3; i++)
	{
		glActiveTexture(GL_TEXTURE4+i);
		glPopMatrix();
	}

	glActiveTexture(GL_TEXTURE0);
	glMatrixMode(GL_MODELVIEW);
}
