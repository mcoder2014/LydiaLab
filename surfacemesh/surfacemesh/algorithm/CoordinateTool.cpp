#include "CoordinateTool.h"

using Point = Surface_mesh::Point;
using VertexProperty = Surface_mesh::Vertex_property<Point>;
using Vertex = Surface_mesh::Vertex;

/**
 * @brief transform
 * @param mesh
 * @param transformMatrix
 */
void transformation(Surface_mesh *mesh, Eigen::Matrix4d transformMatrix)
{
    // 计算顶点矩阵
    Eigen::MatrixXd points = Eigen::MatrixXd::Ones(4, static_cast<int>(mesh->n_vertices()));

    VertexProperty vpoints = mesh->get_vertex_property<Point>(SurfaceMesh::VPOINT);
    for(Surface_mesh::Vertex_iterator iter = mesh->vertices_begin();
        iter != mesh->vertices_end(); ++iter) {
        Vertex vertex = Vertex(iter);
        points(0, vertex.idx()) = vpoints[vertex].x();
        points(1, vertex.idx()) = vpoints[vertex].y();
        points(2, vertex.idx()) = vpoints[vertex].z();
    }

    points = transformMatrix * points;

    for(Surface_mesh::Vertex_iterator iter = mesh->vertices_begin();
        iter != mesh->vertices_end(); ++iter) {
        Vertex vertex = Vertex(iter);
        vpoints[vertex].x() = points(0, vertex.idx());
        vpoints[vertex].y() = points(1, vertex.idx());
        vpoints[vertex].z() = points(2, vertex.idx());
    }

}

/**
 * @brief getTransformMatrix
 * 通过三个新坐标基向量在旧坐标系中的向量，构建转换矩阵
 * 返回的矩阵： 旧坐标系转换成新坐标系的转换矩阵
 *  newPos = transMat * oldPos
 * @param ox
 * @param oy
 * @param oz
 * @param origin
 * @return
 */
Eigen::Matrix4d getTransformMatrix(Eigen::Vector3d ox, Eigen::Vector3d oy, Eigen::Vector3d oz, Vector3d origin)
{
    // 先将坐标系平移到原点
    Matrix4d transMatrix = Matrix4d::Identity();
    transMatrix(0, 3) = -origin.x();
    transMatrix(1, 3) = -origin.y();
    transMatrix(2, 3) = -origin.z();

    // 构造旋转矩阵
    Matrix4d rotationMatrix = Matrix4d::Identity();
    rotationMatrix.topLeftCorner(3,3) << ox, oy, oz;
    rotationMatrix.transpose();

    return rotationMatrix * transMatrix;
}
