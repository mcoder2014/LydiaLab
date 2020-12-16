#ifndef REMOVEUNCONNECTEDVERTEX_H
#define REMOVEUNCONNECTEDVERTEX_H

#include <vector>
#include <set>

#include <SurfaceMeshModel.h>

using std::vector;
using std::set;

using SurfaceMesh::SurfaceMeshModel;
using SurfaceMesh::Vertex;
using SurfaceMesh::Face;
using SurfaceMesh::Halfedge;

// 复制一个网格，移除掉离散面片
SurfaceMeshModel *removeUnconnectedFace(SurfaceMeshModel* model);

// 将模型划分为联通的点集
vector<vector<Vertex>> splitModelIntoVertexSet(SurfaceMeshModel* model);

// 由一个顶点找到所有联通的顶点
vector<Vertex> getConnectedVertex(Vertex vertex, SurfaceMeshModel* model, set<Vertex>& readVertexSet);


#endif // REMOVEUNCONNECTEDVERTEX_H
