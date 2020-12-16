#ifndef COORDINATETOOL_H
#define COORDINATETOOL_H

#include <Eigen/Dense>

#include <SurfaceMeshModel.h>

using Eigen::Vector3d;
using SurfaceMesh::SurfaceMeshModel;
using SurfaceMesh::Halfedge;
using SurfaceMesh::Vertex;
using SurfaceMesh::Face;

// 将模型的坐标按照转换矩阵变换到另一个做表空间
void transformation(Surface_mesh* mesh, Eigen::Matrix4d transformMatrix);

#endif // COORDINATETOOL_H
