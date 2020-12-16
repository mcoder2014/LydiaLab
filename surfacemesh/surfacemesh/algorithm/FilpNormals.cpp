#include "FilpNormals.h"

using SurfaceMesh::Vertex;

/**
 * @brief filpNormals
 * 翻转面片
 * @param model
 */
void filpNormals(SurfaceMesh::SurfaceMeshModel *model)
{
    /// 翻转面法向量
    SurfaceMesh::SurfaceMeshModel* tmp = new SurfaceMesh::SurfaceMeshModel();
    Surface_mesh::Vertex_property<Surface_mesh::Point> vpoints = model->vertex_coordinates();
    for(SurfaceMesh::SurfaceMeshModel::Vertex vertex : model->vertices()) {
        tmp->add_vertex(vpoints[vertex]);
    }

    // 复制时翻转顶点顺序
    std::vector<SurfaceMesh::SurfaceMeshModel::Vertex> vFace;
    for(SurfaceMesh::Face f : model->faces()) {
        vFace.clear();
        SurfaceMesh::SurfaceMeshForEachVertexOnFaceHelper vit = model->vertices(f);
        vit = vit.begin();
        SurfaceMesh::SurfaceMeshForEachVertexOnFaceHelper vend = vit;
        for(;vit!=vend;++vit) {
            vFace.push_back(SurfaceMesh::Vertex((*vit).idx()));
        }
        std::reverse(vFace.begin(), vFace.end());

        tmp->add_face(vFace);
    }

    model->Surface_mesh::assign(*tmp);
    delete tmp;

    /// 更新法线
    model->updateBoundingBox();
    model->update_vertex_normals();
    model->update_face_normals();
}

/**
 * @brief offsetAlongVNormals
 * 沿着点法线移动距离，需要已经有法线数据
 * @param model
 * @param distance
 */
void offsetAlongVNormals(SurfaceMesh::SurfaceMeshModel *model, double distance)
{
    Surface_mesh::Vertex_property<Vector3d> vnormals
            = model->get_vertex_property<Vector3d>(SurfaceMesh::VNORMAL);
    Surface_mesh::Vertex_property<Vector3d> vpoints
            = model->get_vertex_property<Vector3d>(SurfaceMesh::VPOINT);

    for(Vertex vertex : model->vertices()) {
        vpoints[vertex] += vnormals[vertex].normalized() * distance;
    }
}
