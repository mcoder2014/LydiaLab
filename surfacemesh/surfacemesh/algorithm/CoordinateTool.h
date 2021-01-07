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

// 通过三个新坐标基向量(单位向量)在旧坐标系中的表示，构建转换矩阵
Eigen::Matrix4d getTransformMatrix(Vector3d ox, Vector3d oy, Vector3d oz, Vector3d origin);

#endif // COORDINATETOOL_H
