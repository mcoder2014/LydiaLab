#include "SurfaceMeshHelper.h"
#include "surfacemesh_filter_laplacian_smoothing.h"

using namespace SurfaceMesh;

void smooth(bool boundaryVertex, bool internalVertex, SurfaceMesh::SurfaceMeshModel* model);

void surfacemesh_filter_laplacian_smoothing::initParameters(RichParameterSet *pars){
    pars->addParam(new RichInt("Iterations", 1, "Iterations"));
    pars->addParam(new RichBool("InternalVertex", true, "Internal Vertex"));
    pars->addParam(new RichBool("BoundaryVertex", true, "Boundary Vertex"));
}

void surfacemesh_filter_laplacian_smoothing::applyFilter(RichParameterSet* pars){
    int iterations = 1;
    bool boundaryVertex = true;
    bool internalVertex = true;

    if(pars) {
        iterations = pars->getInt("Iterations");
        boundaryVertex = pars->getBool("BoundaryVertex");
        internalVertex = pars->getBool("InternalVertex");
    }

    for(int i = 0; i < iterations; i++) {
        smooth(boundaryVertex, internalVertex, safeCast(document()->selectedModel()));
    }

    SurfaceMeshModel* model = safeCast(document()->selectedModel());
    model->updateBoundingBox();
    model->update_face_normals();
    model->update_vertex_normals();
}


/**
 * @brief smooth
 * @param boundaryVertex
 * @param internalVertex
 * @param model
 */
void smooth(bool boundaryVertex, bool internalVertex, SurfaceMesh::SurfaceMeshModel* model) {
    if(!boundaryVertex && !internalVertex) return;

    Surface_mesh::Vertex_property<Point> vpoints = model->vertex_coordinates();
    std::vector<Point> newValues(model->n_vertices(), Point::Zero());

    for(Vertex vertex:model->vertices()) {
        if((!model->is_boundary(vertex) && internalVertex)
            || (model->is_boundary(vertex) && boundaryVertex)) {
            for(Halfedge halfEdge : model->onering_hedges(vertex)) {
                newValues[static_cast<size_t>(vertex.idx())]
                        += vpoints[model->to_vertex(halfEdge)];
            }
            newValues[static_cast<size_t>(vertex.idx())] /= model->valence(vertex);
        } else {
            newValues[static_cast<size_t>(vertex.idx())] = vpoints[vertex];
        }
    }

    for(Vertex vertex : model->vertices()) {
        vpoints[vertex] = newValues[static_cast<size_t>(vertex.idx())];
    }
}
