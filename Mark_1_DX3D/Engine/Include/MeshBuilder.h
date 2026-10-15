#pragma once

#include <vector>
#include "ModelClass.h"

namespace MeshBuilder
{
    // Appends one quad (2 triangles, 6 vertices). p[0..3] go around the quad, uv[] matches.
    // The renderer has culling off (D3dClass: D3D11_CULL_NONE), so winding order does not matter.
    inline void AddQuad(std::vector<ModelClass::MeshVertex>& vertices, const XMFLOAT3 p[4], const XMFLOAT2 uv[4], XMFLOAT3 normal)
    {
        static const int order[6] = { 0, 1, 2, 0, 2, 3 };
        int k, i;

        for(k=0; k<6; k++)
        {
            i = order[k];

            ModelClass::MeshVertex v = { p[i].x, p[i].y, p[i].z, uv[i].x, uv[i].y, normal.x, normal.y, normal.z };
            vertices.push_back(v);
        }
    }
}
