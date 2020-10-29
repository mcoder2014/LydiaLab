#include <QTreeWidgetItem>

#include "SurfaceMeshModel.h"
using namespace SurfaceMesh;

namespace SurfaceMesh{
    bool isA(StarlabModel* model){
        SurfaceMeshModel* mesh = dynamic_cast<SurfaceMeshModel*>(model);
        return (mesh!=NULL);
    }
    SurfaceMeshModel* safe_cast(StarlabModel* model){
        SurfaceMeshModel* mesh = dynamic_cast<SurfaceMeshModel*>(model);
        if(!mesh) 
            throw StarlabException("Model is not a SurfaceMeshModel");
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

SurfaceMeshModel *SurfaceMeshModel::clone()
{
    std::vector<Surface_mesh::Face> selected_faces;
    foreach(Face f, faces()) selected_faces.push_back(f);
    return clone(selected_faces);
}

SurfaceMeshModel *SurfaceMeshModel::clone(std::vector<Surface_mesh::Vertex> subset)
{
    /// Remove possible duplicates
    std::vector<Surface_mesh::Face> selected_faces;
    std::sort( subset.begin(), subset.end() );
    subset.erase( unique( subset.begin(), subset.end() ), subset.end() );

    foreach(Vertex v, subset)
        foreach(Halfedge h, onering_hedges(v))
            selected_faces.push_back(face(h));

    return clone(selected_faces);
}

SurfaceMeshModel *SurfaceMeshModel::clone(std::vector<Surface_mesh::Face> subset)
{
    /// Remove possible duplicates
    std::sort( subset.begin(), subset.end() );
    subset.erase( unique( subset.begin(), subset.end() ), subset.end() );

    SurfaceMeshModel * m = new SurfaceMeshModel("clone.obj", this->name + "_clone");

    Vector3VertexProperty points = vertex_coordinates();

    QSet<int> vertSet;
    QMap<Vertex,Vertex> vmap;
    foreach(Face f, subset){
        if(!is_valid(f)) continue;
        Surface_mesh::Vertex_around_face_circulator vit = vertices(f),vend=vit;
        do{ vertSet.insert(Vertex(vit).idx()); } while(++vit != vend);
    }
    foreach(int vidx, vertSet){
        vmap[Vertex(vidx)] = Vertex(vmap.size());
        m->add_vertex( points[Vertex(vidx)] );
    }
    foreach(Face f, subset){
        if(!is_valid(f)) continue;
        std::vector<Vertex> pnts;
        Surface_mesh::Vertex_around_face_circulator vit = vertices(f),vend=vit;
        do{ pnts.push_back(vmap[vit]); } while(++vit != vend);
        m->add_face(pnts);
    }

    m->update_face_normals();
    m->update_vertex_normals();
    m->updateBoundingBox();

    return m;
}

void SurfaceMeshModel::updateBoundingBox(){
    Vector3VertexProperty points = vertex_coordinates();
    _bbox.setNull();
    for(Vertex vit : this->vertices()) {
        _bbox.extend( points[vit] );
    }
    this->position = _bbox.center();
}

void SurfaceMeshModel::remove_vertex(Vertex v){
#if 0
    /// More needs to be done.. halfedges need to be cleaned up
    if( !is_valid(v) ) return;        
    foreach(Face f, this->faces(v))
        this->fdeleted_[f] = true;
#endif
    this->vdeleted_[v] = true;
    this->garbage_ = true;
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
