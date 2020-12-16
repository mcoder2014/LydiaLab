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
