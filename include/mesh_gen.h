#ifndef _MESHGEN_H
#define _MESHGEN_H

class VERTEXARRAY;

namespace MESHGEN
{

void mg_tire(VERTEXARRAY & tire, float sectionWidth_mm, float aspectRatio, float rimDiameter_in);

void mg_rim(VERTEXARRAY & rim, float sectionWidth_mm, float aspectRatio, float rimDiameter_in, float flangeDisplacement_mm);

};

#endif
