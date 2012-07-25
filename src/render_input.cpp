#include "render_input.h"
#include "matrix4.h"
#include "frustum.h"
#include "glew.h"

void RENDER_INPUT::ExtractFrustum(FRUSTUM & frustum)
{
	float   proj[16];
	float   modl[16];
	float   clip[16];
	float   t;

	/* Get the current PROJECTION matrix from OpenGL */
	glGetFloatv( GL_PROJECTION_MATRIX, proj );

	/* Get the current MODELVIEW matrix from OpenGL */
	glGetFloatv( GL_MODELVIEW_MATRIX, modl );

	/* Combine the two matrices (multiply projection by modelview) */
	clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
	clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
	clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
	clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];

	clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
	clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
	clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
	clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];

	clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] + modl[10] * proj[ 8] + modl[11] * proj[12];
	clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] + modl[11] * proj[13];
	clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
	clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];

	clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] + modl[14] * proj[ 8] + modl[15] * proj[12];
	clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] + modl[15] * proj[13];
	clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
	clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];

	/* Extract the numbers for the RIGHT plane */
	frustum.frustum[0][0] = clip[ 3] - clip[ 0];
	frustum.frustum[0][1] = clip[ 7] - clip[ 4];
	frustum.frustum[0][2] = clip[11] - clip[ 8];
	frustum.frustum[0][3] = clip[15] - clip[12];

	/* Normalize the result */
	t = sqrt( frustum.frustum[0][0] * frustum.frustum[0][0] + frustum.frustum[0][1] * frustum.frustum[0][1] + frustum.frustum[0][2] * frustum.frustum[0][2] );
	frustum.frustum[0][0] /= t;
	frustum.frustum[0][1] /= t;
	frustum.frustum[0][2] /= t;
	frustum.frustum[0][3] /= t;

	/* Extract the numbers for the LEFT plane */
	frustum.frustum[1][0] = clip[ 3] + clip[ 0];
	frustum.frustum[1][1] = clip[ 7] + clip[ 4];
	frustum.frustum[1][2] = clip[11] + clip[ 8];
	frustum.frustum[1][3] = clip[15] + clip[12];

	/* Normalize the result */
	t = sqrt( frustum.frustum[1][0] * frustum.frustum[1][0] + frustum.frustum[1][1] * frustum.frustum[1][1] + frustum.frustum[1][2] * frustum.frustum[1][2] );
	frustum.frustum[1][0] /= t;
	frustum.frustum[1][1] /= t;
	frustum.frustum[1][2] /= t;
	frustum.frustum[1][3] /= t;

	/* Extract the BOTTOM plane */
	frustum.frustum[2][0] = clip[ 3] + clip[ 1];
	frustum.frustum[2][1] = clip[ 7] + clip[ 5];
	frustum.frustum[2][2] = clip[11] + clip[ 9];
	frustum.frustum[2][3] = clip[15] + clip[13];

	/* Normalize the result */
	t = sqrt( frustum.frustum[2][0] * frustum.frustum[2][0] + frustum.frustum[2][1] * frustum.frustum[2][1] + frustum.frustum[2][2] * frustum.frustum[2][2] );
	frustum.frustum[2][0] /= t;
	frustum.frustum[2][1] /= t;
	frustum.frustum[2][2] /= t;
	frustum.frustum[2][3] /= t;

	/* Extract the TOP plane */
	frustum.frustum[3][0] = clip[ 3] - clip[ 1];
	frustum.frustum[3][1] = clip[ 7] - clip[ 5];
	frustum.frustum[3][2] = clip[11] - clip[ 9];
	frustum.frustum[3][3] = clip[15] - clip[13];

	/* Normalize the result */
	t = sqrt( frustum.frustum[3][0] * frustum.frustum[3][0] + frustum.frustum[3][1] * frustum.frustum[3][1] + frustum.frustum[3][2] * frustum.frustum[3][2] );
	frustum.frustum[3][0] /= t;
	frustum.frustum[3][1] /= t;
	frustum.frustum[3][2] /= t;
	frustum.frustum[3][3] /= t;

	/* Extract the FAR plane */
	frustum.frustum[4][0] = clip[ 3] - clip[ 2];
	frustum.frustum[4][1] = clip[ 7] - clip[ 6];
	frustum.frustum[4][2] = clip[11] - clip[10];
	frustum.frustum[4][3] = clip[15] - clip[14];

	/* Normalize the result */
	t = sqrt( frustum.frustum[4][0] * frustum.frustum[4][0] + frustum.frustum[4][1] * frustum.frustum[4][1] + frustum.frustum[4][2] * frustum.frustum[4][2] );
	frustum.frustum[4][0] /= t;
	frustum.frustum[4][1] /= t;
	frustum.frustum[4][2] /= t;
	frustum.frustum[4][3] /= t;

	/* Extract the NEAR plane */
	frustum.frustum[5][0] = clip[ 3] + clip[ 2];
	frustum.frustum[5][1] = clip[ 7] + clip[ 6];
	frustum.frustum[5][2] = clip[11] + clip[10];
	frustum.frustum[5][3] = clip[15] + clip[14];

	/* Normalize the result */
	t = sqrt( frustum.frustum[5][0] * frustum.frustum[5][0] + frustum.frustum[5][1] * frustum.frustum[5][1] + frustum.frustum[5][2] * frustum.frustum[5][2] );
	frustum.frustum[5][0] /= t;
	frustum.frustum[5][1] /= t;
	frustum.frustum[5][2] /= t;
	frustum.frustum[5][3] /= t;
}

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
