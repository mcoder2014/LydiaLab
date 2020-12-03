#include <QTreeWidgetItem>

#include "SurfaceMeshModel.h"
using namespace SurfaceMesh;

namespace SurfaceMesh{
bool isA(StarlabModel* model){
    SurfaceMeshModel* mesh = dynamic_cast<SurfaceMeshModel*>(model);
    return mesh != nullptr;
}
SurfaceMeshModel* safe_cast(StarlabModel* model){
    SurfaceMeshModel* mesh = dynamic_cast<SurfaceMeshModel*>(model);
    if(!mesh) throw StarlabException("Model is not a SurfaceMeshModel");
    return mesh;
}
SurfaceMeshModel* safeCast(StarlabModel* model){
    return safe_cast(model);
}
}

SurfaceMeshModel::SurfaceMeshModel(QString path, QString name) : Model(path, name){
    /// Allocate rendering system
    this->color = Qt::darkGray;
}

SurfaceMeshModel::SurfaceMeshModel(const SurfaceMeshModel &model):Surface_mesh(model)
{
    Model::assign(model);
}

/**
 * @brief SurfaceMeshModel::operator =
 * @param model
 * @return
 */
SurfaceMeshModel SurfaceMeshModel::operator=(const SurfaceMeshModel &model)
{
    Surface_mesh::operator=(model);
    Model::assign(model);
}

/**
 * @brief SurfaceMeshModel::decorateLayersWidgedItem
 * 装饰 Layer 窗口
 * @param parent
 */
void SurfaceMeshModel::decorateLayersWidgedItem(QTreeWidgetItem* parent){
    /// Show face count on layer
    {
        QTreeWidgetItem *fileItem = new QTreeWidgetItem();
        fileItem->setText(1, "Vertices");
        fileItem->setText(2, QString::number( n_vertices() ));
        parent->addChild(fileItem);
    }
    /// Show face count on layer
    {
        QTreeWidgetItem *fileItem = new QTreeWidgetItem();
        fileItem->setText(1, "Faces");
        fileItem->setText(2, QString::number( n_faces() ));
        parent->addChild(fileItem);
    }
    /// Show path
    {
        QTreeWidgetItem *fileItem = new QTreeWidgetItem();
        fileItem->setText(1, "Path");
        fileItem->setText(2, this->path);
        parent->addChild(fileItem);
    }
}

/**
 * @brief SurfaceMeshModel::clone
 * 克隆一个网格模型
 * @return
 */
SurfaceMeshModel *SurfaceMeshModel::clone()
{
    std::vector<Surface_mesh::Face> selected_faces;
    for(Face f : faces()) {
        selected_faces.push_back(f);
    }
    return clone(selected_faces);
}

/**
 * @brief SurfaceMeshModel::clone
 * 克隆一个包含指定顶点的网格模型
 * 1. 找到点集中每个顶点含有的半边及其所能够到达的面片
 * @param subset
 * @return
 */
SurfaceMeshModel *SurfaceMeshModel::clone(std::vector<Surface_mesh::Vertex> subset)
{
    /// Remove possible duplicates
    std::vector<Surface_mesh::Face> selected_faces;
    std::sort( subset.begin(), subset.end() );
    subset.erase( unique( subset.begin(), subset.end() ), subset.end() );

    for(Vertex v : subset) {
        for(Halfedge h : onering_hedges(v)) {
            selected_faces.push_back(face(h));
        }
    }

    return clone(selected_faces);
}

/**
 * @brief SurfaceMeshModel::clone
 * 克隆一个包含指定面片的网格模型
 * @param subset
 * @return
 */
SurfaceMeshModel *SurfaceMeshModel::clone(std::vector<Surface_mesh::Face> faceSet)
{
    /// Remove possible duplicates
    std::sort( faceSet.begin(), faceSet.end() );
    faceSet.erase( unique( faceSet.begin(), faceSet.end() ), faceSet.end() );

    SurfaceMeshModel * destModel = new SurfaceMeshModel("clone.obj", this->name + "_clone");
    Vector3VertexProperty points = vertex_coordinates();

    /// 构建顶点、面片集合
    // 顶点集合
    QSet<int> vertSet;

    // 顶点->顶点 map
    QMap<Vertex,Vertex> vmap;
    for(Face f : faceSet){
        if(!is_valid(f)) continue;
        Surface_mesh::Vertex_around_face_circulator vit = vertices(f),vend=vit;
        do{
            vertSet.insert(Vertex(vit).idx());
        } while(++vit != vend);
    }

    // 将顶点集合的顶点加入模型
    for(int vidx : vertSet){
        vmap[Vertex(vidx)] = Vertex(vmap.size());
        destModel->add_vertex( points[Vertex(vidx)] );
    }

    // 添加面
    for(Face f : faceSet){
        if(!is_valid(f)) continue;
        std::vector<Vertex> pnts;
        Surface_mesh::Vertex_around_face_circulator vit = vertices(f),vend=vit;
        do {
            pnts.push_back(vmap[vit]);
        } while(++vit != vend);
        destModel->add_face(pnts);
    }

    destModel->update_face_normals();
    destModel->update_vertex_normals();
    destModel->updateBoundingBox();

    // 拷贝 transform 数据
    destModel->position = this->position;
    destModel->rotation = this->rotation;
    destModel->scale = this->scale;

    return destModel;
}

void SurfaceMeshModel::assign(const SurfaceMeshModel &model)
{
    Surface_mesh::assign(model);
    Model::assign(model);
}

void SurfaceMeshModel::updateBoundingBox(){
    Vector3VertexProperty points = vertex_coordinates();
    _bbox.setNull();
    for(Vertex vit : this->vertices()) {
        _bbox.extend( points[vit] );
    }
}

void SurfaceMeshModel::remove_vertex(Vertex v){
#if 0
    /// More needs to be done.. halfedges need to be cleaned up
    if( !is_valid(v) ) return;
    foreach(Face f, this->faces(v))
        this->fdeleted_[f] = true;
#endif
    if(vdeleted_[v]) {
        return;
    }
    this->vdeleted_[v] = true;
    ++deleted_vertices_;
    this->garbage_ = true;
}

/**
 * @brief SurfaceMeshModel::remove_face
 * 移除一个面片
 * 1. 标记面片为删除状态
 * 2. 检查边、顶点、半边，不被其他顶点边引用的则标记为删除状态
 * @param face
 */
void SurfaceMeshModel::remove_face(Surface_mesh::Face face)
{
    if(fdeleted_[face]) {
        return;
    }

    // 检查周围的边是否被孤立，然后选择移除边
    Halfedge halfedgeEnd = halfedge(face);
    Halfedge halfedgeIter = halfedgeEnd;

    std::vector<Halfedge> halfdegeVector;

    do{
        Halfedge oppositeHalfedge = opposite_halfedge(halfedgeIter);
        if(is_boundary(oppositeHalfedge)
                || fdeleted_[this->face(oppositeHalfedge)]){
            halfdegeVector.push_back(halfedgeIter);
        }
        halfedgeIter = next_halfedge(halfedgeIter);
        std::cout << "Face " << face.idx() << " halfedge " << halfedgeIter.idx() << std::endl;
    }while(halfedgeIter != halfedgeEnd);

    fdeleted_[face] = true;
    deleted_faces_++;
    for(Halfedge halfedge : halfdegeVector) {
        remove_edge(edge(halfedge));
    }
    garbage_ = true;
}

/**
 * @brief SurfaceMeshModel::remove_edge
 * 从模型中删除一边，检查边的顶点是否需要移除
 * @param edge
 */
void SurfaceMeshModel::remove_edge(Surface_mesh::Edge edge)
{
    if(edeleted_[edge]) {
        return;
    }

    edeleted_[edge] = true;
    ++deleted_edges_;

    /// 删除没有直接引用边的顶点
    auto removeIsolateVertex = [&](Vertex vertex) {
        int counts = 0;
        for(Halfedge halfedge : this->onering_hedges(vertex)) {
            if(!edeleted_[this->edge(halfedge)]) {
                counts ++;
            }
        }
        if(counts == 0) {
            this->remove_vertex(vertex);
        }
    };

    Halfedge halfedge = this->halfedge(edge, 0);
    Vertex fromVertex = from_vertex(halfedge);
    Vertex toVertex = to_vertex(halfedge);

    removeIsolateVertex(fromVertex);
    removeIsolateVertex(toVertex);
    garbage_ = true;
}

SurfaceMeshForEachHalfedgeHelper SurfaceMeshModel::halfedges(){
    return SurfaceMeshForEachHalfedgeHelper(this);
}

SurfaceMeshForEachVertexHelper SurfaceMeshModel::vertices(){
    return SurfaceMeshForEachVertexHelper(this);
}

SurfaceMeshForEachVertexOnFaceHelper SurfaceMeshModel::vertices(Surface_mesh::Face f){
    return SurfaceMeshForEachVertexOnFaceHelper(this,f);
}

SurfaceMeshForEachEdgeHelper SurfaceMeshModel::edges(){
    return SurfaceMeshForEachEdgeHelper(this);
}

SurfaceMeshForEachFaceHelper SurfaceMeshModel::faces(){
    return SurfaceMeshForEachFaceHelper(this);
}

SurfaceMeshForEachFaceAtVertex SurfaceMeshModel::faces(Surface_mesh::Vertex v){
    return SurfaceMeshForEachFaceAtVertex(this,v);
}

SurfaceMeshForEachOneRingEdgesHelper SurfaceMeshModel::onering_hedges(Surface_mesh::Vertex v){
    return SurfaceMeshForEachOneRingEdgesHelper(this,v);
}

Vector3VertexProperty SurfaceMeshModel::vertex_coordinates(bool create_if_missing){
    if(create_if_missing)
        return vertex_property<Vector3>(VPOINT,Vector3(0.0,0.0,0.0));
    else
        return get_vertex_property<Vector3>(VPOINT);
}

Vector3VertexProperty SurfaceMeshModel::vertex_normals(bool create_if_missing){
    if(create_if_missing)
        return vertex_property<Vector3>(VNORMAL,Vector3(0.0,0.0,1.0));
    else
        return get_vertex_property<Vector3>(VNORMAL);
}

Vector3FaceProperty SurfaceMeshModel::face_normals(bool create_if_missing){
    if(create_if_missing)
        return face_property<Vector3>(FNORMAL,Vector3(0.0,0.0,1.0));
    else
        return get_face_property<Vector3>(FNORMAL);
}

QDebug operator<< (QDebug d, const Surface_mesh::Edge& edge) {
    d.nospace() << "Edge[" << edge.idx() << "]";
    return d.space();
}
