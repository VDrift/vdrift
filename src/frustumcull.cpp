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

#include "mathvector.h"
#include "matrix4.h"
#include "frustum.h"
#include "frustumcull.h"
#include "unittest.h"

QT_TEST(frustum_cull_test)
{
	Mat4 ident;
	Mat4 ortho;
	ortho.SetOrthographic(-10, 10, -5, 5, 1, -9);

	Frustum frustum;
	frustum.Extract(ident.GetArray(), ortho.GetArray());


	QT_CHECK(FrustumCull(frustum.frustum, Vec3(2, 7, 1), 1.0f));
	QT_CHECK(FrustumCull(frustum.frustum, Vec3(2, -7, 1), 1.0f));
	QT_CHECK(FrustumCull(frustum.frustum, Vec3(12, 0, 1), 1.0f));
	QT_CHECK(FrustumCull(frustum.frustum, Vec3(-12, 0, 1), 1.0f));

	QT_CHECK(!FrustumCull(frustum.frustum, Vec3(2, 7, 1), 3.0f));
	QT_CHECK(!FrustumCull(frustum.frustum, Vec3(2, -7, 1), 3.0f));
	QT_CHECK(!FrustumCull(frustum.frustum, Vec3(12, 0, 1), 3.0f));
	QT_CHECK(!FrustumCull(frustum.frustum, Vec3(-12, 0, 1), 3.0f));

	QT_CHECK(FrustumCull(frustum.frustum, Vec3(2, 7, 1), Vec3(2, 1, 3)));
	QT_CHECK(FrustumCull(frustum.frustum, Vec3(2, -7, 1), Vec3(2, 1, 3)));
	QT_CHECK(FrustumCull(frustum.frustum, Vec3(12, 0, 1), Vec3(1, 2, 3)));
	QT_CHECK(FrustumCull(frustum.frustum, Vec3(-12, 0, 1), Vec3(1, 2, 3)));

	QT_CHECK(!FrustumCull(frustum.frustum, Vec3(2, 7, 1), Vec3(2, 3, 1)));
	QT_CHECK(!FrustumCull(frustum.frustum, Vec3(2, -7, 1), Vec3(2, 3, 1)));
	QT_CHECK(!FrustumCull(frustum.frustum, Vec3(12, 0, 1), Vec3(3, 2, 1)));
	QT_CHECK(!FrustumCull(frustum.frustum, Vec3(-12, 0, 1), Vec3(3, 2, 1)));
}
