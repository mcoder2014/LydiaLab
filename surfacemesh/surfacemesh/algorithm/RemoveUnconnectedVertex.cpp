#include "RemoveUnconnectedVertex.h"

#include <deque>

using std::deque;

/**
 * @brief removeUnconnectedFace
 * 复制一个新的网格，移除掉当前网格中的非连接项
 * 1. 遍历顶点，将顶点按能够连接起来划分为几个顶点组；
 * 2. 将顶点数目最多的组的复制出来。
 * @param model
 * @return
 */
SurfaceMesh::SurfaceMeshModel *removeUnconnectedFace(SurfaceMesh::SurfaceMeshModel *model)
{
    // 将模型按顶点连接性划分
    vector<vector<Vertex>> vertexGroups = splitModelIntoVertexSet(model);

    // 找到数量最多的组
    size_t index = 0;
    size_t groupSize = 0;
    for(size_t i = 0; i < vertexGroups.size(); i++) {
        if(vertexGroups[i].size() > groupSize) {
            index = i;
            groupSize = vertexGroups[i].size();
        }
    }

    // 拷贝
    SurfaceMeshModel* target = model->clone(vertexGroups[index]);
    return target;
}

/**
 * @brief splitModelIntoVertexSet
 * 将模型划分为联通的点集
 * @param model
 * @return
 */
vector<vector<SurfaceMesh::Vertex>> splitModelIntoVertexSet(SurfaceMesh::SurfaceMeshModel *model)
{
    // 点集
    vector<vector<Vertex>> vertexGroups;
    // 已经被处理过的点集
    set<Vertex> readVertexSet;

    for(Vertex vertex : model->vertices()) {
        if(readVertexSet.find(vertex)!=readVertexSet.end()) {
            // 已经处理过的顶点
            continue;
        }

        /// 递归处理顶点
        vector<Vertex> connectedVertex = getConnectedVertex(vertex, model, readVertexSet);
        vertexGroups.emplace_back(connectedVertex);
    }

    return vertexGroups;
}

/**
 * @brief getConnectedVertex
 * 采用广度有限方式遍历顶点的邻接顶点
 * @param vertex
 * @param model
 * @param readVertexSet
 * @return
 */
vector<SurfaceMesh::Vertex> getConnectedVertex(
        SurfaceMesh::Vertex vertex,
        SurfaceMesh::SurfaceMeshModel *model,
        set<SurfaceMesh::Vertex> &readVertexSet)
{
    vector<SurfaceMesh::Vertex> resultSet;
    deque<Vertex> searchList;
    searchList.push_back(vertex);

    // lambda 表达式，用于判断该顶点是否已经被处理过了
    auto hasProcessed = [&readVertexSet](Vertex& v) -> bool{
        if(readVertexSet.find(v) != readVertexSet.end()) {
            return true;
        } else {
            return false;
        }
    };

    while(!searchList.empty()) {
        Vertex searchVertex = *searchList.begin();
        searchList.pop_front();

        // 如果当前顶点已经被处理过了
        if(hasProcessed(searchVertex)) {
            continue;
        }

        // 将当前顶点插入集合
        readVertexSet.insert(searchVertex);
        resultSet.push_back(searchVertex);

        // 将相邻顶点加入搜索队列
        for(Halfedge hf : model->onering_hedges(searchVertex)) {
            Vertex v = model->to_vertex(hf);
            if(hasProcessed(v)) {
                continue;
            }
            searchList.push_back(v);
        }
    }

    return resultSet;
}
