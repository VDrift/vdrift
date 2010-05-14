#ifndef _MESHGEN_H
#define _MESHGEN_H

class VERTEXARRAY;

namespace MESHGEN
{
void mesh_gen_tire(VERTEXARRAY *tire, float sectionWidth_mm, float aspectRatio, float rimDiameter_in);
};

#endif
