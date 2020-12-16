#ifndef FILPNORMALS_H
#define FILPNORMALS_H

#include <SurfaceMeshModel.h>

// 翻转面片
void filpNormals(SurfaceMesh::SurfaceMeshModel* model);

// 延点法线方向位移顶点位置
void offsetAlongVNormals(SurfaceMesh::SurfaceMeshModel* model, double distance);

#endif // FILPNORMALS_H
